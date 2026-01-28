/*
  ==============================================================================
    RIPPLE - Spectral Texture Engine Implementation
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#if HAS_PROJECT_DATA
#include "ProjectData.h"
#endif

#if BEATCONNECT_ACTIVATION_ENABLED
#include <beatconnect/Activation.h>
#endif

namespace ParamIDs
{
    static constexpr const char* freeze = "freeze";
    static constexpr const char* smear = "smear";
    static constexpr const char* scatter = "scatter";
    static constexpr const char* shift = "shift";
    static constexpr const char* tilt = "tilt";
    static constexpr const char* feedback = "feedback";
}

static constexpr int kStateVersion = 2;

//==============================================================================
RippleProcessor::RippleProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    loadProjectData();
}

RippleProcessor::~RippleProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout RippleProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::freeze, 1 }, "Freeze",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::smear, 1 }, "Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::scatter, 1 }, "Diffuse",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::shift, 1 }, "Shift",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));  // -1 to +1 for pitch shift

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::tilt, 1 }, "Tilt",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));  // -1 dark, +1 bright

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::feedback, 1 }, "Feedback",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String RippleProcessor::getName() const { return JucePlugin_Name; }
bool RippleProcessor::acceptsMidi() const { return false; }
bool RippleProcessor::producesMidi() const { return false; }
bool RippleProcessor::isMidiEffect() const { return false; }
double RippleProcessor::getTailLengthSeconds() const { return 2.0; }
int RippleProcessor::getNumPrograms() { return 1; }
int RippleProcessor::getCurrentProgram() { return 0; }
void RippleProcessor::setCurrentProgram(int) {}
const juce::String RippleProcessor::getProgramName(int) { return {}; }
void RippleProcessor::changeProgramName(int, const juce::String&) {}

//==============================================================================
void RippleProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    spectralProcessor.prepare(sampleRate, samplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    reverb.prepare(spec);
    reverbParams.roomSize = 0.5f;
    reverbParams.damping = 0.5f;
    reverbParams.wetLevel = 0.3f;
    reverbParams.dryLevel = 0.7f;
    reverbParams.width = 1.0f;
    reverb.setParameters(reverbParams);
}

void RippleProcessor::releaseResources()
{
    spectralProcessor.reset();
    reverb.reset();
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
void RippleProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Input level
    float inLevel = 0.0f;
    for (int ch = 0; ch < totalNumInputChannels; ++ch)
        inLevel = std::max(inLevel, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    inputLevel.store(inLevel);

    // Get parameters
    float freeze = apvts.getRawParameterValue(ParamIDs::freeze)->load();
    float smear = apvts.getRawParameterValue(ParamIDs::smear)->load();
    float scatter = apvts.getRawParameterValue(ParamIDs::scatter)->load();
    float shift = apvts.getRawParameterValue(ParamIDs::shift)->load();
    float tilt = apvts.getRawParameterValue(ParamIDs::tilt)->load();
    float feedback = apvts.getRawParameterValue(ParamIDs::feedback)->load();

    // Update spectral processor
    spectralProcessor.setFreezeAmount(freeze);
    spectralProcessor.setSmearAmount(smear);
    spectralProcessor.setScatterAmount(scatter);
    spectralProcessor.setShiftAmount(shift);
    spectralProcessor.setTiltAmount(tilt);
    spectralProcessor.setFeedbackAmount(feedback);

    // Process spectral
    spectralProcessor.process(buffer);

    // Output level
    float outLevel = 0.0f;
    for (int ch = 0; ch < totalNumOutputChannels; ++ch)
        outLevel = std::max(outLevel, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    outputLevel.store(outLevel);
}

//==============================================================================
void RippleProcessor::getSpectrum(float* magnitudes) const
{
    spectralProcessor.getMagnitudeSpectrum(magnitudes, SpectralProcessor::NUM_BINS);
}

void RippleProcessor::getFrozenSpectrum(float* magnitudes) const
{
    spectralProcessor.getFrozenSpectrum(magnitudes, SpectralProcessor::NUM_BINS);
}

void RippleProcessor::setInteraction(float y, float radius, bool active)
{
    spectralProcessor.setInteractionY(y);
    spectralProcessor.setInteractionRadius(radius);
    spectralProcessor.setInteractionActive(active);
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
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
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
