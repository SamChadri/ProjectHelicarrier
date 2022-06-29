/*
  ==============================================================================

    EngineAudioSource.h
    Created: 28 Jun 2022 3:19:51pm
    Author:  Samuel Chadri

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class EngineAudioSource : public juce::AudioSource
{
public:
    EngineAudioSource();
    
    void prepareToPlay(int,double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    
    void setEdit(std::unique_ptr<tracktion_engine::Edit> edit);
    
    virtual inline tracktion_engine::Edit& getEdit();
    
    tracktion_engine::Engine& getEngine();
    
    template<typename Function>
    void callFunctionOnMessageThread (Function&& func)
    {
        if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        {
            func();
        }
        else
        {
            jassert (! juce::MessageManager::getInstance()->currentThreadHasLockedMessageManager());
            juce::WaitableEvent finishedSignal;
            juce::MessageManager::callAsync ([&]
                                       {
                                           func();
                                           finishedSignal.signal();
                                       });
            finishedSignal.wait (-1);
        }
    }
    
    
    
    //
private:
    tracktion_engine::Engine engine {ProjectInfo::projectName};
    std::unique_ptr<tracktion_engine::Edit> edit;
    tracktion_engine::HostedAudioDeviceInterface& audioInterface;
    
    //
};
