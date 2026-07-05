#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
const juce::StringArray ScaleLockAudioProcessor::noteNames
    { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

const juce::StringArray ScaleLockAudioProcessor::scaleNames
{
    "Major (Ionian)", "Natural Minor (Aeolian)", "Harmonic Minor", "Melodic Minor",
    "Dorian", "Phrygian", "Lydian", "Mixolydian", "Locrian",
    "Major Pentatonic", "Minor Pentatonic", "Blues", "Chromatic",
    // -- Hindustani thaats --
    "Bilawal", "Kalyan (Yaman)", "Khamaj", "Bhairav", "Poorvi",
    "Marwa", "Kafi", "Asavari", "Bhairavi", "Todi",
    // -- Popular ragas --
    "Bhoopali", "Malkauns", "Hamsadhwani", "Charukeshi", "Kirwani"
};

const std::vector<std::vector<int>> ScaleLockAudioProcessor::scaleIntervals
{
    { 0, 2, 4, 5, 7, 9, 11 },             // Major
    { 0, 2, 3, 5, 7, 8, 10 },             // Natural Minor
    { 0, 2, 3, 5, 7, 8, 11 },             // Harmonic Minor
    { 0, 2, 3, 5, 7, 9, 11 },             // Melodic Minor (ascending)
    { 0, 2, 3, 5, 7, 9, 10 },             // Dorian
    { 0, 1, 3, 5, 7, 8, 10 },             // Phrygian
    { 0, 2, 4, 6, 7, 9, 11 },             // Lydian
    { 0, 2, 4, 5, 7, 9, 10 },             // Mixolydian
    { 0, 1, 3, 5, 6, 8, 10 },             // Locrian
    { 0, 2, 4, 7, 9 },                    // Major Pentatonic
    { 0, 3, 5, 7, 10 },                   // Minor Pentatonic
    { 0, 3, 5, 6, 7, 10 },                // Blues
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }, // Chromatic (no snapping)

    // -- Hindustani thaats (Sa fixed at root) --
    { 0, 2, 4, 5, 7, 9, 11 },             // Bilawal
    { 0, 2, 4, 6, 7, 9, 11 },             // Kalyan (Yaman) - tivra Ma
    { 0, 2, 4, 5, 7, 9, 10 },             // Khamaj
    { 0, 1, 4, 5, 7, 8, 11 },             // Bhairav
    { 0, 1, 4, 6, 7, 8, 11 },             // Poorvi
    { 0, 1, 4, 6, 7, 9, 11 },             // Marwa
    { 0, 2, 3, 5, 7, 9, 10 },             // Kafi
    { 0, 2, 3, 5, 7, 8, 10 },             // Asavari
    { 0, 1, 3, 5, 7, 8, 10 },             // Bhairavi
    { 0, 1, 3, 6, 7, 8, 11 },             // Todi

    // -- Popular ragas --
    { 0, 2, 4, 7, 9 },                    // Bhoopali (major pentatonic shape)
    { 0, 3, 5, 8, 10 },                   // Malkauns
    { 0, 2, 4, 7, 11 },                   // Hamsadhwani
    { 0, 2, 4, 5, 7, 8, 10 },             // Charukeshi
    { 0, 2, 3, 5, 7, 8, 11 },             // Kirwani
};

const juce::StringArray ScaleLockAudioProcessor::snapModeNames
    { "Nearest", "Snap Down", "Snap Up" };

//==============================================================================
ScaleLockAudioProcessor::ScaleLockAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    addParameter (rootNoteParam = new juce::AudioParameterChoice (
        "root", "Root Note", noteNames, 0));

    addParameter (scaleTypeParam = new juce::AudioParameterChoice (
        "scale", "Scale", scaleNames, 0));

    addParameter (snapModeParam = new juce::AudioParameterChoice (
        "snapMode", "Snap Mode", snapModeNames, 0));

    for (int i = 0; i < 8; ++i)
        synth.addVoice (new ScaleLockVoice());

    synth.addSound (new ScaleLockSound());

    formatManager.addDefaultFormats(); // enables VST3/AU hosting so ScaleLock can load your other synths

    // The scale-correction itself adds zero samples of delay - notes are
    // remapped instantly within the same audio block, no lookahead buffer.
    setLatencySamples (0);
}

ScaleLockAudioProcessor::~ScaleLockAudioProcessor() {}

//==============================================================================
void ScaleLockAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;
    synth.setCurrentPlaybackSampleRate (sampleRate);

    if (hostedInstrument != nullptr)
        hostedInstrument->prepareToPlay (sampleRate, samplesPerBlock);
}

void ScaleLockAudioProcessor::releaseResources() {}

bool ScaleLockAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
int ScaleLockAudioProcessor::snapNoteToScale (int midiNoteNumber) const
{
    const int root = rootNoteParam->getIndex();
    const auto& intervals = scaleIntervals[(size_t) scaleTypeParam->getIndex()];
    const int snapMode = snapModeParam->getIndex(); // 0 nearest, 1 down, 2 up

    const int pitchClass = ((midiNoteNumber - root) % 12 + 12) % 12;

    // Already in scale
    for (int iv : intervals)
        if (iv == pitchClass)
            return midiNoteNumber;

    // Find nearest scale tones above and below the played pitch class
    int below = -100, above = 100;
    for (int iv : intervals)
    {
        int diffDown = pitchClass - iv;   // positive if iv is below pitchClass
        int diffUp = iv - pitchClass;     // positive if iv is above pitchClass

        if (diffDown > 0 && diffDown < (pitchClass - below))
            below = iv;
        if (diffUp > 0 && diffUp < (above - pitchClass))
            above = iv;
    }
    // Handle wraparound (e.g. nothing found below within the octave)
    if (below == -100) below = intervals.back() - 12;
    if (above == 100) above = intervals.front() + 12;

    int distDown = pitchClass - below;
    int distUp = above - pitchClass;

    int chosenInterval;
    if (snapMode == 1)       chosenInterval = below;                       // Snap Down
    else if (snapMode == 2)  chosenInterval = above;                       // Snap Up
    else                     chosenInterval = (distDown <= distUp) ? below : above; // Nearest

    return midiNoteNumber + (chosenInterval - pitchClass);
}

//==============================================================================
void ScaleLockAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Merge in notes from the on-screen piano / computer-keyboard typing,
    // so you can play with no MIDI hardware plugged in at all.
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    juce::MidiBuffer processedMidi;

    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        const auto samplePos = metadata.samplePosition;

        if (message.isNoteOn() || message.isNoteOff())
        {
            int snapped = snapNoteToScale (message.getNoteNumber());
            snapped = juce::jlimit (0, 127, snapped);

            juce::MidiMessage newMessage = message.isNoteOn()
                ? juce::MidiMessage::noteOn (message.getChannel(), snapped, message.getFloatVelocity())
                : juce::MidiMessage::noteOff (message.getChannel(), snapped);

            newMessage.setTimeStamp (message.getTimeStamp());
            processedMidi.addEvent (newMessage, samplePos);
        }
        else
        {
            processedMidi.addEvent (message, samplePos);
        }
    }

    // If you've loaded one of your own synths, its sound plays here instead
    // of the built-in fallback tone — same scale-correction either way.
    if (hostedInstrument != nullptr)
        hostedInstrument->processBlock (buffer, processedMidi);
    else
        synth.renderNextBlock (buffer, processedMidi, 0, buffer.getNumSamples());
}

//==============================================================================
void ScaleLockAudioProcessor::loadInstrumentPlugin (
    const juce::File& pluginFile,
    std::function<void (bool success, const juce::String& message)> onComplete)
{
    juce::OwnedArray<juce::PluginDescription> found;
    for (auto* format : formatManager.getFormats())
        format->findAllTypesForFile (found, pluginFile.getFullPathName());

    if (found.isEmpty())
    {
        if (onComplete)
            onComplete (false, "Couldn't read that plugin file: " + pluginFile.getFullPathName());
        return;
    }

    formatManager.createPluginInstanceAsync (
        *found.getFirst(), currentSampleRate, currentBlockSize,
        [this, onComplete] (std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error)
        {
            if (instance == nullptr)
            {
                if (onComplete)
                    onComplete (false, error.isNotEmpty() ? error : "Failed to load plugin.");
                return;
            }

            suspendProcessing (true);
            hostedInstrument = std::move (instance);
            hostedInstrument->enableAllBuses();
            hostedInstrument->setPlayConfigDetails (0, getTotalNumOutputChannels(), currentSampleRate, currentBlockSize);
            hostedInstrument->prepareToPlay (currentSampleRate, currentBlockSize);
            // If your synth has its own internal latency (e.g. oversampling),
            // report it upward so Serato's delay compensation stays accurate.
            setLatencySamples (hostedInstrument->getLatencySamples());
            suspendProcessing (false);

            if (onComplete)
                onComplete (true, hostedInstrument->getName() + " loaded.");
        });
}

void ScaleLockAudioProcessor::unloadInstrumentPlugin()
{
    suspendProcessing (true);
    hostedInstrument.reset();
    setLatencySamples (0);
    suspendProcessing (false);
}

juce::String ScaleLockAudioProcessor::getHostedInstrumentName() const
{
    return hostedInstrument != nullptr ? hostedInstrument->getName() : juce::String();
}

juce::AudioProcessorEditor* ScaleLockAudioProcessor::createHostedInstrumentEditor()
{
    if (hostedInstrument != nullptr && hostedInstrument->hasEditor())
        return hostedInstrument->createEditorIfNeeded();
    return nullptr;
}

//==============================================================================
juce::AudioProcessorEditor* ScaleLockAudioProcessor::createEditor()
{
    return new ScaleLockAudioProcessorEditor (*this);
}

//==============================================================================
void ScaleLockAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream (destData, true);
    stream.writeInt (rootNoteParam->getIndex());
    stream.writeInt (scaleTypeParam->getIndex());
    stream.writeInt (snapModeParam->getIndex());
}

void ScaleLockAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream (data, (size_t) sizeInBytes, false);
    if (stream.getNumBytesRemaining() >= 12)
    {
        *rootNoteParam = stream.readInt();
        *scaleTypeParam = stream.readInt();
        *snapModeParam = stream.readInt();
    }
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ScaleLockAudioProcessor();
}
