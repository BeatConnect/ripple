/*
  ==============================================================================
    RIPPLE Texture Engine - Audio Processor
    Transform any sound into evolving hypnotic textures

    A modulation-driven audio effect built around a 16-band ripple filter system
    with algorithmic reverb for lush, evolving soundscapes.
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#if BEATCONNECT_ACTIVATION_ENABLED
#include <beatconnect/Activation.h>
#endif

//==============================================================================
class RippleProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    RippleProcessor();
    ~RippleProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Parameter Access
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    //==============================================================================
    // BeatConnect Integration
    juce::String getPluginId() const { return pluginId_; }
    juce::String getApiBaseUrl() const { return apiBaseUrl_; }
    juce::String getSupabaseKey() const { return supabasePublishableKey_; }
    bool hasActivationEnabled() const;

#if BEATCONNECT_ACTIVATION_ENABLED
    beatconnect::Activation* getActivation() { return activation_.get(); }
    const beatconnect::Activation* getActivation() const { return activation_.get(); }
    bool hasActivation() const { return activation_ != nullptr; }
#endif

    //==============================================================================
    // Visualizer Data
    float getInputLevel() const { return inputLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }

    // Ripple filter band magnitudes for visualization
    static constexpr int NUM_RIPPLE_BANDS = 16;
    void getRippleBandMagnitudes(float* magnitudes) const;

    // LFO values for visualization (0-1 normalized)
    float getLFOValue(int lfoIndex) const;

    //==============================================================================
    // Randomization
    void randomizeParameters();

private:
    //==============================================================================
    // Parameters
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    // BeatConnect project data
    void loadProjectData();
    juce::String pluginId_;
    juce::String apiBaseUrl_;
    juce::String supabasePublishableKey_;
    juce::var buildFlags_;

    //==============================================================================
    // Level metering
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };

    //==============================================================================
    // DSP Processing
    juce::dsp::ProcessSpec currentSpec;

    //==============================================================================
    // Ripple Filter: 16-band bandpass filter bank
    static constexpr std::array<float, NUM_RIPPLE_BANDS> bandFrequencies = {
        31.5f, 50.0f, 80.0f, 125.0f, 200.0f, 315.0f, 500.0f, 800.0f,
        1250.0f, 2000.0f, 3150.0f, 5000.0f, 8000.0f, 10000.0f, 12500.0f, 16000.0f
    };

    std::array<juce::dsp::IIR::Filter<float>, NUM_RIPPLE_BANDS> bandFiltersL;
    std::array<juce::dsp::IIR::Filter<float>, NUM_RIPPLE_BANDS> bandFiltersR;
    std::array<float, NUM_RIPPLE_BANDS> bandGains;  // Current band modulation gains
    std::atomic<float> rippleBandMagnitudes[NUM_RIPPLE_BANDS];

    float rippleWavePhase = 0.0f;

    //==============================================================================
    // Reverb (FDN-based)
    static constexpr int kFDNSize = 8;
    static constexpr float kFDNDelayMs[kFDNSize] = {
        29.7f, 37.1f, 41.1f, 43.7f, 53.0f, 59.9f, 67.7f, 79.3f
    };
    static constexpr float kHadamard = 0.35355339f;

    std::array<juce::AudioBuffer<float>, kFDNSize> fdnDelayLines;
    std::array<int, kFDNSize> fdnWritePos;
    std::array<int, kFDNSize> fdnDelaySamples;
    std::array<float, kFDNSize> fdnFilterState;

    juce::SmoothedValue<float> smoothedReverbMix;
    juce::SmoothedValue<float> smoothedReverbSize;
    juce::SmoothedValue<float> smoothedReverbDamping;

    //==============================================================================
    // LFO System (4 LFOs)
    std::array<float, 4> lfoPhases = { 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<std::atomic<float>, 4> lfoValues;

    float computeLFOValue(int lfoIndex, float phase);

    //==============================================================================
    // Modulation Matrix
    float getModulatedValue(const char* paramId, float baseValue);

    //==============================================================================
    // Smoothed parameter values
    juce::SmoothedValue<float> smoothedRippleRate;
    juce::SmoothedValue<float> smoothedRippleMultiply;
    juce::SmoothedValue<float> smoothedRippleAmount;
    juce::SmoothedValue<float> smoothedRippleWidth;
    juce::SmoothedValue<float> smoothedRippleMix;
    juce::SmoothedValue<float> smoothedInputGain;
    juce::SmoothedValue<float> smoothedOutputGain;

    //==============================================================================
    // Processing methods
    void updateRippleFilter();
    void processRippleFilter(juce::AudioBuffer<float>& buffer);
    void processReverb(juce::AudioBuffer<float>& buffer);
    void updateLFOs(int numSamples);

    float processAllpass(float input, float* buffer, int& writePos, int bufSize,
                         int delaySamples, float feedback);

    //==============================================================================
    // Random number generator for randomization
    juce::Random random;

#if BEATCONNECT_ACTIVATION_ENABLED
    std::unique_ptr<beatconnect::Activation> activation_;
#endif

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RippleProcessor)
};
