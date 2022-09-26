/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DspTempAudioProcessor::DspTempAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
treeState(*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
    treeState.addParameterListener("input", this);
    treeState.addParameterListener("threh", this);
    treeState.addParameterListener("ratio", this);
    treeState.addParameterListener("attck", this);
    treeState.addParameterListener("release", this);
    treeState.addParameterListener("output", this);
    
    
}

DspTempAudioProcessor::~DspTempAudioProcessor()
{
    treeState.removeParameterListener("input", this);
    treeState.removeParameterListener("threh", this);
    treeState.removeParameterListener("ratio", this);
    treeState.removeParameterListener("attck", this);
    treeState.removeParameterListener("release", this);
    treeState.removeParameterListener("output", this);
    
}

juce::AudioProcessorValueTreeState::ParameterLayout DspTempAudioProcessor::createParameterLayout()
{
    std::vector <std::unique_ptr<juce::RangedAudioParameter>> params;
    //puting in the midle
    
    juce::NormalisableRange<float>attckRange = juce::NormalisableRange<float>();
    attckRange.setSkewForCentre(50.0f);
    
    juce::NormalisableRange<float>releaseRange = juce::NormalisableRange<float>();
    releaseRange.setSkewForCentre(160.0f);
    

    
    auto pinput = std::make_unique<juce::AudioParameterFloat>("input", "Input", -60.0f, 24.0f, 0.0f);
    auto pthreh = std::make_unique<juce::AudioParameterFloat>("threh", "Threh", -60.0f, 10.0f, 0.0f);
    auto pratio = std::make_unique<juce::AudioParameterFloat>("ratio", "Ratio", 1.0f, 20.0f, 1.0f);
    auto pattck = std::make_unique<juce::AudioParameterFloat>("attck", "Attck", attckRange, 50.0f);
    auto prelease = std::make_unique<juce::AudioParameterFloat>("release", "Release", releaseRange, 160.0f);
    auto poutput = std::make_unique<juce::AudioParameterFloat>("output", "Output", -60.0f, 24.0f, 0.0f);
    auto pbypass = std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false);

    
    
    
    params.push_back(std::move(pinput));
    params.push_back(std::move(pthreh));
    params.push_back(std::move(pratio));
    params.push_back(std::move(pattck));
    params.push_back(std::move(prelease));
    params.push_back(std::move(poutput));
    params.push_back(std::move(pbypass));

    
    return { params.begin(), params.end() };
    
}

void DspTempAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{

    
    
    updatParameters();
   
    
}

void DspTempAudioProcessor::updatParameters()
{
    inputModule.setRampDurationSeconds(treeState.getRawParameterValue("input")->load());
    outputModule.setRampDurationSeconds(treeState.getRawParameterValue("output")->load());
    compressorModule.setThreshold(treeState.getRawParameterValue("threh")->load());
    compressorModule.setRatio(treeState.getRawParameterValue("ratio")->load());
    compressorModule.setAttack(treeState.getRawParameterValue("attck")->load());
    compressorModule.setRelease(treeState.getRawParameterValue("release")->load());
    
}

//==============================================================================
const juce::String DspTempAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DspTempAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DspTempAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DspTempAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DspTempAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DspTempAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DspTempAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DspTempAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DspTempAudioProcessor::getProgramName (int index)
{
    return {};
}

void DspTempAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DspTempAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = getTotalNumInputChannels();
    
    inputModule.prepare(spec);
    inputModule.setRampDurationSeconds(0.02);
    outputModule.setRampDurationSeconds(0.02);
    outputModule.prepare(spec);
    compressorModule.prepare(spec);
    
    updatParameters();
    

}

void DspTempAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DspTempAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void DspTempAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    juce::dsp::AudioBlock<float>block {buffer};
    inputModule.process(juce::dsp::ProcessContextReplacing<float>(block));
    compressorModule.process(juce::dsp::ProcessContextReplacing<float>(block));
    outputModule.process(juce::dsp::ProcessContextReplacing<float>(block));


}

//==============================================================================
bool DspTempAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DspTempAudioProcessor::createEditor()
{
    //return new DspTempAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void DspTempAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DspTempAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DspTempAudioProcessor();
}
