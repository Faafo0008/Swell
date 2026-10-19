#pragma once
#include <JuceHeader.h>
#include <vector>
#include "PluginProcessor.h"

//==============================================================================
/** Custom professional styling: machined rotary knobs with tick rings and
    value arcs, gradient buttons with a lit state for the 1-click switch. */
class ReverseReverbLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ReverseReverbLookAndFeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;

    void drawButtonText(juce::Graphics&, juce::TextButton&,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;
};

//==============================================================================
/** Live swell waveform display.

    Draws seconds of history as DAW-style symmetric peak waveforms:
      violet filled shape = wet reverse-reverb contribution
      pale cyan waveform  = delayed dry contribution
    Because both come from separate post-gain taps inside the engine and are
    sample-aligned, you can literally watch the violet swell crescendo into
    each pale dry transient. The visible time window scales with the Swell
    Length parameter so a full swell always fits on screen. */
class SwellWaveformDisplay : public juce::Component
{
public:
    explicit SwellWaveformDisplay(ReverseReverbAudioProcessor& processor);

    void refresh();               // copy latest scope data + repaint (GUI thread)
    void paint(juce::Graphics&) override;

private:
    ReverseReverbAudioProcessor& processorRef;
    std::vector<float> dryPeaks;
    std::vector<float> wetPeaks;
    int numColumns = 0;
    float visibleSeconds = 2.4f;
    bool swellEnabled = true;
};

//==============================================================================
/** Compact segmented input/output peak meters with dB readouts. */
class LevelMeterPanel : public juce::Component
{
public:
    void updateLevels(float newInputPeak, float newOutputPeak);
    void paint(juce::Graphics&) override;

private:
    static float peakToDb(float peak);
    float inputLevel = 0.0f, outputLevel = 0.0f;
    float inputHold = 0.0f, outputHold = 0.0f;
    int inputHoldTicks = 0, outputHoldTicks = 0;
};

//==============================================================================
class ReverseReverbAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit ReverseReverbAudioProcessorEditor(ReverseReverbAudioProcessor&);
    ~ReverseReverbAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& text);
    void updateEngineStateLabel();

    ReverseReverbAudioProcessor& processorRef;
    ReverseReverbLookAndFeel lookAndFeel;

    SwellWaveformDisplay waveformDisplay;
    LevelMeterPanel meterPanel;

    juce::TextButton oneClickButton { "1-CLICK REVERSE SWELL" };
    juce::Label engineStateLabel;
    juce::TextButton vocalButton  { "Vocal" };
    juce::TextButton snareButton  { "Snare Pop" };
    juce::TextButton guitarButton { "Guitar Wash" };
    juce::TextButton padButton    { "Synth Pad" };

    juce::Slider swellSlider, amountSlider, decaySlider, toneSlider, mixSlider;
    juce::Label  swellLabel, amountLabel, decayLabel, toneLabel, mixLabel;

    juce::Rectangle<int> headerBounds, waveformBounds, sideBounds,
                         controlsBounds, footerBounds;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<ButtonAttachment> oneClickAttachment;
    std::unique_ptr<SliderAttachment> swellAttachment, amountAttachment,
                                      decayAttachment, toneAttachment, mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverseReverbAudioProcessorEditor)
};
