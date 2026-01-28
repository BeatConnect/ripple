/*
  ==============================================================================
    RIPPLE Texture Engine - Plugin Editor (JUCE 8 WebView UI)
    Transform any sound into evolving hypnotic textures

    Uses JUCE 8's native WebView relay system for bidirectional parameter sync.
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

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    //==============================================================================
    RippleProcessor& processorRef;

    //==============================================================================
    // WebView component
    std::unique_ptr<juce::WebBrowserComponent> webView;
    juce::File resourcesDir;

    //==============================================================================
    // JUCE 8 Parameter Relays - Ripple Filter
    std::unique_ptr<juce::WebSliderRelay> rippleRateRelay;
    std::unique_ptr<juce::WebSliderRelay> rippleMultiplyRelay;
    std::unique_ptr<juce::WebSliderRelay> rippleAmountRelay;
    std::unique_ptr<juce::WebSliderRelay> rippleWidthRelay;
    std::unique_ptr<juce::WebSliderRelay> rippleLowBypassRelay;
    std::unique_ptr<juce::WebSliderRelay> rippleHighBypassRelay;
    std::unique_ptr<juce::WebSliderRelay> rippleMixRelay;

    // Reverb
    std::unique_ptr<juce::WebToggleButtonRelay> reverbEnabledRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbSizeRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbDampingRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbMixRelay;

    // LFOs (4)
    std::unique_ptr<juce::WebSliderRelay> lfo1RateRelay;
    std::unique_ptr<juce::WebComboBoxRelay> lfo1ShapeRelay;
    std::unique_ptr<juce::WebSliderRelay> lfo1PhaseRelay;

    std::unique_ptr<juce::WebSliderRelay> lfo2RateRelay;
    std::unique_ptr<juce::WebComboBoxRelay> lfo2ShapeRelay;
    std::unique_ptr<juce::WebSliderRelay> lfo2PhaseRelay;

    std::unique_ptr<juce::WebSliderRelay> lfo3RateRelay;
    std::unique_ptr<juce::WebComboBoxRelay> lfo3ShapeRelay;
    std::unique_ptr<juce::WebSliderRelay> lfo3PhaseRelay;

    std::unique_ptr<juce::WebSliderRelay> lfo4RateRelay;
    std::unique_ptr<juce::WebComboBoxRelay> lfo4ShapeRelay;
    std::unique_ptr<juce::WebSliderRelay> lfo4PhaseRelay;

    // Modulation Matrix (4 slots)
    std::unique_ptr<juce::WebComboBoxRelay> mod1SourceRelay;
    std::unique_ptr<juce::WebComboBoxRelay> mod1TargetRelay;
    std::unique_ptr<juce::WebSliderRelay> mod1DepthRelay;

    std::unique_ptr<juce::WebComboBoxRelay> mod2SourceRelay;
    std::unique_ptr<juce::WebComboBoxRelay> mod2TargetRelay;
    std::unique_ptr<juce::WebSliderRelay> mod2DepthRelay;

    std::unique_ptr<juce::WebComboBoxRelay> mod3SourceRelay;
    std::unique_ptr<juce::WebComboBoxRelay> mod3TargetRelay;
    std::unique_ptr<juce::WebSliderRelay> mod3DepthRelay;

    std::unique_ptr<juce::WebComboBoxRelay> mod4SourceRelay;
    std::unique_ptr<juce::WebComboBoxRelay> mod4TargetRelay;
    std::unique_ptr<juce::WebSliderRelay> mod4DepthRelay;

    // Global
    std::unique_ptr<juce::WebToggleButtonRelay> bypassRelay;
    std::unique_ptr<juce::WebSliderRelay> inputGainRelay;
    std::unique_ptr<juce::WebSliderRelay> outputGainRelay;

    //==============================================================================
    // Parameter Attachments - Ripple Filter
    std::unique_ptr<juce::WebSliderParameterAttachment> rippleRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> rippleMultiplyAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> rippleAmountAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> rippleWidthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> rippleLowBypassAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> rippleHighBypassAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> rippleMixAttachment;

    // Reverb
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> reverbEnabledAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbSizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbDampingAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbMixAttachment;

    // LFOs
    std::unique_ptr<juce::WebSliderParameterAttachment> lfo1RateAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> lfo1ShapeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lfo1PhaseAttachment;

    std::unique_ptr<juce::WebSliderParameterAttachment> lfo2RateAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> lfo2ShapeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lfo2PhaseAttachment;

    std::unique_ptr<juce::WebSliderParameterAttachment> lfo3RateAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> lfo3ShapeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lfo3PhaseAttachment;

    std::unique_ptr<juce::WebSliderParameterAttachment> lfo4RateAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> lfo4ShapeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lfo4PhaseAttachment;

    // Modulation Matrix
    std::unique_ptr<juce::WebComboBoxParameterAttachment> mod1SourceAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> mod1TargetAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mod1DepthAttachment;

    std::unique_ptr<juce::WebComboBoxParameterAttachment> mod2SourceAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> mod2TargetAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mod2DepthAttachment;

    std::unique_ptr<juce::WebComboBoxParameterAttachment> mod3SourceAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> mod3TargetAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mod3DepthAttachment;

    std::unique_ptr<juce::WebComboBoxParameterAttachment> mod4SourceAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> mod4TargetAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mod4DepthAttachment;

    // Global
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> bypassAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> inputGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;

    //==============================================================================
    void setupWebView();
    void setupRelaysAndAttachments();
    void sendVisualizerData();

    //==============================================================================
    // Activation handlers
    void sendActivationState();
    void handleActivateLicense(const juce::var& data);
    void handleDeactivateLicense(const juce::var& data);
    void handleGetActivationStatus();

    // Randomization handler
    void handleRandomize();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RippleEditor)
};
