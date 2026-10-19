#pragma once
// ReverseReverbEngine.h — Classic (tape-style) reverse reverb, in real time.
//
// THE CLASSIC STUDIO TRICK:
//   1) Reverse the track  2) Print a big 100%-wet reverb  3) Reverse it back
//   => the reverb now swells *into* each note/word instead of trailing after it.
//
// HOW THIS ENGINE DOES IT LIVE (with lookahead latency):
//   * Incoming audio is captured in overlapping blocks of length B = "Swell Length".
//   * Each block is:  reversed -> Freeverb tank -> reversed back.
//   * The result is overlap-added (50% overlap, periodic Hann window) into a wet
//     output ring, sample-aligned with the dry signal delayed by exactly B samples.
//   * The host is told about B samples of latency (REAPER compensates via PDC),
//     so in your mix everything stays in time — you just hear the swell arrive
//     BEFORE the vocal/note, exactly like the old tape trick.
//   * Because every moment is covered by two blocks (in one of which it sits in
//     the second half, giving it a long pre-swell), swells are smooth and continuous.
//
// This header has no JUCE dependency so it can be unit-tested standalone
// (see VST3/Tools/TestHarness.cpp — the demo WAVs were rendered by this exact code).

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include "FreeverbTank.h"

struct ReverseReverbParameters
{
    bool  enabled      = true;   // the 1-CLICK switch
    float swellSeconds = 1.2f;   // 0.30 .. 3.00  (also = latency)
    float amount       = 0.80f;  // 0..1  wet swell strength before the Mix law
    float decay        = 0.65f;  // 0..1  reverb decay / room size
    float tone         = 0.60f;  // 0..1  0 = dark, 1 = bright
    float mix          = 0.40f;  // 0..1  equal-power dry/wet blend
};

class ReverseReverbEngine
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 48000.0;
        maxBlock = (int) std::ceil(3.10 * sr) + 64;

        ringSize = 1;
        while (ringSize < maxBlock * 4) ringSize <<= 1;
        ringMask = ringSize - 1;

        capture[0].assign((size_t) ringSize, 0.0f);
        capture[1].assign((size_t) ringSize, 0.0f);
        wetRing[0].assign((size_t) ringSize, 0.0f);
        wetRing[1].assign((size_t) ringSize, 0.0f);
        workX.assign((size_t) maxBlock, 0.0f);
        workYr.assign((size_t) maxBlock, 0.0f);

        tank[0].prepare(sr, 0);
        tank[1].prepare(sr, 1);

        params = ReverseReverbParameters{};
        blockLen = 0; // force a (re)configure on first setParameters
        applyBlockSize(params.swellSeconds, true);
    }

    void setParameters(const ReverseReverbParameters& p)
    {
        params = p;
        if (std::abs(p.swellSeconds - configuredSwell) > 0.01f)
            applyBlockSize(p.swellSeconds, true);
        updateGains();
    }

    int getLatencySamples() const { return blockLen; }

    // Metering/scope taps for the editor's swell waveform display.
    // These are the per-sample *contributions* after the Amount/Mix gains,
    // as a stereo peak (max of |L|,|R|), so the GUI can draw the wet swell
    // and the delayed dry as separate, correctly-aligned waveforms.
    float getLastDryComponentPeak() const { return lastDryComponentPeak; }
    float getLastWetComponentPeak() const { return lastWetComponentPeak; }

    void reset()
    {
        for (auto& c : capture) std::fill(c.begin(), c.end(), 0.0f);
        for (auto& w : wetRing) std::fill(w.begin(), w.end(), 0.0f);
        tank[0].reset(); tank[1].reset();
        sampleCount = 0;
        lastDryComponentPeak = 0.0f;
        lastWetComponentPeak = 0.0f;
    }

    // Process one stereo sample. Output is delayed by getLatencySamples().
    void processSample(float inL, float inR, float& outL, float& outR)
    {
        const int64_t n = sampleCount;
        const int writePos = (int) (n & ringMask);
        capture[0][(size_t) writePos] = inL;
        capture[1][(size_t) writePos] = inR;

        const int64_t dryIndex = n - blockLen;
        float dryL = 0.0f, dryR = 0.0f;
        if (dryIndex >= 0)
        {
            const int rp = (int) (dryIndex & ringMask);
            dryL = capture[0][(size_t) rp];
            dryR = capture[1][(size_t) rp];
        }

        float wetL = wetRing[0][(size_t) writePos];
        float wetR = wetRing[1][(size_t) writePos];
        wetRing[0][(size_t) writePos] = 0.0f;
        wetRing[1][(size_t) writePos] = 0.0f;

        float dryCompL = dryL, dryCompR = dryR;
        float wetCompL = 0.0f, wetCompR = 0.0f;
        if (params.enabled)
        {
            dryCompL = dryL * dryGain;
            dryCompR = dryR * dryGain;
            wetCompL = wetL * wetGain;
            wetCompR = wetR * wetGain;
            outL = dryCompL + wetCompL;
            outR = dryCompR + wetCompR;
        }
        else
        {
            outL = dryL; // stay latency-matched so toggling never shifts timing
            outR = dryR;
        }
        lastDryComponentPeak = std::max(std::abs(dryCompL), std::abs(dryCompR));
        lastWetComponentPeak = std::max(std::abs(wetCompL), std::abs(wetCompR));
        outL = softSafety(outL);
        outR = softSafety(outR);

        // A block ends at sample n every `hop` samples once we have B samples.
        const int64_t sinceStart = (n + 1) - blockLen;
        if (sinceStart >= 0 && (sinceStart % hopLen) == 0)
            processBlockEndingAt(n);

        ++sampleCount;
    }

private:
    void applyBlockSize(float swellSec, bool doReset)
    {
        configuredSwell = swellSec;
        int B = (int) std::lround(std::clamp(swellSec, 0.30f, 3.00f) * sr);
        B = std::clamp(B, 1024, maxBlock - 64);
        if (B & 1) ++B; // keep even so hop = B/2 is exact
        blockLen = B;
        hopLen = std::max(256, B / 2);
        if (doReset) reset();
        updateGains();
    }

    void updateGains()
    {
        const float mix = std::clamp(params.mix, 0.0f, 1.0f);
        const float amt = std::clamp(params.amount, 0.0f, 1.5f);
        dryGain = std::cos(mix * 1.5707963f);
        // *2.2 compensates the Hann overlap-add + tank scaling so Amount feels strong/predictable
        wetGain = std::sin(mix * 1.5707963f) * amt * 2.2f;
    }

    static float softSafety(float x)
    {
        // Transparent below ~0.95, then a gentle knee so huge swells can't clip harshly.
        if (x >  0.95f) return  0.95f + 0.25f * std::tanh((x - 0.95f) * 4.0f);
        if (x < -0.95f) return -0.95f - 0.25f * std::tanh((-x - 0.95f) * 4.0f);
        return x;
    }

    void processBlockEndingAt(int64_t endIndex)
    {
        const int B = blockLen;
        const int64_t startIndex = endIndex - B + 1; // capture block = [start .. end]
        const int64_t outStart   = endIndex + 1;     // aligns with delayed dry (latency = B)

        const float feedback = 0.55f + std::clamp(params.decay, 0.0f, 1.0f) * 0.42f; // 0.55..0.97
        const float damping  = 0.85f - std::clamp(params.tone,  0.0f, 1.0f) * 0.72f;  // bright = low damping

        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < B; ++i)
                workX[(size_t) i] = capture[ch][(size_t) ((startIndex + i) & ringMask)];

            tank[ch].reset(); // fresh tank per block = faithful to the offline tape trick
            tank[ch].setParameters(feedback, damping);

            // reverse -> reverb
            for (int i = 0; i < B; ++i)
                workYr[(size_t) i] = tank[ch].process(workX[(size_t) (B - 1 - i)]);

            // reverse back, window (periodic Hann), overlap-add aligned with delayed dry
            for (int i = 0; i < B; ++i)
            {
                const float y = workYr[(size_t) (B - 1 - i)];
                const float w = 0.5f - 0.5f * std::cos(6.2831853f * (float) i / (float) B);
                wetRing[ch][(size_t) ((outStart + i) & ringMask)] += y * w;
            }
        }
    }

    double sr = 48000.0;
    ReverseReverbParameters params;
    float configuredSwell = -1.0f;
    int maxBlock = 0;
    int blockLen = 0;
    int hopLen = 256;
    int ringSize = 0;
    int ringMask = 0;
    float dryGain = 1.0f, wetGain = 0.0f;
    float lastDryComponentPeak = 0.0f;
    float lastWetComponentPeak = 0.0f;
    int64_t sampleCount = 0;

    std::vector<float> capture[2];
    std::vector<float> wetRing[2];
    std::vector<float> workX, workYr;
    FreeverbTank tank[2];
};
