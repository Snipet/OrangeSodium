/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
// OrangeSodium
#include "program.h"

//==============================================================================
OrangeSodiumTestingPlaygroundAudioProcessor::OrangeSodiumTestingPlaygroundAudioProcessor()
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
    synth = nullptr;
    synthLoaded = false;
    //initSynth();
}

OrangeSodiumTestingPlaygroundAudioProcessor::~OrangeSodiumTestingPlaygroundAudioProcessor()
{
    // Clean up singleton program if created
    //OrangeSodium::Program::destroyInstance();
}

//==============================================================================
const juce::String OrangeSodiumTestingPlaygroundAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool OrangeSodiumTestingPlaygroundAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool OrangeSodiumTestingPlaygroundAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool OrangeSodiumTestingPlaygroundAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double OrangeSodiumTestingPlaygroundAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int OrangeSodiumTestingPlaygroundAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int OrangeSodiumTestingPlaygroundAudioProcessor::getCurrentProgram()
{
    return 0;
}

void OrangeSodiumTestingPlaygroundAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String OrangeSodiumTestingPlaygroundAudioProcessor::getProgramName (int index)
{
    return {};
}

void OrangeSodiumTestingPlaygroundAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void OrangeSodiumTestingPlaygroundAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    if (!synthLoaded) {
        initSynth();
        synthLoaded = true;
    }
    synth->prepare(2, samplesPerBlock, sampleRate);
    //synth->setSampleRate(sampleRate);
}

void OrangeSodiumTestingPlaygroundAudioProcessor::releaseResources()
{
    //delete synth;
    //OrangeSodium::Program::destroyInstance();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool OrangeSodiumTestingPlaygroundAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void OrangeSodiumTestingPlaygroundAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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


    // Handle MIDI
    for (const auto metadata : midiMessages)
    {
        const auto& msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            if (synth)
                synth->processMidiEvent(msg.getNoteNumber(), true);
        }
        else if (msg.isNoteOff())
        {
            if (synth)
                synth->processMidiEvent(msg.getNoteNumber(), false);
        }
    }

    // Prepare for dynamic changes in buffer size/channels
    if (synth)
    {
        const auto nCh = static_cast<size_t>(buffer.getNumChannels());
        const auto nSmps = static_cast<size_t>(buffer.getNumSamples());
        //synth->prepare(nCh, nSmps);

        // Provide channel pointers directly to the synth
        float* outs[64] = { nullptr }; // support up to 64 channels for safety
        for (int c = 0; c < buffer.getNumChannels() && c < 64; ++c)
            outs[c] = buffer.getWritePointer(c);

        // Zero the buffer before rendering (synth writes fresh data)
        buffer.clear();
        synth->processBlock(outs, static_cast<size_t>(buffer.getNumChannels()), static_cast<size_t>(buffer.getNumSamples()));
    }
}

//==============================================================================
bool OrangeSodiumTestingPlaygroundAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* OrangeSodiumTestingPlaygroundAudioProcessor::createEditor()
{
    return new OrangeSodiumTestingPlaygroundAudioProcessorEditor (*this);
}

//==============================================================================
void OrangeSodiumTestingPlaygroundAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void OrangeSodiumTestingPlaygroundAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrangeSodiumTestingPlaygroundAudioProcessor();
}

// ==================== Internal helpers ====================
juce::File OrangeSodiumTestingPlaygroundAudioProcessor::findDefaultScript() const
{
    // Try a few likely locations relative to the plugin and project root
    const juce::String relPath = "C:/users/seant/Documents/projects/OrangeSodium/examples/basic/script.lua";

    // 1) Try relative to current working directory
    {
        auto f = juce::File::getCurrentWorkingDirectory().getChildFile(relPath);
        if (f.existsAsFile()) return f;
    }

    // 2) Try relative to the plugin source directory (during dev)
    {
        auto here = juce::File(__FILE__);
        auto projRoot = here.getParentDirectory() // PluginProcessor.cpp
                            .getParentDirectory() // Source
                            .getParentDirectory(); // OrangeSodiumTestingPlayground
        auto f = projRoot.getSiblingFile(relPath);
        if (f.existsAsFile()) return f;
    }

    // 3) Walk up from executable location
    {
        auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
        auto dir = exe.getParentDirectory();
        for (int i = 0; i < 6; ++i)
        {
            auto f = dir.getChildFile(relPath);
            if (f.existsAsFile()) return f;
            dir = dir.getParentDirectory();
        }
    }

    return {};
}

void OrangeSodiumTestingPlaygroundAudioProcessor::initSynth()
{

    auto scriptFile = findDefaultScript();
    if (!scriptFile.existsAsFile())
    {
        juce::Logger::writeToLog("OrangeSodium: Could not find default script.lua");
        return;
    }
    //synth = OrangeSodium::createSynthesizerFromScript(scriptFile.getFullPathName().toStdString());
    //synth = OrangeSodium::createSynthesizerFromScript("scriptFile.getFullPathName().toStdString()");

    try
    {
        synth = OrangeSodium::createSynthesizerFromScript(scriptFile.getFullPathName().toStdString());
        if (synth)
        {
            //synth->setSampleRate(static_cast<float>(sampleRate));
            synth->buildSynthFromProgram();
            //synth->prepare(static_cast<size_t>(getTotalNumOutputChannels()), static_cast<size_t>(samplesPerBlock));
        }
    }
    catch (...) {
        juce::Logger::writeToLog("OrangeSodium: Exception while initializing synthesizer");
        //synth.reset();
    }
}
