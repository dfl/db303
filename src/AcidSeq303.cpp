#include "AcidSeq303.h"
#include "gui/acidseq/AcidSeqGui.h"

//==============================================================================
AcidSeq303::AcidSeq303()
    : AudioProcessor(BusesProperties())  // MIDI effect: no audio buses
{
    // Start with empty pattern - user will enter notes via tracker interface
}

AcidSeq303::~AcidSeq303()
{
}

void AcidSeq303::clearPattern()
{
    auto* pattern = sequencer.getPattern(0);
    if (pattern) {
        pattern->clear();
    }
}

void AcidSeq303::randomizePattern()
{
    auto* pattern = sequencer.getPattern(0);
    if (!pattern) return;

    // TB-303 style randomization (like changing batteries)
    for (int step = 0; step < patternLength; ++step) {
        // ~60% chance of gate being on
        bool gate = random.nextFloat() < 0.6f;
        pattern->setGate(step, gate);

        if (gate) {
            // Random note (0-11 for chromatic, but favor certain notes for acid sound)
            int noteWeights[] = {0, 0, 0, 2, 3, 5, 5, 7, 7, 7, 10, 10}; // Favor root, 5th, minor 7th
            int key = noteWeights[random.nextInt(12)];
            pattern->setKey(step, key);

            // Random octave (-1 to +1, favor 0)
            int octave = random.nextInt(3) - 1;
            pattern->setOctave(step, octave);

            // ~25% chance of accent
            pattern->setAccent(step, random.nextFloat() < 0.25f);

            // ~20% chance of slide
            pattern->setSlide(step, random.nextFloat() < 0.20f);
        } else {
            pattern->setKey(step, 0);
            pattern->setOctave(step, 0);
            pattern->setAccent(step, false);
            pattern->setSlide(step, false);
        }
    }

    // Clear steps beyond pattern length
    for (int step = patternLength; step < 16; ++step) {
        pattern->setGate(step, false);
        pattern->setKey(step, 0);
        pattern->setOctave(step, 0);
        pattern->setAccent(step, false);
        pattern->setSlide(step, false);
    }
}

void AcidSeq303::scramblePattern()
{
    auto* pattern = sequencer.getPattern(0);
    if (!pattern) return;

    // Fisher-Yates shuffle of steps within pattern length
    for (int i = patternLength - 1; i > 0; --i) {
        int j = random.nextInt(i + 1);
        if (i != j) {
            // Swap step i and step j
            auto* noteI = pattern->getNote(i);
            auto* noteJ = pattern->getNote(j);

            // Temp storage
            int keyI = noteI->key, octI = noteI->octave;
            bool accI = noteI->accent, sldI = noteI->slide, gateI = noteI->gate;

            // Copy j to i
            pattern->setKey(i, noteJ->key);
            pattern->setOctave(i, noteJ->octave);
            pattern->setAccent(i, noteJ->accent);
            pattern->setSlide(i, noteJ->slide);
            pattern->setGate(i, noteJ->gate);

            // Copy temp to j
            pattern->setKey(j, keyI);
            pattern->setOctave(j, octI);
            pattern->setAccent(j, accI);
            pattern->setSlide(j, sldI);
            pattern->setGate(j, gateI);
        }
    }
}

void AcidSeq303::transposePattern(int semitones)
{
    auto* pattern = sequencer.getPattern(0);
    if (!pattern) return;

    for (int step = 0; step < patternLength; ++step) {
        if (pattern->getGate(step)) {
            int key = pattern->getKey(step);
            int octave = pattern->getOctave(step);

            // Convert to absolute semitone position
            int totalSemitones = octave * 12 + key + semitones;

            // Clamp to reasonable range (-2 to +2 octaves from base)
            totalSemitones = juce::jlimit(-24, 36, totalSemitones);

            // Convert back to key and octave
            int newOctave = totalSemitones / 12;
            int newKey = totalSemitones % 12;
            if (newKey < 0) {
                newKey += 12;
                newOctave -= 1;
            }

            pattern->setKey(step, newKey);
            pattern->setOctave(step, newOctave);
        }
    }
}

void AcidSeq303::cyclePattern(int steps)
{
    auto* pattern = sequencer.getPattern(0);
    if (!pattern) return;

    // Use the built-in circular shift, but only within pattern length
    // We need to implement our own since the built-in shifts all 16 steps

    if (steps == 0 || patternLength <= 1) return;

    // Normalize steps to positive modulo
    steps = ((steps % patternLength) + patternLength) % patternLength;
    if (steps == 0) return;

    // Create temp arrays for the shift
    struct StepData {
        int key, octave;
        bool accent, slide, gate;
    };
    StepData temp[16];

    // Copy current pattern
    for (int i = 0; i < patternLength; ++i) {
        auto* note = pattern->getNote(i);
        temp[i] = {note->key, note->octave, note->accent, note->slide, note->gate};
    }

    // Write back shifted (positive steps = shift right)
    for (int i = 0; i < patternLength; ++i) {
        int srcIdx = (i - steps + patternLength) % patternLength;
        pattern->setKey(i, temp[srcIdx].key);
        pattern->setOctave(i, temp[srcIdx].octave);
        pattern->setAccent(i, temp[srcIdx].accent);
        pattern->setSlide(i, temp[srcIdx].slide);
        pattern->setGate(i, temp[srcIdx].gate);
    }
}

//==============================================================================
const juce::String AcidSeq303::getName() const
{
    return JucePlugin_Name;
}

bool AcidSeq303::acceptsMidi() const
{
    return false;
}

bool AcidSeq303::producesMidi() const
{
    return true;
}

bool AcidSeq303::isMidiEffect() const
{
    return true;
}

double AcidSeq303::getTailLengthSeconds() const
{
    return 0.0;
}

int AcidSeq303::getNumPrograms()
{
    return 1;
}

int AcidSeq303::getCurrentProgram()
{
    return 0;
}

void AcidSeq303::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String AcidSeq303::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void AcidSeq303::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void AcidSeq303::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    sampleRate = newSampleRate;
    sequencer.setSampleRate(newSampleRate);
    currentStep = -1;
    previousStep = -1;
    lastNoteOn = -1;
    lastNoteSlide = false;
}

void AcidSeq303::releaseResources()
{
}

bool AcidSeq303::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // MIDI effect - accept any layout
    juce::ignoreUnused(layouts);
    return true;
}

void AcidSeq303::generateMidiForStep(juce::MidiBuffer& midi, int samplePosition,
                                      rosic::AcidNote* note, int step, int baseNote)
{
    juce::ignoreUnused(step);  // Step index available if needed for future features

    int midiNote = baseNote + note->key + (note->octave * 12);
    int velocity = note->accent ? 127 : 100;

    if (note->gate) {
        // For legato (slide): send new note ON before old note OFF
        if (lastNoteSlide && lastNoteOn >= 0) {
            // Slide: new note first, then release old (creates overlap for legato)
            midi.addEvent(juce::MidiMessage::noteOn(1, midiNote, (juce::uint8)velocity),
                          samplePosition);
            midi.addEvent(juce::MidiMessage::noteOff(1, lastNoteOn),
                          samplePosition + 1);
        } else {
            // No slide: normal note off then note on
            if (lastNoteOn >= 0) {
                midi.addEvent(juce::MidiMessage::noteOff(1, lastNoteOn),
                              samplePosition);
            }
            midi.addEvent(juce::MidiMessage::noteOn(1, midiNote, (juce::uint8)velocity),
                          samplePosition + 1);
        }
        lastNoteOn = midiNote;
        lastNoteSlide = note->slide;
    } else {
        // Gate closed - note off (rest)
        if (lastNoteOn >= 0) {
            midi.addEvent(juce::MidiMessage::noteOff(1, lastNoteOn), samplePosition);
            lastNoteOn = -1;
        }
        lastNoteSlide = false;
    }
}

void AcidSeq303::queuePreviewNote(int noteNumber, int velocity, int durationMs)
{
    previewVelocity.store(velocity);
    previewDurationSamples.store(static_cast<int>(sampleRate * durationMs / 1000.0));
    previewNoteToPlay.store(noteNumber);  // Set last so other values are ready
}

void AcidSeq303::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output buffer - we're a MIDI effect
    buffer.clear();
    midi.clear();

    int numSamples = buffer.getNumSamples();

    // Handle new preview note request (from GUI)
    int noteToStart = previewNoteToPlay.exchange(-1);
    if (noteToStart >= 0) {
        // Turn off any currently playing preview note first
        if (previewActiveNote >= 0) {
            midi.addEvent(juce::MidiMessage::noteOff(1, previewActiveNote), 0);
        }
        // Start the new note
        int velocity = previewVelocity.load();
        midi.addEvent(juce::MidiMessage::noteOn(1, noteToStart, (juce::uint8)velocity), 1);
        previewActiveNote = noteToStart;
        previewNoteOffCountdown = previewDurationSamples.load();
    }

    // Handle preview note-off countdown
    if (previewNoteOffCountdown > 0 && previewActiveNote >= 0) {
        if (previewNoteOffCountdown <= numSamples) {
            midi.addEvent(juce::MidiMessage::noteOff(1, previewActiveNote), previewNoteOffCountdown);
            previewActiveNote = -1;
            previewNoteOffCountdown = 0;
        } else {
            previewNoteOffCountdown -= numSamples;
        }
    }

    auto* playHead = getPlayHead();
    if (!playHead) return;

    auto position = playHead->getPosition();
    if (!position.hasValue()) return;

    bool isPlaying = position->getIsPlaying();
    double bpm = position->getBpm().orFallback(120.0);
    auto ppqPositionOpt = position->getPpqPosition();

    if (!ppqPositionOpt.hasValue()) return;
    double ppqPosition = *ppqPositionOpt;

    // Handle transport stop - send note off
    if (wasPlaying && !isPlaying) {
        if (lastNoteOn >= 0) {
            midi.addEvent(juce::MidiMessage::noteOff(1, lastNoteOn), 0);
            lastNoteOn = -1;
        }
        wasPlaying = false;
        currentStep = -1;
        previousStep = -1;
        return;
    }

    if (!isPlaying) return;
    wasPlaying = true;

    // Calculate timing
    double samplesPerBeat = (sampleRate * 60.0) / bpm;
    int baseNote = 36; // C2

    // Process each sample to find step boundaries
    for (int sample = 0; sample < numSamples; ++sample) {
        double samplePpq = ppqPosition + (static_cast<double>(sample) / samplesPerBeat);

        // 4 steps per beat (16th notes), wrap at patternLength for polyrhythms
        double stepsFromStart = samplePpq * 4.0;
        int stepAtSample = static_cast<int>(std::floor(stepsFromStart)) % patternLength;

        // Handle negative PPQ (before song start)
        if (samplePpq < 0) {
            stepAtSample = ((stepAtSample % patternLength) + patternLength) % patternLength;
        }

        if (stepAtSample != currentStep) {
            // New step - generate MIDI
            previousStep = currentStep;
            currentStep = stepAtSample;

            auto* note = sequencer.getPattern(0)->getNote(currentStep);
            if (note) {
                generateMidiForStep(midi, sample, note, currentStep, baseNote);
            }
        }
    }
}

//==============================================================================
bool AcidSeq303::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AcidSeq303::createEditor()
{
    return new AcidSeqEditor(*this);
}

//==============================================================================
void AcidSeq303::getStateInformation(juce::MemoryBlock& destData)
{
    // Save pattern data as XML
    juce::XmlElement xml("AcidSeq303State");
    xml.setAttribute("patternLength", patternLength);

    auto* pattern = sequencer.getPattern(0);
    if (pattern) {
        auto* stepsXml = xml.createNewChildElement("Steps");
        for (int i = 0; i < 16; ++i) {
            auto* stepXml = stepsXml->createNewChildElement("Step");
            stepXml->setAttribute("index", i);
            stepXml->setAttribute("key", pattern->getKey(i));
            stepXml->setAttribute("octave", pattern->getOctave(i));
            stepXml->setAttribute("accent", pattern->getAccent(i));
            stepXml->setAttribute("slide", pattern->getSlide(i));
            stepXml->setAttribute("gate", pattern->getGate(i));
        }
    }

    copyXmlToBinary(xml, destData);
}

void AcidSeq303::setStateInformation(const void* data, int sizeInBytes)
{
    // Load pattern data from XML
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml && xml->hasTagName("AcidSeq303State")) {
        patternLength = xml->getIntAttribute("patternLength", 16);

        auto* pattern = sequencer.getPattern(0);
        if (pattern) {
            auto* stepsXml = xml->getChildByName("Steps");
            if (stepsXml) {
                for (auto* stepXml : stepsXml->getChildIterator()) {
                    int i = stepXml->getIntAttribute("index", 0);
                    if (i >= 0 && i < 16) {
                        pattern->setKey(i, stepXml->getIntAttribute("key", 0));
                        pattern->setOctave(i, stepXml->getIntAttribute("octave", 0));
                        pattern->setAccent(i, stepXml->getBoolAttribute("accent", false));
                        pattern->setSlide(i, stepXml->getBoolAttribute("slide", false));
                        pattern->setGate(i, stepXml->getBoolAttribute("gate", false));
                    }
                }
            }
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AcidSeq303();
}
