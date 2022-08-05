//
//  SynthAudioSource.hpp
//  MidiProject - App
//
//  Created by Samuel Chadri on 5/25/22.
//

#pragma once
#include <JuceHeader.h>
struct SynthPresetInfo
{
    int identifier;
    std::string name;
};

class SynthAudioSource : public juce::AudioSource, public tracktion_engine::Plugin
{
public:
    SynthAudioSource(tracktion_engine::PluginCreationInfo info);
    
    ~SynthAudioSource() override;
    
    void setKeyState(juce::MidiKeyboardState* keyState);
    //========================================================================
    juce::String getName() override;
    juce::String getPluginType() override;
    bool needsConstantBufferSize() override;
    juce::String getSelectableDescription() override;
    
    void initialise(const tracktion_engine::PluginInitialisationInfo&) override;
    void deinitialise() override;
    
    void applyToBuffer(const tracktion_engine::PluginRenderContext& fc) override;
    void restorePluginStateFromValueTree(const juce::ValueTree& v) override;
    
    //========================================================================
    void setUsingWaveSound();
    void prepareToPlay(int,double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    juce::MidiMessageCollector* getMidiCollector();
    
    void setSynthPreset(int synthPreset);
    static juce::Array<SynthPresetInfo> getSynthList();
    
    static int SINE_PRESET;
    static int POLYPHONIC_PRESET;
    static char* xmlTypeName;
    
private:
    juce::MidiKeyboardState* keyboardState;
    juce::Synthesiser synth;
    juce::MidiMessageCollector midiCollector;
    static juce::Array<SynthPresetInfo> synthList;
    static bool sInit;
    static std::map<int,std::string> sList;
    

};
