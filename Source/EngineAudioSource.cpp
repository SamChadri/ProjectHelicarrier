/*
  ==============================================================================

    EngineAudioSource.cpp
    Created: 28 Jun 2022 3:19:51pm
    Author:  Samuel Chadri

  ==============================================================================
*/

#include "EngineAudioSource.h"


EngineAudioSource::EngineAudioSource(juce::MidiKeyboardState& keyState):audioInterface(engine.getDeviceManager().getHostedAudioDeviceInterface()), keyboardState (keyState)
{
    tracktion_engine::HostedAudioDeviceInterface::Parameters param = {};
    param.sampleRate = 44100.0;
    param.blockSize = 512;
    param.useMidiDevices = false;
    param.inputChannels = 2;
    param.outputChannels = 2;
    param.fixedBlockSize = true;// I assume this is supposed to be true, if not I have no idea what is going on then.
    
    audioInterface.initialise(param);
    setupOutputs();

}


void EngineAudioSource::prepareToPlay(int expectedBlockSize,double sampleRate)
{

    callFunctionOnMessageThread([&] { audioInterface.prepareToPlay(sampleRate, expectedBlockSize); });

    
}

void EngineAudioSource::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill)
{
    juce::MidiBuffer incomingMidi;
    
   // keyboardState.processNextMidiBuffer (incomingMidi, bufferToFill.startSample,bufferToFill.numSamples, true);
    
    
    
    audioInterface.processBlock(*bufferToFill.buffer, incomingMidi);
}

inline tracktion_engine::Edit& EngineAudioSource::getEdit(){
    return  *edit;
}

void EngineAudioSource::setEdit(std::unique_ptr<tracktion_engine::Edit> new_edit)
{
    edit = std::move(new_edit);
}

tracktion_engine::Engine& EngineAudioSource::getEngine()
{
    return engine;
}

void EngineAudioSource::releaseResources()
{
    
}

void EngineAudioSource::setupOutputs ()
{
    auto& dm = engine.getDeviceManager();

    for (int i = 0; i < dm.getNumMidiOutDevices(); i++)
    {
        auto dev = dm.getMidiOutDevice (i);
        dev->setEnabled (true);
    }
/*
    edit->playInStopEnabled = true;
    edit->getTransport().ensureContextAllocated (true);

    // Set track 2 to send to midi output
    if (auto t = getOrInsertAudioTrackAt (*edit, 1))
    {
        auto& output = t->getOutput();
        output.setOutputToDefaultDevice (true);
    }

    //edit->restartPlayback();
 */
}


tracktion_engine::AudioTrack* EngineAudioSource::getOrInsertAudioTrackAt(tracktion_engine::Edit &edit, int index)
{
    edit.ensureNumberOfAudioTracks(index + 1);
    return tracktion_engine::getAudioTracks(edit)[index];
    
}








