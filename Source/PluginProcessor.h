#pragma once

#include <JuceHeader.h>

//==============================================================================
// A simple sawtooth voice with an ADSR envelope so the plugin is audible
// the moment it's loaded, with no extra setup.
//==============================================================================
struct ScaleLockSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class ScaleLockVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<ScaleLockSound*> (sound) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        currentAngle = 0.0;
        level = velocity * 0.15;
        frequency = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        angleDelta = frequency * 2.0 * juce::MathConstants<double>::pi / currentSampleRate;
        adsr.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
            adsr.noteOff();
        else
        {
            clearCurrentNote();
            angleDelta = 0.0;
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void setCurrentPlaybackSampleRate (double newRate) override
    {
        juce::SynthesiserVoice::setCurrentPlaybackSampleRate (newRate);
        adsr.setSampleRate (newRate);
        adsrParams.attack = 0.002f;
        adsrParams.decay = 0.1f;
        adsrParams.sustain = 0.8f;
        adsrParams.release = 0.15f;
        adsr.setParameters (adsrParams);
    }

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (angleDelta == 0.0)
            return;

        while (--numSamples >= 0)
        {
            // Simple band-limited-ish saw approximation via summed harmonics (cheap, good enough)
            float sample = 0.0f;
            double phase = currentAngle;
            for (int h = 1; h <= 6; ++h)
                sample += (float) (std::sin (phase * h) / h);
            sample *= (float) level * 0.6f;

            float envVal = adsr.getNextSample();
            sample *= envVal;

            for (int ch = outputBuffer.getNumChannels(); --ch >= 0;)
                outputBuffer.addSample (ch, startSample, sample);

            currentAngle += angleDelta;
            ++startSample;

            if (! adsr.isActive())
            {
                clearCurrentNote();
                angleDelta = 0.0;
                break;
            }
        }
    }

private:
    double currentAngle = 0.0, frequency = 0.0, angleDelta = 0.0, level = 0.0;
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;
};

//==============================================================================
class ScaleLockAudioProcessor : public juce::AudioProcessor
{
public:
    ScaleLockAudioProcessor();
    ~ScaleLockAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- Scale data, shared with the editor ----
    static const juce::StringArray noteNames;
    static const juce::StringArray scaleNames;
    static const std::vector<std::vector<int>> scaleIntervals; // semitone offsets from root
    static const juce::StringArray snapModeNames; // Nearest / Snap Down / Snap Up

    juce::AudioParameterChoice* rootNoteParam = nullptr;
    juce::AudioParameterChoice* scaleTypeParam = nullptr;
    juce::AudioParameterChoice* snapModeParam = nullptr;

    // ---- On-screen / computer-keyboard playing ----
    // Shared with the editor's MidiKeyboardComponent so clicking or typing
    // plays notes even with no MIDI hardware connected.
    juce::MidiKeyboardState keyboardState;

    // ---- Hosting another VST3 instrument as the sound source ----
    // Lets you keep using your favourite synth's actual sound while
    // ScaleLock corrects the notes before they reach it.
    void loadInstrumentPlugin (const juce::File& pluginFile,
                                std::function<void (bool success, const juce::String& message)> onComplete);
    void unloadInstrumentPlugin();
    bool hasHostedInstrument() const noexcept { return hostedInstrument != nullptr; }
    juce::String getHostedInstrumentName() const;
    juce::AudioProcessorEditor* createHostedInstrumentEditor(); // nullptr if none loaded / no editor

private:
    juce::Synthesiser synth; // fallback sound used until you load your own instrument

    juce::AudioPluginFormatManager formatManager;
    std::unique_ptr<juce::AudioPluginInstance> hostedInstrument;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // Snaps a MIDI note number to the nearest/lowest/highest note in the
    // currently selected root + scale.
    int snapNoteToScale (int midiNoteNumber) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScaleLockAudioProcessor)
};
