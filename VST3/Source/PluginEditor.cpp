#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace
{
juce::Colour ink(uint32_t rgb) { return juce::Colour(rgb); }

juce::String withCommas(int value)
{
    auto digits = juce::String(std::abs(value));
    juce::String result;
    int count = 0;
    for (int i = digits.length() - 1; i >= 0; --i)
    {
        result = juce::String::charToString(digits[i]) + result;
        if (++count == 3 && i > 0)
        {
            result = "," + result;
            count = 0;
        }
    }
    return value < 0 ? "-" + result : result;
}

void drawPanel(juce::Graphics& g, juce::Rectangle<int> bounds,
               juce::Colour top, juce::Colour bottom, float corner = 12.0f)
{
    if (bounds.isEmpty())
        return;
    auto area = bounds.toFloat();
    g.setGradientFill(juce::ColourGradient(top, area.getTopLeft(),
                                           bottom, area.getBottomRight(), false));
    g.fillRoundedRectangle(area, corner);
    g.setColour(juce::Colours::white.withAlpha(0.10f));
    g.drawRoundedRectangle(area.reduced(0.5f), corner, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.045f));
    g.fillRoundedRectangle(area.removeFromTop(1.0f).reduced(10.0f, 0.0f), 1.0f);
}

void drawSectionTitle(juce::Graphics& g, const juce::String& text, int x, int y)
{
    g.setColour(juce::Colours::white.withAlpha(0.92f));
    g.setFont(juce::Font(11.5f).boldened());
    g.drawText(text, x, y, 240, 16, juce::Justification::centredLeft);
    g.setColour(ink(0xff7a4dff));
    g.fillRoundedRectangle((float) x, (float) (y + 18), 28.0f, 2.0f, 1.0f);
}

void drawChip(juce::Graphics& g, juce::Rectangle<int> bounds,
              const juce::String& label, const juce::String& value)
{
    auto area = bounds.toFloat();
    g.setGradientFill(juce::ColourGradient(ink(0xff211a3d), area.getTopLeft(),
                                           ink(0xff141026), area.getBottomLeft(), false));
    g.fillRoundedRectangle(area, 7.0f);
    g.setColour(juce::Colours::white.withAlpha(0.11f));
    g.drawRoundedRectangle(area.reduced(0.5f), 7.0f, 1.0f);

    g.setColour(ink(0xffa89fd6));
    g.setFont(juce::Font(8.5f).boldened());
    g.drawText(label, bounds.getX() + 9, bounds.getY() + 4, bounds.getWidth() - 18, 11,
               juce::Justification::centredLeft);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(13.0f).boldened());
    g.drawText(value, bounds.getX() + 9, bounds.getY() + 15, bounds.getWidth() - 18, 16,
               juce::Justification::centredLeft);
}

void drawScrew(juce::Graphics& g, float cx, float cy)
{
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillEllipse(cx - 4.5f, cy - 3.5f, 9.0f, 9.0f);
    g.setGradientFill(juce::ColourGradient(ink(0xff6b6389), cx - 4.0f, cy - 4.0f,
                                           ink(0xff241d3d), cx + 4.0f, cy + 4.0f, false));
    g.fillEllipse(cx - 4.0f, cy - 4.0f, 8.0f, 8.0f);
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.drawLine(cx - 2.6f, cy + 2.6f, cx + 2.6f, cy - 2.6f, 1.4f);
}
} // namespace

//==============================================================================
// LookAndFeel
//==============================================================================
ReverseReverbLookAndFeel::ReverseReverbLookAndFeel()
{
    setColour(juce::Slider::rotarySliderFillColourId, ink(0xff9a6bff));
    setColour(juce::Slider::rotarySliderOutlineColourId, ink(0xff292241));
    setColour(juce::Slider::thumbColourId, juce::Colours::white);
    setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour(juce::Slider::textBoxBackgroundColourId, ink(0xff0d0a1c));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::white.withAlpha(0.10f));
    setColour(juce::Label::textColourId, ink(0xffc9c2ff));
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.88f));
}

void ReverseReverbLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y,
                                                int width, int height,
                                                float sliderPosProportional,
                                                float rotaryStartAngle,
                                                float rotaryEndAngle,
                                                juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
    auto centre = bounds.getCentre();
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    if (radius <= 6.0f)
        return;

    const float toAngle = rotaryStartAngle
                        + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Tick ring (11 ticks, lit up to the current value).
    for (int i = 0; i <= 10; ++i)
    {
        const float frac = (float) i / 10.0f;
        const float a = rotaryStartAngle + frac * (rotaryEndAngle - rotaryStartAngle)
                      - juce::MathConstants<float>::halfPi;
        auto p1 = centre.getPointOnCircumference(radius - 1.0f, a);
        auto p2 = centre.getPointOnCircumference(radius - (i % 5 == 0 ? 7.0f : 5.0f), a);
        g.setColour(frac <= sliderPosProportional
                        ? ink(0xffb79cff).withAlpha(0.95f)
                        : juce::Colours::white.withAlpha(0.16f));
        g.drawLine(juce::Line<float>(p1, p2), 1.4f);
    }

    // Value arc around the knob body.
    const float arcRadius = radius - 10.0f;
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centre.getX(), centre.getY(), arcRadius, arcRadius,
                                0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(slider.findColour(juce::Slider::rotarySliderOutlineColourId));
    g.strokePath(backgroundArc, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc(centre.getX(), centre.getY(), arcRadius, arcRadius,
                               0.0f, rotaryStartAngle, toAngle, true);
        g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
        g.strokePath(valueArc, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // Knob body: shadow, machined gradient disc, top highlight, pointer.
    const float bodyRadius = radius - 16.0f;
    auto body = juce::Rectangle<float>(bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre(centre);
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillEllipse(body.translated(0.0f, 2.5f));

    g.setGradientFill(juce::ColourGradient(ink(0xff4a4172),
                                           centre.getX() - bodyRadius * 0.55f,
                                           centre.getY() - bodyRadius * 0.65f,
                                           ink(0xff17122a),
                                           centre.getX() + bodyRadius * 0.45f,
                                           centre.getY() + bodyRadius * 0.75f, true));
    g.fillEllipse(body);
    g.setColour(juce::Colours::white.withAlpha(0.14f));
    g.drawEllipse(body.reduced(0.5f), 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.07f));
    g.fillEllipse(body.reduced(bodyRadius * 0.32f).withCentre(
        { centre.getX() - bodyRadius * 0.18f, centre.getY() - bodyRadius * 0.24f }));

    const float pointerAngle = toAngle - juce::MathConstants<float>::halfPi;
    auto pointerEnd = centre.getPointOnCircumference(bodyRadius - 5.0f, pointerAngle);
    auto pointerStart = centre.getPointOnCircumference(bodyRadius * 0.30f, pointerAngle);
    g.setColour(juce::Colours::white);
    g.drawLine(juce::Line<float>(pointerStart, pointerEnd), 3.0f);
    g.fillEllipse(juce::Rectangle<float>(7.0f, 7.0f).withCentre(pointerEnd));
    g.setColour(ink(0xff241d3d));
    g.fillEllipse(juce::Rectangle<float>(8.0f, 8.0f).withCentre(centre));
}

void ReverseReverbLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                    const juce::Colour& backgroundColour,
                                                    bool highlighted, bool down)
{
    auto area = button.getLocalBounds().toFloat().reduced(0.5f);
    const bool isMain = button.getName() == "oneClickButton";
    const float corner = isMain ? 11.0f : 7.0f;

    auto base = backgroundColour;
    if (down)
        base = base.darker(0.22f);
    else if (highlighted)
        base = base.brighter(0.14f);

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillRoundedRectangle(area.translated(0.0f, 2.0f), corner);

    g.setGradientFill(juce::ColourGradient(base.brighter(0.18f), area.getTopLeft(),
                                           base.darker(0.18f), area.getBottomLeft(), false));
    g.fillRoundedRectangle(area, corner);

    const bool on = button.getToggleState();
    g.setColour(on && isMain ? ink(0xffc4b5fd).withAlpha(0.85f)
                             : juce::Colours::white.withAlpha(highlighted ? 0.28f : 0.13f));
    g.drawRoundedRectangle(area, corner, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.10f));
    g.fillRoundedRectangle(area.withHeight(2.0f).reduced(9.0f, 0.0f), 1.0f);

    if (isMain)
    {
        // Status LED at the left edge of the big switch.
        auto led = juce::Rectangle<float>(10.0f, 10.0f)
                       .withCentre({ area.getX() + 17.0f, area.getCentreY() });
        if (on)
        {
            g.setColour(ink(0xff5eead4).withAlpha(0.28f));
            g.fillEllipse(led.expanded(6.0f));
            g.setColour(ink(0xff5eead4));
        }
        else
        {
            g.setColour(juce::Colours::white.withAlpha(0.22f));
        }
        g.fillEllipse(led);
    }

    if (button.hasKeyboardFocus(true))
    {
        g.setColour(ink(0xff67e8f9).withAlpha(0.65f));
        g.drawRoundedRectangle(area.expanded(1.5f), corner + 1.0f, 1.2f);
    }
}

juce::Font ReverseReverbLookAndFeel::getTextButtonFont(juce::TextButton& button, int buttonHeight)
{
    const bool isMain = button.getName() == "oneClickButton";
    const float size = isMain ? juce::jmin(16.0f, buttonHeight * 0.34f)
                              : juce::jmin(13.5f, buttonHeight * 0.42f);
    return juce::Font(size).boldened();
}

void ReverseReverbLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                              bool highlighted, bool down)
{
    auto font = getTextButtonFont(button, button.getHeight());
    g.setFont(font);
    auto colour = button.findColour(button.getToggleState()
                                        ? juce::TextButton::textColourOnId
                                        : juce::TextButton::textColourOffId);
    if (! button.isEnabled())
        colour = colour.withAlpha(0.45f);
    else if (highlighted)
        colour = colour.brighter(0.08f);
    g.setColour(colour);

    auto area = button.getLocalBounds();
    if (button.getName() == "oneClickButton")
        area.removeFromLeft(24); // keep the text clear of the status LED
    if (down)
        area.translate(0, 1);

    g.drawFittedText(button.getButtonText(), area, juce::Justification::centred, 1);
}

//==============================================================================
// Swell waveform display
//==============================================================================
SwellWaveformDisplay::SwellWaveformDisplay(ReverseReverbAudioProcessor& processor)
    : processorRef(processor),
      dryPeaks((size_t) ReverseReverbAudioProcessor::scopeColumnCount, 0.0f),
      wetPeaks((size_t) ReverseReverbAudioProcessor::scopeColumnCount, 0.0f)
{
    refresh();
}

void SwellWaveformDisplay::refresh()
{
    float swell = 1.2f;
    if (auto* p = processorRef.apvts.getRawParameterValue("swell"))
        swell = p->load();
    if (auto* p = processorRef.apvts.getRawParameterValue("oneclick"))
        swellEnabled = p->load() > 0.5f;

    visibleSeconds = juce::jlimit(1.2f, 4.25f, swell * 1.55f + 0.55f);
    numColumns = juce::jlimit(16, ReverseReverbAudioProcessor::scopeColumnCount,
                              (int) std::lround(visibleSeconds
                                  * ReverseReverbAudioProcessor::scopeColumnsPerSecond));
    processorRef.copyScopeData(dryPeaks.data(), wetPeaks.data(), numColumns);
    repaint();
}

void SwellWaveformDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    drawPanel(g, bounds, ink(0xff100c24), ink(0xff090716));

    // Header: title, legend, window length.
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(12.5f).boldened());
    g.drawText("SWELL WAVEFORM", 16, 10, 180, 18, juce::Justification::centredLeft);

    const int legendY = 17;
    const int wetLegendX = juce::jmax(196, bounds.getWidth() - 318);
    g.setColour(ink(0xffa78bfa));
    g.fillRoundedRectangle((float) wetLegendX, (float) legendY, 20.0f, 4.0f, 2.0f);
    g.setColour(ink(0xffc9c2ff));
    g.setFont(juce::Font(10.5f));
    g.drawText("Wet swell", wetLegendX + 26, 10, 62, 18, juce::Justification::centredLeft);
    g.setColour(ink(0xffdff8ff));
    g.fillRoundedRectangle((float) (wetLegendX + 96), (float) legendY, 20.0f, 4.0f, 2.0f);
    g.setColour(ink(0xffc9c2ff));
    g.drawText("Delayed dry", wetLegendX + 122, 10, 78, 18, juce::Justification::centredLeft);
    g.drawText(juce::String(visibleSeconds, 2) + " s window",
               bounds.getWidth() - 112, 10, 96, 18, juce::Justification::centredRight);

    // Plot area.
    auto plot = juce::Rectangle<float>(42.0f, 40.0f,
                                       (float) bounds.getWidth() - 58.0f,
                                       (float) bounds.getHeight() - 82.0f);
    g.setColour(juce::Colours::black.withAlpha(0.42f));
    g.fillRoundedRectangle(plot, 7.0f);
    g.setColour(juce::Colours::white.withAlpha(0.09f));
    g.drawRoundedRectangle(plot, 7.0f, 1.0f);

    const float centreY = plot.getCentreY();
    const float halfH = plot.getHeight() * 0.5f - 5.0f;

    // Amplitude grid + dB labels.
    struct GridLine { float amp; const char* label; };
    const GridLine grid[] = { { 1.0f, "0" }, { 0.5f, "-6" }, { 0.25f, "-12" }, { 0.0f, "dB" } };
    g.setFont(juce::Font(9.5f));
    for (auto line : grid)
    {
        const float yTop = centreY - line.amp * halfH;
        const float yBot = centreY + line.amp * halfH;
        g.setColour(juce::Colours::white.withAlpha(line.amp == 0.0f ? 0.16f : 0.055f));
        g.drawHorizontalLine((int) yTop, plot.getX(), plot.getRight());
        if (line.amp > 0.0f)
            g.drawHorizontalLine((int) yBot, plot.getX(), plot.getRight());
        g.setColour(ink(0xff8f86bd));
        g.drawText(line.label, 2, (int) yTop - 7, 36, 14, juce::Justification::centredRight);
    }

    // Time grid + labels.
    const float stepSeconds = visibleSeconds > 3.1f ? 1.0f : 0.5f;
    g.setFont(juce::Font(9.5f));
    for (float ago = std::floor(visibleSeconds / stepSeconds) * stepSeconds;
         ago >= -0.001f; ago -= stepSeconds)
    {
        const float x = plot.getRight() - (ago / visibleSeconds) * plot.getWidth();
        if (x < plot.getX() - 1.0f)
            continue;
        g.setColour(juce::Colours::white.withAlpha(ago < 0.001f ? 0.28f : 0.05f));
        g.drawVerticalLine((int) x, plot.getY(), plot.getBottom());
        g.setColour(ink(0xff8f86bd));
        auto text = ago < 0.001f ? juce::String("NOW")
                                 : "-" + juce::String(ago, stepSeconds < 1.0f ? 1 : 0) + "s";
        g.drawText(text, (int) x - 24, (int) plot.getBottom() + 4, 48, 14,
                   juce::Justification::centred);
    }

    // Aggregate peak columns to display pixels.
    const int pixels = juce::jmax(1, (int) plot.getWidth());
    std::vector<float> wetPix((size_t) pixels, 0.0f), dryPix((size_t) pixels, 0.0f);
    float overallPeak = 0.0f;
    if (numColumns > 0)
    {
        for (int px = 0; px < pixels; ++px)
        {
            const int start = (px * numColumns) / pixels;
            const int end = juce::jmax(start + 1, ((px + 1) * numColumns) / pixels);
            float w = 0.0f, d = 0.0f;
            for (int c = start; c < end && c < numColumns; ++c)
            {
                w = juce::jmax(w, wetPeaks[(size_t) c]);
                d = juce::jmax(d, dryPeaks[(size_t) c]);
            }
            wetPix[(size_t) px] = juce::jlimit(0.0f, 1.05f, w);
            dryPix[(size_t) px] = juce::jlimit(0.0f, 1.05f, d);
            overallPeak = juce::jmax(overallPeak, w, d);
        }
    }

    g.saveState();
    g.reduceClipRegion(plot.toNearestInt());

    // Wet swell: filled symmetric silhouette.
    if (pixels > 1)
    {
        juce::Path wetPath;
        wetPath.startNewSubPath(plot.getX(), centreY - wetPix[0] * halfH);
        for (int px = 1; px < pixels; ++px)
            wetPath.lineTo(plot.getX() + (float) px, centreY - wetPix[(size_t) px] * halfH);
        for (int px = pixels - 1; px >= 0; --px)
            wetPath.lineTo(plot.getX() + (float) px, centreY + wetPix[(size_t) px] * halfH);
        wetPath.closeSubPath();

        g.setGradientFill(juce::ColourGradient(ink(0xff7a4dff).withAlpha(0.50f), plot.getX(), centreY,
                                               ink(0xff4c1d95).withAlpha(0.22f), plot.getRight(), centreY, false));
        g.fillPath(wetPath);
        g.setColour(ink(0xffc4b5fd).withAlpha(0.72f));
        g.strokePath(wetPath, juce::PathStrokeType(1.0f));

        // Delayed dry: crisp pale waveform over the swell.
        g.setColour(ink(0xffdff8ff).withAlpha(0.92f));
        for (int px = 0; px < pixels; ++px)
        {
            const float amp = dryPix[(size_t) px] * halfH;
            if (amp > 0.6f)
                g.drawVerticalLine((int) (plot.getX() + (float) px),
                                   centreY - amp, centreY + amp);
        }
    }

    // Centre + "now" lines.
    g.setColour(juce::Colours::white.withAlpha(0.20f));
    g.drawHorizontalLine((int) centreY, plot.getX(), plot.getRight());
    g.setColour(ink(0xff67e8f9).withAlpha(0.55f));
    g.drawVerticalLine((int) plot.getRight() - 1, plot.getY(), plot.getBottom());

    // Reading guide inside the plot.
    g.setColour(juce::Colours::white.withAlpha(0.42f));
    g.setFont(juce::Font(10.0f));
    g.drawText("swell rises  >  dry hit", (int) plot.getX() + 8,
               (int) plot.getBottom() - 19, 150, 14, juce::Justification::centredLeft);
    g.restoreState();

    if (overallPeak < 0.002f)
    {
        g.setColour(juce::Colours::white.withAlpha(0.52f));
        g.setFont(juce::Font(13.5f));
        g.drawText("Waiting for audio - press play in REAPER",
                   plot.toNearestInt(), juce::Justification::centred);
    }

    if (! swellEnabled)
    {
        g.setColour(juce::Colours::black.withAlpha(0.48f));
        g.fillRoundedRectangle(plot, 7.0f);
        g.setColour(juce::Colours::white.withAlpha(0.88f));
        g.setFont(juce::Font(14.0f).boldened());
        g.drawText("SWELL BYPASSED", plot.toNearestInt().withTrimmedBottom(18),
                   juce::Justification::centred);
        g.setFont(juce::Font(11.0f));
        g.setColour(juce::Colours::white.withAlpha(0.62f));
        g.drawText("latency-matched delayed dry only",
                   plot.toNearestInt().withTrimmedTop(18), juce::Justification::centred);
    }
}

//==============================================================================
// Level meters
//==============================================================================
float LevelMeterPanel::peakToDb(float peak)
{
    return juce::jmax(-60.0f, 20.0f * std::log10(juce::jmax(peak, 0.00001f)));
}

void LevelMeterPanel::updateLevels(float newInputPeak, float newOutputPeak)
{
    auto ballistics = [](float& level, float& hold, int& holdTicks, float newPeak)
    {
        level = juce::jmax(newPeak, level * 0.78f);
        if (newPeak >= hold)
        {
            hold = newPeak;
            holdTicks = 24;
        }
        else if (holdTicks > 0)
        {
            --holdTicks;
        }
        else
        {
            hold *= 0.92f;
        }
    };
    ballistics(inputLevel, inputHold, inputHoldTicks, newInputPeak);
    ballistics(outputLevel, outputHold, outputHoldTicks, newOutputPeak);
    repaint();
}

void LevelMeterPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    const int rowH = juce::jmax(16, (bounds.getHeight() - 8) / 2);
    struct Row { const char* label; float level; float hold; };
    const Row rows[] = { { "IN", inputLevel, inputHold }, { "OUT", outputLevel, outputHold } };

    for (int r = 0; r < 2; ++r)
    {
        const int y = r * (rowH + 8);
        g.setColour(ink(0xffa89fd6));
        g.setFont(juce::Font(10.0f).boldened());
        g.drawText(rows[r].label, 0, y, 30, rowH, juce::Justification::centredLeft);

        auto track = juce::Rectangle<float>(34.0f, (float) y + rowH * 0.5f - 5.0f,
                                            (float) bounds.getWidth() - 94.0f, 10.0f);
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.fillRoundedRectangle(track, 3.0f);

        auto toNorm = [](float peak)
        {
            return juce::jlimit(0.0f, 1.0f, (peakToDb(peak) + 42.0f) / 42.0f);
        };
        const int segments = juce::jmax(8, (int) (track.getWidth() / 8.0f));
        const float activeFrac = toNorm(rows[r].level);
        for (int s = 0; s < segments; ++s)
        {
            const float frac = (float) s / (float) segments;
            auto seg = juce::Rectangle<float>(track.getX() + frac * track.getWidth() + 1.0f,
                                              track.getY() + 1.5f,
                                              track.getWidth() / (float) segments - 2.5f, 7.0f);
            if (frac < activeFrac)
                g.setColour(frac < 0.68f ? ink(0xff34d399)
                                         : (frac < 0.86f ? ink(0xfffbbf24) : ink(0xfff87171)));
            else
                g.setColour(juce::Colours::white.withAlpha(0.07f));
            g.fillRoundedRectangle(seg, 1.5f);
        }

        // Peak-hold marker.
        const float holdX = track.getX() + toNorm(rows[r].hold) * track.getWidth();
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.fillRect(holdX - 1.0f, track.getY() - 2.0f, 2.0f, track.getHeight() + 4.0f);

        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.setFont(juce::Font(10.5f));
        auto dbText = rows[r].level <= 0.0001f ? juce::String("-inf dB")
                                               : juce::String(peakToDb(rows[r].level), 1) + " dB";
        g.drawText(dbText, bounds.getWidth() - 56, y, 56, rowH,
                   juce::Justification::centredRight);
    }
}

//==============================================================================
// Editor
//==============================================================================
ReverseReverbAudioProcessorEditor::ReverseReverbAudioProcessorEditor(
    ReverseReverbAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p), waveformDisplay(p)
{
    setLookAndFeel(&lookAndFeel);
    setResizable(true, true);
    setResizeLimits(900, 630, 1240, 820);
    setSize(940, 630);

    addAndMakeVisible(waveformDisplay);
    addAndMakeVisible(meterPanel);

    oneClickButton.setName("oneClickButton");
    oneClickButton.setClickingTogglesState(true);
    oneClickButton.setColour(juce::TextButton::buttonOnColourId, ink(0xff7a4dff));
    oneClickButton.setColour(juce::TextButton::buttonColourId, ink(0xff2a2340));
    oneClickButton.setTooltip("Turn the classic lookahead reverse-reverb swell on or off. "
                              "When off, you hear latency-matched dry so timing never jumps.");
    addAndMakeVisible(oneClickButton);
    oneClickAttachment = std::make_unique<ButtonAttachment>(
        processorRef.apvts, "oneclick", oneClickButton);

    engineStateLabel.setJustificationType(juce::Justification::centred);
    engineStateLabel.setFont(juce::Font(11.0f));
    addAndMakeVisible(engineStateLabel);

    // Preset values: (Swell s, Amount %, Decay %, Tone %, Mix %)
    vocalButton.onClick  = [this] { processorRef.applyPreset(1.2f, 90.0f, 70.0f, 60.0f, 50.0f); };
    snareButton.onClick  = [this] { processorRef.applyPreset(0.45f, 110.0f, 55.0f, 70.0f, 45.0f); };
    guitarButton.onClick = [this] { processorRef.applyPreset(1.8f, 85.0f, 78.0f, 50.0f, 55.0f); };
    padButton.onClick    = [this] { processorRef.applyPreset(2.6f, 95.0f, 88.0f, 45.0f, 65.0f); };
    for (auto* b : { &vocalButton, &snareButton, &guitarButton, &padButton })
    {
        b->setColour(juce::TextButton::buttonColourId, ink(0xff241d3d));
        b->setTooltip("Load this preset and enable the swell.");
        addAndMakeVisible(b);
    }

    setupKnob(swellSlider,  swellLabel,  "Swell Length");
    setupKnob(amountSlider, amountLabel, "Amount");
    setupKnob(decaySlider,  decayLabel,  "Decay");
    setupKnob(toneSlider,   toneLabel,   "Tone");
    setupKnob(mixSlider,    mixLabel,    "Mix");
    swellSlider.setTooltip("How far before each sound the swell begins. This is also the lookahead latency reported to REAPER.");
    amountSlider.setTooltip("Level of the reversed reverb swell before the Mix control.");
    decaySlider.setTooltip("Size and density of the reverb inside the swell.");
    toneSlider.setTooltip("Swell brightness: left is dark and smoky, right is bright and airy.");
    mixSlider.setTooltip("Equal-power dry/wet blend. Use 100% when the plugin is on a send/return track.");

    swellAttachment  = std::make_unique<SliderAttachment>(processorRef.apvts, "swell",  swellSlider);
    amountAttachment = std::make_unique<SliderAttachment>(processorRef.apvts, "amount", amountSlider);
    decayAttachment  = std::make_unique<SliderAttachment>(processorRef.apvts, "decay",  decaySlider);
    toneAttachment   = std::make_unique<SliderAttachment>(processorRef.apvts, "tone",   toneSlider);
    mixAttachment    = std::make_unique<SliderAttachment>(processorRef.apvts, "mix",    mixSlider);

    updateEngineStateLabel();
    waveformDisplay.refresh();
    startTimerHz(30);
}

ReverseReverbAudioProcessorEditor::~ReverseReverbAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void ReverseReverbAudioProcessorEditor::setupKnob(juce::Slider& slider, juce::Label& label,
                                                  const juce::String& text)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setColour(juce::Slider::rotarySliderFillColourId, ink(0xff9a6bff));
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, ink(0xff292241));
    slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, ink(0xff0d0a1c));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::white.withAlpha(0.10f));
    addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, ink(0xffc9c2ff));
    label.setFont(juce::Font(12.5f).boldened());
    addAndMakeVisible(label);
}

void ReverseReverbAudioProcessorEditor::updateEngineStateLabel()
{
    bool active = true;
    if (auto* p = processorRef.apvts.getRawParameterValue("oneclick"))
        active = p->load() > 0.5f;

    const juce::String text = active ? "ACTIVE  -  lookahead swell engaged"
                                     : "BYPASSED  -  latency-matched dry";
    if (engineStateLabel.getText() != text)
    {
        engineStateLabel.setText(text, juce::dontSendNotification);
        engineStateLabel.setColour(juce::Label::textColourId,
                                   active ? ink(0xff5eead4) : ink(0xffa89fd6));
    }
}

void ReverseReverbAudioProcessorEditor::timerCallback()
{
    waveformDisplay.refresh();
    meterPanel.updateLevels(processorRef.getAndResetInputPeak(),
                            processorRef.getAndResetOutputPeak());
    updateEngineStateLabel();
    repaint(headerBounds); // keep lookahead / PDC / sample-rate chips live
}

void ReverseReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto full = getLocalBounds().toFloat();
    g.setGradientFill(juce::ColourGradient(ink(0xff0a0718), full.getTopLeft(),
                                           ink(0xff1c1136), full.getBottomRight(), false));
    g.fillAll();

    // Soft purple glow behind the header + faint machining texture.
    g.setGradientFill(juce::ColourGradient(ink(0xff7a4dff).withAlpha(0.20f),
                                           full.getCentreX(), -80.0f,
                                           juce::Colours::transparentBlack,
                                           full.getCentreX(), 260.0f, true));
    g.fillEllipse(full.getCentreX() - 420.0f, -230.0f, 840.0f, 420.0f);
    g.setColour(juce::Colours::white.withAlpha(0.018f));
    for (int x = 24; x < getWidth(); x += 28)
        g.drawVerticalLine(x, 0.0f, (float) getHeight());

    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawRect(full.reduced(0.5f), 1.0f);
    drawScrew(g, 13.0f, 13.0f);
    drawScrew(g, (float) getWidth() - 13.0f, 13.0f);
    drawScrew(g, 13.0f, (float) getHeight() - 13.0f);
    drawScrew(g, (float) getWidth() - 13.0f, (float) getHeight() - 13.0f);

    // ---- Header -------------------------------------------------------------
    if (! headerBounds.isEmpty())
    {
        auto logo = juce::Rectangle<float>((float) headerBounds.getX(),
                                           (float) headerBounds.getY() + 8.0f, 46.0f, 46.0f);
        g.setGradientFill(juce::ColourGradient(ink(0xff8b5cf6), logo.getTopLeft(),
                                               ink(0xff2563eb), logo.getBottomRight(), false));
        g.fillRoundedRectangle(logo, 11.0f);
        g.setColour(juce::Colours::white.withAlpha(0.35f));
        g.drawRoundedRectangle(logo, 11.0f, 1.0f);

        // Logo glyph: a tiny swell crescendo into a transient spike.
        juce::Path glyph;
        const float gy = logo.getCentreY();
        glyph.startNewSubPath(logo.getX() + 7.0f, gy);
        glyph.lineTo(logo.getX() + 12.0f, gy - 3.0f);
        glyph.lineTo(logo.getX() + 17.0f, gy + 5.0f);
        glyph.lineTo(logo.getX() + 22.0f, gy - 9.0f);
        glyph.lineTo(logo.getX() + 27.0f, gy + 4.0f);
        glyph.lineTo(logo.getX() + 31.0f, gy - 15.0f);
        glyph.lineTo(logo.getX() + 34.0f, gy + 13.0f);
        glyph.lineTo(logo.getX() + 39.0f, gy);
        g.setColour(juce::Colours::white);
        g.strokePath(glyph, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        const int titleX = headerBounds.getX() + 60;
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(24.0f).boldened());
        g.drawText("REVERSE REVERB", titleX, headerBounds.getY() + 6, 330, 30,
                   juce::Justification::centredLeft);
        g.setColour(ink(0xffa89fd6));
        g.setFont(juce::Font(11.5f).boldened());
        g.drawText("ONE-CLICK SWELL  -  CLASSIC LOOKAHEAD REVERSE REVERB",
                   titleX + 1, headerBounds.getY() + 37, 430, 16,
                   juce::Justification::centredLeft);

        // Live status chips (repainted by the timer).
        const int latency = juce::jmax(0, processorRef.getLatencySamplesForDisplay());
        const double sr = processorRef.getSampleRate();
        const float latencySeconds = sr > 0.0 ? (float) (latency / sr) : 0.0f;
        const int chipY = headerBounds.getY() + 13;
        int chipX = headerBounds.getRight();
        auto chip = [&](int w, const juce::String& label, const juce::String& value)
        {
            chipX -= w;
            drawChip(g, { chipX, chipY, w, 36 }, label, value);
            chipX -= 8;
        };
        chip(96, "SAMPLE RATE", sr > 0.0 ? juce::String(sr / 1000.0, 1) + " kHz" : "-- kHz");
        chip(128, "PDC LATENCY", withCommas(latency) + " smp");
        chip(118, "LOOKAHEAD", juce::String(latencySeconds, 2) + " s");

        g.setColour(juce::Colours::white.withAlpha(0.09f));
        g.drawHorizontalLine(headerBounds.getBottom(), (float) headerBounds.getX(),
                             (float) headerBounds.getRight());
    }

    // ---- Side + control panels ----------------------------------------------
    drawPanel(g, sideBounds, ink(0xff17112e), ink(0xff100b22));
    drawPanel(g, controlsBounds, ink(0xff17112e), ink(0xff0f0a20));

    if (! sideBounds.isEmpty())
    {
        drawSectionTitle(g, "SWELL ENGINE", sideBounds.getX() + 16, sideBounds.getY() + 12);
        drawSectionTitle(g, "PRESETS", sideBounds.getX() + 16, vocalButton.getY() - 26);
        drawSectionTitle(g, "LEVELS", sideBounds.getX() + 16, meterPanel.getY() - 26);
    }

    if (! controlsBounds.isEmpty())
    {
        drawSectionTitle(g, "SWELL SHAPING", controlsBounds.getX() + 16,
                         controlsBounds.getY() + 12);
        g.setColour(ink(0xff8f86bd));
        g.setFont(juce::Font(10.5f));
        g.drawText("All parameters are host-automatable",
                   controlsBounds.getRight() - 256, controlsBounds.getY() + 13, 240, 16,
                   juce::Justification::centredRight);
    }

    // ---- Footer --------------------------------------------------------------
    if (! footerBounds.isEmpty())
    {
        g.setColour(ink(0xff9d94c4));
        g.setFont(juce::Font(11.0f));
        g.drawText("Tip: for vocals, run this on a send at Mix 100% and ride the return so swells bloom into phrase starts.",
                   footerBounds.getX(), footerBounds.getY(), footerBounds.getWidth() - 230,
                   footerBounds.getHeight(), juce::Justification::centredLeft);
        g.setColour(ink(0xff6f6894));
        g.drawText("Reverse Reverb One-Click  v1.1  -  Lookahead / PDC",
                   footerBounds.getRight() - 300, footerBounds.getY(), 300,
                   footerBounds.getHeight(), juce::Justification::centredRight);
    }
}

void ReverseReverbAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(18);

    headerBounds = area.removeFromTop(62);
    area.removeFromTop(10);
    footerBounds = area.removeFromBottom(22);
    area.removeFromBottom(8);
    controlsBounds = area.removeFromBottom(180);
    area.removeFromBottom(14);

    auto mainArea = area;
    const int sideW = juce::jlimit(264, 312, (int) (mainArea.getWidth() * 0.315));
    sideBounds = mainArea.removeFromRight(sideW);
    mainArea.removeFromRight(14);
    waveformBounds = mainArea;
    waveformDisplay.setBounds(waveformBounds);

    // Side panel contents.
    auto side = sideBounds.reduced(16, 0);
    oneClickButton.setBounds(side.getX(), sideBounds.getY() + 38, side.getWidth(), 56);
    engineStateLabel.setBounds(side.getX(), oneClickButton.getBottom() + 4, side.getWidth(), 18);

    const int gridTop = sideBounds.getY() + 140;
    const int presetW = (side.getWidth() - 8) / 2;
    vocalButton.setBounds(side.getX(), gridTop, presetW, 32);
    snareButton.setBounds(side.getX() + presetW + 8, gridTop, presetW, 32);
    guitarButton.setBounds(side.getX(), gridTop + 40, presetW, 32);
    padButton.setBounds(side.getX() + presetW + 8, gridTop + 40, presetW, 32);

    const int meterTop = gridTop + 72 + 30;
    meterPanel.setBounds(side.getX(), meterTop, side.getWidth(),
                         juce::jmax(36, sideBounds.getBottom() - 14 - meterTop));

    // Knob row.
    auto knobArea = controlsBounds.reduced(12, 0);
    const int cellW = knobArea.getWidth() / 5;
    struct Pair { juce::Slider* s; juce::Label* l; };
    int i = 0;
    for (auto pr : { Pair{ &swellSlider, &swellLabel }, Pair{ &amountSlider, &amountLabel },
                     Pair{ &decaySlider, &decayLabel }, Pair{ &toneSlider, &toneLabel },
                     Pair{ &mixSlider, &mixLabel } })
    {
        const int cx = knobArea.getX() + i * cellW;
        pr.l->setBounds(cx + 4, controlsBounds.getY() + 38, cellW - 8, 18);
        pr.s->setBounds(cx + 6, controlsBounds.getY() + 58, cellW - 12, 108);
        ++i;
    }
}
