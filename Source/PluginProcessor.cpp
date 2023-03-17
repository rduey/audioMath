/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <math.h>

double pi = 2*asin(1.0);
double g_ratio = (1 + sqrt(5)) / 2;

//==============================================================================
AudioMathAudioProcessor::AudioMathAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                    #if ! JucePlugin_IsMidiEffect
                       #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                    #endif
                       .withInput  ("Sidechain", juce::AudioChannelSet::stereo())
                       )
#endif
, state (*this, nullptr, "STATE", {
    std::make_unique<juce::AudioParameterChoice> ("choice", "Operation",
                                                 
    juce::StringArray ("Multiply",
                       "Sin Multiply",
                       "Minimum",
                       "Maximum",
                       "Plus*Minus",
                       "Triplication",
                       "SinTan")      , 0),
    std::make_unique<juce::AudioParameterFloat> ("mix", "Dry / Wet", 0.0f, 1.0f, 0.5f)

})
{
    /*
    addParameter(new juce::AudioParameterChoice ("choice", "Choose",
                                                 
    juce::StringArray ("Multiply", "Sin Multiply", "Minimum", "Maximum")      , 0));
     */
}

AudioMathAudioProcessor::~AudioMathAudioProcessor()
{
}

//==============================================================================
const juce::String AudioMathAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioMathAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioMathAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioMathAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioMathAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioMathAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioMathAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioMathAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AudioMathAudioProcessor::getProgramName (int index)
{
    return {};
}

void AudioMathAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AudioMathAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    choiceInc = 1.0f / (numChoices - 1);
}

void AudioMathAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AudioMathAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
        // the sidechain can take any layout, the main bus needs to be the same on the input and output
        return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
                 && ! layouts.getMainInputChannelSet().isDisabled();
}
#endif

void AudioMathAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    auto mainInputOutput = getBusBuffer (buffer, true, 0);                                  // [5]
    auto sideChainInput  = getBusBuffer (buffer, true, 1);
    
    auto choiceValue = state.getParameter("choice")->getValue();
    int choice = choiceValue / choiceInc;
    
    float mix = state.getParameter ("mix")->getValue();
    
    for (auto j = 0; j < buffer.getNumSamples(); ++j)                                       // [7]
    {
        
        auto leastChannels = std::min(sideChainInput.getNumChannels(), mainInputOutput.getNumChannels());
        float drySample;
        float wetSample;
        
        
        switch(choice) {
            // Multiply
            case 0:
                for (auto i = 0; i < leastChannels; ++i) {
                    auto mainIn = *mainInputOutput.getReadPointer (i, j);
                    drySample = *mainInputOutput.getReadPointer (i, j);
                    auto sideIn = sideChainInput.getReadPointer (i) [j];
                    wetSample =
                    sin(mainIn * abs(sideIn) * pi / 2);
                    *mainInputOutput.getWritePointer (i, j) =
                    (drySample * (1.0f - mix)) + (wetSample * mix);
                }
                break;
            // Sin Multiply
            case 1:
                for (auto i = 0; i < leastChannels; ++i) {
                    auto mainIn = *mainInputOutput.getReadPointer (i, j);
                    drySample = *mainInputOutput.getReadPointer (i, j);
                    auto sideIn = sideChainInput.getReadPointer (i) [j];
                    wetSample =
                    sin(mainIn * pi) * sin(sideIn * pi);
                    *mainInputOutput.getWritePointer (i, j) =
                    (drySample * (1.0f - mix)) + (wetSample * mix);
                }
                break;
            // Minimum
            case 2:
                for (auto i = 0; i < leastChannels; ++i) {
                    auto mainIn = *mainInputOutput.getReadPointer (i, j);
                    drySample = *mainInputOutput.getReadPointer (i, j);
                    auto sideIn = sideChainInput.getReadPointer (i) [j];
                    if (abs(sideIn) < abs(mainIn)) {
                        wetSample = sideIn;
                    }
                    else {
                        wetSample = mainIn;
                    }
                    *mainInputOutput.getWritePointer (i, j) =
                    (drySample * (1.0f - mix)) + (wetSample * mix);
                }
                break;
            // Maximum
            case 3:
                for (auto i = 0; i < leastChannels; ++i) {
                    auto mainIn = *mainInputOutput.getReadPointer (i, j);
                    drySample = *mainInputOutput.getReadPointer (i, j);
                    auto sideIn = sideChainInput.getReadPointer (i) [j];
                    if (abs(sideIn) > abs(mainIn)) {
                        wetSample = sideIn;
                    }
                    else {
                        wetSample = mainIn;
                    }
                    *mainInputOutput.getWritePointer (i, j) =
                    (drySample * (1.0f - mix)) + (wetSample * mix);
                }
                break;
            // Plus * Minus
            case 4:
                for (auto i = 0; i < leastChannels; ++i) {
                    auto mainIn = *mainInputOutput.getReadPointer (i, j);
                    drySample = *mainInputOutput.getReadPointer (i, j);
                    auto sideIn = sideChainInput.getReadPointer (i) [j];
                    wetSample =
                    (mainIn + sideIn) * (mainIn - sideIn);
                    *mainInputOutput.getWritePointer (i, j) =
                    (drySample * (1.0f - mix)) + (wetSample * mix);
                }
                break;
            // Triplication
            case 5:
                for (auto i = 0; i < leastChannels; ++i) {
                    auto mainIn = *mainInputOutput.getReadPointer (i, j);
                    drySample = *mainInputOutput.getReadPointer (i, j);
                    auto sideIn = sideChainInput.getReadPointer (i) [j];
                    wetSample =
                    (mainIn + sideIn) * (mainIn - sideIn) * (sideIn - mainIn);
                    
                    *mainInputOutput.getWritePointer (i, j) =
                    (drySample * (1.0f - mix)) + (wetSample * mix);
                }
                break;
            // SinTan
            case 6:
                for (auto i = 0; i < leastChannels; ++i) {
                    auto mainIn = *mainInputOutput.getReadPointer (i, j);
                    drySample = *mainInputOutput.getReadPointer (i, j);
                    auto sideIn = sideChainInput.getReadPointer (i) [j];
                    wetSample =
                    -sin(tan(mainIn + sideIn)) * sin(tan(mainIn - sideIn)) * sin(tan(sideIn - mainIn));
                    *mainInputOutput.getWritePointer (i, j) =
                    (drySample * (1.0f - mix)) + (wetSample * mix);
                }
                break;
            default:
                for (auto i = 0; i < leastChannels; ++i) {
                    *mainInputOutput.getWritePointer (i, j) =
                    *mainInputOutput.getReadPointer (i, j);
                }
        }
    }
}

//==============================================================================
bool AudioMathAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioMathAudioProcessor::createEditor()
{
    //return new AudioMathAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void AudioMathAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    if (auto xmlState = state.copyState().createXml())
        copyXmlToBinary (*xmlState, destData);
}

void AudioMathAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    if (auto xmlState = getXmlFromBinary (data, sizeInBytes))
        state.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioMathAudioProcessor();
}
