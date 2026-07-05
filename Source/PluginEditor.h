#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ScaleLockLookAndFeel.h"

class ScaleLockAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit ScaleLockAudioProcessorEditor (ScaleLockAudioProcessor&);
    ~ScaleLockAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ScaleLockAudioProcessor& processor;
    ScaleLockLookAndFeel lookAndFeel;

    juce::Label taglineLabel;
    juce::Rectangle<int> titleBounds;

    // The two big, primary controls - everything else is secondary.
    juce::Label keyCaption, scaleCaption;
    juce::ComboBox rootBox, scaleBox;

    // Secondary / advanced - small and out of the way.
    juce::Label snapLabel;
    juce::ComboBox snapBox;

    juce::MidiKeyboardComponent keyboardComponent;

    juce::TextButton soundButton { "USE MY OWN SOUND" };
    juce::TextButton editInstrumentButton { "EDIT" };
    juce::Label instrumentStatusLabel;

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::DocumentWindow> hostedEditorWindow;

    void refreshInstrumentStatus();
    void openHostedInstrumentEditor();
    void loadOwnSound();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScaleLockAudioProcessorEditor)
};
