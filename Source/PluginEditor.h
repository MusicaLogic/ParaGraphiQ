/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/EQDrawingArea.h"
#include "UI/TouchBand.h"
#include "UI/BandSelector.h"

//==============================================================================
/**
*/
class PGQ_VSTAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                        public juce::ChangeListener
{
public:
    static constexpr int numBands = 31;
    
    PGQ_VSTAudioProcessorEditor (PGQ_VSTAudioProcessor&);
    ~PGQ_VSTAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    void setInteractionMode(
        EQDrawingArea::InteractionMode newMode);

    EQDrawingArea::InteractionMode getInteractionMode() const noexcept
    {
        return interactionMode;
    }

    const std::array<float, numBands>& getGains() const noexcept
    {
        return gains;
    }
    
    // listen to processor for loading saved states
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PGQ_VSTAudioProcessor& audioProcessor;
    
    //==============================================================
    // UI components
    //==============================================================

    EQDrawingArea eqDrawingArea;

    TouchBand smoothnessBand {
        TouchBand::Direction::UpOnly
    };

    TouchBand gainBand {
        TouchBand::Direction::UpDown
    };

    BandSelector bandSelector;


    //==============================================================
    // EQ state
    //==============================================================

    int selectedBand = 15;

    std::array<float, numBands> gains {};
    std::array<float, numBands> gainsAtGestureStart {};
    float gainAtGestureStart = 0.0f;
    
    EQDrawingArea::InteractionMode interactionMode =
            EQDrawingArea::InteractionMode::drawing;
    
    // UI communication
    void setupCallbacks();

    void setSelectedBand(int band);

    //==============================================================
    // Interaction
    //==============================================================

    void handleSmoothnessGesture(float amount);
    void applySmoothness(
        std::array<float, numBands>& targetGains,
        int centreBand,
        float amount);

    void handleGainGesture(float amount);

    // DSP bridge
    void updateDSP();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PGQ_VSTAudioProcessorEditor)
};
