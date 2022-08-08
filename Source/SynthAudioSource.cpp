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
    
    static constexpr auto maxLevel = 0.45;
    static constexpr auto maxLevelDb = 31.0;
    static constexpr auto smoothingLengthInSeconds = 0.01;
    
};

std::map<int,std::string> SynthAudioSource::sList  = {
    {1,"Sine Wave"},
    {2,"Poly Wave"}
};
//===========================SYNTHAUDIO SOURCE================================


int SynthAudioSource::SINE_PRESET = 0;
int SynthAudioSource::POLYPHONIC_PRESET = 1;
char * SynthAudioSource::xmlTypeName = "SynthAudioSourcePlugin";

bool SynthAudioSource::sInit = false;

juce::Array<SynthPresetInfo> SynthAudioSource::synthList = juce::Array<SynthPresetInfo>();



SynthAudioSource::SynthAudioSource(tracktion_engine::PluginCreationInfo info):Plugin(info)
{
    for(auto i= 0; i < 4; i++){
        
        synth.addVoice(new SineWaveVoice());
        
    }
    synth.addSound(new SineWaveSound());
    
}

SynthAudioSource::~SynthAudioSource()
{
    
}


void SynthAudioSource::setKeyState(juce::MidiKeyboardState* keyState)
{
    keyboardState = keyState;
}
//--------------------------PLUGIN FUNCTIONS----------------------------------

juce::String SynthAudioSource::getName()
{
    return NEEDS_TRANS("SynthAudio");
}


juce::String SynthAudioSource::getPluginType()
{
    return xmlTypeName;
}

bool SynthAudioSource::needsConstantBufferSize()
{
    return false;
}

juce::String SynthAudioSource::getSelectableDescription()
{
    return getName();
}

void SynthAudioSource::initialise(const tracktion_engine::PluginInitialisationInfo &)
{
    
}


void SynthAudioSource::deinitialise()
{
    
}

void SynthAudioSource::applyToBuffer(const tracktion_engine::PluginRenderContext &fc)
{
    juce::MidiBuffer midi;
    
    if(fc.destBuffer != nullptr)
    {
        if(fc.bufferForMidiMessages != nullptr)
        {
            if(fc.bufferForMidiMessages->isAllNotesOff)
                //turn of notes here maybe
                DBG("SDF");
        }
        
        for(auto m : *fc.bufferForMidiMessages)
        {
            midi.addEvent(m, m.getTimeStamp());
        }
        
        synth.renderNextBlock(*fc.destBuffer, midi, fc.bufferStartSample, fc.bufferNumSamples);
        
    }
    
    
}

void SynthAudioSource::restorePluginStateFromValueTree(const juce::ValueTree &v)
{
    //DON'T KNOW WHAT TO DO WITH THIS FUNCTION
}


//----------------------------------------------------------------------------

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
    
    keyboardState->processNextMidiBuffer(incomingMidi, bufferToFill.startSample, bufferToFill.numSamples, true);
    
    
    /*
     //SEEMS LIKE IT WAS RENDERING THE NEXT BLOCK TWICE SINCE I THINK THEY BOTH DO THE SAME THING
     //NOTE: WATCH TO SEE IF THIS BECOMES A PROBLEM LATER ON
    synth.renderNextBlock(*bufferToFill.buffer, incomingMidi, bufferToFill.startSample, bufferToFill.numSamples);
    */
}

juce::MidiMessageCollector* SynthAudioSource::getMidiCollector()
{
    return &midiCollector;
}
