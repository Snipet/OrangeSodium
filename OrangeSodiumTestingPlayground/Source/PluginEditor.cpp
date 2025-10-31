/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
OrangeSodiumTestingPlaygroundAudioProcessorEditor::OrangeSodiumTestingPlaygroundAudioProcessorEditor (OrangeSodiumTestingPlaygroundAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    programEditor = std::make_unique<juce::CodeEditorComponent>(codeDocument, new juce::LuaTokeniser());

    addAndMakeVisible(programEditor.get());
    programEditor->setTabSize(4, true);
    //programEditor->setFont(juce::Font("Courier New", 14.f, juce::Font::plain));
    programEditor->setColour(juce::CodeEditorComponent::backgroundColourId, juce::Colours::white);
    juce::File scriptFile = juce::File("C:/users/seant/Documents/projects/OrangeSodium/examples/basic/script.lua");
    setProgramEditorText(scriptFile.loadFileAsString());


    addAndMakeVisible(sendProgramButton);
    sendProgramButton.setButtonText("Update");
    sendProgramButton.onClick = [this] {
        audioProcessor.updateProgram(codeDocument.getAllContent());
    };

    addAndMakeVisible(reloadDebugButton);
    reloadDebugButton.setButtonText("Reload debug");
    reloadDebugButton.onClick = [this] {
        audioProcessor.getLogText(debugText);
        debugEditor.setText(debugText);
    };

    addAndMakeVisible(debugEditor);
    debugEditor.setMultiLine(true);
    debugEditor.setReadOnly(true);
    debugEditor.setScrollbarsShown(true);
    debugEditor.setText("foooo");

    setResizable(true, true);
    setSize (1000, 600);
}

OrangeSodiumTestingPlaygroundAudioProcessorEditor::~OrangeSodiumTestingPlaygroundAudioProcessorEditor()
{
}

//==============================================================================
void OrangeSodiumTestingPlaygroundAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void OrangeSodiumTestingPlaygroundAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Bottom button
    reloadDebugButton.setBounds(area.removeFromBottom(30));
    area.removeFromBottom(10); // spacing

    // Program editor takes 2/3 of remaining space
    int remainingHeight = area.getHeight() - 30 - 20; // subtract middle button height and spacing
    int programHeight = (remainingHeight * 2) / 3;

    programEditor->setBounds(area.removeFromTop(programHeight));
    area.removeFromTop(10); // spacing

    // Middle button
    sendProgramButton.setBounds(area.removeFromTop(30));
    area.removeFromTop(10); // spacing

    // Debug editor takes remaining space
    debugEditor.setBounds(area);
}


void OrangeSodiumTestingPlaygroundAudioProcessorEditor::setProgramEditorText(juce::String& text) {
    codeDocument.replaceAllContent(text);
}