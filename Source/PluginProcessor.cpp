/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PGQ_VSTAudioProcessor::PGQ_VSTAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

PGQ_VSTAudioProcessor::~PGQ_VSTAudioProcessor()
{
}

//==============================================================================
const juce::String PGQ_VSTAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PGQ_VSTAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PGQ_VSTAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PGQ_VSTAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PGQ_VSTAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PGQ_VSTAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PGQ_VSTAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PGQ_VSTAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String PGQ_VSTAudioProcessor::getProgramName (int index)
{
    return {};
}

void PGQ_VSTAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void PGQ_VSTAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    for (auto& eq : graphicEQ_)
        eq.prepare(sampleRate, EQConstants::frequencies);
}

void PGQ_VSTAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PGQ_VSTAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void PGQ_VSTAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    const auto numSamples =
            static_cast<std::size_t>(buffer.getNumSamples());
    const auto numChannels =
            std::min(
                static_cast<std::size_t>(buffer.getNumChannels()),
                NumChannels);
    for (int channel = 0; channel < numChannels; ++channel)
    {
//        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
        graphicEQ_[channel].process(
                    buffer.getWritePointer(
                        static_cast<int>(channel)),
                    numSamples);
    }
}

//==============================================================================
bool PGQ_VSTAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PGQ_VSTAudioProcessor::createEditor()
{
    return new PGQ_VSTAudioProcessorEditor (*this);
}

//==============================================================================
void PGQ_VSTAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ValueTree state("EQState");
    state.setProperty(
        "version",
        1,
        nullptr);

    for (std::size_t i = 0;
         i < EQState::NumBands;
         ++i)
    {
        state.setProperty(
            "gain_" + juce::String(i),
            eqState.gains[i],
            nullptr);
    }

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void PGQ_VSTAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto xml = getXmlFromBinary(
        data,
        sizeInBytes);
    // just to examine if version is the correct one in the future,
    // if we need to update what we save
//    const int version =
//        xml->getIntAttribute("version", 1);

    if (xml == nullptr)
        return;

    if (!xml->hasTagName("EQState"))
        return;

    EQState newState;

    for (std::size_t i = 0;
         i < EQState::NumBands;
         ++i)
    {
        newState.gains[i] =
            static_cast<float>(
                xml->getDoubleAttribute(
                    "gain_" + juce::String(i),
                    0.0));
    }

    setEQState(newState);
}

void PGQ_VSTAudioProcessor::reset()
{
    for (auto& eq : graphicEQ_)
        eq.reset();
}

// DSP-related functions
void PGQ_VSTAudioProcessor::setGain(std::size_t band, float gainDb){
    for (auto& eq : graphicEQ_)
        eq.setGain(band, gainDb);
}
void PGQ_VSTAudioProcessor::setGains(const GraphicEQ::Gains& gains){
    for (std::size_t i = 0;
         i < EQState::NumBands;
         ++i)
    {
        eqState.gains[i] = gains[i];
    }
    for (auto& eq : graphicEQ_)
        eq.setGains(gains);
}
float PGQ_VSTAudioProcessor::getGain(std::size_t band) const{
    if (band >= GraphicEQ::NumBands)
        return 0.0f;

    return graphicEQ_[0].getGain(band);
}

GraphicEQ::Gains PGQ_VSTAudioProcessor::getGains() const
{
    GraphicEQ::Gains gains{};

    for (std::size_t i = 0;
         i < GraphicEQ::NumBands;
         ++i)
    {
        gains[i] = eqState.gains[i];
    }

    return gains;
}

EQState PGQ_VSTAudioProcessor::getEQState() const
{
    return eqState;
}

void PGQ_VSTAudioProcessor::setEQState(
    const EQState& state)
{
    eqState = state;

    for (auto& eq : graphicEQ_)
        eq.setGains(eqState.gains);
    
    // communicate to editor
    sendChangeMessage();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PGQ_VSTAudioProcessor();
}
