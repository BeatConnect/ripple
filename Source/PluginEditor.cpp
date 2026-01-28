/*
  ==============================================================================
    RIPPLE - Spectral Texture Engine Editor Implementation
  ==============================================================================
*/

#include "PluginEditor.h"

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
