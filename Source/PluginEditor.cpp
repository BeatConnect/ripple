/*
  ==============================================================================
    RIPPLE Texture Engine - Plugin Editor Implementation
    Transform any sound into evolving hypnotic textures

    Uses JUCE 8's native WebView relay system for reliable bidirectional
    parameter synchronization between C++ and the web UI.
  ==============================================================================
*/

#include "PluginEditor.h"
#include "ParameterIDs.h"

#if BEATCONNECT_ACTIVATION_ENABLED
#include <beatconnect/Activation.h>
#endif

static constexpr const char* DEV_SERVER_URL = "http://localhost:5173";

//==============================================================================
RippleEditor::RippleEditor(RippleProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p)
{
    setupWebView();
    setupRelaysAndAttachments();

    // Set plugin window size (wide format for visualizer)
    setSize(1000, 700);
    setResizable(false, false);

    // Start timer for sending visualizer data (30 Hz)
    startTimerHz(30);
}

RippleEditor::~RippleEditor()
{
    stopTimer();
}

//==============================================================================
void RippleEditor::setupWebView()
{
    // ===========================================================================
    // STEP 1: Create relays BEFORE creating WebBrowserComponent
    // ===========================================================================

    // Ripple Filter
    rippleRateRelay = std::make_unique<juce::WebSliderRelay>("ripple_rate");
    rippleMultiplyRelay = std::make_unique<juce::WebSliderRelay>("ripple_multiply");
    rippleAmountRelay = std::make_unique<juce::WebSliderRelay>("ripple_amount");
    rippleWidthRelay = std::make_unique<juce::WebSliderRelay>("ripple_width");
    rippleLowBypassRelay = std::make_unique<juce::WebSliderRelay>("ripple_low_bypass");
    rippleHighBypassRelay = std::make_unique<juce::WebSliderRelay>("ripple_high_bypass");
    rippleMixRelay = std::make_unique<juce::WebSliderRelay>("ripple_mix");

    // Reverb
    reverbEnabledRelay = std::make_unique<juce::WebToggleButtonRelay>("reverb_enabled");
    reverbSizeRelay = std::make_unique<juce::WebSliderRelay>("reverb_size");
    reverbDampingRelay = std::make_unique<juce::WebSliderRelay>("reverb_damping");
    reverbMixRelay = std::make_unique<juce::WebSliderRelay>("reverb_mix");

    // LFO 1
    lfo1RateRelay = std::make_unique<juce::WebSliderRelay>("lfo1_rate");
    lfo1ShapeRelay = std::make_unique<juce::WebComboBoxRelay>("lfo1_shape");
    lfo1PhaseRelay = std::make_unique<juce::WebSliderRelay>("lfo1_phase");

    // LFO 2
    lfo2RateRelay = std::make_unique<juce::WebSliderRelay>("lfo2_rate");
    lfo2ShapeRelay = std::make_unique<juce::WebComboBoxRelay>("lfo2_shape");
    lfo2PhaseRelay = std::make_unique<juce::WebSliderRelay>("lfo2_phase");

    // LFO 3
    lfo3RateRelay = std::make_unique<juce::WebSliderRelay>("lfo3_rate");
    lfo3ShapeRelay = std::make_unique<juce::WebComboBoxRelay>("lfo3_shape");
    lfo3PhaseRelay = std::make_unique<juce::WebSliderRelay>("lfo3_phase");

    // LFO 4
    lfo4RateRelay = std::make_unique<juce::WebSliderRelay>("lfo4_rate");
    lfo4ShapeRelay = std::make_unique<juce::WebComboBoxRelay>("lfo4_shape");
    lfo4PhaseRelay = std::make_unique<juce::WebSliderRelay>("lfo4_phase");

    // Mod Slot 1
    mod1SourceRelay = std::make_unique<juce::WebComboBoxRelay>("mod1_source");
    mod1TargetRelay = std::make_unique<juce::WebComboBoxRelay>("mod1_target");
    mod1DepthRelay = std::make_unique<juce::WebSliderRelay>("mod1_depth");

    // Mod Slot 2
    mod2SourceRelay = std::make_unique<juce::WebComboBoxRelay>("mod2_source");
    mod2TargetRelay = std::make_unique<juce::WebComboBoxRelay>("mod2_target");
    mod2DepthRelay = std::make_unique<juce::WebSliderRelay>("mod2_depth");

    // Mod Slot 3
    mod3SourceRelay = std::make_unique<juce::WebComboBoxRelay>("mod3_source");
    mod3TargetRelay = std::make_unique<juce::WebComboBoxRelay>("mod3_target");
    mod3DepthRelay = std::make_unique<juce::WebSliderRelay>("mod3_depth");

    // Mod Slot 4
    mod4SourceRelay = std::make_unique<juce::WebComboBoxRelay>("mod4_source");
    mod4TargetRelay = std::make_unique<juce::WebComboBoxRelay>("mod4_target");
    mod4DepthRelay = std::make_unique<juce::WebSliderRelay>("mod4_depth");

    // Global
    bypassRelay = std::make_unique<juce::WebToggleButtonRelay>("bypass");
    inputGainRelay = std::make_unique<juce::WebSliderRelay>("input_gain");
    outputGainRelay = std::make_unique<juce::WebSliderRelay>("output_gain");

    // ===========================================================================
    // STEP 2: Get resources directory
    // ===========================================================================
    auto executableFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    auto executableDir = executableFile.getParentDirectory();

    resourcesDir = executableDir.getChildFile("Resources").getChildFile("WebUI");

    if (!resourcesDir.isDirectory())
    {
        resourcesDir = executableDir.getParentDirectory()
                           .getChildFile("Resources")
                           .getChildFile("WebUI");
    }

    // ===========================================================================
    // STEP 3: Build WebBrowserComponent with JUCE 8 options
    // ===========================================================================
    auto options = juce::WebBrowserComponent::Options()
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withNativeIntegrationEnabled()
        .withResourceProvider(
            [this](const juce::String& url) -> std::optional<juce::WebBrowserComponent::Resource>
            {
                auto path = url;
                if (path.startsWith("/"))
                    path = path.substring(1);
                if (path.isEmpty())
                    path = "index.html";

                auto file = resourcesDir.getChildFile(path);
                if (!file.existsAsFile())
                    return std::nullopt;

                juce::String mimeType = "application/octet-stream";
                if (path.endsWith(".html")) mimeType = "text/html";
                else if (path.endsWith(".css")) mimeType = "text/css";
                else if (path.endsWith(".js")) mimeType = "application/javascript";
                else if (path.endsWith(".json")) mimeType = "application/json";
                else if (path.endsWith(".png")) mimeType = "image/png";
                else if (path.endsWith(".jpg") || path.endsWith(".jpeg")) mimeType = "image/jpeg";
                else if (path.endsWith(".svg")) mimeType = "image/svg+xml";
                else if (path.endsWith(".woff")) mimeType = "font/woff";
                else if (path.endsWith(".woff2")) mimeType = "font/woff2";
                else if (path.endsWith(".ttf")) mimeType = "font/ttf";

                juce::MemoryBlock data;
                file.loadFileAsData(data);

                return juce::WebBrowserComponent::Resource{
                    std::vector<std::byte>(
                        reinterpret_cast<const std::byte*>(data.getData()),
                        reinterpret_cast<const std::byte*>(data.getData()) + data.getSize()),
                    mimeType.toStdString()
                };
            })
        // Register all relays
        .withOptionsFrom(*rippleRateRelay)
        .withOptionsFrom(*rippleMultiplyRelay)
        .withOptionsFrom(*rippleAmountRelay)
        .withOptionsFrom(*rippleWidthRelay)
        .withOptionsFrom(*rippleLowBypassRelay)
        .withOptionsFrom(*rippleHighBypassRelay)
        .withOptionsFrom(*rippleMixRelay)
        .withOptionsFrom(*reverbEnabledRelay)
        .withOptionsFrom(*reverbSizeRelay)
        .withOptionsFrom(*reverbDampingRelay)
        .withOptionsFrom(*reverbMixRelay)
        .withOptionsFrom(*lfo1RateRelay)
        .withOptionsFrom(*lfo1ShapeRelay)
        .withOptionsFrom(*lfo1PhaseRelay)
        .withOptionsFrom(*lfo2RateRelay)
        .withOptionsFrom(*lfo2ShapeRelay)
        .withOptionsFrom(*lfo2PhaseRelay)
        .withOptionsFrom(*lfo3RateRelay)
        .withOptionsFrom(*lfo3ShapeRelay)
        .withOptionsFrom(*lfo3PhaseRelay)
        .withOptionsFrom(*lfo4RateRelay)
        .withOptionsFrom(*lfo4ShapeRelay)
        .withOptionsFrom(*lfo4PhaseRelay)
        .withOptionsFrom(*mod1SourceRelay)
        .withOptionsFrom(*mod1TargetRelay)
        .withOptionsFrom(*mod1DepthRelay)
        .withOptionsFrom(*mod2SourceRelay)
        .withOptionsFrom(*mod2TargetRelay)
        .withOptionsFrom(*mod2DepthRelay)
        .withOptionsFrom(*mod3SourceRelay)
        .withOptionsFrom(*mod3TargetRelay)
        .withOptionsFrom(*mod3DepthRelay)
        .withOptionsFrom(*mod4SourceRelay)
        .withOptionsFrom(*mod4TargetRelay)
        .withOptionsFrom(*mod4DepthRelay)
        .withOptionsFrom(*bypassRelay)
        .withOptionsFrom(*inputGainRelay)
        .withOptionsFrom(*outputGainRelay)
        // Event listeners
        .withEventListener("activateLicense", [this](const juce::var& data) {
            handleActivateLicense(data);
        })
        .withEventListener("deactivateLicense", [this](const juce::var& data) {
            handleDeactivateLicense(data);
        })
        .withEventListener("getActivationStatus", [this](const juce::var&) {
            handleGetActivationStatus();
        })
        .withEventListener("randomize", [this](const juce::var&) {
            handleRandomize();
        })
        .withWinWebView2Options(
            juce::WebBrowserComponent::Options::WinWebView2()
                .withBackgroundColour(juce::Colour(0xff0a0a12))  // Dark synthwave background
                .withStatusBarDisabled()
                .withUserDataFolder(
                    juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("Ripple_WebView2")));

    webView = std::make_unique<juce::WebBrowserComponent>(options);
    addAndMakeVisible(*webView);

    // ===========================================================================
    // STEP 4: Load URL based on build mode
    // ===========================================================================
#if RIPPLE_DEV_MODE
    webView->goToURL(DEV_SERVER_URL);
#else
    webView->goToURL(webView->getResourceProviderRoot());
#endif
}

//==============================================================================
void RippleEditor::setupRelaysAndAttachments()
{
    auto& apvts = processorRef.getAPVTS();

    // Ripple Filter
    rippleRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::rippleRate), *rippleRateRelay, nullptr);
    rippleMultiplyAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::rippleMultiply), *rippleMultiplyRelay, nullptr);
    rippleAmountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::rippleAmount), *rippleAmountRelay, nullptr);
    rippleWidthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::rippleWidth), *rippleWidthRelay, nullptr);
    rippleLowBypassAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::rippleLowBypass), *rippleLowBypassRelay, nullptr);
    rippleHighBypassAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::rippleHighBypass), *rippleHighBypassRelay, nullptr);
    rippleMixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::rippleMix), *rippleMixRelay, nullptr);

    // Reverb
    reverbEnabledAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter(ParamIDs::reverbEnabled), *reverbEnabledRelay, nullptr);
    reverbSizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::reverbSize), *reverbSizeRelay, nullptr);
    reverbDampingAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::reverbDamping), *reverbDampingRelay, nullptr);
    reverbMixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::reverbMix), *reverbMixRelay, nullptr);

    // LFO 1
    lfo1RateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::lfo1Rate), *lfo1RateRelay, nullptr);
    lfo1ShapeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter(ParamIDs::lfo1Shape), *lfo1ShapeRelay, nullptr);
    lfo1PhaseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::lfo1Phase), *lfo1PhaseRelay, nullptr);

    // LFO 2
    lfo2RateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::lfo2Rate), *lfo2RateRelay, nullptr);
    lfo2ShapeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter(ParamIDs::lfo2Shape), *lfo2ShapeRelay, nullptr);
    lfo2PhaseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::lfo2Phase), *lfo2PhaseRelay, nullptr);

    // LFO 3
    lfo3RateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::lfo3Rate), *lfo3RateRelay, nullptr);
    lfo3ShapeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter(ParamIDs::lfo3Shape), *lfo3ShapeRelay, nullptr);
    lfo3PhaseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::lfo3Phase), *lfo3PhaseRelay, nullptr);

    // LFO 4
    lfo4RateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::lfo4Rate), *lfo4RateRelay, nullptr);
    lfo4ShapeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter(ParamIDs::lfo4Shape), *lfo4ShapeRelay, nullptr);
    lfo4PhaseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::lfo4Phase), *lfo4PhaseRelay, nullptr);

    // Mod Slot 1
    mod1SourceAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter(ParamIDs::mod1Source), *mod1SourceRelay, nullptr);
    mod1TargetAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter(ParamIDs::mod1Target), *mod1TargetRelay, nullptr);
    mod1DepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::mod1Depth), *mod1DepthRelay, nullptr);

    // Mod Slot 2
    mod2SourceAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter(ParamIDs::mod2Source), *mod2SourceRelay, nullptr);
    mod2TargetAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter(ParamIDs::mod2Target), *mod2TargetRelay, nullptr);
    mod2DepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::mod2Depth), *mod2DepthRelay, nullptr);

    // Mod Slot 3
    mod3SourceAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter(ParamIDs::mod3Source), *mod3SourceRelay, nullptr);
    mod3TargetAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter(ParamIDs::mod3Target), *mod3TargetRelay, nullptr);
    mod3DepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::mod3Depth), *mod3DepthRelay, nullptr);

    // Mod Slot 4
    mod4SourceAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter(ParamIDs::mod4Source), *mod4SourceRelay, nullptr);
    mod4TargetAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter(ParamIDs::mod4Target), *mod4TargetRelay, nullptr);
    mod4DepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::mod4Depth), *mod4DepthRelay, nullptr);

    // Global
    bypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter(ParamIDs::bypass), *bypassRelay, nullptr);
    inputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::inputGain), *inputGainRelay, nullptr);
    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParamIDs::outputGain), *outputGainRelay, nullptr);
}

//==============================================================================
void RippleEditor::timerCallback()
{
    sendVisualizerData();
}

void RippleEditor::sendVisualizerData()
{
    if (!webView)
        return;

    juce::DynamicObject::Ptr data = new juce::DynamicObject();

    // Audio levels
    data->setProperty("inputLevel", processorRef.getInputLevel());
    data->setProperty("outputLevel", processorRef.getOutputLevel());

    // Ripple band magnitudes (16 bands)
    std::array<float, RippleProcessor::NUM_RIPPLE_BANDS> bandMagnitudes;
    processorRef.getRippleBandMagnitudes(bandMagnitudes.data());

    juce::Array<juce::var> bandsArray;
    for (int i = 0; i < RippleProcessor::NUM_RIPPLE_BANDS; ++i)
        bandsArray.add(bandMagnitudes[i]);
    data->setProperty("rippleBands", bandsArray);

    // LFO values
    juce::Array<juce::var> lfoArray;
    for (int i = 0; i < 4; ++i)
        lfoArray.add(processorRef.getLFOValue(i));
    data->setProperty("lfoValues", lfoArray);

    webView->emitEventIfBrowserIsVisible("visualizerData", juce::var(data.get()));
}

//==============================================================================
void RippleEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0a0a12));  // Dark synthwave background
}

void RippleEditor::resized()
{
    if (webView)
        webView->setBounds(getLocalBounds());
}

//==============================================================================
void RippleEditor::handleRandomize()
{
    processorRef.randomizeParameters();
}

//==============================================================================
// Activation Handlers
//==============================================================================

void RippleEditor::sendActivationState()
{
    if (!webView)
        return;

    juce::DynamicObject::Ptr data = new juce::DynamicObject();

#if BEATCONNECT_ACTIVATION_ENABLED
    auto* activation = processorRef.getActivation();

    bool isConfigured = activation != nullptr;
    bool isActivated = activation && activation->isActivated();

    data->setProperty("isConfigured", isConfigured);
    data->setProperty("isActivated", isActivated);

    if (isActivated && activation)
    {
        if (auto info = activation->getActivationInfo())
        {
            juce::DynamicObject::Ptr infoObj = new juce::DynamicObject();
            infoObj->setProperty("activationCode", juce::String(info->activationCode));
            infoObj->setProperty("machineId", juce::String(info->machineId));
            infoObj->setProperty("activatedAt", juce::String(info->activatedAt));
            infoObj->setProperty("currentActivations", info->currentActivations);
            infoObj->setProperty("maxActivations", info->maxActivations);
            infoObj->setProperty("isValid", info->isValid);
            data->setProperty("info", juce::var(infoObj.get()));
        }
    }
#else
    data->setProperty("isConfigured", false);
    data->setProperty("isActivated", true);
#endif

    webView->emitEventIfBrowserIsVisible("activationState", juce::var(data.get()));
}

void RippleEditor::handleActivateLicense(const juce::var& data)
{
#if BEATCONNECT_ACTIVATION_ENABLED
    juce::String code = data.getProperty("code", "").toString();
    if (code.isEmpty())
        return;

    juce::Component::SafePointer<RippleEditor> safeThis(this);

    auto* activation = processorRef.getActivation();
    if (!activation)
        return;

    activation->activateAsync(code.toStdString(),
        [safeThis](beatconnect::ActivationStatus status) {
            juce::MessageManager::callAsync([safeThis, status]() {
                if (!safeThis)
                    return;

                juce::DynamicObject::Ptr result = new juce::DynamicObject();

                juce::String statusStr;
                switch (status)
                {
                    case beatconnect::ActivationStatus::Valid:         statusStr = "valid"; break;
                    case beatconnect::ActivationStatus::Invalid:       statusStr = "invalid"; break;
                    case beatconnect::ActivationStatus::Revoked:       statusStr = "revoked"; break;
                    case beatconnect::ActivationStatus::MaxReached:    statusStr = "max_reached"; break;
                    case beatconnect::ActivationStatus::NetworkError:  statusStr = "network_error"; break;
                    case beatconnect::ActivationStatus::ServerError:   statusStr = "server_error"; break;
                    case beatconnect::ActivationStatus::NotConfigured: statusStr = "not_configured"; break;
                    case beatconnect::ActivationStatus::AlreadyActive: statusStr = "already_active"; break;
                    case beatconnect::ActivationStatus::NotActivated:  statusStr = "not_activated"; break;
                    default: statusStr = "unknown"; break;
                }
                result->setProperty("status", statusStr);

                if (status == beatconnect::ActivationStatus::Valid ||
                    status == beatconnect::ActivationStatus::AlreadyActive)
                {
                    auto* activation = safeThis->processorRef.getActivation();
                    if (activation)
                    {
                        if (auto info = activation->getActivationInfo())
                        {
                            juce::DynamicObject::Ptr infoObj = new juce::DynamicObject();
                            infoObj->setProperty("activationCode", juce::String(info->activationCode));
                            infoObj->setProperty("machineId", juce::String(info->machineId));
                            infoObj->setProperty("activatedAt", juce::String(info->activatedAt));
                            infoObj->setProperty("currentActivations", info->currentActivations);
                            infoObj->setProperty("maxActivations", info->maxActivations);
                            infoObj->setProperty("isValid", info->isValid);
                            result->setProperty("info", juce::var(infoObj.get()));
                        }
                    }
                }

                safeThis->webView->emitEventIfBrowserIsVisible("activationResult", juce::var(result.get()));
            });
        });
#else
    juce::ignoreUnused(data);
#endif
}

void RippleEditor::handleDeactivateLicense([[maybe_unused]] const juce::var& data)
{
#if BEATCONNECT_ACTIVATION_ENABLED
    juce::Component::SafePointer<RippleEditor> safeThis(this);

    auto* activation = processorRef.getActivation();
    if (!activation)
        return;

    std::thread([safeThis, activation]() {
        auto status = activation->deactivate();

        juce::MessageManager::callAsync([safeThis, status]() {
            if (!safeThis)
                return;

            juce::DynamicObject::Ptr result = new juce::DynamicObject();
            juce::String statusStr;
            switch (status)
            {
                case beatconnect::ActivationStatus::Valid:        statusStr = "valid"; break;
                case beatconnect::ActivationStatus::NetworkError: statusStr = "network_error"; break;
                case beatconnect::ActivationStatus::ServerError:  statusStr = "server_error"; break;
                case beatconnect::ActivationStatus::NotActivated: statusStr = "not_activated"; break;
                default: statusStr = "unknown"; break;
            }
            result->setProperty("status", statusStr);

            safeThis->webView->emitEventIfBrowserIsVisible("deactivationResult", juce::var(result.get()));
        });
    }).detach();
#endif
}

void RippleEditor::handleGetActivationStatus()
{
    sendActivationState();
}
