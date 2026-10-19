#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

ReverseReverbAudioProcessor::ReverseReverbAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    clearScopeData();
}

ReverseReverbAudioProcessor::~ReverseReverbAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout
ReverseReverbAudioProcessor::createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    p.push_back(std::make_unique<AudioParameterBool>(
        ParameterID{ "oneclick", 1 }, "1-Click Swell", true));
    p.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ "swell", 1 }, "Swell Length",
        NormalisableRange<float>(0.30f, 3.00f, 0.01f), 1.20f,
        AudioParameterFloatAttributes().withLabel("s")));
    p.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ "amount", 1 }, "Amount",
        NormalisableRange<float>(0.0f, 150.0f, 0.1f), 90.0f,
        AudioParameterFloatAttributes().withLabel("%")));
    p.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ "decay", 1 }, "Decay",
        NormalisableRange<float>(0.0f, 100.0f, 0.1f), 70.0f,
        AudioParameterFloatAttributes().withLabel("%")));
    p.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ "tone", 1 }, "Tone",
        NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f,
        AudioParameterFloatAttributes().withLabel("%")));
    p.push_back(std::make_unique<AudioParameterFloat>(
        ParameterID{ "mix", 1 }, "Mix",
        NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        AudioParameterFloatAttributes().withLabel("%")));

    return { std::make_move_iterator(p.begin()), std::make_move_iterator(p.end()) };
}

ReverseReverbParameters ReverseReverbAudioProcessor::readParameters()
{
    ReverseReverbParameters rp;
    rp.enabled      = apvts.getRawParameterValue("oneclick")->load() > 0.5f;
    rp.swellSeconds = apvts.getRawParameterValue("swell")->load();
    rp.amount       = apvts.getRawParameterValue("amount")->load() * 0.01f;
    rp.decay        = apvts.getRawParameterValue("decay")->load() * 0.01f;
    rp.tone         = apvts.getRawParameterValue("tone")->load() * 0.01f;
    rp.mix          = apvts.getRawParameterValue("mix")->load() * 0.01f;
    return rp;
}

void ReverseReverbAudioProcessor::applyPreset(float swell, float amount,
                                              float decay, float tone, float mix)
{
    auto setParam = [this](const char* id, float plainValue)
    {
        if (auto* param = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(id)))
            param->setValueNotifyingHost(param->convertTo0to1(plainValue));
    };
    setParam("swell", swell);   setParam("amount", amount);
    setParam("decay", decay);   setParam("tone", tone);
    setParam("mix", mix);
    if (auto* on = apvts.getParameter("oneclick"))
        on->setValueNotifyingHost(1.0f);
}

void ReverseReverbAudioProcessor::clearScopeData()
{
    for (int i = 0; i < scopeColumnCount; ++i)
    {
        dryScope[(size_t) i].store(0.0f, std::memory_order_relaxed);
        wetScope[(size_t) i].store(0.0f, std::memory_order_relaxed);
    }
    scopeWriteCount.store(0, std::memory_order_relaxed);
    scopeDryAccumulator = 0.0f;
    scopeWetAccumulator = 0.0f;
    scopeSampleCounter = 0;
}

void ReverseReverbAudioProcessor::pushScopeColumn(float dryPeak, float wetPeak)
{
    const uint64_t index = scopeWriteCount.load(std::memory_order_relaxed);
    const size_t slot = (size_t) (index % (uint64_t) scopeColumnCount);
    dryScope[slot].store(dryPeak, std::memory_order_relaxed);
    wetScope[slot].store(wetPeak, std::memory_order_relaxed);
    scopeWriteCount.store(index + 1, std::memory_order_release);
}

void ReverseReverbAudioProcessor::copyScopeData(float* dryDest, float* wetDest,
                                                  int numColumns) const
{
    if (dryDest == nullptr || wetDest == nullptr || numColumns <= 0)
        return;

    for (int i = 0; i < numColumns; ++i) { dryDest[i] = 0.0f; wetDest[i] = 0.0f; }

    const uint64_t written = scopeWriteCount.load(std::memory_order_acquire);
    const int available = (int) juce::jmin<uint64_t>(written,
                                                      (uint64_t) juce::jmin(numColumns, scopeColumnCount));
    const int offset = numColumns - available;

    for (int i = 0; i < available; ++i)
    {
        const uint64_t sequence = written - (uint64_t) available + (uint64_t) i;
        const size_t slot = (size_t) (sequence % (uint64_t) scopeColumnCount);
        dryDest[offset + i] = dryScope[slot].load(std::memory_order_relaxed);
        wetDest[offset + i] = wetScope[slot].load(std::memory_order_relaxed);
    }
}

void ReverseReverbAudioProcessor::prepareToPlay(double sampleRate, int)
{
    engine.prepare(sampleRate);
    engine.setParameters(readParameters());
    lastLatency = engine.getLatencySamples();
    setLatencySamples(lastLatency);

    scopeSamplesPerColumn = juce::jmax(32, (int) std::lround(sampleRate / (double) scopeColumnsPerSecond));
    clearScopeData();
    inputPeak.store(0.0f, std::memory_order_relaxed);
    outputPeak.store(0.0f, std::memory_order_relaxed);
}

void ReverseReverbAudioProcessor::releaseResources() {}

bool ReverseReverbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void ReverseReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    engine.setParameters(readParameters());
    const int latency = engine.getLatencySamples();
    if (latency != lastLatency)
    {
        lastLatency = latency;
        setLatencySamples(latency); // REAPER re-syncs its PDC automatically
    }

    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    float* left = buffer.getWritePointer(0);
    float* right = numCh > 1 ? buffer.getWritePointer(1) : nullptr;

    float blockInputPeak = 0.0f;
    float blockOutputPeak = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = left[i];
        const float inR = right != nullptr ? right[i] : inL;
        blockInputPeak = juce::jmax(blockInputPeak, std::abs(inL), std::abs(inR));

        float outL = 0.0f, outR = 0.0f;
        engine.processSample(inL, inR, outL, outR);
        left[i] = outL;
        if (right != nullptr) right[i] = outR;
        blockOutputPeak = juce::jmax(blockOutputPeak, std::abs(outL), std::abs(outR));

        // Reduce to fixed-rate peak columns for the swell waveform display.
        scopeDryAccumulator = juce::jmax(scopeDryAccumulator, engine.getLastDryComponentPeak());
        scopeWetAccumulator = juce::jmax(scopeWetAccumulator, engine.getLastWetComponentPeak());
        if (++scopeSampleCounter >= scopeSamplesPerColumn)
        {
            pushScopeColumn(scopeDryAccumulator, scopeWetAccumulator);
            scopeDryAccumulator = 0.0f;
            scopeWetAccumulator = 0.0f;
            scopeSampleCounter = 0;
        }
    }

    if (blockInputPeak > inputPeak.load(std::memory_order_relaxed))
        inputPeak.store(blockInputPeak, std::memory_order_relaxed);
    if (blockOutputPeak > outputPeak.load(std::memory_order_relaxed))
        outputPeak.store(blockOutputPeak, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* ReverseReverbAudioProcessor::createEditor()
{
    return new ReverseReverbAudioProcessorEditor(*this);
}

void ReverseReverbAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void ReverseReverbAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReverseReverbAudioProcessor();
}
