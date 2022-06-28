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

class SynthAudioSource : public juce::AudioSource
{
public:
    SynthAudioSource(juce::MidiKeyboardState& keyState);
    
    void setUsingWaveSound();
    void prepareToPlay(int,double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    juce::MidiMessageCollector* getMidiCollector();
    
    void setSynthPreset(int synthPreset);
    static juce::Array<SynthPresetInfo> getSynthList();
    
    static int SINE_PRESET;
    static int POLYPHONIC_PRESET;
    
    
private:
    juce::MidiKeyboardState& keyboardState;
    juce::Synthesiser synth;
    juce::MidiMessageCollector midiCollector;
    static juce::Array<SynthPresetInfo> synthList;
    static bool sInit;
    static std::map<int,std::string> sList;
    

};
