/*
  ==============================================================================

    Metronome.h
    Created: 5 Sep 2022 9:44:31pm
    Author:  Samuel Chadri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Data.h"


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
    
    void prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels)
    {
        juce::dsp::ProcessSpec spec;
        
        spec.maximumBlockSize = samplesPerBlock;
        spec.sampleRate = sampleRate;
        spec.numChannels = outputChannels;
        
        osc.prepareToPlay(spec);
        filterAdsr.setSampleRate(sampleRate);
        filter.prepareToPlay(sampleRate, samplesPerBlock, outputChannels);
        adsr.setSampleRate(sampleRate);
        gain.prepare(spec);
        
        gain.setGainLinear(0.3f);
        
        isPrepared = true;
        
    }
    
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound *, int /*currentPitchWheelPosition*/) override
    {
        /*
        currentAngle = 0.0;
        level = velocity;
        tailOff = 0.0;
        
        auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        auto cyclesPerSample = cyclesPerSecond / getSampleRate();
        angleDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;
        */
        
        osc.setWaveFrequency(midiNoteNumber);
        adsr.noteOn();
        filterAdsr.noteOn();
        
    }
    
    
    
    void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {
        
        
        synthBuffer.setSize (outputBuffer.getNumChannels(), numSamples, false, false, true);
        filterAdsr.applyEnvelopeToBuffer (outputBuffer, 0, numSamples);
        synthBuffer.clear();
            
        juce::dsp::AudioBlock<float> audioBlock { synthBuffer };
        osc.getNextAudioBlock (audioBlock);
        //audioBlock.copyTo(synthBuffer);
        adsr.applyEnvelopeToBuffer (synthBuffer, 0, synthBuffer.getNumSamples());
        filter.process (synthBuffer);
        gain.process (juce::dsp::ProcessContextReplacing<float> (audioBlock));
        
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
           // outputBuffer.addFrom (channel, startSample, synthBuffer, channel, 0, numSamples);
            
            if (! adsr.isActive())
                clearCurrentNote();
        }
        juce::dsp::AudioBlock<float> (outputBuffer)
            .getSubBlock ((size_t) startSample, (size_t) numSamples)
            .add (audioBlock);
        /*
        if(angleDelta != 0.0)
        {
            if(tailOff > 0.0)
            {
                for(auto i = 0; i < numSamples; i++)
                {
                    auto currentSample = (float) (std::sin(currentAngle) * level * tailOff);
                    for(auto k = 0; k < outputBuffer.getNumChannels(); k++)
                    {
                        outputBuffer.addSample(k, startSample, currentSample);
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
                    for(auto k = 0; k < outputBuffer.getNumChannels(); k++)
                    {
                        outputBuffer.addSample(k, startSample, currentSample);
                    }

                    currentAngle += angleDelta;
                    ++startSample;
                }
            }
        }
        
        */
    }
    
    void stopNote(float/*velocity*/, bool allowTailOff) override
    {
        /*
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
         */
        
        adsr.noteOff();
        filterAdsr.noteOff();
        
        if(! allowTailOff || ! adsr.isActive())
            clearCurrentNote();
    }
    
    void updateFilter (const int filterType, const float frequency, const float resonance)
    {
        auto modulator = filterAdsr.getNextSample();
        filter.updateParameters (modulator, filterType, frequency, resonance);
    }
    
    OscData& getOscillator() { return osc; }
    AdsrData& getAdsr() { return adsr; }
    AdsrData& getFilterAdsr() { return filterAdsr; }
    FilterData& getFilter() { return filter; }
    

    
    void pitchWheelMoved (int) override      {}
    void controllerMoved (int, int) override {}
    
private:
    float angleDelta = 0.0f;
    float currentAngle = 0.0f;
    double currentSampleRate = 0;
    double tailOff = 0;
    double level = 0;
    //double gain = 0.3;
    
    //==============================================================

    juce::AudioBuffer<float> synthBuffer;

    OscData osc;
    AdsrData adsr;
    AdsrData filterAdsr;
    FilterData filter;
    juce::dsp::Gain<float> gain;
    
    bool isPrepared {false};
    
};

class Metronome : public AudioSource, public juce::HighResolutionTimer, public AudioProcessor
{
public:
    enum TransportState
    {
        Stopped,
        Starting,
        Playing,
        Stopping
    };
    
    Metronome() : parameters(*this,nullptr,juce::Identifier ("Metronome"),createParams())
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
        
        for (int i = 0; i < synth.getNumVoices(); i++)
        {
            if (auto voice = dynamic_cast<MetronomeVoice*>(synth.getVoice(i)))
            {
                voice->prepareToPlay (sampleRate, samplesPerBlockExpected, 1);
            }
        }

    }
    
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        for (int i = 0; i < synth.getNumVoices(); i++)
        {
            if (auto voice = dynamic_cast<MetronomeVoice*>(synth.getVoice(i)))
            {
                voice->prepareToPlay (sampleRate, maximumExpectedSamplesPerBlock, getTotalNumOutputChannels());
            }
        }
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
            MidiMessage mOn = MidiMessage::noteOn(1, 72, .3f);
            auto timestamp = (int) time * mSampleRate;
            DBG("TIMESTAMP MON: " << timestamp);
            mOn.setTimeStamp(timestamp);
            metroNotes.push_back(mOn);
            metroTimes.push_back(totalSamples);
            //midiCollector.addMessageToQueue(mOn);

            noteCounter++;
            
            MidiMessage mOff = MidiMessage::noteOff(1,72, .3f);
            timestamp = (int) (time + (1/8))* (mSampleRate);
            mOff.setTimeStamp(timestamp);
            metroNotes.push_back(mOff);
            auto noteOffTime = totalSamples + ((int) mSampleRate/8);
            //midiCollector.addMessageToQueue(mOff);
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
    
    
    void mReset()
    {
        totalSamples = 0;
    }
    
    void addMessageToBuffer( int sampleBlock)
    {
        auto endOfBuffer = totalSamples + sampleBlock;
        auto nextSample = metroTimes[currentNote];
        
        while(nextSample < endOfBuffer && currentNote < metroTimes.size())
        {
            //DIS A LIE. I ON KNOW WHO CAME UP WIT THIS BULLSHIT.
            
            auto position = int(nextSample % sampleBlock);
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
        incomingMidi.clear();
        
        if(currState == TransportState::Playing)
        {
            countSamples(bufferToFill.numSamples);
            if(!metroNotes.empty())
                addMessageToBuffer(bufferToFill.numSamples);

        }
        for(int i = 0; i < synth.getNumVoices(); i++)
        {
            if (auto voice = dynamic_cast<MetronomeVoice*>(synth.getVoice(i)))
            {
                // Osc
                auto& oscWaveChoice = *parameters.getRawParameterValue ("OSC1WAVETYPE");
                
                // FM
                auto& fmFreq = *parameters.getRawParameterValue ("OSC1FMFREQ");
                auto& fmDepth = *parameters.getRawParameterValue ("OSC1FMDEPTH");
                
                // Amp Adsr
                auto& attack = *parameters.getRawParameterValue ("ATTACK");
                auto& decay = *parameters.getRawParameterValue ("DECAY");
                auto& sustain = *parameters.getRawParameterValue ("SUSTAIN");
                auto& release = *parameters.getRawParameterValue ("RELEASE");
                
                // Filter Adsr
                auto& fAttack = *parameters.getRawParameterValue ("FILTERATTACK");
                auto& fDecay = *parameters.getRawParameterValue ("FILTERDECAY");
                auto& fSustain = *parameters.getRawParameterValue ("FILTERSUSTAIN");
                auto& fRelease = *parameters.getRawParameterValue ("FILTERRELEASE");
                
                // Filter
                auto& filterType = *parameters.getRawParameterValue ("FILTERTYPE");
                auto& cutoff = *parameters.getRawParameterValue ("FILTERFREQ");
                auto& resonance = *parameters.getRawParameterValue ("FILTERRES");
                
                // Update voice
                
                voice->getOscillator().setWaveType (oscWaveChoice);
                voice->getOscillator().updateFm (fmFreq, fmDepth);
                voice->getAdsr().update (attack.load(), decay.load(), sustain.load(), release.load());
                voice->getFilterAdsr().update (fAttack.load(), fDecay.load(), fSustain.load(), fRelease.load());
                voice->updateFilter (filterType, cutoff, resonance);
            }
        }
        

        
        midiCollector.removeNextBlockOfMessages(incomingMidi, bufferToFill.numSamples);
        //processBlock(*bufferToFill.buffer, incomingMidi);
        synth.renderNextBlock(*bufferToFill.buffer, incomingMidi, bufferToFill.startSample, bufferToFill.numSamples);
    }
    void processBlock (AudioBuffer< float > &buffer, MidiBuffer &midiMessages) override
    {

        
        //synth.renderNextBlock(buffer, incomingMidi, 0, buffer.getNumSamples());
    }
    
    void hiResTimerCallback() override
    {
        startTimer(1);
        startTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
        samplesPerBeat = 60 / bpm * mSampleRate;
    }
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParams()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        
        // OSC select
        params.push_back (std::make_unique<juce::AudioParameterChoice>("OSC1WAVETYPE", "Osc 1 Wave Type", juce::StringArray { "Sine", "Saw", "Square" }, 0));
        
        // FM
        params.push_back (std::make_unique<juce::AudioParameterFloat>("OSC1FMFREQ", "Osc 1 FM Frequency", juce::NormalisableRange<float> { 0.0f, 1000.0f, 0.01f, 0.3f }, 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat>("OSC1FMDEPTH", "Osc 1 FM Depth", juce::NormalisableRange<float> { 0.0f, 1000.0f, 0.01f, 0.3f }, 0.0f));
        
        // ADSR
        params.push_back (std::make_unique<juce::AudioParameterFloat>("ATTACK", "Attack", juce::NormalisableRange<float> { 0.1f, 1.0f, 0.1f }, 0.1f));
        params.push_back (std::make_unique<juce::AudioParameterFloat>("DECAY", "Decay", juce::NormalisableRange<float> { 0.1f, 1.0f, 0.1f }, 0.1f));
        params.push_back (std::make_unique<juce::AudioParameterFloat>("SUSTAIN", "Sustain", juce::NormalisableRange<float> { 0.1f, 1.0f, 0.1f }, 1.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat>("RELEASE", "Release", juce::NormalisableRange<float> { 0.1f, 3.0f, 0.1f }, 0.4f));
        
        // Filter ADSR
        params.push_back (std::make_unique<juce::AudioParameterFloat>("FILTERATTACK", "Filter Attack", juce::NormalisableRange<float> { 0.1f, 1.0f, 0.1f }, 0.1f));
        params.push_back (std::make_unique<juce::AudioParameterFloat>("FILTERDECAY", "Filter Decay", juce::NormalisableRange<float> { 0.1f, 1.0f, 0.1f }, 0.1f));
        params.push_back (std::make_unique<juce::AudioParameterFloat>("FILTERSUSTAIN", "Filter Sustain", juce::NormalisableRange<float> { 0.1f, 1.0f, 0.1f }, 1.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat>("FILTERRELEASE", "Filter Release", juce::NormalisableRange<float> { 0.1f, 3.0f, 0.1f }, 0.4f));
        
        // Filter
        params.push_back (std::make_unique<juce::AudioParameterChoice>("FILTERTYPE", "Filter Type", juce::StringArray { "Low-Pass", "Band-Pass", "High-Pass" }, 0));
        params.push_back (std::make_unique<juce::AudioParameterFloat>("FILTERFREQ", "Filter Freq", juce::NormalisableRange<float> { 20.0f, 20000.0f, 0.1f, 0.6f }, 200.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat>("FILTERRES", "Filter Resonance", juce::NormalisableRange<float> { 1.0f, 10.0f, 0.1f }, 1.0f));
        
        return { params.begin(), params.end() };
    }
    
    //========================================================================================================
    
    bool hasEditor() const override
    {
        return false;
    }
    
    juce::AudioProcessorEditor * createEditor() override
    {
        return nullptr;
    }
    
    int getNumPrograms() override
    {
        return 1;
    }
    
    int getCurrentProgram() override
    {
        return 0;
    }
    
    void setCurrentProgram (int index) override
    {
        
    }
    
    const juce::String getProgramName (int index) override
    {
        return {};
    }
    
    
    void changeProgramName (int index, const juce::String& newName) override
    {
        
    }
    const juce::String getName() const override
    {
        return "Metronome";
    }
    
    double getTailLengthSeconds() const override
    {
        return 0.0;
    }
    bool acceptsMidi() const override
    {
        return false;
    }
    
    bool producesMidi() const override
    {
        return true;//
    }
    
    
    
    
    
    void getStateInformation (juce::MemoryBlock& destData) override
    {
        
    }
    
    void setStateInformation (const void* data, int sizeInBytes) override
    {
        
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
    
    juce::AudioProcessorValueTreeState parameters;
    
};
