#pragma once

#include <JuceHeader.h>
#include "dsp/open303/rosic_AcidSequencer.h"

//==============================================================================
class AcidSeq303 : public juce::AudioProcessor
{
public:
    //==============================================================================
    AcidSeq303();
    ~AcidSeq303() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

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
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Pattern access for GUI
    rosic::AcidSequencer* getSequencer() { return &sequencer; }
    int getCurrentStep() const { return currentStep; }

    // Pattern length for polyrhythms (1-32)
    int getPatternLength() const { return patternLength; }
    void setPatternLength(int length) { patternLength = juce::jlimit(1, 32, length); }

    // Pattern operations
    void clearPattern();
    void randomizePattern();
    void scramblePattern();      // Shuffle existing notes randomly
    void transposePattern(int semitones);  // Move all notes up/down
    void cyclePattern(int steps);  // Rotate pattern in time (positive = right)

    // Preview note (for GUI feedback when editing)
    void queuePreviewNote(int noteNumber, int velocity = 100, int durationMs = 150);
    int getBaseNote() const { return 36; }  // C2 base note

private:
    void generateMidiForStep(juce::MidiBuffer& midi, int samplePosition,
                             rosic::AcidNote* note, int step, int baseNote);

    rosic::AcidSequencer sequencer;

    // Transport state
    double sampleRate = 44100.0;
    int currentStep = -1;
    int previousStep = -1;
    bool wasPlaying = false;
    int lastNoteOn = -1;        // For legato: track last note
    bool lastNoteSlide = false; // Was last note a slide?

    // Pattern settings
    int patternLength = 16;     // Length for polyrhythms (1-32)
    juce::Random random;        // For randomization

    // Preview note state
    std::atomic<int> previewNoteToPlay{-1};      // Note number to start playing (-1 = none)
    std::atomic<int> previewVelocity{100};
    std::atomic<int> previewDurationSamples{0};
    int previewActiveNote = -1;                   // Currently playing preview note
    int previewNoteOffCountdown = 0;              // Samples until note-off

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AcidSeq303)
};
