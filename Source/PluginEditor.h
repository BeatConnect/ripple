/*
  ==============================================================================
    RIPPLE - Spectral Texture Engine Editor
    Interactive spectral visualization with direct manipulation
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

//==============================================================================
class RippleEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit RippleEditor(RippleProcessor&);
    ~RippleEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    RippleProcessor& processorRef;

    std::unique_ptr<juce::WebBrowserComponent> webView;
    juce::File resourcesDir;

    // Parameter relays
    std::unique_ptr<juce::WebSliderRelay> freezeRelay;
    std::unique_ptr<juce::WebSliderRelay> smearRelay;
    std::unique_ptr<juce::WebSliderRelay> scatterRelay;
    std::unique_ptr<juce::WebSliderRelay> shiftRelay;
    std::unique_ptr<juce::WebSliderRelay> tiltRelay;
    std::unique_ptr<juce::WebSliderRelay> feedbackRelay;

    // Parameter attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> freezeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> smearAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> scatterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> shiftAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> tiltAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> feedbackAttachment;

    void setupWebView();
    void setupRelaysAndAttachments();
    void sendSpectrumData();
    void handleInteraction(const juce::var& data);

#if BEATCONNECT_ACTIVATION_ENABLED
    void sendActivationState();
    void handleActivateLicense(const juce::var& data);
    void handleDeactivateLicense(const juce::var& data);
    void handleGetActivationStatus();
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RippleEditor)
};
