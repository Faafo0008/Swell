#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cstdint>
#include "DSP/ReverseReverbEngine.h"

class ReverseReverbAudioProcessor : public juce::AudioProcessor
{
public:
    ReverseReverbAudioProcessor();
    ~ReverseReverbAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    void applyPreset(float swell, float amount, float decay, float tone, float mix);

    // ---- Editor visualisation data -------------------------------------
    // The audio thread reduces the signal to peak "columns" at a fixed
    // 240 columns/second (independent of host block size), separately for
    // the delayed-dry contribution and the wet-swell contribution.
    // 1024 columns ~= 4.27 seconds of history, which is what lets the
    // waveform display show a swell rising *before* its dry transient.
    static constexpr int scopeColumnCount = 1024;
    static constexpr int scopeColumnsPerSecond = 240;

    // Fills dryDest/wetDest (numColumns each, oldest -> newest, right-aligned
    // and zero-padded until enough audio has been captured). GUI-thread safe.
    void copyScopeData(float* dryDest, float* wetDest, int numColumns) const;

    float getAndResetInputPeak()  { return inputPeak.exchange(0.0f, std::memory_order_relaxed); }
    float getAndResetOutputPeak() { return outputPeak.exchange(0.0f, std::memory_order_relaxed); }
    int getLatencySamplesForDisplay() const { return lastLatency; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    ReverseReverbParameters readParameters();
    void pushScopeColumn(float dryPeak, float wetPeak);
    void clearScopeData();

    ReverseReverbEngine engine;
    int lastLatency = -1;

    mutable std::array<std::atomic<float>, scopeColumnCount> dryScope;
    mutable std::array<std::atomic<float>, scopeColumnCount> wetScope;
    mutable std::atomic<uint64_t> scopeWriteCount { 0 };
    std::atomic<float> inputPeak { 0.0f };
    std::atomic<float> outputPeak { 0.0f };

    // Audio-thread-only scope reduction state.
    float scopeDryAccumulator = 0.0f;
    float scopeWetAccumulator = 0.0f;
    int scopeSampleCounter = 0;
    int scopeSamplesPerColumn = 200;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverseReverbAudioProcessor)
};
