/*
  ==============================================================================

    Metronome.h
    Created: 5 Sep 2022 9:44:31pm
    Author:  Samuel Chadri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>



struct MetronomeSound: public juce::SynthesiserSound
{
    MetronomeSound(){}
    
    bool appliesToNote (int) override {return true;}
    bool appliesToChannel (int) override {return true;}
};


struct MetronomeVoice: public juce::SynthesiserVoice
{
public:
    MetronomeVoice(){}
    
    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<MetronomeSound*>(sound) != nullptr;
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
                    
                    tailOff *= 0.59;
                    
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
    double gain = 0.3;

    
};

class Metronome : public AudioSource, public juce::HighResolutionTimer
{
public:
    enum TransportState
    {
        Stopped,
        Starting,
        Playing,
        Stopping
    };
    
    Metronome()
    {
        for(auto i= 0; i < 4; i++){
            synth.addVoice(new MetronomeVoice());
        }
        
        synth.addSound(new MetronomeSound());
    }
    
    ~Metronome()
    {
        
    }
    
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
    {
        mSampleRate = sampleRate;
        DBG("SAMPLE RATE: " << sampleRate);
        samplesPerBeat = 60 / bpm * sampleRate;
        synth.setCurrentPlaybackSampleRate(sampleRate);
        midiCollector.reset(sampleRate);
    }
    
    void countSamples(int samplesPerBlock)
    {
        totalSamples += samplesPerBlock;
        //DBG("TOTAL SAMPLES: " << totalSamples);
        //DBG("SAMPLES PER BLOCK: " <<  samplesPerBlock);
        //DBG("SAMPLES PER BEAT: " << samplesPerBeat);
        int beatNum = totalSamples / samplesPerBeat;
        
        if(beatNum != totalBeatNum)
        {
            totalBeatNum = beatNum;
            DBG("BEAT NUMBER : " << totalBeatNum);
            double time = juce::Time::getMillisecondCounterHiRes() * 0.001 - startTime;
            DBG("TOTAL SAMPLES: " << totalSamples);
            MidiMessage mOn = MidiMessage::noteOn(1, 72, 1.f);
            auto timestamp = (int) time * mSampleRate;
    
            metroNotes.push_back(mOn);
            metroTimes.push_back(totalSamples);
            noteCounter++;
            
            MidiMessage mOff = MidiMessage::noteOff(1,72);
            metroNotes.push_back(mOff);
            auto noteOffTime = totalSamples + ((int) mSampleRate/8);
            metroTimes.push_back(noteOffTime);
            noteCounter++;
            DBG("NOTE OFF SAMPLES: "  << noteOffTime);
            //midiCollector.addMessageToQueue(mOff);
            
        }
        
        modBeatNum = beatNum % 4;
        
        
    }
    
    void setBpm(double newBpm)
    {
        bpm = newBpm;
    }
    
    
    void reset()
    {
        totalSamples = 0;
    }
    
    void addMessageToBuffer( int sampleBlock)
    {
        auto endOfBuffer = totalSamples + sampleBlock;
        auto nextSample = metroTimes[currentNote];
        
        while(nextSample <= endOfBuffer && currentNote < metroTimes.size())
        {
            //DIS A LIE. I ON KNOW WHO CAME UP WIT THIS BULLSHIT.
            auto position = int(nextSample - totalSamples);
            DBG("SAMPLE POSTION: " << position);
            
           incomingMidi.addEvent(metroNotes[currentNote], nextSample);
            
            currentNote++;
            
            if(currentNote >= metroTimes.size())
                break;
            
            nextSample = metroTimes[currentNote];
            
        }
    }
    
    void setTransportState(TransportState state)
    {
        currState = state;
    }
    
    void releaseResources() override
    {
        
    }
    
    void getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) override
    {
        bufferToFill.clearActiveBufferRegion();
        
        if(currState == TransportState::Playing)
        {
            countSamples(bufferToFill.numSamples);
            if(!metroNotes.empty())
                addMessageToBuffer(bufferToFill.numSamples);

        }
        
        
        
        midiCollector.removeNextBlockOfMessages(incomingMidi, bufferToFill.numSamples);
        
        synth.renderNextBlock(*bufferToFill.buffer, incomingMidi, bufferToFill.startSample, bufferToFill.numSamples);
    }
    
    void hiResTimerCallback() override
    {
        startTimer(1);
        startTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
        samplesPerBeat = 60 / bpm * mSampleRate;
    }
    
private:
    double mSampleRate;
    double samplesPerBeat;
    double bpm {120} ;
    double totalSamples;
    int totalBeatNum = -1;
    int modBeatNum;
    int nextSampleNum;
    
    int currentNote {0};
    std::vector<juce::int64> metroTimes;
    std::vector<juce::MidiMessage> metroNotes;
    int noteCounter {0};
    double startTime;
    
    
    TransportState currState {TransportState::Stopped};
    
    juce::Synthesiser synth;
    juce::MidiMessageCollector midiCollector;
    MidiBuffer incomingMidi;
};
