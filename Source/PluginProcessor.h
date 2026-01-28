/*
  ==============================================================================
    RIPPLE - Spectral Texture Engine
    Interactive spectral freeze, smear, and manipulation
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "SpectralProcessor.h"

#if BEATCONNECT_ACTIVATION_ENABLED
#include <beatconnect/Activation.h>
#endif

//==============================================================================
class RippleProcessor : public juce::AudioProcessor
{
public:
    RippleProcessor();
    ~RippleProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // BeatConnect
    juce::String getPluginId() const { return pluginId_; }
    juce::String getApiBaseUrl() const { return apiBaseUrl_; }
    juce::String getSupabaseKey() const { return supabasePublishableKey_; }
    bool hasActivationEnabled() const;

#if BEATCONNECT_ACTIVATION_ENABLED
    beatconnect::Activation* getActivation() { return activation_.get(); }
    const beatconnect::Activation* getActivation() const { return activation_.get(); }
    bool hasActivation() const { return activation_ != nullptr; }
#endif

    // Metering
    float getInputLevel() const { return inputLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }

    // Spectral data for visualization
    static constexpr int SPECTRUM_SIZE = SpectralProcessor::NUM_BINS;
    void getSpectrum(float* magnitudes) const;
    void getFrozenSpectrum(float* magnitudes) const;

    // Interaction from UI
    void setInteraction(float y, float radius, bool active);

private:
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void loadProjectData();
    juce::String pluginId_;
    juce::String apiBaseUrl_;
    juce::String supabasePublishableKey_;
    juce::var buildFlags_;

    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };

    // Spectral processing
    SpectralProcessor spectralProcessor;

    // Simple reverb for added space
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters reverbParams;

    juce::Random random;

#if BEATCONNECT_ACTIVATION_ENABLED
    std::unique_ptr<beatconnect::Activation> activation_;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RippleProcessor)
};
