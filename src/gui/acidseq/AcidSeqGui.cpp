#include "AcidSeqGui.h"

//==============================================================================
AcidSeqEditor::AcidSeqEditor(AcidSeq303& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(320, 530);
    setWantsKeyboardFocus(true);
    addKeyListener(this);
    startTimerHz(30);

    // Create pattern length label and text editor
    lengthLabel = std::make_unique<juce::Label>("lengthLabel", "Length:");
    lengthLabel->setBounds(getWidth() - 105, 5, 50, 20);
    lengthLabel->setJustificationType(juce::Justification::centredRight);
    lengthLabel->setColour(juce::Label::textColourId, juce::Colours::orange);
    addAndMakeVisible(*lengthLabel);

    lengthEditor = std::make_unique<juce::Label>("lengthEditor", juce::String(processorRef.getPatternLength()));
    lengthEditor->setBounds(getWidth() - 50, 5, 40, 20);
    lengthEditor->setEditable(true);
    lengthEditor->setJustificationType(juce::Justification::centred);
    lengthEditor->setColour(juce::Label::backgroundColourId, juce::Colour(0xff2a2a3e));
    lengthEditor->setColour(juce::Label::textColourId, juce::Colours::white);
    lengthEditor->setColour(juce::Label::outlineColourId, juce::Colours::grey);
    addAndMakeVisible(*lengthEditor);

    // Set up text editor change handler
    lengthEditor->onTextChange = [this] {
        int newLength = lengthEditor->getText().getIntValue();
        if (newLength >= 1 && newLength <= 32) {
            capturePatternState();
            processorRef.setPatternLength(newLength);
            // Adjust scrollOffset if needed
            if (newLength <= maxVisibleRows) {
                scrollOffset = 0;
            } else if (scrollOffset + maxVisibleRows > newLength) {
                scrollOffset = juce::jmax(0, newLength - maxVisibleRows);
            }
            // Ensure editStep is within new length
            if (editStep >= newLength) {
                editStep = newLength - 1;
            }
            repaint();
        } else {
            // Reset to current value if invalid
            lengthEditor->setText(juce::String(processorRef.getPatternLength()), juce::dontSendNotification);
        }
    };

    // D-pad buttons for transpose/shift
    auto buttonColor = juce::Colour(0xff3d3d5c);
    auto textColor = juce::Colours::white;

    transposeUpBtn = std::make_unique<juce::TextButton>("+");
    transposeUpBtn->setBounds(getWidth() - 70, getHeight() - 100, 30, 22);
    transposeUpBtn->setColour(juce::TextButton::buttonColourId, buttonColor);
    transposeUpBtn->setColour(juce::TextButton::textColourOffId, textColor);
    transposeUpBtn->setTooltip("Transpose up (+1 semitone)");
    transposeUpBtn->onClick = [this] {
        capturePatternState();
        processorRef.transposePattern(1);
        repaint();
    };
    addAndMakeVisible(*transposeUpBtn);

    transposeDownBtn = std::make_unique<juce::TextButton>("-");
    transposeDownBtn->setBounds(getWidth() - 70, getHeight() - 55, 30, 22);
    transposeDownBtn->setColour(juce::TextButton::buttonColourId, buttonColor);
    transposeDownBtn->setColour(juce::TextButton::textColourOffId, textColor);
    transposeDownBtn->setTooltip("Transpose down (-1 semitone)");
    transposeDownBtn->onClick = [this] {
        capturePatternState();
        processorRef.transposePattern(-1);
        repaint();
    };
    addAndMakeVisible(*transposeDownBtn);

    shiftLeftBtn = std::make_unique<juce::TextButton>("<");
    shiftLeftBtn->setBounds(getWidth() - 100, getHeight() - 78, 30, 22);
    shiftLeftBtn->setColour(juce::TextButton::buttonColourId, buttonColor);
    shiftLeftBtn->setColour(juce::TextButton::textColourOffId, textColor);
    shiftLeftBtn->setTooltip("Shift pattern left");
    shiftLeftBtn->onClick = [this] {
        capturePatternState();
        processorRef.cyclePattern(-1);
        repaint();
    };
    addAndMakeVisible(*shiftLeftBtn);

    shiftRightBtn = std::make_unique<juce::TextButton>(">");
    shiftRightBtn->setBounds(getWidth() - 40, getHeight() - 78, 30, 22);
    shiftRightBtn->setColour(juce::TextButton::buttonColourId, buttonColor);
    shiftRightBtn->setColour(juce::TextButton::textColourOffId, textColor);
    shiftRightBtn->setTooltip("Shift pattern right");
    shiftRightBtn->onClick = [this] {
        capturePatternState();
        processorRef.cyclePattern(1);
        repaint();
    };
    addAndMakeVisible(*shiftRightBtn);

    // View mode toggle button
    viewToggleBtn = std::make_unique<juce::TextButton>("Grid");
    viewToggleBtn->setBounds(10, 5, 50, 20);
    viewToggleBtn->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a5a7f));
    viewToggleBtn->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    viewToggleBtn->setTooltip("Toggle between Tracker and Piano Roll view");
    viewToggleBtn->onClick = [this] {
        pianoRollMode = !pianoRollMode;
        viewToggleBtn->setButtonText(pianoRollMode ? "List" : "Grid");
        repaint();
    };
    addAndMakeVisible(*viewToggleBtn);

    // Capture initial pattern state for undo
    capturePatternState();
}

AcidSeqEditor::~AcidSeqEditor()
{
    removeKeyListener(this);
    stopTimer();
}

//==============================================================================
juce::String AcidSeqEditor::getNoteNameForKey(int key)
{
    static const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "C+"};
    if (key >= 0 && key <= 12)
        return noteNames[key];
    return "C";
}

void AcidSeqEditor::drawTrackerRow(juce::Graphics& g, int step, int y, bool isCursor, bool isPlaying, bool isInSelection)
{
    auto* pattern = processorRef.getSequencer()->getPattern(0);
    if (!pattern) return;

    auto* note = pattern->getNote(step);
    int x = 10;
    int pLen = processorRef.getPatternLength();
    bool beyondLength = (step >= pLen);
    bool hasNoteData = (note->key != 0 || note->octave != 0 || note->accent || note->slide);
    bool gateOn = note->gate;

    // Row background
    juce::Colour rowBg = juce::Colour(0xff1a1a2e);
    if (beyondLength)
        rowBg = juce::Colour(0xff101018);  // Darker for steps beyond pattern length
    else if (!gateOn && hasNoteData)
        rowBg = juce::Colour(0xff2a1a1a);  // Red tint for muted (gate off but has note)
    else if (isPlaying && gateOn)
        rowBg = juce::Colour(0xff2a4a2a);  // Green tint for playing
    if (isInSelection && !isCursor)
        rowBg = juce::Colour(0xff2a3a5a);  // Blue tint for selection
    if (isCursor)
        rowBg = rowBg.brighter(0.3f);

    g.setColour(rowBg);
    g.fillRect(x, y, colStep + colNote + colOct + colAcc + colSld + colGate, rowHeight);

    // Step number
    juce::Colour stepColor = juce::Colours::grey;
    if (isPlaying && gateOn) stepColor = juce::Colours::lime;
    if (beyondLength) stepColor = juce::Colours::darkgrey;
    g.setColour(stepColor);
    g.setFont(14.0f);
    g.drawText(juce::String::formatted("%02d", step + 1), x, y, colStep, rowHeight, juce::Justification::centred);
    x += colStep;

    // Dim everything if beyond pattern length
    float alpha = beyondLength ? 0.4f : 1.0f;

    // Determine if we should show note data (either gate on, or has data)
    bool showNote = gateOn || hasNoteData;

    // Note column - show strikethrough for muted (gate off)
    juce::Colour noteColor = juce::Colours::grey.withAlpha(alpha);
    if (showNote) {
        noteColor = gateOn ? juce::Colours::white.withAlpha(alpha) : juce::Colours::red.withAlpha(0.6f);
    }
    if (isCursor && editColumn == 0 && !beyondLength) {
        g.setColour(juce::Colour(0xff4a6a9f));
        g.fillRect(x, y, colNote, rowHeight);
    }
    g.setColour(noteColor);
    if (showNote) {
        g.drawText(getNoteNameForKey(note->key), x, y, colNote, rowHeight, juce::Justification::centred);
        // Draw strikethrough for muted (gate off)
        if (!gateOn) {
            g.drawLine(x + 5, y + rowHeight/2, x + colNote - 5, y + rowHeight/2, 1.0f);
        }
    } else {
        g.drawText("---", x, y, colNote, rowHeight, juce::Justification::centred);
    }
    x += colNote;

    // Octave column
    if (isCursor && editColumn == 1 && !beyondLength) {
        g.setColour(juce::Colour(0xff4a6a9f));
        g.fillRect(x, y, colOct, rowHeight);
    }
    g.setColour(noteColor);
    if (showNote) {
        g.drawText(juce::String(note->octave + 2), x, y, colOct, rowHeight, juce::Justification::centred);
    } else {
        g.drawText("-", x, y, colOct, rowHeight, juce::Justification::centred);
    }
    x += colOct;

    // Accent column
    if (isCursor && editColumn == 2 && !beyondLength) {
        g.setColour(juce::Colour(0xff4a6a9f));
        g.fillRect(x, y, colAcc, rowHeight);
    }
    g.setColour((note->accent ? juce::Colours::red : juce::Colours::grey).withAlpha(alpha));
    g.drawText(note->accent ? "A" : ".", x, y, colAcc, rowHeight, juce::Justification::centred);
    x += colAcc;

    // Slide column
    if (isCursor && editColumn == 3 && !beyondLength) {
        g.setColour(juce::Colour(0xff4a6a9f));
        g.fillRect(x, y, colSld, rowHeight);
    }
    g.setColour((note->slide ? juce::Colours::yellow : juce::Colours::grey).withAlpha(alpha));
    g.drawText(note->slide ? "S" : ".", x, y, colSld, rowHeight, juce::Justification::centred);
    x += colSld;

    // Gate column (shows play/mute status)
    if (isCursor && editColumn == 4 && !beyondLength) {
        g.setColour(juce::Colour(0xff4a6a9f));
        g.fillRect(x, y, colGate, rowHeight);
    }
    if (gateOn) {
        g.setColour(juce::Colours::cyan.withAlpha(alpha));
        g.drawText("ON", x, y, colGate, rowHeight, juce::Justification::centred);
    } else if (hasNoteData) {
        g.setColour(juce::Colours::red.withAlpha(alpha));
        g.drawText("off", x, y, colGate, rowHeight, juce::Justification::centred);
    } else {
        g.setColour(juce::Colours::darkgrey.withAlpha(alpha));
        g.drawText(".", x, y, colGate, rowHeight, juce::Justification::centred);
    }
}

void AcidSeqEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff12121c));

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawText(pianoRollMode ? "AcidSeq-303 Grid" : "AcidSeq-303 Tracker", 65, 5, getWidth() - 130, 25, juce::Justification::centred);

    if (pianoRollMode) {
        drawPianoRollView(g);
        return;
    }

    // Header row
    int pLen = processorRef.getPatternLength();
    int y = 35;
    int x = 10;
    g.setColour(juce::Colour(0xff3d3d5c));
    g.fillRect(x, y, colStep + colNote + colOct + colAcc + colSld + colGate, headerHeight - 5);

    g.setColour(juce::Colours::lightgrey);
    g.setFont(12.0f);
    g.drawText("Step", x, y, colStep, headerHeight - 5, juce::Justification::centred);
    x += colStep;
    g.drawText("Note", x, y, colNote, headerHeight - 5, juce::Justification::centred);
    x += colNote;
    g.drawText("Oct", x, y, colOct, headerHeight - 5, juce::Justification::centred);
    x += colOct;
    g.drawText("Acc", x, y, colAcc, headerHeight - 5, juce::Justification::centred);
    x += colAcc;
    g.drawText("Sld", x, y, colSld, headerHeight - 5, juce::Justification::centred);
    x += colSld;
    g.drawText("Gate", x, y, colGate, headerHeight - 5, juce::Justification::centred);

    // Pattern rows - only draw visible rows
    y = 35 + headerHeight;
    int playingStep = processorRef.getCurrentStep();

    // Calculate how many rows to display
    int numRowsToDisplay = juce::jmin(pLen, maxVisibleRows);
    int endStep = juce::jmin(scrollOffset + numRowsToDisplay, pLen);

    int selStart = getSelectionStart();
    int selEnd = getSelectionEnd();
    for (int step = scrollOffset; step < endStep; ++step) {
        bool isCursor = (step == editStep);
        bool isInSelection = (step >= selStart && step <= selEnd);
        bool isPlaying = (step == playingStep);
        drawTrackerRow(g, step, y, isCursor, isPlaying, isInSelection);
        y += rowHeight;
    }

    // Help text
    g.setColour(juce::Colours::grey);
    g.setFont(9.0f);

    int helpY = getHeight() - 105;
    g.drawText("A-G=note  #=sharp  0-4=octave  X=accent  S=slide", 10, helpY, getWidth() - 120, 13, juce::Justification::left);
    helpY += 12;
    g.drawText("M=mute  Del=clear  Shift+\xe2\x86\x91\xe2\x86\x93=select range", 10, helpY, getWidth() - 120, 13, juce::Justification::left);
    helpY += 12;
    g.drawText("[/]=length  R=random  T=scramble", 10, helpY, getWidth() - 120, 13, juce::Justification::left);
    helpY += 12;
    g.drawText("Ctrl+Z/Y=undo/redo", 10, helpY, getWidth() - 120, 13, juce::Justification::left);
    helpY += 12;
    g.setColour(juce::Colours::darkgrey);
    g.drawText("Pattern loops at step " + juce::String(pLen) + " | MIDI out", 10, helpY, getWidth() - 120, 13, juce::Justification::left);
}

void AcidSeqEditor::resized()
{
}

bool AcidSeqEditor::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    auto* pattern = processorRef.getSequencer()->getPattern(0);
    if (!pattern) return false;

    bool handled = true;

    // Check for modifier keys
    bool ctrlOrCmd = key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown();
    bool shiftPressed = key.getModifiers().isShiftDown();

    // Undo (Ctrl+Z or Cmd+Z)
    if (ctrlOrCmd && !shiftPressed && (key.getTextCharacter() == 'z' || key.getTextCharacter() == 'Z')) {
        undo();
    }
    // Redo (Ctrl+Shift+Z or Ctrl+Y)
    else if (ctrlOrCmd && ((shiftPressed && (key.getTextCharacter() == 'z' || key.getTextCharacter() == 'Z')) ||
                           (!shiftPressed && (key.getTextCharacter() == 'y' || key.getTextCharacter() == 'Y')))) {
        redo();
    }
    // Clear all pattern (Ctrl+C or Cmd+C)
    else if (ctrlOrCmd && (key.getTextCharacter() == 'c' || key.getTextCharacter() == 'C')) {
        capturePatternState();
        processorRef.clearPattern();
    }
    // Randomize pattern (R)
    else if (key.getTextCharacter() == 'r' || key.getTextCharacter() == 'R') {
        capturePatternState();
        processorRef.randomizePattern();
    }
    // Scramble pattern (T for "toss")
    else if (key.getTextCharacter() == 't' || key.getTextCharacter() == 'T') {
        capturePatternState();
        processorRef.scramblePattern();
    }
    // Transpose down (,)
    else if (key.getTextCharacter() == ',' || key.getTextCharacter() == '<') {
        capturePatternState();
        processorRef.transposePattern(-1);
    }
    // Transpose up (.)
    else if (key.getTextCharacter() == '.' || key.getTextCharacter() == '>') {
        capturePatternState();
        processorRef.transposePattern(1);
    }
    // Cycle left ({)
    else if (key.getTextCharacter() == '{') {
        capturePatternState();
        processorRef.cyclePattern(-1);
    }
    // Cycle right (})
    else if (key.getTextCharacter() == '}') {
        capturePatternState();
        processorRef.cyclePattern(1);
    }
    // Pattern length decrease ([)
    else if (key.getTextCharacter() == '[') {
        capturePatternState();
        int oldLen = processorRef.getPatternLength();
        processorRef.setPatternLength(oldLen - 1);
        int newLen = processorRef.getPatternLength();
        lengthEditor->setText(juce::String(newLen), juce::dontSendNotification);
        // Adjust scrollOffset if needed
        if (newLen <= maxVisibleRows) {
            scrollOffset = 0;
        } else if (scrollOffset + maxVisibleRows > newLen) {
            scrollOffset = juce::jmax(0, newLen - maxVisibleRows);
        }
        // Ensure editStep is within new length
        if (editStep >= newLen) {
            editStep = newLen - 1;
        }
    }
    // Pattern length increase (])
    else if (key.getTextCharacter() == ']') {
        capturePatternState();
        processorRef.setPatternLength(processorRef.getPatternLength() + 1);
        lengthEditor->setText(juce::String(processorRef.getPatternLength()), juce::dontSendNotification);
    }
    // Toggle gate (mute/unmute) with 'm'
    else if (!ctrlOrCmd && (key.getTextCharacter() == 'm' || key.getTextCharacter() == 'M')) {
        capturePatternState();
        pattern->setGate(editStep, !pattern->getGate(editStep));
    }
    // Navigation (Shift+Up/Down extends selection)
    else if (key.isKeyCode(juce::KeyPress::upKey)) {
        int pLen = processorRef.getPatternLength();
        if (shiftPressed) {
            // Start or extend selection
            if (selectionAnchor < 0) {
                selectionAnchor = editStep;  // Anchor at current position
            }
        } else {
            clearSelection();  // Clear selection when moving without shift
        }
        editStep = (editStep - 1 + pLen) % pLen;
        // Auto-scroll to keep editStep visible
        if (editStep < scrollOffset) {
            scrollOffset = editStep;
        }
    }
    else if (key.isKeyCode(juce::KeyPress::downKey)) {
        int pLen = processorRef.getPatternLength();
        if (shiftPressed) {
            // Start or extend selection
            if (selectionAnchor < 0) {
                selectionAnchor = editStep;  // Anchor at current position
            }
        } else {
            clearSelection();  // Clear selection when moving without shift
        }
        editStep = (editStep + 1) % pLen;
        // Auto-scroll to keep editStep visible
        if (editStep >= scrollOffset + maxVisibleRows) {
            scrollOffset = editStep - maxVisibleRows + 1;
        }
    }
    else if (key.isKeyCode(juce::KeyPress::leftKey)) {
        editColumn = (editColumn - 1 + 5) % 5;
    }
    else if (key.isKeyCode(juce::KeyPress::rightKey)) {
        editColumn = (editColumn + 1) % 5;
    }
    else if (key.isKeyCode(juce::KeyPress::returnKey)) {
        // Enter advances to next step (like a tracker)
        int pLen = processorRef.getPatternLength();
        editStep = (editStep + 1) % pLen;
        // Auto-scroll to keep editStep visible
        if (editStep >= scrollOffset + maxVisibleRows) {
            scrollOffset = editStep - maxVisibleRows + 1;
        } else if (editStep == 0) {
            // Wrapped around to beginning
            scrollOffset = 0;
        }
    }
    // Note entry (C, D, E, F, G, A, B) - only if not using Ctrl
    else if (!ctrlOrCmd && (key.getTextCharacter() == 'c' || key.getTextCharacter() == 'C')) {
        capturePatternState();
        pattern->setKey(editStep, 0);
        pattern->setGate(editStep, true);
        triggerPreviewNote(editStep);
        clearSelection();
        int pLen = processorRef.getPatternLength();
        editStep = (editStep + 1) % pLen;
        if (editStep >= scrollOffset + maxVisibleRows) scrollOffset = editStep - maxVisibleRows + 1;
        else if (editStep == 0) scrollOffset = 0;
    }
    else if (key.getTextCharacter() == 'd' || key.getTextCharacter() == 'D') {
        capturePatternState();
        pattern->setKey(editStep, 2);
        pattern->setGate(editStep, true);
        triggerPreviewNote(editStep);
        clearSelection();
        int pLen = processorRef.getPatternLength();
        editStep = (editStep + 1) % pLen;
        if (editStep >= scrollOffset + maxVisibleRows) scrollOffset = editStep - maxVisibleRows + 1;
        else if (editStep == 0) scrollOffset = 0;
    }
    else if (key.getTextCharacter() == 'e' || key.getTextCharacter() == 'E') {
        capturePatternState();
        pattern->setKey(editStep, 4);
        pattern->setGate(editStep, true);
        triggerPreviewNote(editStep);
        clearSelection();
        int pLen = processorRef.getPatternLength();
        editStep = (editStep + 1) % pLen;
        if (editStep >= scrollOffset + maxVisibleRows) scrollOffset = editStep - maxVisibleRows + 1;
        else if (editStep == 0) scrollOffset = 0;
    }
    else if (key.getTextCharacter() == 'f' || key.getTextCharacter() == 'F') {
        capturePatternState();
        pattern->setKey(editStep, 5);
        pattern->setGate(editStep, true);
        triggerPreviewNote(editStep);
        clearSelection();
        int pLen = processorRef.getPatternLength();
        editStep = (editStep + 1) % pLen;
        if (editStep >= scrollOffset + maxVisibleRows) scrollOffset = editStep - maxVisibleRows + 1;
        else if (editStep == 0) scrollOffset = 0;
    }
    else if (key.getTextCharacter() == 'g' || key.getTextCharacter() == 'G') {
        capturePatternState();
        pattern->setKey(editStep, 7);
        pattern->setGate(editStep, true);
        triggerPreviewNote(editStep);
        clearSelection();
        int pLen = processorRef.getPatternLength();
        editStep = (editStep + 1) % pLen;
        if (editStep >= scrollOffset + maxVisibleRows) scrollOffset = editStep - maxVisibleRows + 1;
        else if (editStep == 0) scrollOffset = 0;
    }
    else if (!ctrlOrCmd && (key.getTextCharacter() == 'a' || key.getTextCharacter() == 'A')) {
        capturePatternState();
        pattern->setKey(editStep, 9);
        pattern->setGate(editStep, true);
        triggerPreviewNote(editStep);
        clearSelection();
        int pLen = processorRef.getPatternLength();
        editStep = (editStep + 1) % pLen;
        if (editStep >= scrollOffset + maxVisibleRows) scrollOffset = editStep - maxVisibleRows + 1;
        else if (editStep == 0) scrollOffset = 0;
    }
    else if (key.getTextCharacter() == 'b' || key.getTextCharacter() == 'B') {
        capturePatternState();
        pattern->setKey(editStep, 11);
        pattern->setGate(editStep, true);
        triggerPreviewNote(editStep);
        clearSelection();
        int pLen = processorRef.getPatternLength();
        editStep = (editStep + 1) % pLen;
        if (editStep >= scrollOffset + maxVisibleRows) scrollOffset = editStep - maxVisibleRows + 1;
        else if (editStep == 0) scrollOffset = 0;
    }
    // Sharp - adds 1 to current note (making it sharp)
    else if (key.getTextCharacter() == '#') {
        capturePatternState();
        int currentKey = pattern->getKey(editStep);
        if (currentKey < 11) {
            pattern->setKey(editStep, currentKey + 1);
            triggerPreviewNote(editStep);
        }
        clearSelection();
    }
    // Octave (0-4 for octaves 0-4)
    else if (key.getTextCharacter() >= '0' && key.getTextCharacter() <= '4') {
        capturePatternState();
        int octave = key.getTextCharacter() - '0';
        pattern->setOctave(editStep, octave - 2);  // Offset by -2 since base is octave 2
        triggerPreviewNote(editStep);
        clearSelection();
    }
    // Toggle accent (X)
    else if (key.getTextCharacter() == 'x' || key.getTextCharacter() == 'X') {
        capturePatternState();
        pattern->setAccent(editStep, !pattern->getAccent(editStep));
    }
    // Toggle slide (S)
    else if (!ctrlOrCmd && (key.getTextCharacter() == 's' || key.getTextCharacter() == 'S')) {
        capturePatternState();
        pattern->setSlide(editStep, !pattern->getSlide(editStep));
    }
    // Delete/backspace clears selected steps (or current step if no selection)
    else if (key.isKeyCode(juce::KeyPress::deleteKey) || key.isKeyCode(juce::KeyPress::backspaceKey)) {
        capturePatternState();
        int startStep = getSelectionStart();
        int endStep = getSelectionEnd();
        for (int i = startStep; i <= endStep; ++i) {
            pattern->setGate(i, false);
            pattern->setAccent(i, false);
            pattern->setSlide(i, false);
            pattern->setKey(i, 0);
            pattern->setOctave(i, 0);
        }
        clearSelection();
    }
    else {
        handled = false;
    }

    if (handled) {
        repaint();
    }

    return handled;
}

void AcidSeqEditor::timerCallback()
{
    int currentStep = processorRef.getCurrentStep();
    if (currentStep != lastPlayingStep) {
        lastPlayingStep = currentStep;
        repaint();
    }
}

void AcidSeqEditor::capturePatternState()
{
    auto* pattern = processorRef.getSequencer()->getPattern(0);
    if (!pattern) return;

    PatternSnapshot snapshot;

    // Copy all 32 notes
    for (int i = 0; i < 32; ++i) {
        auto* note = pattern->getNote(i);
        snapshot.notes[i] = *note;
    }

    // Store pattern length
    snapshot.patternLength = processorRef.getPatternLength();

    // If we're in the middle of the undo stack (after undo), remove all redo states
    if (undoStackPosition < (int)undoStack.size() - 1) {
        undoStack.erase(undoStack.begin() + undoStackPosition + 1, undoStack.end());
    }

    // Add new snapshot
    undoStack.push_back(snapshot);

    // Limit stack size
    if (undoStack.size() > maxUndoStates) {
        undoStack.erase(undoStack.begin());
    } else {
        undoStackPosition++;
    }
}

void AcidSeqEditor::restorePatternState(const PatternSnapshot& snapshot)
{
    auto* pattern = processorRef.getSequencer()->getPattern(0);
    if (!pattern) return;

    // Restore all 32 notes
    for (int i = 0; i < 32; ++i) {
        pattern->setKey(i, snapshot.notes[i].key);
        pattern->setOctave(i, snapshot.notes[i].octave);
        pattern->setAccent(i, snapshot.notes[i].accent);
        pattern->setSlide(i, snapshot.notes[i].slide);
        pattern->setGate(i, snapshot.notes[i].gate);
    }

    // Restore pattern length
    processorRef.setPatternLength(snapshot.patternLength);
}

void AcidSeqEditor::undo()
{
    if (undoStackPosition <= 0) return;  // Nothing to undo

    undoStackPosition--;
    restorePatternState(undoStack[static_cast<size_t>(undoStackPosition)]);
    lengthEditor->setText(juce::String(processorRef.getPatternLength()), juce::dontSendNotification);
    repaint();
}

void AcidSeqEditor::redo()
{
    if (undoStackPosition >= (int)undoStack.size() - 1) return;  // Nothing to redo

    undoStackPosition++;
    restorePatternState(undoStack[static_cast<size_t>(undoStackPosition)]);
    lengthEditor->setText(juce::String(processorRef.getPatternLength()), juce::dontSendNotification);
    repaint();
}

void AcidSeqEditor::triggerPreviewNote(int step)
{
    auto* pattern = processorRef.getSequencer()->getPattern(0);
    if (!pattern) return;

    auto* note = pattern->getNote(step);
    if (!note || !note->gate) return;  // Don't preview muted steps

    // Calculate MIDI note number: baseNote + key + (octave * 12)
    int baseNote = processorRef.getBaseNote();
    int midiNote = baseNote + note->key + (note->octave * 12);

    // Use higher velocity for accented notes
    int velocity = note->accent ? 120 : 80;

    processorRef.queuePreviewNote(midiNote, velocity, 150);
}

void AcidSeqEditor::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    int pLen = processorRef.getPatternLength();

    // Only allow scrolling if pattern length > maxVisibleRows
    if (pLen <= maxVisibleRows)
        return;

    // Scroll up/down
    if (wheel.deltaY > 0) {
        scrollOffset = juce::jmax(0, scrollOffset - 1);
    } else if (wheel.deltaY < 0) {
        scrollOffset = juce::jmin(pLen - maxVisibleRows, scrollOffset + 1);
    }

    repaint();
}

void AcidSeqEditor::drawPianoRollView(juce::Graphics& g)
{
    auto* pattern = processorRef.getSequencer()->getPattern(0);
    if (!pattern) return;

    int pLen = processorRef.getPatternLength();
    int playingStep = processorRef.getCurrentStep();

    // Layout constants for piano roll
    const int startY = 35;
    const int cellWidth = 18;
    const int cellHeight = 13;  // Slightly smaller to fit 13 keys
    const int numKeys = 13;  // C to C+ (high C), like original TB-303
    const int leftMargin = 25;  // Space for note labels
    const int maxStepsVisible = 16;  // Show 16 steps at a time

    // Calculate visible range with scrolling support
    int stepsToShow = juce::jmin(pLen, maxStepsVisible);
    int pianoRollScrollOffset = scrollOffset;
    if (pianoRollScrollOffset + stepsToShow > pLen) {
        pianoRollScrollOffset = juce::jmax(0, pLen - stepsToShow);
    }

    // Note names (from top to bottom: C+ to C, like TB-303)
    static const char* noteNames[] = {"C+", "B", "A#", "A", "G#", "G", "F#", "F", "E", "D#", "D", "C#", "C"};
    static const int noteValues[] = {12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    static const bool isBlackKey[] = {false, false, true, false, true, false, true, false, false, true, false, true, false};

    // Draw note labels on left
    g.setFont(10.0f);
    for (int i = 0; i < numKeys; ++i) {
        int y = startY + i * cellHeight;
        g.setColour(isBlackKey[i] ? juce::Colours::grey : juce::Colours::lightgrey);
        g.drawText(noteNames[i], 2, y, leftMargin - 4, cellHeight, juce::Justification::centredRight);
    }

    // Draw step numbers at top
    g.setColour(juce::Colours::grey);
    for (int s = 0; s < stepsToShow; ++s) {
        int step = pianoRollScrollOffset + s;
        int x = leftMargin + s * cellWidth;
        g.drawText(juce::String(step + 1), x, startY - 15, cellWidth, 14, juce::Justification::centred);
    }

    // Draw grid and notes
    for (int s = 0; s < stepsToShow; ++s) {
        int step = pianoRollScrollOffset + s;
        int x = leftMargin + s * cellWidth;
        auto* note = pattern->getNote(step);

        bool isPlaying = (step == playingStep);
        bool isSelected = (step == editStep);
        bool hasGate = note->gate;

        for (int k = 0; k < numKeys; ++k) {
            int y = startY + k * cellHeight;
            int keyValue = noteValues[k];

            // Cell background
            juce::Colour cellBg;
            if (isBlackKey[k]) {
                cellBg = juce::Colour(0xff1a1a2e);
            } else {
                cellBg = juce::Colour(0xff252540);
            }

            if (isPlaying && hasGate) {
                cellBg = cellBg.brighter(0.3f);
            }

            g.setColour(cellBg);
            g.fillRect(x, y, cellWidth - 1, cellHeight - 1);

            // Draw note if this key is active for this step
            if (hasGate && note->key == keyValue) {
                juce::Colour noteColor = juce::Colours::cyan;
                if (note->accent) noteColor = juce::Colours::red;
                if (note->slide) noteColor = noteColor.interpolatedWith(juce::Colours::yellow, 0.5f);

                g.setColour(noteColor);
                g.fillRoundedRectangle((float)x + 1, (float)y + 1, (float)cellWidth - 3, (float)cellHeight - 3, 2.0f);
            }

            // Highlight selected step column
            if (isSelected) {
                g.setColour(juce::Colours::white.withAlpha(0.1f));
                g.fillRect(x, y, cellWidth - 1, cellHeight - 1);
            }
        }
    }

    // Draw octave indicator row below piano roll
    int octaveRowY = startY + numKeys * cellHeight + 5;
    g.setColour(juce::Colours::grey);
    g.setFont(9.0f);
    g.drawText("Oct", 2, octaveRowY, leftMargin - 4, 16, juce::Justification::centredRight);

    for (int s = 0; s < stepsToShow; ++s) {
        int step = pianoRollScrollOffset + s;
        int x = leftMargin + s * cellWidth;
        auto* note = pattern->getNote(step);

        if (note->gate) {
            g.setColour(juce::Colours::orange);
            g.drawText(juce::String(note->octave + 2), x, octaveRowY, cellWidth, 16, juce::Justification::centred);
        }
    }

    // Draw accent row
    int accentRowY = octaveRowY + 18;
    g.setColour(juce::Colours::grey);
    g.drawText("Acc", 2, accentRowY, leftMargin - 4, 14, juce::Justification::centredRight);

    for (int s = 0; s < stepsToShow; ++s) {
        int step = pianoRollScrollOffset + s;
        int x = leftMargin + s * cellWidth;
        auto* note = pattern->getNote(step);

        juce::Colour accColor = note->accent ? juce::Colours::red : juce::Colour(0xff2a2a3e);
        g.setColour(accColor);
        g.fillRoundedRectangle((float)x + 2, (float)accentRowY + 2, (float)cellWidth - 5, 10.0f, 2.0f);
    }

    // Draw slide row
    int slideRowY = accentRowY + 16;
    g.setColour(juce::Colours::grey);
    g.drawText("Sld", 2, slideRowY, leftMargin - 4, 14, juce::Justification::centredRight);

    for (int s = 0; s < stepsToShow; ++s) {
        int step = pianoRollScrollOffset + s;
        int x = leftMargin + s * cellWidth;
        auto* note = pattern->getNote(step);

        juce::Colour sldColor = note->slide ? juce::Colours::yellow : juce::Colour(0xff2a2a3e);
        g.setColour(sldColor);
        g.fillRoundedRectangle((float)x + 2, (float)slideRowY + 2, (float)cellWidth - 5, 10.0f, 2.0f);
    }

    // Draw gate row
    int gateRowY = slideRowY + 16;
    g.setColour(juce::Colours::grey);
    g.drawText("Gate", 2, gateRowY, leftMargin - 4, 14, juce::Justification::centredRight);

    for (int s = 0; s < stepsToShow; ++s) {
        int step = pianoRollScrollOffset + s;
        int x = leftMargin + s * cellWidth;
        auto* note = pattern->getNote(step);

        juce::Colour gateColor = note->gate ? juce::Colours::cyan : juce::Colour(0xff2a2a3e);
        g.setColour(gateColor);
        g.fillRoundedRectangle((float)x + 2, (float)gateRowY + 2, (float)cellWidth - 5, 10.0f, 2.0f);
    }

    // Help text for piano roll mode
    g.setColour(juce::Colours::grey);
    g.setFont(9.0f);
    int helpY = getHeight() - 45;
    g.drawText("Click grid to set note | Click Oct/Acc/Sld/Gate to toggle", 10, helpY, getWidth() - 120, 13, juce::Justification::left);
    helpY += 12;
    g.drawText("Arrow keys=navigate | +/-=octave | Scroll=pan steps", 10, helpY, getWidth() - 120, 13, juce::Justification::left);
}

void AcidSeqEditor::mouseDown(const juce::MouseEvent& event)
{
    if (!pianoRollMode) return;

    handlePianoRollClick(event.x, event.y);
}

void AcidSeqEditor::handlePianoRollClick(int x, int y)
{
    auto* pattern = processorRef.getSequencer()->getPattern(0);
    if (!pattern) return;

    int pLen = processorRef.getPatternLength();

    // Layout constants (must match drawPianoRollView)
    const int startY = 35;
    const int cellWidth = 18;
    const int cellHeight = 13;  // Must match drawPianoRollView
    const int numKeys = 13;  // C to C+ (high C)
    const int leftMargin = 25;
    const int maxStepsVisible = 16;

    int stepsToShow = juce::jmin(pLen, maxStepsVisible);
    int pianoRollScrollOffset = scrollOffset;
    if (pianoRollScrollOffset + stepsToShow > pLen) {
        pianoRollScrollOffset = juce::jmax(0, pLen - stepsToShow);
    }

    static const int noteValues[] = {12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

    // Check if click is in the grid area
    if (x >= leftMargin && x < leftMargin + stepsToShow * cellWidth) {
        int stepClicked = (x - leftMargin) / cellWidth + pianoRollScrollOffset;
        if (stepClicked >= pLen) return;

        // Check which row was clicked
        int keyRowEnd = startY + numKeys * cellHeight;
        int octaveRowY = keyRowEnd + 5;
        int accentRowY = octaveRowY + 18;
        int slideRowY = accentRowY + 16;
        int gateRowY = slideRowY + 16;

        auto* note = pattern->getNote(stepClicked);

        if (y >= startY && y < keyRowEnd) {
            // Clicked on piano roll grid - set note
            int keyIndex = (y - startY) / cellHeight;
            if (keyIndex >= 0 && keyIndex < numKeys) {
                capturePatternState();
                int newKey = noteValues[keyIndex];
                pattern->setKey(stepClicked, newKey);
                pattern->setGate(stepClicked, true);
                editStep = stepClicked;
                triggerPreviewNote(stepClicked);
                repaint();
            }
        }
        else if (y >= octaveRowY && y < octaveRowY + 16) {
            // Clicked on octave row - cycle octave
            capturePatternState();
            int newOctave = (note->octave + 1);
            if (newOctave > 2) newOctave = -2;
            pattern->setOctave(stepClicked, newOctave);
            editStep = stepClicked;
            if (note->gate) triggerPreviewNote(stepClicked);
            repaint();
        }
        else if (y >= accentRowY && y < accentRowY + 14) {
            // Clicked on accent row - toggle
            capturePatternState();
            pattern->setAccent(stepClicked, !note->accent);
            editStep = stepClicked;
            repaint();
        }
        else if (y >= slideRowY && y < slideRowY + 14) {
            // Clicked on slide row - toggle
            capturePatternState();
            pattern->setSlide(stepClicked, !note->slide);
            editStep = stepClicked;
            repaint();
        }
        else if (y >= gateRowY && y < gateRowY + 14) {
            // Clicked on gate row - toggle
            capturePatternState();
            pattern->setGate(stepClicked, !note->gate);
            editStep = stepClicked;
            repaint();
        }
    }
}
