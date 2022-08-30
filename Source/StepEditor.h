/*
  ==============================================================================

    StepEditor.h
    Created: 10 Aug 2022 9:07:51pm
    Author:  Samuel Chadri

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "../includes/common/Components.h"
#include "../includes/common/BinaryData.h"
#include "../includes/common/Utilities.h"


//============================================================================

namespace
{
    static void loadFileIntoSamplerChannel (te::StepClip& clip, int channelIndex, const File& f)
    {
        // Find SamplerPlugin for the Clip's Track
        if (auto sampler = clip.getTrack()->pluginList.findFirstPluginOfType<te::SamplerPlugin>())
        {
            // Update the Sound layer source
            sampler->setSoundMedia (channelIndex, f.getFullPathName());

            // Then update the channel name
            clip.getChannels()[channelIndex]->name = f.getFileNameWithoutExtension();
        }
        else
        {
            jassertfalse; // No SamplerPlugin added yet?
        }
    }
}

//============================================================================

struct StepEditor: public juce::Component, private tracktion_engine::SelectableListener
{
    StepEditor(tracktion_engine::StepClip& sc)
    : clip(sc), transport(sc.edit.getTransport())
    {
        for(auto c: clip.getChannels())
        {
            addAndMakeVisible(channelConfigs.add(new ChannelConfig(*this, c->getIndex())));
        }
        
        addAndMakeVisible(patternEditor);
        timer.setCallback ([this] {patternEditor.updatePaths(); });
        timer.startTimerHz(15);
        
        clip.addSelectableListener(this);
        
    }
    
    ~StepEditor() override
    {
        clip.removeSelectableListener (this);
    }
    
    tracktion_engine::StepClip::Pattern getPattern() const
    {
        return clip.getPattern (0);
    }
    
    void paint(Graphics&) override
    {
        
    }
    
    void updatePaths()
    {
        patternEditor.resized();
    }
    
    int getPatternWidth()
    {
        return patternEditor.getWidth();
    }
    
    void resized()override
    {
        auto r = getLocalBounds();
        auto configR = r.removeFromLeft (150);

        for (auto c : channelConfigs)
        {
            auto channelR = configR.toFloat();
            channelR.setVerticalRange (getChannelYRange (c->channelIndex));
            c->setBounds (channelR.getSmallestIntegerContainer());
        }
        patternEditor.setBounds (r);
    }
    
    //------------------------------------------------------------------------
 
    
    struct ChannelConfig : public juce::Component
    {
        ChannelConfig(StepEditor& se, int channel)
        : editor(se), channelIndex(channel)
        {
            jassert(isPositiveAndBelow(channelIndex, editor.clip.getChannels().size()));
            nameLabel.getTextValue().referTo(editor.clip.getChannels()[channelIndex]->name.getPropertyAsValue());
            loadButton.onClick =  [this] {browseForAndLoadSample(); };
            randomiseButton.onClick = [this] {randomiseChannel(); };
            
            volumeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            volumeSlider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
            volumeSlider.setRange(0,127,1);
            volumeSlider.getValueObject().referTo (editor.clip.getChannels()[channelIndex]->noteValue.getPropertyAsValue());
            volumeSlider.setDoubleClickReturnValue (true, te::StepClip::defaultNoteValue);
            
            
            loadButton.setShape(Icons::getFolderPath(), false, true, false);
            loadButton.setBorderSize (BorderSize<int> (2));
            randomiseButton.setShape (Icons::getDicePath(), false, true, false);
            
            addAndMakeVisible(nameLabel);
            addAndMakeVisible(loadButton);
            addAndMakeVisible(randomiseButton);
            addAndMakeVisible(volumeSlider);
            
            
            
        }
        
        void browseForAndLoadSample()
        {
            EngineHelpers::browseForAudioFile(editor.clip.edit.engine,
                                              [this] (const File &f) {loadFileIntoSamplerChannel (editor.clip, channelIndex, f);});
        }
        
        void randomiseChannel()
        {
            editor.getPattern().randomiseChannel(channelIndex);
        }
        
        void resized() override
        {
            auto r = getLocalBounds();
            const int buttonH = roundToInt (r.getHeight() * 0.7);
            loadButton.setBounds (r.removeFromLeft (buttonH).reduced (2));
            randomiseButton.setBounds (r.removeFromLeft (buttonH).reduced (2));
            volumeSlider.setBounds (r.removeFromLeft (r.getHeight()));
            nameLabel.setBounds (r.reduced (2));
        }
        
        //----------------------------VARIABLES------------------------------
        StepEditor& editor;
        const int channelIndex;
        ShapeButton loadButton {"L", Colours::white, Colours::white, Colours::white};
        ShapeButton randomiseButton {"R", Colours::white, Colours::white, Colours::white};
        Label nameLabel;
        Slider volumeSlider;
    };
    
    //------------------------------------------------------------------------
    struct PatternEditor : public juce::Component
    {
        
        PatternEditor(StepEditor& se)
            :editor(se)
        {
            
        }
        
        void updatePaths()
        {
            playheadRect = {};
            grid.clear();
            activeCells.clear();
            playingCells.clear();

            const auto pattern (editor.clip.getPattern (0));
            const int numChans = editor.clip.getChannels().size();
            const float indent = 2.0f;

            const bool isPlaying = editor.transport.isPlaying();
            const float playheadX = getPlayheadX();

            for (int i = 0; i < numChans; ++i)
            {
                const BigInteger cache (pattern.getChannel (i));
                const Range<float> y (editor.getChannelYRange (i));
                int index = 0;
                float lastX = 0.0f;

                grid.addWithoutMerging ({ 0.0f, y.getStart() - 0.25f, (float) getWidth(), 0.5f });

                for (float x : noteXes)
                {
                    if (cache[index])
                    {
                        Path& path = (isPlaying && playheadX >= lastX && playheadX < x)
                                         ? playingCells : activeCells;

                        const Rectangle<float> r (lastX, (float) y.getStart(), x - lastX, (float) y.getLength());
                        path.addRoundedRectangle (r.reduced (jlimit (0.5f, indent, r.getWidth()  / 8.0f),
                                                             jlimit (0.5f, indent, r.getHeight() / 8.0f)), 2.0f);
                    }

                    lastX = x;
                    ++index;
                }
            }

            // Add the vertical lines
            for (float x : noteXes)
                grid.addWithoutMerging ({ x - 0.25f, 0.0f, 0.5f, (float) getHeight() });

            // Add the missing right and bottom edges
            {
                auto r = getLocalBounds().toFloat();
                grid.addWithoutMerging (r.removeFromBottom (0.5f).translated (0.0f, -0.25f));
                grid.addWithoutMerging (r.removeFromLeft (0.5f).translated (-0.25f, 0.0f));
            }

            // Calculate playhead rect
            {
                auto r = getLocalBounds().toFloat();
                float lastX = 0.0f;

                for (float x : noteXes)
                {
                    if (playheadX >= lastX && playheadX < x)
                    {
                        playheadRect = r.withX (lastX).withRight (x);
                        break;
                    }

                    lastX = x;
                }
            }

            repaint();
        }
        
        void paint(Graphics &g) override
        {
            g.setColour (Colours::white.withMultipliedAlpha (0.5f));
            g.fillRectList (grid);

            g.setColour (Colours::white.withMultipliedAlpha (0.7f));
            g.fillPath (activeCells);

            g.setColour (Colours::white);
            g.fillPath (playingCells);

            g.setColour (Colours::white.withMultipliedAlpha (0.5f));
            g.fillRect (playheadRect);

            const Range<float> x = getSequenceIndexXRange (mouseOverCellIndex);
            const Range<float> y = editor.getChannelYRange (mouseOverChannel);
            g.setColour (Colours::red);
            g.drawRect (Rectangle<float>::leftTopRightBottom (x.getStart(), y.getStart(), x.getEnd(), y.getEnd()), 2.0f);
        }
        
        void resized() override
        {
            updateNoteXes();
            updatePaths();
        }
        
        void mouseEnter(const MouseEvent& e) override
        {
            updateNoteUnderMouse (e);
        }
        
        void mouseMove (const MouseEvent& e) override
        {
            updateNoteUnderMouse (e);
        }
        
        void mouseDown (const MouseEvent& e) override
        {
            updateNoteUnderMouse (e);

            paintSolidCells = true;

            if (e.mods.isCtrlDown() || e.mods.isCommandDown())
                paintSolidCells = false;
            else if (! e.mods.isShiftDown())
                paintSolidCells = ! editor.clip.getPattern (0).getNote (mouseOverChannel, mouseOverCellIndex);

            setCellAtLastMousePosition (paintSolidCells);
        }
        
        void mouseDrag(const MouseEvent& e) override
        {
            updateNoteUnderMouse (e);
            setCellAtLastMousePosition (paintSolidCells);
        }
        
        void mouseExit(const MouseEvent&) override
        {
            setNoteUnderMouse (-1, -1);
        }
        
    private:
        
        StepEditor& editor;
        Array<float> noteXes;
        RectangleList<float> grid;
        Path activeCells, playingCells;
        Rectangle<float> playheadRect;
        
        int mouseOverCellIndex = -1, mouseOverChannel = -1;
        bool paintSolidCells = true;
        
        void updateNoteXes()
        {
            noteXes.clearQuick();

            auto r = getLocalBounds().toFloat();
            const auto pattern (editor.clip.getPattern (0));
            const int numNotes = pattern.getNumNotes();
            const float w = r.getWidth() / std::max (1, numNotes);

            for (int i = 0; i < numNotes; ++i)
                noteXes.add ((i + 1) * w);
        }
        
        float getPlayheadX() const
        {
            auto clipRange = editor.clip.getEditTimeRange();

            if (clipRange.isEmpty())
                return 0.0f;

            const double position = editor.transport.position;
            const auto proportion = position / clipRange.getEnd();
            auto r = getLocalBounds().toFloat();

            return r.getWidth() * float (proportion);
            
        }
        
        int xToSequenceIndex(float x) const
        {
            if (x >= 0)
                for (int i = 0; i < noteXes.size(); ++i)
                    if (x < noteXes.getUnchecked (i))
                        return i;

            return -1;
            
        }
        Range<float> getSequenceIndexXRange(int index) const
        {
            if (! isPositiveAndBelow (index, noteXes.size()) || noteXes.size() <= 2)
                return {};

            if (index == 0)
                return { 0.0f, noteXes.getUnchecked (index) };

            return { noteXes.getUnchecked (index - 1), noteXes.getUnchecked (index) };
            
        }
        
        bool setNoteUnderMouse (int newIndex, int newChannel)
        {
            if (newIndex != mouseOverCellIndex || newChannel != mouseOverChannel)
            {
                mouseOverCellIndex = newIndex;
                mouseOverChannel = newChannel;
                resized();
                repaint();
                return true;
            }

            return false;
            
        }
        
        bool updateNoteUnderMouse(const MouseEvent& e)
        {
            return setNoteUnderMouse (xToSequenceIndex ((float) e.x), editor.yToChannel ((float) e.y));
        }
        
        void setCellAtLastMousePosition(bool value)
        {
            editor.clip.getPattern (0).setNote (mouseOverChannel, mouseOverCellIndex, value);
        }
        
    };
    
private:
    //----------------------PRIVATE VARIABLES HERE---------------------------
    tracktion_engine::StepClip& clip;
    tracktion_engine::TransportControl& transport;
    tracktion_engine::LambdaTimer timer;
    
    OwnedArray<ChannelConfig> channelConfigs;
    PatternEditor patternEditor {*this};
    
    
    TextButton stepPlayButton {"Play"};
    
    int yToChannel (float y) const
    {
        auto r = getLocalBounds().toFloat();
        const int numChans = clip.getChannels().size();

        if (r.isEmpty())
            return -1;

        return (int) std::floor (y / r.getHeight() * numChans);
    }
    
    Range<float> getChannelYRange(int channelIndex) const
    {
        auto r = getLocalBounds().toFloat();
        const int numChans = clip.getChannels().size();

        if (numChans == 0)
            return {};

        const float h = r.getHeight() / numChans;

        return Range<float>::withStartAndLength (channelIndex * h, h);    }
    
    void selectableObjectChanged(tracktion_engine::Selectable*) override
    {
        // This is our StepClip telling us that one of it's properties has changed
        patternEditor.updatePaths();
    }
    
    void selectableObjectAboutToBeDeleted(tracktion_engine::Selectable*)override
    {
        jassertfalse;
    }
    
};
