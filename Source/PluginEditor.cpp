#include "PluginProcessor.h"
#include "PluginEditor.h"

ScaleLockAudioProcessorEditor::ScaleLockAudioProcessorEditor (ScaleLockAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      keyboardComponent (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lookAndFeel);

    // ---- Branding ----
    // "LIFE KEY" is drawn by hand in paint() as a stamped gold poster logo -
    // see paint() below.

    taglineLabel.setText ("by Jay Skilly", juce::dontSendNotification);
    // Script/signature-style font for the name - macOS ships "Snell Roundhand"
    // by default, which reads like a handwritten signature/tag.
    taglineLabel.setFont (juce::Font ("Snell Roundhand", 22.0f, juce::Font::bold));
    taglineLabel.setJustificationType (juce::Justification::centred);
    taglineLabel.setColour (juce::Label::textColourId, juce::Colour (0xfff2c94c)); // gold
    addAndMakeVisible (taglineLabel);

    // ---- The two big controls: Key + Scale ----
    keyCaption.setText ("KEY", juce::dontSendNotification);
    keyCaption.setJustificationType (juce::Justification::centred);
    keyCaption.setFont (juce::Font (12.0f, juce::Font::bold));
    keyCaption.setColour (juce::Label::textColourId, ScaleLockLookAndFeel::dimTextColour);
    addAndMakeVisible (keyCaption);

    for (int i = 0; i < ScaleLockAudioProcessor::noteNames.size(); ++i)
        rootBox.addItem (ScaleLockAudioProcessor::noteNames[i], i + 1);
    rootBox.setSelectedItemIndex (processor.rootNoteParam->getIndex(), juce::dontSendNotification);
    rootBox.onChange = [this] { *processor.rootNoteParam = rootBox.getSelectedItemIndex(); };
    addAndMakeVisible (rootBox);

    scaleCaption.setText ("SCALE", juce::dontSendNotification);
    scaleCaption.setJustificationType (juce::Justification::centred);
    scaleCaption.setFont (juce::Font (12.0f, juce::Font::bold));
    scaleCaption.setColour (juce::Label::textColourId, ScaleLockLookAndFeel::dimTextColour);
    addAndMakeVisible (scaleCaption);

    for (int i = 0; i < ScaleLockAudioProcessor::scaleNames.size(); ++i)
        scaleBox.addItem (ScaleLockAudioProcessor::scaleNames[i], i + 1);
    scaleBox.setSelectedItemIndex (processor.scaleTypeParam->getIndex(), juce::dontSendNotification);
    scaleBox.onChange = [this] { *processor.scaleTypeParam = scaleBox.getSelectedItemIndex(); };
    addAndMakeVisible (scaleBox);

    // ---- Secondary: note correction mode (tucked away, most people leave it on default) ----
    snapLabel.setText ("Note correction:", juce::dontSendNotification);
    snapLabel.setFont (juce::Font (11.5f));
    snapLabel.setColour (juce::Label::textColourId, ScaleLockLookAndFeel::dimTextColour);
    addAndMakeVisible (snapLabel);

    for (int i = 0; i < ScaleLockAudioProcessor::snapModeNames.size(); ++i)
        snapBox.addItem (ScaleLockAudioProcessor::snapModeNames[i], i + 1);
    snapBox.setSelectedItemIndex (processor.snapModeParam->getIndex(), juce::dontSendNotification);
    snapBox.onChange = [this] { *processor.snapModeParam = snapBox.getSelectedItemIndex(); };
    addAndMakeVisible (snapBox);

    // ---- Sound source ----
    soundButton.onClick = [this] { loadOwnSound(); };
    addAndMakeVisible (soundButton);

    editInstrumentButton.onClick = [this] { openHostedInstrumentEditor(); };
    addAndMakeVisible (editInstrumentButton);

    instrumentStatusLabel.setJustificationType (juce::Justification::centredLeft);
    instrumentStatusLabel.setFont (juce::Font (12.5f));
    instrumentStatusLabel.setColour (juce::Label::textColourId, ScaleLockLookAndFeel::dimTextColour);
    addAndMakeVisible (instrumentStatusLabel);

    // ---- The playable keyboard - the main event ----
    addAndMakeVisible (keyboardComponent);
    keyboardComponent.setAvailableRange (36, 96);
    keyboardComponent.setKeyWidth (20.0f);
    keyboardComponent.grabKeyboardFocus();

    refreshInstrumentStatus();

    setSize (560, 400);
}

ScaleLockAudioProcessorEditor::~ScaleLockAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
    hostedEditorWindow.reset();
}

void ScaleLockAudioProcessorEditor::loadOwnSound()
{
    if (processor.hasHostedInstrument())
    {
        processor.unloadInstrumentPlugin();
        hostedEditorWindow.reset();
        refreshInstrumentStatus();
        return;
    }

    fileChooser = std::make_unique<juce::FileChooser> (
        "Choose a VST3 or AU instrument to use for sound",
        juce::File(), "*.vst3;*.component");

    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file == juce::File())
            return;

        instrumentStatusLabel.setText ("Loading " + file.getFileNameWithoutExtension() + "...",
                                        juce::dontSendNotification);

        processor.loadInstrumentPlugin (file, [this] (bool success, const juce::String& message)
        {
            juce::MessageManager::callAsync ([this, success, message]
            {
                instrumentStatusLabel.setText (message, juce::dontSendNotification);
                refreshInstrumentStatus();
            });
        });
    });
}

void ScaleLockAudioProcessorEditor::refreshInstrumentStatus()
{
    const bool hasInstrument = processor.hasHostedInstrument();
    editInstrumentButton.setVisible (hasInstrument);
    soundButton.setButtonText (hasInstrument ? "BUILT-IN SOUND" : "USE MY OWN SOUND");

    if (hasInstrument)
        instrumentStatusLabel.setText ("Playing: " + processor.getHostedInstrumentName(),
                                        juce::dontSendNotification);
    else
        instrumentStatusLabel.setText ("Playing: Life Key's built-in sound",
                                        juce::dontSendNotification);
    resized();
}

void ScaleLockAudioProcessorEditor::openHostedInstrumentEditor()
{
    auto* childEditor = processor.createHostedInstrumentEditor();
    if (childEditor == nullptr)
        return;

    hostedEditorWindow = std::make_unique<juce::DocumentWindow> (
        processor.getHostedInstrumentName(),
        juce::Colours::darkgrey,
        juce::DocumentWindow::closeButton);

    hostedEditorWindow->setUsingNativeTitleBar (true);
    hostedEditorWindow->setContentOwned (childEditor, true);
    hostedEditorWindow->centreWithSize (childEditor->getWidth(), childEditor->getHeight());
    hostedEditorWindow->setVisible (true);
    hostedEditorWindow->setResizable (true, false);
}

void ScaleLockAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (ScaleLockLookAndFeel::backgroundColour);

    // One clean rounded card behind everything - simple, like a single hardware panel.
    auto card = getLocalBounds().reduced (10).toFloat();
    g.setColour (ScaleLockLookAndFeel::panelColour.withAlpha (0.5f));
    g.fillRoundedRectangle (card, 12.0f);
    g.setColour (ScaleLockLookAndFeel::panelBorder);
    g.drawRoundedRectangle (card, 12.0f, 1.0f);

    // ---- "LIFE KEY" - bold gold stamped poster logo ----
    juce::Font titleFont ("Futura", (float) titleBounds.getHeight() * 0.85f, juce::Font::bold);
    g.setFont (titleFont);

    // Wide letter-spacing, drawn glyph by glyph so it reads like a stamped poster wordmark.
    juce::String text = "LIFE KEY";
    float totalWidth = 0.0f;
    const float tracking = 6.0f;
    for (auto ch : text)
        totalWidth += titleFont.getStringWidthFloat (juce::String::charToString (ch)) + tracking;

    float x = (float) titleBounds.getCentreX() - totalWidth / 2.0f;
    float y = (float) titleBounds.getY();
    float h = (float) titleBounds.getHeight();

    for (auto ch : text)
    {
        juce::String s = juce::String::charToString (ch);
        float w = titleFont.getStringWidthFloat (s);

        // Drop shadow for a stamped/stickered look
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawText (s, juce::Rectangle<float> (x + 2.0f, y + 2.0f, w + 4.0f, h), juce::Justification::centred, false);

        // Gold fill
        g.setColour (juce::Colour (0xffe0b020));
        g.drawText (s, juce::Rectangle<float> (x, y, w + 4.0f, h), juce::Justification::centred, false);

        x += w + tracking;
    }
}

void ScaleLockAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (24);

    titleBounds = area.removeFromTop (40);
    taglineLabel.setBounds (area.removeFromTop (26));
    area.removeFromTop (18);

    // Two big side-by-side selectors - the only decision that matters.
    auto bigRow = area.removeFromTop (72);
    auto keyArea = bigRow.removeFromLeft (bigRow.getWidth() / 2).reduced (10, 0);
    auto scaleArea = bigRow.reduced (10, 0);

    keyCaption.setBounds (keyArea.removeFromTop (18));
    rootBox.setBounds (keyArea.removeFromTop (44));

    scaleCaption.setBounds (scaleArea.removeFromTop (18));
    scaleBox.setBounds (scaleArea.removeFromTop (44));

    area.removeFromTop (10);

    // Secondary control - small, tucked to one side
    auto snapRow = area.removeFromTop (26);
    snapLabel.setBounds (snapRow.removeFromLeft (110));
    snapBox.setBounds (snapRow.removeFromLeft (160));

    area.removeFromTop (14);

    // Sound source row
    auto soundRow = area.removeFromTop (30);
    soundButton.setBounds (soundRow.removeFromLeft (160));
    soundRow.removeFromLeft (8);
    if (editInstrumentButton.isVisible())
    {
        editInstrumentButton.setBounds (soundRow.removeFromLeft (70));
        soundRow.removeFromLeft (8);
    }
    instrumentStatusLabel.setBounds (soundRow);

    area.removeFromTop (16);

    keyboardComponent.setBounds (area);
}
