#include "AcidSeqGui.h"

//==============================================================================
AcidSeqEditor::AcidSeqEditor(AcidSeq303& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(320, 530);
    setWantsKeyboardFocus(true);
    addKeyListener(this);
    startTimerHz(30);
}

AcidSeqEditor::~AcidSeqEditor()
{
    removeKeyListener(this);
    stopTimer();
}

//==============================================================================
juce::String AcidSeqEditor::getNoteNameForKey(int key)
{
    static const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    if (key >= 0 && key < 12)
        return noteNames[key];
    return "C";
}

void AcidSeqEditor::drawTrackerRow(juce::Graphics& g, int step, int y, bool isSelected, bool isPlaying)
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
    if (isSelected)
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
    if (isSelected && editColumn == 0 && !beyondLength) {
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
    if (isSelected && editColumn == 1 && !beyondLength) {
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
    if (isSelected && editColumn == 2 && !beyondLength) {
        g.setColour(juce::Colour(0xff4a6a9f));
        g.fillRect(x, y, colAcc, rowHeight);
    }
    g.setColour((note->accent ? juce::Colours::red : juce::Colours::grey).withAlpha(alpha));
    g.drawText(note->accent ? "A" : ".", x, y, colAcc, rowHeight, juce::Justification::centred);
    x += colAcc;

    // Slide column
    if (isSelected && editColumn == 3 && !beyondLength) {
        g.setColour(juce::Colour(0xff4a6a9f));
        g.fillRect(x, y, colSld, rowHeight);
    }
    g.setColour((note->slide ? juce::Colours::yellow : juce::Colours::grey).withAlpha(alpha));
    g.drawText(note->slide ? "S" : ".", x, y, colSld, rowHeight, juce::Justification::centred);
    x += colSld;

    // Gate column (shows play/mute status)
    if (isSelected && editColumn == 4 && !beyondLength) {
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
    g.drawText("AcidSeq-303 Tracker", 0, 5, getWidth(), 25, juce::Justification::centred);

    // Pattern length display
    int pLen = processorRef.getPatternLength();
    g.setColour(juce::Colours::orange);
    g.setFont(13.0f);
    g.drawText("Len: " + juce::String(pLen), getWidth() - 70, 8, 60, 20, juce::Justification::right);

    // Header row
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

    // Pattern rows
    y = 35 + headerHeight;
    int playingStep = processorRef.getCurrentStep();

    for (int step = 0; step < numSteps; ++step) {
        bool isSelected = (step == editStep);
        bool isPlaying = (step == playingStep);
        drawTrackerRow(g, step, y, isSelected, isPlaying);
        y += rowHeight;
    }

    // Help text
    g.setColour(juce::Colours::grey);
    g.setFont(9.0f);

    int helpY = getHeight() - 105;
    g.drawText("A-G=note  #=sharp  0-4=octave  X=accent  S=slide", 10, helpY, getWidth() - 20, 13, juce::Justification::left);
    helpY += 12;
    g.drawText("Space=mute/unmute  Del=clear  Arrows=navigate", 10, helpY, getWidth() - 20, 13, juce::Justification::left);
    helpY += 12;
    g.drawText("[/]=length  ,/.=transpose  {/}=cycle", 10, helpY, getWidth() - 20, 13, juce::Justification::left);
    helpY += 12;
    g.drawText("R=random  T=scramble  Ctrl+C=clear all", 10, helpY, getWidth() - 20, 13, juce::Justification::left);
    helpY += 12;
    g.setColour(juce::Colours::darkgrey);
    g.drawText("Pattern loops at step " + juce::String(pLen) + " | MIDI out", 10, helpY, getWidth() - 20, 13, juce::Justification::left);
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

    // Clear all pattern (Ctrl+C or Cmd+C)
    if (ctrlOrCmd && (key.getTextCharacter() == 'c' || key.getTextCharacter() == 'C')) {
        processorRef.clearPattern();
    }
    // Randomize pattern (R)
    else if (key.getTextCharacter() == 'r' || key.getTextCharacter() == 'R') {
        processorRef.randomizePattern();
    }
    // Scramble pattern (T for "toss")
    else if (key.getTextCharacter() == 't' || key.getTextCharacter() == 'T') {
        processorRef.scramblePattern();
    }
    // Transpose down (,)
    else if (key.getTextCharacter() == ',' || key.getTextCharacter() == '<') {
        processorRef.transposePattern(-1);
    }
    // Transpose up (.)
    else if (key.getTextCharacter() == '.' || key.getTextCharacter() == '>') {
        processorRef.transposePattern(1);
    }
    // Cycle left ({)
    else if (key.getTextCharacter() == '{') {
        processorRef.cyclePattern(-1);
    }
    // Cycle right (})
    else if (key.getTextCharacter() == '}') {
        processorRef.cyclePattern(1);
    }
    // Pattern length decrease ([)
    else if (key.getTextCharacter() == '[') {
        processorRef.setPatternLength(processorRef.getPatternLength() - 1);
    }
    // Pattern length increase (])
    else if (key.getTextCharacter() == ']') {
        processorRef.setPatternLength(processorRef.getPatternLength() + 1);
    }
    // Toggle gate (mute/unmute) with space
    else if (key.isKeyCode(juce::KeyPress::spaceKey)) {
        pattern->setGate(editStep, !pattern->getGate(editStep));
    }
    // Navigation
    else if (key.isKeyCode(juce::KeyPress::upKey)) {
        editStep = (editStep - 1 + numSteps) % numSteps;
    }
    else if (key.isKeyCode(juce::KeyPress::downKey)) {
        editStep = (editStep + 1) % numSteps;
    }
    else if (key.isKeyCode(juce::KeyPress::leftKey)) {
        editColumn = (editColumn - 1 + 5) % 5;
    }
    else if (key.isKeyCode(juce::KeyPress::rightKey)) {
        editColumn = (editColumn + 1) % 5;
    }
    else if (key.isKeyCode(juce::KeyPress::returnKey)) {
        // Enter advances to next step (like a tracker)
        editStep = (editStep + 1) % numSteps;
    }
    // Note entry (C, D, E, F, G, A, B) - only if not using Ctrl
    else if (!ctrlOrCmd && (key.getTextCharacter() == 'c' || key.getTextCharacter() == 'C')) {
        pattern->setKey(editStep, 0);
        pattern->setGate(editStep, true);
        editStep = (editStep + 1) % numSteps;
    }
    else if (key.getTextCharacter() == 'd' || key.getTextCharacter() == 'D') {
        pattern->setKey(editStep, 2);
        pattern->setGate(editStep, true);
        editStep = (editStep + 1) % numSteps;
    }
    else if (key.getTextCharacter() == 'e' || key.getTextCharacter() == 'E') {
        pattern->setKey(editStep, 4);
        pattern->setGate(editStep, true);
        editStep = (editStep + 1) % numSteps;
    }
    else if (key.getTextCharacter() == 'f' || key.getTextCharacter() == 'F') {
        pattern->setKey(editStep, 5);
        pattern->setGate(editStep, true);
        editStep = (editStep + 1) % numSteps;
    }
    else if (key.getTextCharacter() == 'g' || key.getTextCharacter() == 'G') {
        pattern->setKey(editStep, 7);
        pattern->setGate(editStep, true);
        editStep = (editStep + 1) % numSteps;
    }
    else if (!ctrlOrCmd && (key.getTextCharacter() == 'a' || key.getTextCharacter() == 'A')) {
        pattern->setKey(editStep, 9);
        pattern->setGate(editStep, true);
        editStep = (editStep + 1) % numSteps;
    }
    else if (key.getTextCharacter() == 'b' || key.getTextCharacter() == 'B') {
        pattern->setKey(editStep, 11);
        pattern->setGate(editStep, true);
        editStep = (editStep + 1) % numSteps;
    }
    // Sharp - adds 1 to current note (making it sharp)
    else if (key.getTextCharacter() == '#') {
        int currentKey = pattern->getKey(editStep);
        if (currentKey < 11) {
            pattern->setKey(editStep, currentKey + 1);
        }
    }
    // Octave (0-4 for octaves 0-4)
    else if (key.getTextCharacter() >= '0' && key.getTextCharacter() <= '4') {
        int octave = key.getTextCharacter() - '0';
        pattern->setOctave(editStep, octave - 2);  // Offset by -2 since base is octave 2
    }
    // Toggle accent (X)
    else if (key.getTextCharacter() == 'x' || key.getTextCharacter() == 'X') {
        pattern->setAccent(editStep, !pattern->getAccent(editStep));
    }
    // Toggle slide (S)
    else if (!ctrlOrCmd && (key.getTextCharacter() == 's' || key.getTextCharacter() == 'S')) {
        pattern->setSlide(editStep, !pattern->getSlide(editStep));
    }
    // Delete/backspace clears the step completely
    else if (key.isKeyCode(juce::KeyPress::deleteKey) || key.isKeyCode(juce::KeyPress::backspaceKey)) {
        pattern->setGate(editStep, false);
        pattern->setAccent(editStep, false);
        pattern->setSlide(editStep, false);
        pattern->setKey(editStep, 0);
        pattern->setOctave(editStep, 0);
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
