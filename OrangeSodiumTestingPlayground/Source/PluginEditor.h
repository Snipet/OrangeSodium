/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class OrangeSodiumTestingPlaygroundAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    OrangeSodiumTestingPlaygroundAudioProcessorEditor (OrangeSodiumTestingPlaygroundAudioProcessor&);
    ~OrangeSodiumTestingPlaygroundAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void setProgramEditorText(juce::String& text);

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    OrangeSodiumTestingPlaygroundAudioProcessor& audioProcessor;
    //juce::TextEditor programEditor;
    juce::TextEditor debugEditor;
    juce::TextButton sendProgramButton;
    juce::TextButton reloadDebugButton;
    juce::String debugText;

    juce::CodeDocument codeDocument;
    std::unique_ptr<juce::CodeEditorComponent> programEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrangeSodiumTestingPlaygroundAudioProcessorEditor)
};
