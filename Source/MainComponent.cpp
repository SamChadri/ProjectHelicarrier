#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : synthAudioSource(keyboardState),
      keyboardComponent(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
      thumbnailCache(5),
      thumbnail(512, formatManager, thumbnailCache),
      audioMixer(),
      engineAudioSource(keyboardState),
      selectionManager(engineAudioSource.getEngine())
      
    
{
    
    //========================================================================
    //audioMixer.addInputSource(&transportSource, true);
    audioMixer.addInputSource(&engineAudioSource, true);
    audioMixer.addInputSource(&synthAudioSource, true);
    
    //========================================================================

    
    formatManager.registerBasicFormats();
    
    
    playButton.onClick = [this] {playButtonClicked();};
    stopButton.onClick = [this] {stopButtonClicked();};
    openButton.onClick = [this]{openButtonClicked();};
    
    //========================================================================
    addAndMakeVisible(openButton);
    
    
    addAndMakeVisible(playButton);
    
    addAndMakeVisible(stopButton);
    
    thumbnail.addChangeListener(this);
    //=======================================================================
    addAndMakeVisible(synthLabel);
    synthLabel.setText("Synthesizer Presets: ", juce::dontSendNotification);
    synthLabel.attachToComponent(&synthList, true);
    
    
    //========================================================================
    
    auto synthPresets = SynthAudioSource::getSynthList();
    addAndMakeVisible(synthList);
    synthList.setTextWhenNoChoicesAvailable("No Presets Available");
    
    juce::StringArray synthNames;
    
    for(auto synth: synthPresets)
    {
        synthNames.add(synth.name);
    }
    synthList.addItemList(synthNames, 1);
    synthList.onChange = [this] {setSynth(synthList.getSelectedItemIndex());};
        
    //========================================================================
    addAndMakeVisible(midiInputLabel);
    midiInputLabel.setText("MIDI Input:", juce::dontSendNotification);
    midiInputLabel.attachToComponent(&midiInputList, true);
    
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    addAndMakeVisible(midiInputList);
    midiInputList.setTextWhenNoChoicesAvailable("No MIDI Inputs Enabled");
    
    juce::StringArray midiInputNames;
    
    for(auto input: midiInputs)
    {
        midiInputNames.add(input.name);
    }
    
    midiInputList.addItemList(midiInputNames, 1);
    midiInputList.onChange = [this] {setMidiInput (midiInputList.getSelectedItemIndex());};
    
    
    
    for(auto input: midiInputs)
    {
        if(deviceManager.isMidiInputDeviceEnabled(input.identifier))
        {
            setMidiInput(midiInputs.indexOf(input));
            break;
        }
    }
    if(midiInputList.getSelectedId() == 0)
    {
        setMidiInput(0);
    }
    
    //========================================================================
    addAndMakeVisible(keyboardComponent);
    setAudioChannels(2, 2);
    
    auto* device = deviceManager.getCurrentAudioDevice();
    auto activeInputChannels  = device->getActiveInputChannels();
    auto activeOutputChannels = device->getActiveOutputChannels();
    
    
    DBG("AUDIO INPUT CHANNELLS: " << activeInputChannels.toString(10));
    DBG("AUDIO OUTPUT CHANELLS: " << activeOutputChannels.toString(10));
    // Make sure you set the size of the component after
    // you add any child components.
    createOrLoadEdit();
    
    
    setSize (800, 500);
    startTimer(400);
    

    // Some platforms require permissions to open input channels so request that here
    /*
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }*/
}


MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}


/*
void MainComponent::removeAllClips(tracktion_engine::AudioTrack &track)
{
    auto clips = track.getClips();

    for (int i = clips.size(); --i >= 0;)
        clips.getUnchecked (i)->removeFromParentTrack();
}

tracktion_engine::AudioTrack* MainComponent::getOrInsertAudioTrackAt(tracktion_engine::Edit &edit, int index)
{
    edit.ensureNumberOfAudioTracks(index + 1);
    return tracktion_engine::getAudioTracks(edit)[index];
    
}

tracktion_engine::WaveAudioClip::Ptr MainComponent::loadAudioFileAsClip(tracktion_engine::Edit &edit, const juce::File &file)
{
    if(auto track = getOrInsertAudioTrackAt(edit, 0))
    {
        removeAllClips(*track);
        
        tracktion_engine::AudioFile audioFile(edit.engine, file);
        
        if(audioFile.isValid())
        {
            if(auto newClip = track->insertWaveClip(file.getFileNameWithoutExtension(), file, { { 0.0, audioFile.getLength() }, 0.0 }, false))
            {
                return newClip;
            }
        }
        
    }
    return {};
}

*/
//============================================================================
//--------------------------TRACK FUNCTIONS-----------------------------------

void MainComponent::createTracksAndAssignInputs()
{
    auto & deviceManager = engineAudioSource.getEngine().getDeviceManager();
    
    for(auto i = 0; i < deviceManager.getNumWaveInDevices(); i++)
    {
        if(auto wid = deviceManager.getWaveInDevice(i))
        {
            //wid->setStereoPair(false);
            //wid->setEndToEnd(true);
            //wid->setEnabled(true);
        }
    }
    
    
    auto& edit = engineAudioSource.getEdit();
    edit.getTransport().ensureContextAllocated();
    
    int trackNum = 0;
    for(auto instance: edit.getAllInputDevices())
    {
        if(instance->getInputDevice().getDeviceType() == tracktion_engine::InputDevice::waveDevice)
        {
            if(auto track = EngineHelpers::getOrInsertAudioTrackAt(edit, trackNum))
            {
                instance->setTargetTrack(*track, 0, true);
                instance->setRecordingEnabled(*track, true);
                
                trackNum++;
            }
        }
    }
    
    edit.restartPlayback();
}



void MainComponent::createOrLoadEdit()
{
    const auto editFilePath = juce::JUCEApplication::getCommandLineParameters().replace ("-NSDocumentRevisionsDebugMode YES", "").unquoted().trim();
    const juce::File editFile (editFilePath);
    
    DBG("Creating Edit From File....");
    
    engineAudioSource.setEdit(tracktion_engine::createEmptyEdit(engineAudioSource.getEngine(), editFile));
    auto& edit = engineAudioSource.getEdit();
    DBG("Edit Loaded...");
    edit.getTransport().addChangeListener(this);
    
    editComponent = nullptr;
    
    if(!selectionManager.pasteSelected()){
        DBG("SELECTION MANAGER VALID");
    }
    createTracksAndAssignInputs();
    
    
    
    tracktion_engine::EditFileOperations (edit).save(true, true, false);

    
    editComponent = std::make_unique<EditComponent> (edit, selectionManager);
    
    addAndMakeVisible(*editComponent);
    resized();
}


//============================================================================

//---------------------SYNTHESISER FUNCTIONS----------------------------------
void MainComponent::setMidiInput(int index)
{
    
    auto list = juce::MidiInput::getAvailableDevices();
    deviceManager.removeMidiInputDeviceCallback(list[lastInputIndex].identifier, synthAudioSource.getMidiCollector());
    
    auto newInput = list[index];
    
    if(! deviceManager.isMidiInputDeviceEnabled(newInput.identifier))
    {
        deviceManager.setMidiInputDeviceEnabled(newInput.identifier, true);
    }
    deviceManager.addMidiInputDeviceCallback(newInput.identifier, synthAudioSource.getMidiCollector());
    midiInputList.setSelectedId(index + 1, juce::dontSendNotification);
    
    lastInputIndex = index;
    
}

void MainComponent::setSynth(int index){
    synthAudioSource.setSynthPreset(index);
    lastSynthIndex = index;
    
}

void MainComponent::timerCallback()
{
    keyboardComponent.grabKeyboardFocus();
    stopTimer();
}

//============================================================================

//-----------------------TRANSPORT FUNCTIONS--------------------------

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if(source == &thumbnail)
    {
        thumbnailChanged();
    }
    if(source == &transportSource)
    {
        //transportSourceChanged();
    }
}

void MainComponent::thumbnailChanged()
{
    repaint();
}


void MainComponent::playButtonClicked()
{
    if(playState == TransportState::Stopped)
    {
        auto& edit = engineAudioSource.getEdit();
        edit.getTransport().play(false);
        transportSource.start();
        playState = TransportState::Playing;
        DBG("Playing transport...");

    }

}


void MainComponent::stopButtonClicked()
{
    if(playState == TransportState::Playing)
    {
        auto& edit = engineAudioSource.getEdit();
        edit.getTransport().stop(false, false);
        transportSource.stop();
        playState = TransportState::Stopped;
        DBG("Stopping trasport...");
    }
}


void MainComponent::openButtonClicked()
{
    chooser = std::make_unique<juce::FileChooser>("Select a Wave file to play...", juce::File{}, "*.wav");
    
    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        
        if(file != juce::File{} && file.existsAsFile())
        {
            
            auto& edit = engineAudioSource.getEdit();
            auto clip = EngineHelpers::loadAudioFileAsClip(edit, file);
            auto & transport = clip->edit.getTransport();
            DBG("Loaded Audio file as Clip");
            transport.setLoopRange(clip->getEditTimeRange());
            transport.looping = true;
            transport.position = 0.0;
            transport.play(false);
            playState = TransportState::Playing;
            DBG("Playing audio Clip...");
            
            
            
            /*
            auto* reader = formatManager.createReaderFor(file);
            if(reader != nullptr)
            {
                auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
                transportSource.setSource (newSource.get(), 0, nullptr, reader->sampleRate);
                playButton.setEnabled(true);
                
                readerSource.reset(newSource.release());
                transportSource.start();
                playState = TransportState::Playing;
            }*/
            
        }
        
    });
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
    /*
    synthAudioSource.prepareToPlay (samplesPerBlockExpected, sampleRate);
    transportSource.prepareToPlay (samplesPerBlockExpected, sampleRate);
    */
    
    audioMixer.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    //bufferToFill.clearActiveBufferRegion();
    
    /*
    synthAudioSource.getNextAudioBlock (bufferToFill);
    
    
    if (readerSource.get() == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }
    
    transportSource.getNextAudioBlock (bufferToFill);
    */
    
    audioMixer.getNextAudioBlock(bufferToFill);
    
    
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
    /*
    synthAudioSource.releaseResources();
    transportSource.releaseResources();
    */
    
    audioMixer.releaseResources();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    std::cout << "Resized \n";
    auto buttonWidth = getWidth() - 100;
    auto xPos = (getWidth()/ 2) - (buttonWidth/2);
    openButton.setBounds(xPos, 5, getWidth() - 100, 20);
    playButton.setBounds(xPos, 30, getWidth() - 100, 20);
    stopButton.setBounds(xPos, 55,getWidth() - 100, 20);
    synthList.setBounds(200, 220, getWidth() - 210, 20);
    midiInputList.setBounds (200, 250, getWidth() - 210, 20);
    keyboardComponent.setBounds (10,  280, getWidth() - 20, 100);
    
    if (editComponent != nullptr){
        editComponent->setBounds (20,  100, getWidth() - 20, 100);
        DBG("EDIT COMPONENT RESIZED");
    }
        
}
