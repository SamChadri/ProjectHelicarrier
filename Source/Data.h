/*
  ==============================================================================

    Data.h
    Created: 8 Sep 2022 3:51:03pm
    Author:  Samuel Chadri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
//=========================THIS WILL BECOME THE NEW SYNTH CLASS LATER ON=====================================
class OscData: public juce::dsp::Oscillator<float>
{
public:
    void prepareToPlay (juce::dsp::ProcessSpec& spec)
    {
        prepare(
    }
    
    void setWaveType (const int choice)
    {
        
    }
    
    void setWaveFrequency (const int midiNoteNumber)
    {
        
    }
    
    void getNextAudioBlock (juce::dsp::AudioBlock<float>& block)
    {
        
    }
    
    void updateFm (const float freq, const float depth)
    {
        
    }
    
private:
    void processFmOsc (juce::dsp::AudioBlock<float>& block);
    juce::dsp::Oscillator<float> fmOsc { [](float x) { return std::sin (x); } };
    
    float fmMod {0.0f};
    float fmDepth {0.0f};
    float lastMidiNote {0};
};

class AdsrData: public juce::ADSR
{
public:
    
private:
    
};


class FilterData
{
public:
    
private:
    
};

//============================THIS WILL BECOME THE NEW SYNTH CLASS LATER ON=================================



