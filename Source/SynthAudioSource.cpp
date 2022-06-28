//
//  SynthAudioSource.cpp
//  MidiProject - App
//
//  Created by Samuel Chadri on 5/25/22.
//

#include "SynthAudioSource.h"



struct SineWaveSound: public juce::SynthesiserSound
{
    SineWaveSound(){}
    
    bool appliesToNote (int) override {return true;}
    bool appliesToChannel (int) override {return true;}
};


struct SineWaveVoice: public juce::SynthesiserVoice
{
public:
    SineWaveVoice(){}
    
    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<SineWaveSound*>(sound) != nullptr;
    }
    
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound *, int /*currentPitchWheelPosition*/) override
    {
        currentAngle = 0.0;
        level = velocity;
        tailOff = 0.0;
        
        auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        auto cyclesPerSample = cyclesPerSecond / getSampleRate();
        angleDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;
        
    }
    
    
    
    void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {
        
        if(angleDelta != 0.0)
        {
            if(tailOff > 0.0)
            {
                for(auto i = 0; i < numSamples; i++)
                {
                    auto currentSample = (float) (std::sin(currentAngle) * level * tailOff);
                    for(auto i = 0; i < outputBuffer.getNumChannels(); i++)
                    {
                        outputBuffer.addSample(i, startSample, currentSample);
                    }

                    currentAngle += angleDelta;
                    ++startSample;
                    
                    tailOff *= 0.99;
                    
                    if(tailOff <= 0.005)
                    {
                        clearCurrentNote();
                        
                        angleDelta = 0.0;
                        break;
                    }
                }

            }
            else
            {
                for(auto i = 0; i < numSamples; i++)
                {
                    auto currentSample = (float) (std::sin(currentAngle) * level);
                    for(auto i = 0; i < outputBuffer.getNumChannels(); i++)
                    {
                        outputBuffer.addSample(i, startSample, currentSample);
                    }

                    currentAngle += angleDelta;
                    ++startSample;
                }
            }
        }
        
        
    }
    
    void stopNote(float/*velocity*/, bool allowTailOff) override
    {
        if(allowTailOff)
        {
            if(tailOff == 0.0)
            {
                tailOff = 1.0;
            }
            
        }else{
            clearCurrentNote();
            angleDelta = 0.0;
        }
        
    }
    

    
    void pitchWheelMoved (int) override      {}
    void controllerMoved (int, int) override {}
    
private:
    float angleDelta = 0.0f;
    float currentAngle = 0.0f;
    double currentSampleRate = 0;
    double tailOff = 0;
    double level = 0;

    
};
struct PolyphonicSound: public juce::SynthesiserSound
{
    PolyphonicSound(){}
    
    bool appliesToNote (int) override {return true;}
    bool appliesToChannel (int) override {return true;}
};

struct PolyphonicVoice: public juce::SynthesiserVoice
{
public:
    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<PolyphonicSound*>(sound) != nullptr;
    }
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound *, int /*currentPitchWheelPosition*/) override
    {
        currentAngle = 0.0;
        level = velocity;
        tailOff = 0.0;
        
        auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        auto cyclesPerSample = cyclesPerSecond / getSampleRate();
        angleDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;
        
    }
    void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {
        
        if(angleDelta != 0.0)
        {
            if(tailOff > 0.0)
            {
                for(auto i = 0; i < numSamples; i++)
                {
                    auto currentSample = (float) (getNextSample() * tailOff);
                    for(auto i = 0; i < outputBuffer.getNumChannels(); i++)
                    {
                        outputBuffer.addSample(i, startSample, currentSample);
                    }

                    currentAngle = std::fmod (currentAngle + angleDelta, 2.0 * juce::MathConstants<double>::pi);
                    ++startSample;
                    
                    tailOff *= 0.99;
                    
                    if(tailOff <= 0.005)
                    {
                        clearCurrentNote();
                        
                        angleDelta = 0.0;
                        break;
                    }
                }

            }
            else
            {
                for(auto i = 0; i < numSamples; i++)
                {
                    auto currentSample = (float) (getNextSample());
                    for(auto i = 0; i < outputBuffer.getNumChannels(); i++)
                    {
                        outputBuffer.addSample(i, startSample, currentSample);
                    }

                    currentAngle = std::fmod (currentAngle + angleDelta, 2.0 * juce::MathConstants<double>::pi);
                    ++startSample;
                }
            }
        }
        
        
    }
    
    float getNextSample() noexcept
    {
        auto levelDb = (level - 1.0) * maxLevelDb;
        auto amplitude = std::pow(10.0f, 0.05f * levelDb) * maxLevel;
        
        auto f1 = std::sin(currentAngle);
        auto f2 = std::copysign(1.0, f1);
        
        auto a2 = 0.69;
        auto a1 = 1.0 - a2;
        
        
        auto nextSample = float(amplitude * ((a1 * f1) + (a2 * f2)));
        return nextSample;
        
        
    }
    
    void stopNote(float/*velocity*/, bool allowTailOff) override
    {
        if(allowTailOff)
        {
            if(tailOff == 0.0)
            {
                tailOff = 1.0;
            }
            
        }else{
            clearCurrentNote();
            angleDelta = 0.0;
        }
        
    }
    
    void pitchWheelMoved (int) override      {}
    void controllerMoved (int, int) override {}
    
    
    
    
private:
    float angleDelta = 0.0f;
    float currentAngle = 0.0f;
    double currentSampleRate = 0;
    double tailOff = 0;
    double level = 0;
    
    static constexpr auto maxLevel = 0.05;
    static constexpr auto maxLevelDb = 31.0;
    static constexpr auto smoothingLengthInSeconds = 0.01;
    
};

std::map<int,std::string> SynthAudioSource::sList  = {
    {1,"Sine Wave"},
    {2,"Poly Wave"}
};

int SynthAudioSource::SINE_PRESET = 0;
int SynthAudioSource::POLYPHONIC_PRESET = 1;

bool SynthAudioSource::sInit = false;

juce::Array<SynthPresetInfo> SynthAudioSource::synthList = juce::Array<SynthPresetInfo>();

SynthAudioSource::SynthAudioSource(juce::MidiKeyboardState& keyState)
: keyboardState(keyState){
      for(auto i= 0; i < 4; i++){
          
          synth.addVoice(new SineWaveVoice());
          
      }
      synth.addSound(new SineWaveSound());
      

    
}

void SynthAudioSource::setUsingWaveSound(){
    synth.clearSounds();

}

void SynthAudioSource::setSynthPreset(int synthPreset)
{
    synth.clearVoices();
    synth.clearSounds();
    
    if(synthPreset == SynthAudioSource::SINE_PRESET)
    {
        for(auto i =0; i < 5; i++)
        {
            synth.addVoice(new SineWaveVoice());
        }
        synth.addSound(new SineWaveSound());
        
    }else{
        for(auto i = 0; i < 5; i++)
        {
            synth.addVoice(new PolyphonicVoice());
        }
        synth.addSound(new PolyphonicSound());
    }
    

}

void SynthAudioSource::prepareToPlay(int /*samplesPerBlockExpected*/, double sampleRate)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    midiCollector.reset(sampleRate);
}

void SynthAudioSource::releaseResources() {}

juce::Array<SynthPresetInfo> SynthAudioSource::getSynthList()
{
    if(!SynthAudioSource::sInit)
    {
        for(auto& item: sList)
        {
            auto info = SynthPresetInfo();
            info.identifier = item.first;
            info.name = item.second;
            synthList.add(info);
        }
    }

    return synthList;
}


void SynthAudioSource::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
    
    juce::MidiBuffer incomingMidi;
    midiCollector.removeNextBlockOfMessages(incomingMidi, bufferToFill.numSamples);
    
    keyboardState.processNextMidiBuffer(incomingMidi, bufferToFill.startSample, bufferToFill.numSamples, true);
    
    
    synth.renderNextBlock(*bufferToFill.buffer, incomingMidi, bufferToFill.startSample, bufferToFill.numSamples);
    
}

juce::MidiMessageCollector* SynthAudioSource::getMidiCollector()
{
    return &midiCollector;
}
