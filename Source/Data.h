/*
  ==============================================================================

    Data.h
    Created: 8 Sep 2022 3:51:03pm
    Author:  Samuel Chadri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class CustomOscillator
{
public:
    //==============================================================================
    CustomOscillator()
    {
        setWaveform (Waveform::sine);

        auto& gain = processorChain.template get<gainIndex>();
        gain.setRampDurationSeconds (3e-2);
        gain.setGainLinear ((0));
        

    }

    //==============================================================================
    enum class Waveform
    {
        sine,
        saw,
        square,
        triangle
    };

    void setWaveform (Waveform waveform)
    {
        switch (waveform)
        {
        case Waveform::sine:
            processorChain.template get<oscIndex>().initialise ([] (float x)
            {
               return std::sin (x);
            }, 128);
            break;

        case Waveform::saw:
            processorChain.template get<oscIndex>().initialise ([] (float x)
            {
               return juce::jmap (x,
                                  float (-juce::MathConstants<double>::pi),
                                  float (juce::MathConstants<double>::pi),
                                  float (-1),
                                  float (1));
            }, 128);
            break;
        case Waveform::triangle:
            processorChain.template get<oscIndex>().initialise ([] (float x)
            {
                return std::abs((int)x % 2);
            }, 128);
            break;
        case Waveform::square:
            processorChain.template get<oscIndex>().initialise ([] (float x)
            {
                return  x < 0.0f ? 1.0f: -1.0f;;
            }, 128);
            break;

        default:
            jassertfalse;
            break;
        }
    }

    //==============================================================================
    void setFrequency (int newValue, bool force = false)
    {
        processorChain.template get<oscIndex>().setFrequency (juce::MidiMessage::getMidiNoteInHertz(newValue), force);
    }

    void setLevel (float newValue)
    {
        processorChain.template get<gainIndex>().setGainLinear (newValue);
    }

    void reset() noexcept
    {
        processorChain.reset();
    }

    //==============================================================================
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        auto&& outBlock = context.getOutputBlock();
        auto blockToUse = tempBlock.getSubBlock (0, outBlock.getNumSamples());
        juce::dsp::ProcessContextReplacing<float> tempContext (blockToUse);
        processorChain.process (tempContext);

        outBlock.copyFrom (context.getInputBlock()).add (blockToUse);
    }

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        tempBlock = juce::dsp::AudioBlock<float> (heapBlock, spec.numChannels, spec.maximumBlockSize);
        processorChain.prepare (spec);
    }

private:
    //==============================================================================
    juce::HeapBlock<char> heapBlock;
    juce::dsp::AudioBlock<float> tempBlock;

    enum
    {
        oscIndex,
        gainIndex,
    };

    juce::dsp::ProcessorChain<juce::dsp::Oscillator<float>, juce::dsp::Gain<float>> processorChain;
};

class Distortion
{
public:

private:
    
};

class OscData: public juce::dsp::Oscillator<float>
{
public:
    void prepareToPlay (juce::dsp::ProcessSpec& spec)
    {
        prepare(spec);
        fmOsc.prepare(spec);
        processorChain.template get<0>().prepare(spec);
        processorChain.template get<0>().initialise ([] (float x)
        {
           return std::sin (x);
        }, 128);
    }
    
    void setWaveType (const int choice)
    {
        switch (choice) {
            case 0:
                //Sine
                initialise([](float x) {return std::sin(x);});
                break;
            case 1:
                //Saw Wave
                initialise([](float x) {return x / juce::MathConstants<float>::pi;});
                break;
            case 2:
                //Square Wave
                initialise([](float x) {return x < 0.0f ? -1.0f: -1.0f;});
                break;
            
            default:
                jassertfalse; // Didn't pick a specified option
                break;
        }
        
    }
    
    void setWaveFrequency (const int midiNoteNumber)
    {
        setFrequency(juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));
        fmOsc.setFrequency(juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));
        processorChain.template get<0>().setFrequency(juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber));
        lastMidiNote = midiNoteNumber;
        
    }
    
    void getNextAudioBlock (juce::dsp::AudioBlock<float>& block)
    {
        //processFmOsc (block);
        //fmOsc.process(juce::dsp::ProcessContextReplacing<float>(block));
        //processorChain.process (juce::dsp::ProcessContextReplacing<float>(block));
        process (juce::dsp::ProcessContextReplacing<float> (block));
    }
    
    void processFmOsc(juce::dsp::AudioBlock<float>& block)
    {
        for(int c=0; c < block.getNumChannels(); c++)
        {
            for (int s = 0; s < block.getNumSamples(); s++) {
                fmMod = fmOsc.processSample(block.getSample(c ,s)) * fmDepth;
            }
        }
    }
    
    void updateFm (const float freq, const float depth)
    {
        fmOsc.setFrequency(freq);
        fmDepth = depth;
        auto currentFreq = juce::MidiMessage::getMidiNoteInHertz(lastMidiNote) + fmMod;
        setFrequency(currentFreq);
    }
    
private:
    juce::dsp::Oscillator<float> fmOsc { [](float x) { return std::sin (x); } };
    juce::dsp::ProcessorChain<juce::dsp::Oscillator<float>, juce::dsp::Gain<float>> processorChain;
    float fmMod {0.0f};
    float fmDepth {0.0f};
    float lastMidiNote {0};
};

class AdsrData: public juce::ADSR
{
    
public:
    void update(const float attack, const float decay, const float sustain, const float release)
    {
        
        adsrParams.attack = attack;
        adsrParams.decay = decay;
        adsrParams.sustain = sustain;
        adsrParams.release = release;
        
        setParameters(adsrParams);
        
    }
private:
    juce::ADSR::Parameters adsrParams;
};


class FilterData
{
public:
    void prepareToPlay(double sampleRate, int samplesPerBlock, int numChannels)
    {
        filter.reset();
        
        juce::dsp::ProcessSpec spec;
        spec.maximumBlockSize = samplesPerBlock;
        spec.sampleRate = sampleRate;
        spec.numChannels = numChannels;
        
        filter.prepare(spec);
        
        isPrepared = true;
        
    }
    
    void process (juce::AudioBuffer<float>& buffer)
    {
        //jassert(isPrepared);
        
        juce::dsp::AudioBlock<float> block {buffer};
        filter.process(juce::dsp::ProcessContextReplacing<float> {block});
    }
    
    void updateParameters (const float modulator, const int filterType, const float frequency, const float resonance)
    {
        switch (filterType) {
            case 0:
                filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
                break;
            case 1:
                filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
                break;
            case 2:
                filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
                break;
            default:
                break;
        }
        
        float modulatedFreq = frequency * modulator;
        modulatedFreq = std::fmax(std::fmin(modulatedFreq, 20000.0f), 20.0f);
        
        filter.setCutoffFrequency(modulatedFreq);
        filter.setResonance(resonance);
    }
    
    void setCutOffFrequency( float frequency)
    {
        filter.setCutoffFrequency(frequency);
    }
    
    void reset()
    {
        filter.reset();
    }
private:
    
    juce::dsp::StateVariableTPTFilter<float> filter;
    bool isPrepared { false };
    
};

//============================THIS WILL BECOME THE NEW SYNTH CLASS LATER ON=================================



