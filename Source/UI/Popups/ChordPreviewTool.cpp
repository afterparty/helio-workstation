/*
    This file is part of Helio Workstation.

    Helio is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Helio is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Helio. If not, see <http://www.gnu.org/licenses/>.
*/

//[Headers]
#include "Common.h"
//[/Headers]

#include "ChordPreviewTool.h"

//[MiscUserDefs]
#include "PianoSequence.h"
#include "PianoRoll.h"
#include "Transport.h"
#include "ChordsManager.h"
#include "KeySignaturesSequence.h"
#include "KeySignatureEvent.h"

#include "CommandIDs.h"

#define CHORD_BUILDER_LABEL_SIZE           (28)
#define CHORD_BUILDER_NOTE_LENGTH          (4)

#define SHOW_CHORD_TOOLTIP(ROOT_KEY, FUNCTION_NAME) \
if (! App::isRunningOnPhone()) { \
    auto *tip = new ChordTooltip(ROOT_KEY, \
        this->scale->getLocalizedName(), FUNCTION_NAME); \
    App::Layout().showTooltip(tip, this->getScreenBounds()); \
}

static inline String keyName(int key)
{
    return MidiMessage::getMidiNoteName(key, true, true, 3);
}

static Label *createLabel(const String &text)
{
    const int size = CHORD_BUILDER_LABEL_SIZE;
    auto newLabel = new Label(text, text);
    newLabel->setJustificationType(Justification::centred);
    newLabel->setBounds(0, 0, size * 2, size);
    newLabel->setName(text + "_outline");

    const float autoFontSize = float(size - 5.f);
    newLabel->setFont(Font(Font::getDefaultSerifFontName(), autoFontSize, Font::plain));
    return newLabel;
}

//static Array<String> localizedFunctionNames()
//{
//    return {
//        TRANS("popup::chord::function::1"),
//        TRANS("popup::chord::function::2"),
//        TRANS("popup::chord::function::3"),
//        TRANS("popup::chord::function::4"),
//        TRANS("popup::chord::function::5"),
//        TRANS("popup::chord::function::6"),
//        TRANS("popup::chord::function::7")
//    };
//}
//[/MiscUserDefs]

ChordPreviewTool::ChordPreviewTool(PianoRoll &caller, WeakReference<PianoSequence> target, WeakReference<KeySignaturesSequence> harmonicContext)
    : PopupMenuComponent(&caller),
      roll(caller),
      sequence(target),
      harmonicContext(harmonicContext),
      defaultChords(ChordsManager::getInstance().getChords()),
      hasMadeChanges(false),
      draggingStartPosition(0, 0),
      draggingEndPosition(0, 0)
{
    this->newChord.reset(new PopupCustomButton(createLabel("+")));
    this->addAndMakeVisible(newChord.get());

    //[UserPreSize]
    //[/UserPreSize]

    this->setSize(500, 500);

    //[Constructor]

    //sequence(static_cast<PianoSequence *>(targetTrack->getSequence()))
    const int numChordsToDisplay = jmin(16, this->defaultChords.size());
    for (int i = 0; i < numChordsToDisplay; ++i)
    {
        const auto chord = this->defaultChords.getUnchecked(i);
        const float radians = float(i) * (MathConstants<float>::twoPi / float(numChordsToDisplay));
        // 10 items fit well in a radius of 150, but the more chords there are, the larger r should be:
        const int radius = 150 + jlimit(0, 8, numChordsToDisplay - 10) * 10;
        const auto centreOffset = Point<int>(0, -radius).transformedBy(AffineTransform::rotation(radians, 0, 0));
        const auto colour = Colour(float(i) / float(numChordsToDisplay), 0.55f, 1.f, 0.6f);
        auto *chordButton = this->chordButtons.add(new PopupCustomButton(createLabel(chord->getName()), colour));
        chordButton->setSize(this->proportionOfWidth(0.14f), this->proportionOfHeight(0.14f));
        const auto buttonSizeOffset = chordButton->getLocalBounds().getCentre();
        const auto buttonPosition = this->getLocalBounds().getCentre() + centreOffset - buttonSizeOffset;

        chordButton->setTopLeftPosition(buttonPosition);
        this->addAndMakeVisible(chordButton);
    }

    this->setFocusContainer(true);
    this->setInterceptsMouseClicks(false, true);
    this->enterModalState(false, nullptr, true); // deleted when dismissed

    this->newChord->setMouseCursor(MouseCursor::DraggingHandCursor);
    //[/Constructor]
}

ChordPreviewTool::~ChordPreviewTool()
{
    //[Destructor_pre]
    this->stopSound();
    //[/Destructor_pre]

    newChord = nullptr;

    //[Destructor]
    //[/Destructor]
}

void ChordPreviewTool::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void ChordPreviewTool::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    newChord->setBounds(proportionOfWidth (0.5000f) - (proportionOfWidth (0.1280f) / 2), proportionOfHeight (0.5000f) - (proportionOfHeight (0.1280f) / 2), proportionOfWidth (0.1280f), proportionOfHeight (0.1280f));
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void ChordPreviewTool::parentHierarchyChanged()
{
    //[UserCode_parentHierarchyChanged] -- Add your code here...
    this->detectKeyBeatAndContext();
    this->buildNewNote(true);
    this->newChord->setState(true);
    //[/UserCode_parentHierarchyChanged]
}

void ChordPreviewTool::handleCommandMessage (int commandId)
{
    //[UserCode_handleCommandMessage] -- Add your code here...
    if (commandId == CommandIDs::PopupMenuDismiss)
    {
        this->exitModalState(0);
    }
    //[/UserCode_handleCommandMessage]
}

bool ChordPreviewTool::keyPressed (const KeyPress& key)
{
    //[UserCode_keyPressed] -- Add your code here...
    if (key.isKeyCode(KeyPress::escapeKey))
    {
        this->undoChangesIfAny();
    }

    this->dismissAsDone();
    return true;
    //[/UserCode_keyPressed]
}

void ChordPreviewTool::inputAttemptWhenModal()
{
    //[UserCode_inputAttemptWhenModal] -- Add your code here...
    this->dismissAsCancelled();
    //[/UserCode_inputAttemptWhenModal]
}


//[MiscUserCode]

void ChordPreviewTool::onPopupsResetState(PopupButton *button)
{
    // reset all buttons except for the clicked one:
    for (int i = 0; i < this->getNumChildComponents(); ++i)
    {
        if (auto *pb = dynamic_cast<PopupButton *>(this->getChildComponent(i)))
        {
            const bool shouldBeTurnedOn = (pb == button);
            pb->setState(shouldBeTurnedOn);
        }
    }
}

void ChordPreviewTool::onPopupButtonFirstAction(PopupButton *button)
{
    if (button != this->newChord.get())
    {
        App::Layout().hideTooltipIfAny();
        //this->buildChord(todo);
        //SHOW_CHORD_TOOLTIP(this->root, )
    }
}

void ChordPreviewTool::onPopupButtonSecondAction(PopupButton *button)
{
    // TODO alt chords?
    this->dismissAsDone();
}

void ChordPreviewTool::onPopupButtonStartDragging(PopupButton *button)
{
    if (button == this->newChord.get())
    {
        this->draggingStartPosition = this->getPosition();
    }
}

bool ChordPreviewTool::onPopupButtonDrag(PopupButton *button)
{
    if (button == this->newChord.get())
    {
        const Point<int> dragDelta = this->newChord->getDragDelta();
        this->setTopLeftPosition(this->getPosition() + dragDelta);
        const bool keyHasChanged = this->detectKeyBeatAndContext();
        this->buildNewNote(keyHasChanged);

        if (keyHasChanged)
        {
            const String rootKey = keyName(this->targetKey);
            App::Layout().showTooltip(TRANS("popup::chord::rootkey") + ": " + rootKey);
        }

        // reset click state:
        this->newChord->setState(false);
        for (auto *b : this->chordButtons)
        {
            b->setState(false);
        }
    }

    return false;
}

void ChordPreviewTool::onPopupButtonEndDragging(PopupButton *button)
{
    if (button == this->newChord.get())
    {
        this->draggingEndPosition = this->getPosition();
    }
}

static const float kDefaultChordVelocity = 0.35f;

void ChordPreviewTool::buildChord(const Chord::Ptr chord)
{
    if (!chord->isValid()) { return; }

    this->undoChangesIfAny();
    this->stopSound();

    if (!this->hasMadeChanges)
    {
        this->sequence->checkpoint();
    }

    // a hack for stop sound events not mute the forthcoming notes
    //Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 20);

    for (const auto inScaleKey : chord->getScaleKeys())
    {
        // todo offset from target key to root key
        const int key = jmin(128, jmax(0, this->root + this->scale->getChromaticKey(inScaleKey)));
        Note note(this->sequence.get(), key, this->targetBeat, CHORD_BUILDER_NOTE_LENGTH, kDefaultChordVelocity);
        this->sequence->insert(note, true);
        this->sendMidiMessage(MidiMessage::noteOn(note.getTrackChannel(), key, kDefaultChordVelocity));
    }

    this->hasMadeChanges = true;
}

void ChordPreviewTool::buildNewNote(bool shouldSendMidiMessage)
{
    this->undoChangesIfAny();

    if (shouldSendMidiMessage)
    {
        this->stopSound();
    }

    if (!this->hasMadeChanges)
    {
        this->sequence->checkpoint();
    }

    const int key = jmin(128, jmax(0, this->targetKey));

    Note note1(this->sequence.get(), key, this->targetBeat, CHORD_BUILDER_NOTE_LENGTH, kDefaultChordVelocity);
    this->sequence->insert(note1, true);

    if (shouldSendMidiMessage)
    {
        this->sendMidiMessage(MidiMessage::noteOn(note1.getTrackChannel(), key, kDefaultChordVelocity));
    }

    this->hasMadeChanges = true;
}

void ChordPreviewTool::undoChangesIfAny()
{
    if (this->hasMadeChanges)
    {
        this->sequence->undo();
        this->hasMadeChanges = false;
    }
}

bool ChordPreviewTool::detectKeyBeatAndContext()
{
    int newKey = 0;
    const auto myCentreRelativeToRoll = this->roll.getLocalPoint(this->getParentComponent(), this->getBounds().getCentre());
    this->roll.getRowsColsByMousePosition(myCentreRelativeToRoll.x, myCentreRelativeToRoll.y, newKey, this->targetBeat);
    bool hasChanges = (newKey != this->targetKey);
    this->targetKey = newKey;


    const KeySignatureEvent *context = nullptr;
    for (int i = 0; i < this->harmonicContext->size(); ++i)
    {
        const auto *event = this->harmonicContext->getUnchecked(i);
        if (context == nullptr || event->getBeat() <= this->targetBeat)
        {
            // Take the first one no matter where it resides;
            // If event is still before the sequence start, update the context anyway:
            context = static_cast<const KeySignatureEvent *>(event);
        }
        else if (event->getBeat() > this->targetBeat)
        {
            // No need to look further
            break;
        }
    }

    // We've found the only context that doesn't change within a sequence:
    if (context != nullptr)
    {
        hasChanges = hasChanges || this->root != context->getRootKey() ||
            !this->scale->isEquivalentTo(context->getScale());

        this->scale = context->getScale();
        this->root = context->getRootKey();
        return true;
    }
    else
    {
        // TODO use C Ionian as fallback context!
        jassertfalse;
    }

    return hasChanges;
}

//===----------------------------------------------------------------------===//
// Shorthands
//===----------------------------------------------------------------------===//

void ChordPreviewTool::stopSound()
{
    this->roll.getTransport().allNotesControllersAndSoundOff();
}

void ChordPreviewTool::sendMidiMessage(const MidiMessage &message)
{
    const String layerId = this->sequence->getTrackId();
    this->roll.getTransport().sendMidiMessage(layerId, message);
}

//[/MiscUserCode]

#if 0
/*
BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="ChordPreviewTool" template="../../Template"
                 componentName="" parentClasses="public PopupMenuComponent, public PopupButtonOwner"
                 constructorParams="PianoRoll &amp;caller, WeakReference&lt;PianoSequence&gt; target, WeakReference&lt;KeySignaturesSequence&gt; harmonicContext"
                 variableInitialisers="PopupMenuComponent(&amp;caller),&#10;roll(caller),&#10;sequence(target),&#10;harmonicContext(harmonicContext),&#10;defaultChords(ChordsManager::getInstance().getChords()),&#10;hasMadeChanges(false),&#10;draggingStartPosition(0, 0),&#10;draggingEndPosition(0, 0)"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="500" initialHeight="500">
  <METHODS>
    <METHOD name="inputAttemptWhenModal()"/>
    <METHOD name="keyPressed (const KeyPress&amp; key)"/>
    <METHOD name="handleCommandMessage (int commandId)"/>
    <METHOD name="parentHierarchyChanged()"/>
  </METHODS>
  <BACKGROUND backgroundColour="0"/>
  <JUCERCOMP name="" id="6b3cbe21e2061b28" memberName="newChord" virtualName=""
             explicitFocusOrder="0" pos="50%c 50%c 12.8% 12.8%" sourceFile="PopupCustomButton.cpp"
             constructorParams="createLabel(&quot;+&quot;)"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif