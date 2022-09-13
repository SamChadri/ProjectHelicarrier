#pragma once

#include <JuceHeader.h>
#include "SynthAudioSource.h"
#include "EngineAudioSource.h"
#include "../includes/common/Components.h"
#include "StepEditor.h"
#include "Metronome.h"
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
typedef std::function<void(float)> TempoChangedCallback;
class StepEditorWindow : public juce::Component, public juce::Slider::Listener
{
public:
    StepEditorWindow(EngineAudioSource& source)
    : engineAudioSource(source), sampleFiles(createSampleFiles())
    {
        
        addAndMakeVisible(stepPlayPauseButton);
        addAndMakeVisible(stepConfirmButton);
        addAndMakeVisible(stepBarInput);
        
        stepPlayPauseButton.setLookAndFeel(&otherLookAndFeel);
        stepPlayPauseButton.onClick = [this] {playPlauseButtonClicked();};
        stepConfirmButton.onClick = [this] {confirmButtonClicked();};
        
        
        stepBarInput.setText("1", juce::dontSendNotification);
        stepBarInput.setInputRestrictions(2,"0123456789");
        stepBarInput.onTextChange = [this] {stepCountChanged();};
        
        
        addAndMakeVisible(tempoSlider);
        tempoSlider.setRange(40.0, 200.0, 0.1);
        tempoSlider.setTextValueSuffix(" BPM");
        tempoSlider.addListener(this);
        
        addAndMakeVisible(tempoLabel);
        tempoLabel.setText("Tempo", juce::dontSendNotification);
        tempoLabel.attachToComponent(&tempoSlider,true);
        
        
        
        
        

            
    }
    
    
    
    ~StepEditorWindow()
    {
       
        
    }
    
    void initalise()
    {
        
        createStepClip();
        createSamplerPlugin(sampleFiles);
        
        stepEditor = std::make_unique<StepEditor>(*getClip());
        
        
        addAndMakeVisible(stepEditor.get());
        viewPort.setViewedComponent(stepEditor.get(), false);
        addAndMakeVisible(&viewPort);
        setSize(400,600);
        configWidth = stepEditor->getWidth() - stepEditor->getPatternWidth();
        stepInitWidth = stepEditor->getPatternWidth();
        
        tempoSlider.setValue(engineAudioSource.getStepEdit().tempoSequence.getTempos()[0]->getBpm(), juce::dontSendNotification);

        
    }
    
    void playPlauseButtonClicked()
    {
        if(auto stepClip = getClip())
        {
            auto& transport = stepClip->edit.getTransport();
            if(transport.isPlaying())
            {
                transport.stop(false,false);
                stepPlayPauseButton.setButtonText(L"▶");
            }
            else
            {
                transport.play(true);
                //Switch Fonts later
                stepPlayPauseButton.setButtonText(L"\u007C \u007C");
                

                
                
            }
        }
        
    }
    
    void confirmButtonClicked()
    {
        if(auto stepClip = getClip())
        {
            auto  sequence = stepClip->getPatternSequence();
            if(auto track = EngineHelpers::getOrInsertAudioTrackAt(engineAudioSource.getEdit(), 2))
            {
                const tracktion_engine::EditTimeRange editTimeRange(0,(engineAudioSource.getEdit().tempoSequence.barsBeatsToTime ({ barCount, 0.0 })));
                track->insertNewClip(tracktion_engine::TrackItem::Type::step, "Step Clip", editTimeRange, nullptr);
                if(auto track = EngineHelpers::getOrInsertAudioTrackAt(engineAudioSource.getEdit(), 2))
                {
                    if(auto clip = dynamic_cast<tracktion_engine::StepClip*>(track->getClips()[0]))
                    {
                        //CREATE SAMPLER PLUGIN NEXT
                        if(auto sampler = dynamic_cast<tracktion_engine::SamplerPlugin*>(engineAudioSource.getEdit().getPluginCache().createNewPlugin(tracktion_engine::SamplerPlugin::xmlTypeName, {}).get()))
                        {
                            clip->getTrack()->pluginList.insertPlugin(*sampler, 0, nullptr);
                            DBG("INSERTED PLUGIN ");
                            int channelCount = 0;
                            
                            for(auto channel : clip->getChannels())
                            {
                                const auto error = sampler->addSound(sampleFiles[channelCount++].getFullPathName(), channel->name.get(), 0.0, 0.0, 1.0f);
                                sampler->setSoundParams(sampler->getNumSounds() - 1, channel->noteNumber, channel->noteNumber, channel->noteNumber);
                                jassert(error.isEmpty());
                                
                                clip->insertPattern(stepClip->getPattern(channel->getIndex()),channel->getIndex());

                                
    
                            }
                            DBG("STEP CLIP CHANNEL COUNT : " << channelCount);
                            
                        }
                    }
                }
                                
            }
            
        }

    }
    
    void stepCountChanged()
    {
        barCount = stepBarInput.getText().getIntValue();
        if(barCount > 0)
        {
            if(auto stepClip = getClip())
            {
                auto timeDuration = engineAudioSource.getEdit().tempoSequence.barsBeatsToTime({barCount,0});
                
                auto numNotes = 16 * barCount;
                
                for(auto pattern : stepClip->getPatterns())
                {
                    pattern.setNumNotes(numNotes);
                }
                stepClip->setLength(timeDuration,true);
                EngineHelpers::loopAroundClip(*stepClip);
                stepPlayPauseButton.setButtonText(L"\u007C \u007C");
                if(barCount != 1)
                {

                    stepResize = true;
                    auto diffSpace = 400 - (stepInitWidth + configWidth);
                    auto newStepWidth  = (stepInitWidth * barCount) + configWidth; //Change this so it varies per bars
                    auto windowWidth = ((stepInitWidth * 2) + configWidth) + diffSpace;
                    stepEditor->setBounds(10, 70, newStepWidth, 300);
                    setSize(windowWidth,600);//Prolly dont need to call this multiple times
                    resized();

                    
                }
                else
                {
                    setSize(400,600);
                    stepResize = false;
                    stepEditor->setBounds(10, 70, getWidth() - 20, 300);
                    
                }
                stepEditor->updatePaths();
            }
        }

    }
    void setTempo(float tempo)
    {
        
        if(! ModifierKeys::getCurrentModifiers().isAnyMouseButtonDown())
        {
            engineAudioSource.getStepEdit().tempoSequence.getTempos()[0]->setBpm(tempo);
            tempoSlider.setValue(tempo, juce::dontSendNotification);
        }
    }
    
    void setTempoChangedCallback(TempoChangedCallback cb)
    {
        onTempoChanged = cb;
    }
    void sliderValueChanged (Slider *slider) override
    {
        
        if(! ModifierKeys::getCurrentModifiers().isAnyMouseButtonDown())
        {
            engineAudioSource.getStepEdit().tempoSequence.getTempos()[0]->setBpm(tempoSlider.getValue());
            onTempoChanged(tempoSlider.getValue());
        }
    }
    void sliderDragEnded (Slider*) override
    {
        
        engineAudioSource.getStepEdit().tempoSequence.getTempos()[0]->setBpm(tempoSlider.getValue());
        onTempoChanged(tempoSlider.getValue());
        
    }
    
    
    void paint(Graphics&) override
    {
        
    }

    void resized()override
    {
        juce::FlexBox stepAudioControlFb;
        
        stepAudioControlFb.flexWrap = juce::FlexBox::Wrap::wrap;
        stepAudioControlFb.justifyContent = juce::FlexBox::JustifyContent::center;
        stepAudioControlFb.alignContent = juce::FlexBox::AlignContent::center;
        
        
        stepAudioControlFb.items.add(juce::FlexItem(stepPlayPauseButton).withMinWidth(40.0f).withMinHeight(25.0f).withMargin(juce::FlexItem::Margin(2.0f,2.0f,2.0f,2.0f)));
        
        stepAudioControlFb.items.add(juce::FlexItem(stepConfirmButton).withMinWidth(40.0f).withMinHeight(25.0f).withMargin(juce::FlexItem::Margin(2.0f,2.0f,2.0f,2.0f)));
        
        stepAudioControlFb.performLayout(getLocalBounds().removeFromTop(60));
        
        stepBarInput.setBounds(getWidth() - 30, 30, 20, 25);
        viewPort.setBounds(10, 70, getWidth()- 10, 300);
        //viewPort.setBoundsRelative(0.00f, 0.1f, 1.0f, 1.0f);
        //stepPlayPauseButton.setBounds(10, 20, getWidth()-20, 25);
        if(stepResize == false)
        {
            DBG("STEP RESIZE FALSE");
            stepEditor->setBounds(10, 70, getWidth() - 20, 300);
        }
        tempoSlider.setBounds(50, 400, getWidth() - 50, 20);
    }
    
    
    
    
    
    
private:
    juce::TextButton stepSettingsButton {"Settings"};
    
    juce::TextButton stepPlayPauseButton {L"▶"};
    
    juce::TextButton stepConfirmButton {L"\u2713"};
    
    juce::TextButton stepRandomizeButton {"Randomise"};
    
    juce::TextButton stepClearButton {"Clear"};
    
    juce::TextEditor stepBarInput;
    
    juce::String stepBarCount;
    
    juce::Slider tempoSlider;
    juce::Label tempoLabel;
    
    std::unique_ptr<StepEditor> stepEditor;
    
    ButtonLookAndFeel otherLookAndFeel;
    
    EngineAudioSource& engineAudioSource;
    
    Array<File> sampleFiles;
    
    Viewport viewPort;
    
    bool stepResize = false;
    int stepInitWidth;
    int configWidth;
    
    int barCount = 1;
    
    TempoChangedCallback onTempoChanged;
    
    
    tracktion_engine::StepClip::Ptr createStepClip()
    {
        //EDIT: change which track you actually might want to use
        auto & edit = engineAudioSource.getStepEdit();
        if(auto track = EngineHelpers::getOrInsertAudioTrackAt(edit, 0))
        {
            const tracktion_engine::EditTimeRange editTimeRange(0,edit.tempoSequence.barsBeatsToTime ({ 1, 0.0 }));
            track->insertNewClip(tracktion_engine::TrackItem::Type::step, "Step Clip", editTimeRange, nullptr);
            if(auto stepClip = getClip())
            {
                stepPlayPauseButton.setButtonText(L"\u007C \u007C");
                return EngineHelpers::loopAroundClip(*stepClip);
            }
        }
        return {};
    }
    
    Array<File> createSampleFiles()
    {
        Array<File> files;
        const auto destDir = engineAudioSource.getStepEdit().getTempDirectory(true);
        
        jassert(destDir != File());
        
        using namespace DemoBinaryData;
        
        for(int i =0; i < tracktion_engine::TracktionBinaryData::namedResourceListSize; ++i)
        {
            const auto f = destDir.getChildFile(originalFilenames[i]);
            
            int dataSizeInBytes = 0;
            const char* data = getNamedResource(namedResourceList[i], dataSizeInBytes);
            
            jassert(data != nullptr);
            f.replaceWithData(data, dataSizeInBytes);
            files.add(f);
        }
        return files;
    }
    
    void createSamplerPlugin(Array<File> defaultSampleFiles)
    {
        if(auto stepClip = getClip())
        {
            if(auto sampler = dynamic_cast<tracktion_engine::SamplerPlugin*>(engineAudioSource.getStepEdit().getPluginCache().createNewPlugin(tracktion_engine::SamplerPlugin::xmlTypeName, {}).get()))
            {
                stepClip->getTrack()->pluginList.insertPlugin(*sampler, 0, nullptr);
                int channelCount = 0;
                
                for(auto channel : stepClip->getChannels())
                {
                    const auto error = sampler->addSound(defaultSampleFiles[channelCount++].getFullPathName(), channel->name.get(), 0.0, 0.0, 1.0f);
                    sampler->setSoundParams(sampler->getNumSounds() - 1, channel->noteNumber, channel->noteNumber, channel->noteNumber);
                    jassert(error.isEmpty());
                    
                    for(auto &pattern :stepClip->getPatterns())
                    {
                        pattern.randomiseChannel(channel->getIndex());
                    }
                        
                }
                DBG("STEP CLIP CHANNEL COUNT : " << channelCount);
                DBG("PATTERN NOTE COUNT: " <<  stepClip->getPatternSequence()[0]->getPattern().getNumNotes());
                
            }
        }
        else
        {
            jassertfalse; //StepClip not created...
        }
        
    }
    
    tracktion_engine::StepClip::Ptr getClip()
    {
        if(auto track = EngineHelpers::getOrInsertAudioTrackAt(engineAudioSource.getStepEdit(), 0))
        {
            if(auto clip = dynamic_cast<tracktion_engine::StepClip*>(track->getClips()[0]))
            {
                return *clip;
            }
        }
        return {};
        
    }
    
    
};





class MainComponent  : public juce::AudioAppComponent, public juce::Timer, public juce::ChangeListener, public juce::MidiInputCallback, public juce::MidiKeyboardStateListener, public juce::Slider::Listener
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
    
    
    void deleteButtonClicked();
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
    
    void showStepSequencer();
    //========================================================================================================
    void sliderValueChanged(Slider * ) override;
    void sliderDragEnded(Slider *) override;

private:
    //==============================================================================
    // Your private member variables go here...
    juce::MidiKeyboardState keyboardState;
    SynthAudioSource::Ptr synthPluginPtr;
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
    juce::TextButton deleteButton {L"\u274C"};
    
    juce::Slider tempoSlider;
    juce::Label tempoLabel;
    
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
    
    
    Metronome metronome;
    
    //========================================================================
    
    
    StepEditorWindow stepWindow;
    
    juce::ResizableWindow* topWindow;
    
    
    
    
    
    //========================================================================
    
    tracktion_engine::StepClip::Ptr createStepClip();
    
    Array<File> createSampleFiles();
    
    void createSamplerPlugin(Array<File> defaultSampleFiles);
    
    tracktion_engine::StepClip::Ptr getClip();
    

    


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





