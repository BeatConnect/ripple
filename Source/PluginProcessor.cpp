/*
  ==============================================================================
    RIPPLE Texture Engine - Audio Processor Implementation
    Transform any sound into evolving hypnotic textures
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.h"

#if HAS_PROJECT_DATA
#include "ProjectData.h"
#endif

#if BEATCONNECT_ACTIVATION_ENABLED
#include <beatconnect/Activation.h>
#endif

// State version - increment when making breaking parameter changes
static constexpr int kStateVersion = 1;

//==============================================================================
RippleProcessor::RippleProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    loadProjectData();

    // Initialize band gains to neutral
    for (int i = 0; i < NUM_RIPPLE_BANDS; ++i)
    {
        bandGains[i] = 1.0f;
        rippleBandMagnitudes[i].store(1.0f);
    }

    // Initialize LFO values
    for (int i = 0; i < 4; ++i)
        lfoValues[i].store(0.5f);
}

RippleProcessor::~RippleProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout RippleProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // ===========================================================================
    // Ripple Filter Parameters
    // ===========================================================================
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::rippleRate, 1 },
        "Ripple Rate",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.3f),
        0.25f  // Slow, hypnotic default
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::rippleMultiply, 1 },
        "Ripple Multiply",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
        1.0f
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::rippleAmount, 1 },
        "Ripple Amount",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
        0.5f
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::rippleWidth, 1 },
        "Ripple Width",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
        1.5f  // Wide stereo field
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::rippleLowBypass, 1 },
        "Ripple Low Bypass",
        juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.5f),
        80.0f  // Protect sub bass
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::rippleHighBypass, 1 },
        "Ripple High Bypass",
        juce::NormalisableRange<float>(1000.0f, 20000.0f, 1.0f, 0.5f),
        16000.0f
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::rippleMix, 1 },
        "Ripple Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        1.0f  // Full wet by default
    ));

    // ===========================================================================
    // Reverb Parameters
    // ===========================================================================
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::reverbEnabled, 1 },
        "Reverb",
        true
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::reverbSize, 1 },
        "Reverb Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::reverbDamping, 1 },
        "Reverb Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::reverbMix, 1 },
        "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f
    ));

    // ===========================================================================
    // LFO Parameters (4 LFOs)
    // ===========================================================================
    for (int i = 1; i <= 4; ++i)
    {
        juce::String rateId = "lfo" + juce::String(i) + "_rate";
        juce::String shapeId = "lfo" + juce::String(i) + "_shape";
        juce::String phaseId = "lfo" + juce::String(i) + "_phase";

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { rateId, 1 },
            "LFO " + juce::String(i) + " Rate",
            juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.3f),
            0.5f + (i - 1) * 0.2f  // Slightly different default rates
        ));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { shapeId, 1 },
            "LFO " + juce::String(i) + " Shape",
            LFOShapes::shapes,
            0  // Sine default
        ));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { phaseId, 1 },
            "LFO " + juce::String(i) + " Phase",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
            (i - 1) * 0.25f  // Offset phases for variety
        ));
    }

    // ===========================================================================
    // Modulation Matrix (4 Slots)
    // ===========================================================================
    for (int i = 1; i <= 4; ++i)
    {
        juce::String sourceId = "mod" + juce::String(i) + "_source";
        juce::String targetId = "mod" + juce::String(i) + "_target";
        juce::String depthId = "mod" + juce::String(i) + "_depth";

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { sourceId, 1 },
            "Mod " + juce::String(i) + " Source",
            ModSources::sources,
            i  // Default: each slot uses corresponding LFO (1=LFO1, 2=LFO2, etc.)
        ));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { targetId, 1 },
            "Mod " + juce::String(i) + " Target",
            ModTargets::targets,
            0  // None by default
        ));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { depthId, 1 },
            "Mod " + juce::String(i) + " Depth",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f),
            0.0f
        ));
    }

    // ===========================================================================
    // Global Parameters
    // ===========================================================================
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::bypass, 1 },
        "Bypass",
        false
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::inputGain, 1 },
        "Input Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::outputGain, 1 },
        "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f
    ));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { ParamIDs::randomSeed, 1 },
        "Random Seed",
        0, 999999,
        0  // Not automatable, just for reproducibility
    ));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String RippleProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RippleProcessor::acceptsMidi() const { return false; }
bool RippleProcessor::producesMidi() const { return false; }
bool RippleProcessor::isMidiEffect() const { return false; }
double RippleProcessor::getTailLengthSeconds() const { return 3.0; }

int RippleProcessor::getNumPrograms() { return 1; }
int RippleProcessor::getCurrentProgram() { return 0; }
void RippleProcessor::setCurrentProgram(int) {}
const juce::String RippleProcessor::getProgramName(int) { return {}; }
void RippleProcessor::changeProgramName(int, const juce::String&) {}

//==============================================================================
void RippleProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSpec.sampleRate = sampleRate;
    currentSpec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock * 2);  // Headroom
    currentSpec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Initialize smoothed values
    smoothedRippleRate.reset(sampleRate, 0.05);
    smoothedRippleMultiply.reset(sampleRate, 0.02);
    smoothedRippleAmount.reset(sampleRate, 0.02);
    smoothedRippleWidth.reset(sampleRate, 0.02);
    smoothedRippleMix.reset(sampleRate, 0.02);
    smoothedInputGain.reset(sampleRate, 0.02);
    smoothedOutputGain.reset(sampleRate, 0.02);

    smoothedReverbMix.reset(sampleRate, 0.05);
    smoothedReverbSize.reset(sampleRate, 0.05);
    smoothedReverbDamping.reset(sampleRate, 0.05);

    // Initialize ripple filter bands
    for (int i = 0; i < NUM_RIPPLE_BANDS; ++i)
    {
        bandFiltersL[i].prepare(currentSpec);
        bandFiltersR[i].prepare(currentSpec);

        // Create bandpass filter at band frequency
        float freq = bandFrequencies[i];
        float q = 2.0f;  // Moderate Q for musical response
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, freq, q);
        *bandFiltersL[i].coefficients = *coeffs;
        *bandFiltersR[i].coefficients = *coeffs;
    }

    // Initialize FDN reverb
    float srScale = static_cast<float>(sampleRate) / 44100.0f;
    int maxFDNSize = static_cast<int>(100.0f * sampleRate / 1000.0f) + 512;

    for (int i = 0; i < kFDNSize; ++i)
    {
        fdnDelayLines[i].setSize(1, maxFDNSize);
        fdnDelayLines[i].clear();
        fdnWritePos[i] = 0;
        fdnDelaySamples[i] = static_cast<int>(kFDNDelayMs[i] * sampleRate / 1000.0f);
        fdnFilterState[i] = 0.0f;
    }
}

void RippleProcessor::releaseResources()
{
    for (int i = 0; i < kFDNSize; ++i)
        fdnDelayLines[i].clear();
}

bool RippleProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

//==============================================================================
float RippleProcessor::computeLFOValue(int lfoIndex, float phase)
{
    if (lfoIndex < 0 || lfoIndex >= 4)
        return 0.5f;

    juce::String shapeId = "lfo" + juce::String(lfoIndex + 1) + "_shape";
    int shape = static_cast<int>(apvts.getRawParameterValue(shapeId)->load());

    float value = 0.0f;

    switch (shape)
    {
        case 0:  // Sine
            value = 0.5f + 0.5f * std::sin(phase * juce::MathConstants<float>::twoPi);
            break;

        case 1:  // Triangle
            value = (phase < 0.5f) ? (phase * 2.0f) : (2.0f - phase * 2.0f);
            break;

        case 2:  // Square
            value = (phase < 0.5f) ? 1.0f : 0.0f;
            break;

        case 3:  // Saw Up
            value = phase;
            break;

        case 4:  // Saw Down
            value = 1.0f - phase;
            break;

        case 5:  // Random (sample & hold)
            value = random.nextFloat();
            break;

        default:
            value = 0.5f;
    }

    return value;
}

void RippleProcessor::updateLFOs(int numSamples)
{
    auto sampleRate = currentSpec.sampleRate;
    if (sampleRate <= 0.0)
        return;

    for (int i = 0; i < 4; ++i)
    {
        juce::String rateId = "lfo" + juce::String(i + 1) + "_rate";
        juce::String phaseId = "lfo" + juce::String(i + 1) + "_phase";

        float rate = apvts.getRawParameterValue(rateId)->load();
        float phaseOffset = apvts.getRawParameterValue(phaseId)->load();

        float phaseInc = rate / static_cast<float>(sampleRate) * numSamples;
        lfoPhases[i] += phaseInc;
        while (lfoPhases[i] >= 1.0f)
            lfoPhases[i] -= 1.0f;

        float effectivePhase = lfoPhases[i] + phaseOffset;
        while (effectivePhase >= 1.0f)
            effectivePhase -= 1.0f;

        float value = computeLFOValue(i, effectivePhase);
        lfoValues[i].store(value);
    }
}

float RippleProcessor::getModulatedValue(const char* paramId, float baseValue)
{
    float modulation = 0.0f;

    // Check all 4 mod slots
    for (int i = 1; i <= 4; ++i)
    {
        juce::String sourceId = "mod" + juce::String(i) + "_source";
        juce::String targetId = "mod" + juce::String(i) + "_target";
        juce::String depthId = "mod" + juce::String(i) + "_depth";

        int sourceIdx = static_cast<int>(apvts.getRawParameterValue(sourceId)->load());
        int targetIdx = static_cast<int>(apvts.getRawParameterValue(targetId)->load());
        float depth = apvts.getRawParameterValue(depthId)->load();

        if (sourceIdx == 0 || targetIdx == 0 || std::abs(depth) < 0.001f)
            continue;

        // Map target index to parameter ID
        const char* targetParamId = nullptr;
        switch (targetIdx)
        {
            case 1: targetParamId = ParamIDs::rippleRate; break;
            case 2: targetParamId = ParamIDs::rippleMultiply; break;
            case 3: targetParamId = ParamIDs::rippleAmount; break;
            case 4: targetParamId = ParamIDs::rippleWidth; break;
            case 5: targetParamId = ParamIDs::rippleLowBypass; break;
            case 6: targetParamId = ParamIDs::rippleHighBypass; break;
            case 7: targetParamId = ParamIDs::rippleMix; break;
            case 8: targetParamId = ParamIDs::reverbSize; break;
            case 9: targetParamId = ParamIDs::reverbDamping; break;
            case 10: targetParamId = ParamIDs::reverbMix; break;
            default: break;
        }

        if (targetParamId == nullptr || std::strcmp(targetParamId, paramId) != 0)
            continue;

        // Get LFO value (0-1) and convert to bipolar (-1 to +1)
        float lfoValue = lfoValues[sourceIdx - 1].load();
        float bipolarLfo = (lfoValue - 0.5f) * 2.0f;

        modulation += bipolarLfo * depth;
    }

    return baseValue + modulation * baseValue;
}

//==============================================================================
void RippleProcessor::updateRippleFilter()
{
    float rate = apvts.getRawParameterValue(ParamIDs::rippleRate)->load();
    float multiply = apvts.getRawParameterValue(ParamIDs::rippleMultiply)->load();
    float amount = apvts.getRawParameterValue(ParamIDs::rippleAmount)->load();
    float lowBypass = apvts.getRawParameterValue(ParamIDs::rippleLowBypass)->load();
    float highBypass = apvts.getRawParameterValue(ParamIDs::rippleHighBypass)->load();

    // Apply modulation
    rate = getModulatedValue(ParamIDs::rippleRate, rate);
    multiply = getModulatedValue(ParamIDs::rippleMultiply, multiply);
    amount = getModulatedValue(ParamIDs::rippleAmount, amount);

    // Clamp modulated values
    rate = juce::jlimit(0.01f, 20.0f, rate);
    multiply = juce::jlimit(0.0f, 4.0f, multiply);
    amount = juce::jlimit(0.0f, 4.0f, amount);

    // Calculate wave position for each band
    for (int i = 0; i < NUM_RIPPLE_BANDS; ++i)
    {
        float bandFreq = bandFrequencies[i];

        // Check bypass ranges
        if (bandFreq < lowBypass || bandFreq > highBypass)
        {
            bandGains[i] = 1.0f;  // Bypass this band
            rippleBandMagnitudes[i].store(1.0f);
            continue;
        }

        // Calculate band position in wave (0-1 based on frequency)
        float freqNormalized = std::log2(bandFreq / 20.0f) / std::log2(20000.0f / 20.0f);

        // Primary wave
        float wavePos = rippleWavePhase + freqNormalized * multiply;
        float primaryWave = std::sin(wavePos * juce::MathConstants<float>::twoPi);

        // Secondary ripple (wave within wave)
        float secondaryWave = std::sin(wavePos * juce::MathConstants<float>::twoPi * 3.0f) * amount;

        // Combine waves (0-1 range)
        float combinedWave = 0.5f + 0.5f * (primaryWave * 0.7f + secondaryWave * 0.3f);

        // Apply gain modulation
        bandGains[i] = combinedWave;
        rippleBandMagnitudes[i].store(combinedWave);
    }
}

void RippleProcessor::processRippleFilter(juce::AudioBuffer<float>& buffer)
{
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    float rate = smoothedRippleRate.getNextValue();
    float width = smoothedRippleWidth.getNextValue();
    float mix = smoothedRippleMix.getNextValue();

    // Apply modulation
    width = getModulatedValue(ParamIDs::rippleWidth, width);
    mix = getModulatedValue(ParamIDs::rippleMix, mix);

    // Advance wave phase
    float phaseInc = rate / static_cast<float>(currentSpec.sampleRate) * numSamples;
    rippleWavePhase += phaseInc;
    while (rippleWavePhase >= 1.0f)
        rippleWavePhase -= 1.0f;

    // Update band gains
    updateRippleFilter();

    // Create output buffer
    juce::AudioBuffer<float> wetBuffer(numChannels, numSamples);
    wetBuffer.clear();

    // Process each band
    for (int band = 0; band < NUM_RIPPLE_BANDS; ++band)
    {
        float gain = bandGains[band];

        // Apply stereo width (offset phase for right channel)
        float gainL = gain;
        float gainR = gain;

        if (width > 1.0f && numChannels > 1)
        {
            // Offset the gain for right channel based on width
            int bandOffset = static_cast<int>((width - 1.0f) * 4.0f);  // Max 4 bands offset
            int rightBand = (band + bandOffset) % NUM_RIPPLE_BANDS;
            gainR = bandGains[rightBand];
        }

        // Filter each channel
        for (int sample = 0; sample < numSamples; ++sample)
        {
            if (numChannels > 0)
            {
                float inputL = buffer.getSample(0, sample);
                float filteredL = bandFiltersL[band].processSample(inputL) * gainL;
                wetBuffer.addSample(0, sample, filteredL);
            }

            if (numChannels > 1)
            {
                float inputR = buffer.getSample(1, sample);
                float filteredR = bandFiltersR[band].processSample(inputR) * gainR;
                wetBuffer.addSample(1, sample, filteredR);
            }
        }
    }

    // Mix dry/wet
    for (int channel = 0; channel < numChannels; ++channel)
    {
        float* dry = buffer.getWritePointer(channel);
        const float* wet = wetBuffer.getReadPointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            dry[sample] = dry[sample] * (1.0f - mix) + wet[sample] * mix;
        }
    }
}

float RippleProcessor::processAllpass(float input, float* buffer, int& writePos,
                                       int bufSize, int delaySamples, float feedback)
{
    delaySamples = juce::jmin(delaySamples, bufSize - 1);
    int readPos = writePos - delaySamples;
    if (readPos < 0)
        readPos += bufSize;

    float bufOut = buffer[readPos];
    float output = -input + bufOut;
    buffer[writePos] = input + bufOut * feedback;

    if (++writePos >= bufSize)
        writePos = 0;

    return output;
}

void RippleProcessor::processReverb(juce::AudioBuffer<float>& buffer)
{
    if (apvts.getRawParameterValue(ParamIDs::reverbEnabled)->load() < 0.5f)
        return;

    float size = apvts.getRawParameterValue(ParamIDs::reverbSize)->load();
    float damping = apvts.getRawParameterValue(ParamIDs::reverbDamping)->load();
    float mix = apvts.getRawParameterValue(ParamIDs::reverbMix)->load();

    // Apply modulation
    size = getModulatedValue(ParamIDs::reverbSize, size);
    damping = getModulatedValue(ParamIDs::reverbDamping, damping);
    mix = getModulatedValue(ParamIDs::reverbMix, mix);

    // Clamp
    size = juce::jlimit(0.0f, 1.0f, size);
    damping = juce::jlimit(0.0f, 1.0f, damping);
    mix = juce::jlimit(0.0f, 1.0f, mix);

    smoothedReverbSize.setTargetValue(size);
    smoothedReverbDamping.setTargetValue(damping);
    smoothedReverbMix.setTargetValue(mix);

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    float* outL = numChannels > 0 ? buffer.getWritePointer(0) : nullptr;
    float* outR = numChannels > 1 ? buffer.getWritePointer(1) : outL;

    for (int i = 0; i < numSamples; ++i)
    {
        float dryL = outL ? outL[i] : 0.0f;
        float dryR = outR ? outR[i] : 0.0f;

        float currentMix = smoothedReverbMix.getNextValue();
        float currentSize = smoothedReverbSize.getNextValue();
        float currentDamping = smoothedReverbDamping.getNextValue();

        // Calculate feedback from size
        float decayTime = 0.5f + currentSize * 4.5f;
        float avgDelayMs = 50.0f * currentSize;
        float feedback = std::pow(10.0f, -3.0f * (avgDelayMs / 1000.0f) / juce::jmax(decayTime, 0.1f));
        feedback = juce::jlimit(0.0f, 0.95f, feedback);

        // FDN processing
        std::array<float, kFDNSize> delayOuts;

        for (int j = 0; j < kFDNSize; ++j)
        {
            int bufSize = fdnDelayLines[j].getNumSamples();
            float* buf = fdnDelayLines[j].getWritePointer(0);

            int delaySamp = static_cast<int>(fdnDelaySamples[j] * currentSize);
            delaySamp = juce::jlimit(1, bufSize - 1, delaySamp);

            int readPos = fdnWritePos[j] - delaySamp;
            if (readPos < 0)
                readPos += bufSize;

            float out = buf[readPos];

            // Damping filter
            fdnFilterState[j] = out * (1.0f - currentDamping) + fdnFilterState[j] * currentDamping;
            delayOuts[j] = fdnFilterState[j];
        }

        // Hadamard mixing
        std::array<float, kFDNSize> mixed;
        for (int j = 0; j < kFDNSize; ++j)
        {
            mixed[j] = 0.0f;
            for (int k = 0; k < kFDNSize; ++k)
            {
                int popcount = 0;
                int val = j & k;
                while (val) { popcount += val & 1; val >>= 1; }
                float sign = (popcount % 2 == 0) ? 1.0f : -1.0f;
                mixed[j] += delayOuts[k] * sign;
            }
            mixed[j] *= kHadamard;
        }

        // Write back with input
        float inputL = dryL * 0.4f;
        float inputR = dryR * 0.4f;

        for (int j = 0; j < kFDNSize; ++j)
        {
            int bufSize = fdnDelayLines[j].getNumSamples();
            float* buf = fdnDelayLines[j].getWritePointer(0);

            float input = (j < 4) ? inputL : inputR;
            buf[fdnWritePos[j]] = input + mixed[j] * feedback;

            if (++fdnWritePos[j] >= bufSize)
                fdnWritePos[j] = 0;
        }

        // Output
        float wetL = (delayOuts[0] + delayOuts[2] + delayOuts[4] + delayOuts[6]) * 0.3f;
        float wetR = (delayOuts[1] + delayOuts[3] + delayOuts[5] + delayOuts[7]) * 0.3f;

        if (outL) outL[i] = dryL * (1.0f - currentMix) + wetL * currentMix;
        if (outR) outR[i] = dryR * (1.0f - currentMix) + wetR * currentMix;
    }
}

//==============================================================================
void RippleProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                    juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Calculate input level
    float inLevel = 0.0f;
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
        inLevel = std::max(inLevel, buffer.getMagnitude(channel, 0, buffer.getNumSamples()));
    inputLevel.store(inLevel);

    // Check bypass
    if (apvts.getRawParameterValue(ParamIDs::bypass)->load() > 0.5f)
    {
        outputLevel.store(inLevel);
        return;
    }

    // Update smoothed values
    smoothedRippleRate.setTargetValue(apvts.getRawParameterValue(ParamIDs::rippleRate)->load());
    smoothedRippleMultiply.setTargetValue(apvts.getRawParameterValue(ParamIDs::rippleMultiply)->load());
    smoothedRippleAmount.setTargetValue(apvts.getRawParameterValue(ParamIDs::rippleAmount)->load());
    smoothedRippleWidth.setTargetValue(apvts.getRawParameterValue(ParamIDs::rippleWidth)->load());
    smoothedRippleMix.setTargetValue(apvts.getRawParameterValue(ParamIDs::rippleMix)->load());

    float inputGainDb = apvts.getRawParameterValue(ParamIDs::inputGain)->load();
    float outputGainDb = apvts.getRawParameterValue(ParamIDs::outputGain)->load();
    smoothedInputGain.setTargetValue(juce::Decibels::decibelsToGain(inputGainDb));
    smoothedOutputGain.setTargetValue(juce::Decibels::decibelsToGain(outputGainDb));

    // Update LFOs
    updateLFOs(buffer.getNumSamples());

    // Apply input gain
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float gain = smoothedInputGain.getNextValue();
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
            buffer.getWritePointer(channel)[sample] *= gain;
    }

    // Process ripple filter
    processRippleFilter(buffer);

    // Process reverb
    processReverb(buffer);

    // Apply output gain
    smoothedOutputGain.setTargetValue(juce::Decibels::decibelsToGain(outputGainDb));
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float gain = smoothedOutputGain.getNextValue();
        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
            buffer.getWritePointer(channel)[sample] *= gain;
    }

    // Calculate output level
    float outLevel = 0.0f;
    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        outLevel = std::max(outLevel, buffer.getMagnitude(channel, 0, buffer.getNumSamples()));
    outputLevel.store(outLevel);
}

//==============================================================================
void RippleProcessor::getRippleBandMagnitudes(float* magnitudes) const
{
    for (int i = 0; i < NUM_RIPPLE_BANDS; ++i)
        magnitudes[i] = rippleBandMagnitudes[i].load();
}

float RippleProcessor::getLFOValue(int lfoIndex) const
{
    if (lfoIndex >= 0 && lfoIndex < 4)
        return lfoValues[lfoIndex].load();
    return 0.5f;
}

//==============================================================================
void RippleProcessor::randomizeParameters()
{
    // Randomize ripple parameters
    apvts.getParameter(ParamIDs::rippleRate)->setValueNotifyingHost(random.nextFloat());
    apvts.getParameter(ParamIDs::rippleMultiply)->setValueNotifyingHost(random.nextFloat());
    apvts.getParameter(ParamIDs::rippleAmount)->setValueNotifyingHost(random.nextFloat());
    apvts.getParameter(ParamIDs::rippleWidth)->setValueNotifyingHost(random.nextFloat());

    // Randomize LFO rates and shapes
    for (int i = 1; i <= 4; ++i)
    {
        juce::String rateId = "lfo" + juce::String(i) + "_rate";
        juce::String shapeId = "lfo" + juce::String(i) + "_shape";

        apvts.getParameter(rateId)->setValueNotifyingHost(random.nextFloat());
        apvts.getParameter(shapeId)->setValueNotifyingHost(
            static_cast<float>(random.nextInt(6)) / 5.0f);
    }

    // Randomize mod matrix
    for (int i = 1; i <= 4; ++i)
    {
        juce::String targetId = "mod" + juce::String(i) + "_target";
        juce::String depthId = "mod" + juce::String(i) + "_depth";

        apvts.getParameter(targetId)->setValueNotifyingHost(
            static_cast<float>(random.nextInt(11)) / 10.0f);
        apvts.getParameter(depthId)->setValueNotifyingHost(
            random.nextFloat() * 0.6f + 0.2f);  // 0.2 to 0.8 range
    }

    // Randomize reverb
    apvts.getParameter(ParamIDs::reverbSize)->setValueNotifyingHost(random.nextFloat());
    apvts.getParameter(ParamIDs::reverbMix)->setValueNotifyingHost(
        random.nextFloat() * 0.5f + 0.1f);  // 0.1 to 0.6 range
}

//==============================================================================
bool RippleProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* RippleProcessor::createEditor()
{
    return new RippleEditor(*this);
}

//==============================================================================
void RippleProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    xml->setAttribute("stateVersion", kStateVersion);
    copyXmlToBinary(*xml, destData);
}

void RippleProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        int loadedVersion = xmlState->getIntAttribute("stateVersion", 0);
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));

        if (loadedVersion != kStateVersion)
        {
            DBG("State version mismatch - using defaults");
        }
    }
}

//==============================================================================
bool RippleProcessor::hasActivationEnabled() const
{
#if HAS_PROJECT_DATA && BEATCONNECT_ACTIVATION_ENABLED
    return static_cast<bool>(buildFlags_.getProperty("enableActivationKeys", false));
#else
    return false;
#endif
}

void RippleProcessor::loadProjectData()
{
#if HAS_PROJECT_DATA
    int dataSize = 0;
    const char* data = ProjectData::getNamedResource("project_data_json", dataSize);

    if (data == nullptr || dataSize == 0)
        return;

    auto jsonString = juce::String::fromUTF8(data, dataSize);
    auto parsed = juce::JSON::parse(jsonString);

    if (parsed.isVoid())
        return;

    pluginId_ = parsed.getProperty("pluginId", "").toString();
    apiBaseUrl_ = parsed.getProperty("apiBaseUrl", "").toString();
    supabasePublishableKey_ = parsed.getProperty("supabasePublishableKey", "").toString();
    buildFlags_ = parsed.getProperty("flags", juce::var());

#if BEATCONNECT_ACTIVATION_ENABLED
    bool enableActivation = static_cast<bool>(buildFlags_.getProperty("enableActivationKeys", false));
    if (enableActivation && pluginId_.isNotEmpty())
    {
        beatconnect::ActivationConfig config;
        config.apiBaseUrl = apiBaseUrl_.toStdString();
        config.pluginId = pluginId_.toStdString();
        config.supabaseKey = supabasePublishableKey_.toStdString();
        config.pluginName = "Ripple";
        activation_ = beatconnect::Activation::create(config);
    }
#endif
#endif
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RippleProcessor();
}
