#pragma once

#include <JuceHeader.h>

// A dark, modern theme so ScaleLock looks like a commercial instrument
// plugin rather than a default JUCE prototype.
class ScaleLockLookAndFeel : public juce::LookAndFeel_V4
{
public:
    inline static const juce::Colour backgroundColour { 0xff1b1b22 };
    inline static const juce::Colour panelColour       { 0xff26262f };
    inline static const juce::Colour panelBorder       { 0xff35353f };
    inline static const juce::Colour accentColour      { 0xff8b6bff }; // purple accent
    inline static const juce::Colour accentColourDim   { 0xff5a4a99 };
    inline static const juce::Colour textColour        { 0xffe8e8f0 };
    inline static const juce::Colour dimTextColour     { 0xff9a9aab };

    ScaleLockLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, backgroundColour);
        setColour (juce::ComboBox::backgroundColourId, panelColour);
        setColour (juce::ComboBox::textColourId, textColour);
        setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ComboBox::arrowColourId, accentColour);
        setColour (juce::TextButton::buttonColourId, panelColour);
        setColour (juce::TextButton::buttonOnColourId, accentColour);
        setColour (juce::TextButton::textColourOffId, textColour);
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        setColour (juce::Label::textColourId, textColour);
        setColour (juce::PopupMenu::backgroundColourId, panelColour);
        setColour (juce::PopupMenu::textColourId, textColour);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accentColour);
        setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffe8e8f0));
        setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff15151b));
        setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, panelBorder);
        setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, accentColour.withAlpha (0.3f));
        setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, accentColour.withAlpha (0.6f));
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                        int, int, int, int, juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (1.0f);
        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (bounds, 5.0f);
        g.setColour (panelBorder);
        g.drawRoundedRectangle (bounds, 5.0f, 1.0f);

        auto arrowZone = juce::Rectangle<float> ((float) width - 22.0f, 0.0f, 18.0f, (float) height);
        juce::Path path;
        path.startNewSubPath (arrowZone.getX() + 2, arrowZone.getCentreY() - 3);
        path.lineTo (arrowZone.getCentreX() + 2, arrowZone.getCentreY() + 3);
        path.lineTo (arrowZone.getRight() - 2, arrowZone.getCentreY() - 3);
        g.setColour (accentColour);
        g.strokePath (path, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour_,
                                bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        auto base = backgroundColour_;
        if (isButtonDown)       base = base.brighter (0.15f);
        else if (isMouseOverButton) base = base.brighter (0.08f);

        g.setColour (base);
        g.fillRoundedRectangle (bounds, 5.0f);

        if (! button.getToggleState())
        {
            g.setColour (panelBorder);
            g.drawRoundedRectangle (bounds, 5.0f, 1.0f);
        }
    }

    juce::Font getLabelFont (juce::Label&) override
    {
        return juce::Font (13.5f);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (14.0f);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override
    {
        return juce::Font (13.5f, juce::Font::bold);
    }
};
