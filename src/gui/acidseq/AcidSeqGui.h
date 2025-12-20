#pragma once

#include <JuceHeader.h>
#include "../../AcidSeq303.h"
#include "../../dsp/open303/rosic_AcidPattern.h"

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

    // Mouse input
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    // KeyListener interface
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
    void timerCallback() override;
    void drawTrackerRow(juce::Graphics& g, int step, int y, bool isCursor, bool isPlaying, bool isInSelection);
    juce::String getNoteNameForKey(int key);

    // Undo/redo system
    struct PatternSnapshot {
        rosic::AcidNote notes[32];
        int patternLength;
    };

    void capturePatternState();  // Save current state to undo stack
    void undo();
    void redo();
    void restorePatternState(const PatternSnapshot& snapshot);

    std::vector<PatternSnapshot> undoStack;
    int undoStackPosition = -1;  // Current position in undo stack
    static constexpr int maxUndoStates = 50;

    AcidSeq303& processorRef;

    // View mode
    bool pianoRollMode = false;  // false = tracker, true = piano roll
    void drawPianoRollView(juce::Graphics& g);
    void handlePianoRollClick(int x, int y);

    // Tracker state
    static constexpr int numSteps = 32;
    static constexpr int maxVisibleRows = 16;  // Maximum rows visible at once
    int scrollOffset = 0;       // First visible row (for scrolling)
    int editStep = 0;           // Currently selected step for editing
    int editColumn = 0;         // 0=note, 1=octave, 2=accent, 3=slide, 4=gate
    int lastPlayingStep = -1;

    // Range selection
    int selectionAnchor = -1;   // Start of selection (-1 = no selection, just cursor)
    int getSelectionStart() const { return selectionAnchor < 0 ? editStep : juce::jmin(selectionAnchor, editStep); }
    int getSelectionEnd() const { return selectionAnchor < 0 ? editStep : juce::jmax(selectionAnchor, editStep); }
    bool hasSelection() const { return selectionAnchor >= 0 && selectionAnchor != editStep; }
    void clearSelection() { selectionAnchor = -1; }

    // Step preview
    void triggerPreviewNote(int step);

    // Pattern length text editor
    std::unique_ptr<juce::Label> lengthLabel;
    std::unique_ptr<juce::Label> lengthEditor;

    // D-pad buttons for transpose/shift
    std::unique_ptr<juce::TextButton> transposeUpBtn;
    std::unique_ptr<juce::TextButton> transposeDownBtn;
    std::unique_ptr<juce::TextButton> shiftLeftBtn;
    std::unique_ptr<juce::TextButton> shiftRightBtn;

    // View mode toggle
    std::unique_ptr<juce::TextButton> viewToggleBtn;

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
