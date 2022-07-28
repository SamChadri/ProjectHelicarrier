#pragma once

#include <JuceHeader.h>
#include "SynthAudioSource.h"
#include "EngineAudioSource.h"
#include "../includes/common/Components.h"
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class ButtonLookAndFeel : public juce::LookAndFeel_V4
{
    Font getTextButtonFont (TextButton&, int buttonHeight) override
    {
        return Font ("Segoe UI Emoji", 16.0f, Font::plain);
    }
};





class MainComponent  : public juce::AudioAppComponent, public juce::Timer, public juce::ChangeListener, public juce::MidiInputCallback, public juce::MidiKeyboardStateListener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    //================================
    void timerCallback() override;
    
    void setMidiInput(int index);
    
    void setSynth(int index);
    
    //======================================================
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
        
    void thumbnailChanged();
    
    void openButtonClicked();
    
    void playButtonClicked();
    
    void stopButtonClicked();
    
    void recordButtonClicked();
    
    void newTrackButtonClicked();
    
    //======================================================================
    
    void createTracksAndAssignInputs();
    
    void createOrLoadEdit();
    
    tracktion_engine::MidiClip & getMidiClip(const int i);
    
    
    //tracktion_engine::WaveAudioClip::Ptr loadAudioFileAsClip(tracktion_engine::Edit& edit, const juce::File& file);
    
    //tracktion_engine::AudioTrack* getOrInsertAudioTrackAt(tracktion_engine::Edit& edit, int index);
    
    //void removeAllClips(tracktion_engine::AudioTrack& track);
    //========================================================================
    void handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    
    void handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float /*velocity*/) override;
    
    void postMessageToList (const juce::MidiMessage& message, const juce::String& source);
    
    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override;
    
    void processMidiClip(tracktion_engine::MidiClip & midiClip);

private:
    //==============================================================================
    // Your private member variables go here...
    juce::MidiKeyboardState keyboardState;
    SynthAudioSource * synthAudioSource;
    std::unique_ptr<juce::MidiKeyboardComponent> keyboardComponent;
    
    
    bool midiInputSource;
    
    juce::ComboBox midiInputList;
    juce::Label midiInputLabel;
    int lastInputIndex = 0;
    
    juce::ComboBox synthList;
    juce::Label synthLabel;
    int lastSynthIndex = 0;
    
    
    
    
    EngineAudioSource engineAudioSource;
    
    
    std::unique_ptr<EditComponent> editComponent;
    tracktion_engine::SelectionManager selectionManager;
    
    //tracktion_engine::Engine engine {ProjectInfo::projectName};
    //std::unique_ptr<tracktion_engine::Edit> edit;

    std::unique_ptr<juce::FileChooser> chooser;
    
    ButtonLookAndFeel otherLookAndFeel;
    
    juce::TextButton playButton {L"▶"};
    juce::TextButton stopButton {L"\u25FC"};//Set custom font later
    juce::TextButton recordButton{String::fromUTF8(u8"\u26AB")};
    juce::TextButton newTrackButton{L"\u2795"};
    juce::TextButton openButton {"open"};
    
    tracktion_engine::VirtualMidiInputDevice * virtualMidi;
    
    
    
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    
    juce::AudioThumbnailCache thumbnailCache;
    juce::AudioThumbnail thumbnail;
    
    juce::MixerAudioSource audioMixer;
    
    
    enum TransportState
    {
        Stopped,
        Starting,
        Playing,
        Stopping
    };
    
    TransportState playState {TransportState::Stopped};
    
    
    
    


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

class IncomingMessageCallback : public juce::CallbackMessage
{
public:
    IncomingMessageCallback(MainComponent * mcc, const MidiMessage& mm, const juce::String& s): owner(mcc), message(mm), source(s)
    {
        
    }
    
    void messageCallback() override
    {
        if(owner != nullptr)
        {
            //owner->addMessageToList(message, source);
        }
    }
    
    
    Component::SafePointer<MainComponent> owner;
    juce::MidiMessage message;
    juce::String source;
};
