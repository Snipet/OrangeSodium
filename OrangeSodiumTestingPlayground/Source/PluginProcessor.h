/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "orange_sodium.h"

namespace OrangeSodium {
class Program; // forward declaration for cleanup
}

//==============================================================================
/**
*/
class OrangeSodiumTestingPlaygroundAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    OrangeSodiumTestingPlaygroundAudioProcessor();
    ~OrangeSodiumTestingPlaygroundAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void updateProgram(juce::String& program);
    void getLogText(juce::String&);

private:
    // Synth integration
    OrangeSodium::Synthesizer* synth;
    OrangeSodium::Synthesizer* swap_synth;
    bool synthsNeedSwapped;
    juce::File findDefaultScript() const;
    void initSynth();
    bool synthLoaded;
    std::string loadedScript;
    double lastSampleRate;
    int lastSamplesPerBlock;
    std::ostringstream logStream;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrangeSodiumTestingPlaygroundAudioProcessor)
};
