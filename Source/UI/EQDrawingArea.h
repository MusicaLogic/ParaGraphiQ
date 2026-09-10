/*
  ==============================================================================

    EQDrawingArea.h
    Created: 3 Sep 2026 7:25:02am
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Style/VisualStyle.h"
#include "EQBand.h"

class EQDrawingArea : public juce::Component
{
public:

    enum class InteractionMode
    {
        drawing,
        bandSelected
    };

    EQDrawingArea();
    ~EQDrawingArea() override = default;

    // ============================================================
    // JUCE
    // ============================================================

    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;


    // ============================================================
    // EQ state
    // ============================================================

    void setEQState(const EQState& newState);
    const EQState& getEQState() const noexcept { return state; }
    
    void setGains(const std::array<float, 31>& newGains);
    float getBandGain(int band) const noexcept;


    // ============================================================
    // Interaction mode
    // ============================================================

    void setInteractionMode(InteractionMode newMode);
    InteractionMode getInteractionMode() const noexcept
    {
        return interactionMode;
    }


    // ============================================================
    // Selected band
    // ============================================================

    int getSelectedBand() const noexcept
    {
        return selectedBand;
    }
    
    void setSelectedBand(int newSelectedBand) noexcept;
    
    // ============================================================
    // Change notification
    // ============================================================

    std::function<void(const EQState&)> onEQChanged;
    std::function<void(int)> onSelectedBandChanged;
    std::function<void()> onDrawingStarted;

private:

    // ============================================================
    // Drawing
    // ============================================================

    void drawFrame(juce::Graphics& g);
    void drawPlot(juce::Graphics& g);
    void drawPlotFill(juce::Graphics& g);
    void drawSelectedBandLine(juce::Graphics& g);
    void drawAxisGuides(juce::Graphics& g);
    
    static constexpr int numBands = 31;

//    std::array<float, numBands> gains {};


    // ============================================================
    // Coordinate conversion
    // ============================================================

    float frequencyToX(double frequency) const;
    double xToFrequency(float x) const;

    float gainToY(float gain) const;
    float yToGain(float y) const;


    // ============================================================
    // Interaction
    // ============================================================

    int findClosestBand(float x) const;
    void updateFromMousePosition(const juce::Point<float>& position);
    void notifyEQChanged();

    // ============================================================
    // Geometry
    // ============================================================

    juce::Rectangle<float> getPlotBounds() const;


    // ============================================================
    // State
    // ============================================================

    EQState state;
    InteractionMode interactionMode = InteractionMode::drawing;
    int selectedBand = -1;
    
    // Band currently under the mouse cursor.
    // Used for the subtle vertical guide during drawing.
    int hoveredBand = -1;


    // ============================================================
    // EQ display parameters
    // ============================================================

    static constexpr float minGain = -12.0f;
    static constexpr float maxGain =  12.0f;


    // Padding inside component
    static constexpr float plotPadding = 10.0f;


    // Appearance
    const VisualStyle::ColorSet& colourSet = VisualStyle::Palette::green;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EQDrawingArea)
};
