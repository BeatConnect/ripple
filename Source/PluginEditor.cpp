/*
  ==============================================================================
    RIPPLE - Spectral Texture Engine Editor Implementation
  ==============================================================================
*/

#include "PluginEditor.h"
#include <thread>

#if BEATCONNECT_ACTIVATION_ENABLED
#include <beatconnect/Activation.h>
#endif

//==============================================================================
RippleEditor::RippleEditor(RippleProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p)
{
    setupWebView();
    setupRelaysAndAttachments();

    // Force consistent scaling regardless of OS display scaling settings
    setScaleFactor(1.0f);

    setSize(1200, 700);  // Taller for spectral display
    setResizable(false, false);

    startTimerHz(30);  // 30 FPS for spectrum
}

RippleEditor::~RippleEditor()
{
    stopTimer();
}

//==============================================================================
void RippleEditor::setupWebView()
{
    // Create relays
    freezeRelay = std::make_unique<juce::WebSliderRelay>("freeze");
    smearRelay = std::make_unique<juce::WebSliderRelay>("smear");
    scatterRelay = std::make_unique<juce::WebSliderRelay>("scatter");
    shiftRelay = std::make_unique<juce::WebSliderRelay>("shift");
    tiltRelay = std::make_unique<juce::WebSliderRelay>("tilt");
    feedbackRelay = std::make_unique<juce::WebSliderRelay>("feedback");

    // Get resources directory
    auto executableFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    auto executableDir = executableFile.getParentDirectory();

    resourcesDir = executableDir.getChildFile("Resources").getChildFile("WebUI");
    if (!resourcesDir.isDirectory())
        resourcesDir = executableDir.getParentDirectory()
                           .getChildFile("Resources")
                           .getChildFile("WebUI");

    // Build WebView
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
                else if (path.endsWith(".svg")) mimeType = "image/svg+xml";

                juce::MemoryBlock data;
                file.loadFileAsData(data);

                return juce::WebBrowserComponent::Resource{
                    std::vector<std::byte>(
                        reinterpret_cast<const std::byte*>(data.getData()),
                        reinterpret_cast<const std::byte*>(data.getData()) + data.getSize()),
                    mimeType.toStdString()
                };
            })
        .withOptionsFrom(*freezeRelay)
        .withOptionsFrom(*smearRelay)
        .withOptionsFrom(*scatterRelay)
        .withOptionsFrom(*shiftRelay)
        .withOptionsFrom(*tiltRelay)
        .withOptionsFrom(*feedbackRelay)
        .withEventListener("interaction", [this](const juce::var& data) {
            handleInteraction(data);
        })
        .withEventListener("freeze_change", [this](const juce::var& data) {
            float value = static_cast<float>(data.getProperty("value", 0.0));
            if (auto* param = processorRef.getAPVTS().getParameter("freeze"))
                param->setValueNotifyingHost(value);
        })
        .withEventListener("smear_change", [this](const juce::var& data) {
            float value = static_cast<float>(data.getProperty("value", 0.0));
            if (auto* param = processorRef.getAPVTS().getParameter("smear"))
                param->setValueNotifyingHost(value);
        })
        .withEventListener("scatter_change", [this](const juce::var& data) {
            float value = static_cast<float>(data.getProperty("value", 0.0));
            if (auto* param = processorRef.getAPVTS().getParameter("scatter"))
                param->setValueNotifyingHost(value);
        })
        .withEventListener("shift_change", [this](const juce::var& data) {
            float value = static_cast<float>(data.getProperty("value", 0.0));
            // Convert 0-1 to -1 to +1
            float bipolar = value * 2.0f - 1.0f;
            if (auto* param = processorRef.getAPVTS().getParameter("shift"))
                param->setValueNotifyingHost(value);
        })
        .withEventListener("tilt_change", [this](const juce::var& data) {
            float value = static_cast<float>(data.getProperty("value", 0.0));
            if (auto* param = processorRef.getAPVTS().getParameter("tilt"))
                param->setValueNotifyingHost(value);
        })
        .withEventListener("feedback_change", [this](const juce::var& data) {
            float value = static_cast<float>(data.getProperty("value", 0.0));
            if (auto* param = processorRef.getAPVTS().getParameter("feedback"))
                param->setValueNotifyingHost(value);
        })
#if BEATCONNECT_ACTIVATION_ENABLED
        .withEventListener("activateLicense", [this](const juce::var& data) {
            handleActivateLicense(data);
        })
        .withEventListener("deactivateLicense", [this](const juce::var& data) {
            handleDeactivateLicense(data);
        })
        .withEventListener("getActivationStatus", [this](const juce::var&) {
            handleGetActivationStatus();
        })
#endif
        .withWinWebView2Options(
            juce::WebBrowserComponent::Options::WinWebView2()
                .withBackgroundColour(juce::Colour(0xff08080c))
                .withStatusBarDisabled()
                .withUserDataFolder(
                    juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("Ripple_WebView2")));

    webView = std::make_unique<juce::WebBrowserComponent>(options);
    addAndMakeVisible(*webView);

    webView->goToURL(webView->getResourceProviderRoot());
}

//==============================================================================
void RippleEditor::setupRelaysAndAttachments()
{
    auto& apvts = processorRef.getAPVTS();

    freezeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("freeze"), *freezeRelay, nullptr);
    smearAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("smear"), *smearRelay, nullptr);
    scatterAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("scatter"), *scatterRelay, nullptr);
    shiftAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("shift"), *shiftRelay, nullptr);
    tiltAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("tilt"), *tiltRelay, nullptr);
    feedbackAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("feedback"), *feedbackRelay, nullptr);
}

//==============================================================================
void RippleEditor::timerCallback()
{
    sendSpectrumData();
}

void RippleEditor::sendSpectrumData()
{
    if (!webView)
        return;

    juce::DynamicObject::Ptr data = new juce::DynamicObject();

    // Audio levels
    data->setProperty("inputLevel", processorRef.getInputLevel());
    data->setProperty("outputLevel", processorRef.getOutputLevel());

    // Spectrum data (downsample for performance)
    constexpr int SEND_BINS = 256;  // Send fewer bins for performance
    std::vector<float> spectrum(RippleProcessor::SPECTRUM_SIZE);
    processorRef.getSpectrum(spectrum.data());

    juce::Array<juce::var> spectrumArray;
    int step = RippleProcessor::SPECTRUM_SIZE / SEND_BINS;
    for (int i = 0; i < SEND_BINS; ++i)
    {
        // Average bins for this step
        float sum = 0.0f;
        for (int j = 0; j < step; ++j)
            sum += spectrum[i * step + j];
        spectrumArray.add(sum / step);
    }
    data->setProperty("spectrum", spectrumArray);

    // Frozen spectrum
    std::vector<float> frozen(RippleProcessor::SPECTRUM_SIZE);
    processorRef.getFrozenSpectrum(frozen.data());

    juce::Array<juce::var> frozenArray;
    for (int i = 0; i < SEND_BINS; ++i)
    {
        float sum = 0.0f;
        for (int j = 0; j < step; ++j)
            sum += frozen[i * step + j];
        frozenArray.add(sum / step);
    }
    data->setProperty("frozen", frozenArray);

    webView->emitEventIfBrowserIsVisible("spectrumData", juce::var(data.get()));
}

void RippleEditor::handleInteraction(const juce::var& data)
{
    float y = static_cast<float>(data.getProperty("y", 0.5));
    float radius = static_cast<float>(data.getProperty("radius", 0.2));
    bool active = static_cast<bool>(data.getProperty("active", false));

    DBG("Interaction: y=" << y << " radius=" << radius << " active=" << (active ? "true" : "false"));

    processorRef.setInteraction(y, radius, active);
}

#if BEATCONNECT_ACTIVATION_ENABLED
void RippleEditor::sendActivationState()
{
    if (webView == nullptr)
        return;

    auto* activation = processorRef.getActivation();
    juce::DynamicObject::Ptr data = new juce::DynamicObject();

    bool isConfigured = (activation != nullptr);
    bool isActivated = isConfigured && activation->isActivated();

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

    webView->emitEventIfBrowserIsVisible("activationState", juce::var(data.get()));
}

void RippleEditor::handleActivateLicense(const juce::var& data)
{
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
                if (safeThis == nullptr || safeThis->webView == nullptr)
                    return;

                juce::DynamicObject::Ptr result = new juce::DynamicObject();

                juce::String statusStr;
                switch (status) {
                    case beatconnect::ActivationStatus::Valid:         statusStr = "valid"; break;
                    case beatconnect::ActivationStatus::Invalid:       statusStr = "invalid"; break;
                    case beatconnect::ActivationStatus::Revoked:       statusStr = "revoked"; break;
                    case beatconnect::ActivationStatus::MaxReached:    statusStr = "max_reached"; break;
                    case beatconnect::ActivationStatus::NetworkError:  statusStr = "network_error"; break;
                    case beatconnect::ActivationStatus::ServerError:   statusStr = "server_error"; break;
                    case beatconnect::ActivationStatus::NotConfigured: statusStr = "not_configured"; break;
                    case beatconnect::ActivationStatus::AlreadyActive: statusStr = "already_active"; break;
                    case beatconnect::ActivationStatus::NotActivated:  statusStr = "not_activated"; break;
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
}

void RippleEditor::handleDeactivateLicense(const juce::var&)
{
    juce::Component::SafePointer<RippleEditor> safeThis(this);

    auto* activation = processorRef.getActivation();
    if (!activation)
        return;

    std::thread([safeThis, activation]() {
        auto status = activation->deactivate();

        juce::MessageManager::callAsync([safeThis, status]() {
            if (safeThis == nullptr || safeThis->webView == nullptr)
                return;

            juce::DynamicObject::Ptr result = new juce::DynamicObject();

            juce::String statusStr;
            switch (status) {
                case beatconnect::ActivationStatus::Valid:         statusStr = "valid"; break;
                case beatconnect::ActivationStatus::NetworkError:  statusStr = "network_error"; break;
                case beatconnect::ActivationStatus::ServerError:   statusStr = "server_error"; break;
                case beatconnect::ActivationStatus::NotActivated:  statusStr = "not_activated"; break;
                default: statusStr = "server_error"; break;
            }
            result->setProperty("status", statusStr);

            safeThis->webView->emitEventIfBrowserIsVisible("deactivationResult", juce::var(result.get()));
        });
    }).detach();
}

void RippleEditor::handleGetActivationStatus()
{
    sendActivationState();
}
#endif

//==============================================================================
void RippleEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff08080c));
}

void RippleEditor::resized()
{
    if (webView)
        webView->setBounds(getLocalBounds());
}
