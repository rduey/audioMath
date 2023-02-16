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
                       "SinTan",
                       "Current Test")      , 0),

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
    auto mainInputOutput = getBusBuffer (buffer, true, 0);                                  // [5]
    auto sideChainInput  = getBusBuffer (buffer, true, 1);
    
    //juce::AudioProcessorParameter* choiceParameter = getParameters()[0];
    
    //auto choiceValue = choiceParameter->getValue();
    auto choiceValue = state.getParameter("choice")->getValue();
    
    int choice = choiceValue / choiceInc;
    
    for (auto j = 0; j < buffer.getNumSamples(); ++j)                                       // [7]
    {
        
        auto leastChannels = std::min(sideChainInput.getNumChannels(), mainInputOutput.getNumChannels());
        
        
        switch(choice) {
            case 0:
                for (auto i = 0; i < leastChannels; ++i) {
                    auto mainIn = *mainInputOutput.getReadPointer (i, j);
                    auto sideIn = sideChainInput.getReadPointer (i) [j];
                    *mainInputOutput.getWritePointer (i, j) =
                    sin(mainIn * abs(sideIn) * pi / 2);
                }
                break;
            case 1:
                for (auto i = 0; i < leastChannels; ++i) {
                    auto mainIn = *mainInputOutput.getReadPointer (i, j);
                    auto sideIn = sideChainInput.getReadPointer (i) [j];
                    *mainInputOutput.getWritePointer (i, j) =
                    sin(mainIn * pi) * sin(sideIn * pi);
                }
                break;
            case 2:
                for (auto i = 0; i < leastChannels; ++i) {
                    auto mainIn = *mainInputOutput.getReadPointer (i, j);
                    auto sideIn = sideChainInput.getReadPointer (i) [j];
                    if (abs(sideIn) < abs(mainIn)) {
                        *mainInputOutput.getWritePointer (i, j) = sideIn;
                    }
                    else {
                        *mainInputOutput.getWritePointer (i, j) = mainIn;
                    }
                }
                break;
            case 3:
                for (auto i = 0; i < leastChannels; ++i) {
                    auto mainIn = *mainInputOutput.getReadPointer (i, j);
                    auto sideIn = sideChainInput.getReadPointer (i) [j];
                    if (abs(sideIn) > abs(mainIn)) {
                        *mainInputOutput.getWritePointer (i, j) = sideIn;
                    }
                    else {
                        *mainInputOutput.getWritePointer (i, j) = mainIn;
                    }
                }
                break;
            case 4:
                for (auto i = 0; i < leastChannels; ++i) {
                    auto mainIn = *mainInputOutput.getReadPointer (i, j);
                    auto sideIn = sideChainInput.getReadPointer (i) [j];
                    *mainInputOutput.getWritePointer (i, j) =
                    (mainIn + sideIn) * (mainIn - sideIn);
                }
                break;
            case 5:
                for (auto i = 0; i < leastChannels; ++i) {
                    auto mainIn = *mainInputOutput.getReadPointer (i, j);
                    auto sideIn = sideChainInput.getReadPointer (i) [j];
                    *mainInputOutput.getWritePointer (i, j) =
                    (mainIn + sideIn) * (mainIn - sideIn) * (sideIn - mainIn);
                }
                break;
            case 6:
                for (auto i = 0; i < leastChannels; ++i) {
                    auto mainIn = *mainInputOutput.getReadPointer (i, j);
                    auto sideIn = sideChainInput.getReadPointer (i) [j];
                    *mainInputOutput.getWritePointer (i, j) =
                    -sin(tan(mainIn + sideIn)) * sin(tan(mainIn - sideIn)) * sin(tan(sideIn - mainIn));
                }
                break;
                /*
            case 5:
                for (auto i = 0; i < leastChannels; ++i) {
                    auto mainIn = *mainInputOutput.getReadPointer (i, j);
                    auto sideIn = sideChainInput.getReadPointer (i) [j];
                    auto x = std::min(mainIn, sideIn);
                    auto inside = 1.15f * x + float(1/sqrt(50));
                    *mainInputOutput.getWritePointer (i, j) =
                    std::max(0.9f * x,-(inside * inside) + 0.02f);
                }
                break;
                 */
            case 7:
                for (auto i = 0; i < leastChannels; ++i) {
                    auto mainIn = *mainInputOutput.getReadPointer (i, j);
                    auto sideIn = sideChainInput.getReadPointer (i) [j];
                    if (abs(sideIn) < mainIn) {
                        *mainInputOutput.getWritePointer (i, j) = sideIn;
                    }
                    else {
                        *mainInputOutput.getWritePointer (i, j) = mainIn;
                    }
                }
                break;
            default:
                for (auto i = 0; i < leastChannels; ++i) {
                    *mainInputOutput.getWritePointer (i, j) =
                    *mainInputOutput.getReadPointer (i, j);
                }
        }
        
        /*
        for (auto i = 0; i < leastChannels; ++i) {
            *mainInputOutput.getWritePointer (i, j) =
            *mainInputOutput.getReadPointer (i, j) * sideChainInput.getReadPointer (i) [j] * choice;
        }
        */
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
