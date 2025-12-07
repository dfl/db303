#pragma once

#include <JuceHeader.h>
#include "../../AcidSeq303.h"

//==============================================================================
class AcidSeqEditor : public juce::AudioProcessorEditor,
                       private juce::Timer,
                       private juce::KeyListener
{
public:
    explicit AcidSeqEditor(AcidSeq303&);
    ~AcidSeqEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

    // Keyboard input
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

private:
    void timerCallback() override;
    void drawTrackerRow(juce::Graphics& g, int step, int y, bool isSelected, bool isPlaying);
    juce::String getNoteNameForKey(int key);

    AcidSeq303& processorRef;

    // Tracker state
    static constexpr int numSteps = 16;
    int editStep = 0;           // Currently selected step for editing
    int editColumn = 0;         // 0=note, 1=octave, 2=accent, 3=slide, 4=gate
    int lastPlayingStep = -1;

    // Layout constants
    static constexpr int rowHeight = 22;
    static constexpr int headerHeight = 30;
    static constexpr int colStep = 40;
    static constexpr int colNote = 50;
    static constexpr int colOct = 35;
    static constexpr int colAcc = 35;
    static constexpr int colSld = 35;
    static constexpr int colGate = 40;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AcidSeqEditor)
};
