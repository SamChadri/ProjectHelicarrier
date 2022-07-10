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
class MainComponent  : public juce::AudioAppComponent, public juce::Timer, public juce::ChangeListener
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
    
    //======================================================================
    
    void createTracksAndAssignInputs();
    
    void createOrLoadEdit();
    //tracktion_engine::WaveAudioClip::Ptr loadAudioFileAsClip(tracktion_engine::Edit& edit, const juce::File& file);
    
    //tracktion_engine::AudioTrack* getOrInsertAudioTrackAt(tracktion_engine::Edit& edit, int index);
    
    //void removeAllClips(tracktion_engine::AudioTrack& track);
    

private:
    //==============================================================================
    // Your private member variables go here...
    juce::MidiKeyboardState keyboardState;
    SynthAudioSource synthAudioSource;
    juce::MidiKeyboardComponent keyboardComponent;
    
    
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
    
    
    juce::TextButton playButton {"play"};
    juce::TextButton stopButton {"stop"};
    juce::TextButton openButton {"open"};
    
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
