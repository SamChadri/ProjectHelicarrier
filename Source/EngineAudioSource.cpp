/*
  ==============================================================================

    EngineAudioSource.cpp
    Created: 28 Jun 2022 3:19:51pm
    Author:  Samuel Chadri

  ==============================================================================
*/

#include "EngineAudioSource.h"


EngineAudioSource::EngineAudioSource():audioInterface(engine.getDeviceManager().getHostedAudioDeviceInterface())
{
    audioInterface.initialise ({});

}


void EngineAudioSource::prepareToPlay(int expectedBlockSize,double sampleRate)
{

    callFunctionOnMessageThread([&] { audioInterface.prepareToPlay(sampleRate, expectedBlockSize); });

    
}

void EngineAudioSource::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill)
{
    juce::MidiBuffer incomingMidi;
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








