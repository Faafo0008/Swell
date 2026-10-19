#pragma once
// FreeverbTank.h — Schroeder/Freeverb-style reverb tank (mono)
// Public-domain style algorithm (Jezar / Freeverb topology), re-implemented here.
// Used INSIDE the reverse-reverb engine: audio is reversed, run through this
// tank, then reversed back — which is what makes the reverb swell *before*
// the dry sound instead of decaying after it.

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

class FreeverbTank
{
public:
    void prepare(double sampleRate, int channelIndex)
    {
        sr = sampleRate > 0.0 ? sampleRate : 48000.0;
        const double scale = sr / 44100.0;
        const int spread = (channelIndex == 0) ? 0 : 23;

        static const int combBase[8]   = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        static const int allpassBase[4] = { 556, 441, 341, 225 };

        for (int i = 0; i < 8; ++i)
        {
            int len = std::max(16, (int) std::lround((combBase[i] + spread) * scale));
            combBuf[i].assign((size_t) len, 0.0f);
            combIdx[i] = 0;
            combStore[i] = 0.0f;
        }
        for (int i = 0; i < 4; ++i)
        {
            int len = std::max(16, (int) std::lround((allpassBase[i] + spread) * scale));
            allpassBuf[i].assign((size_t) len, 0.0f);
            allpassIdx[i] = 0;
        }
    }

    // feedback 0.50..0.98 (decay), damping 0.05..0.95 (higher = darker)
    void setParameters(float feedbackIn, float dampingIn)
    {
        feedback = std::clamp(feedbackIn, 0.0f, 0.98f);
        damping  = std::clamp(dampingIn, 0.0f, 0.98f);
    }

    void reset()
    {
        for (auto& b : combBuf)    std::fill(b.begin(), b.end(), 0.0f);
        for (auto& b : allpassBuf) std::fill(b.begin(), b.end(), 0.0f);
        for (float& s : combStore) s = 0.0f;
        for (int& i : combIdx)     i = 0;
        for (int& i : allpassIdx)  i = 0;
    }

    float process(float input)
    {
        // Freeverb input scaling keeps the 8 parallel combs stable/sane.
        const float in = input * 0.015f * 12.0f; // *12 compensates so musical levels stay musical
        float out = 0.0f;

        for (int i = 0; i < 8; ++i)
        {
            auto& buf = combBuf[i];
            int idx = combIdx[i];
            float bufOut = buf[(size_t) idx];
            combStore[i] = bufOut * (1.0f - damping) + combStore[i] * damping;
            buf[(size_t) idx] = in + combStore[i] * feedback;
            combIdx[i] = (idx + 1 >= (int) buf.size()) ? 0 : idx + 1;
            out += bufOut;
        }

        for (int i = 0; i < 4; ++i)
        {
            auto& buf = allpassBuf[i];
            int idx = allpassIdx[i];
            float bufOut = buf[(size_t) idx];
            float v = out + bufOut * 0.5f;
            buf[(size_t) idx] = v;
            allpassIdx[i] = (idx + 1 >= (int) buf.size()) ? 0 : idx + 1;
            out = bufOut - out * 0.5f;
        }

        // out is the sum of 8 combs pushed through allpasses; scale to a sane wet level.
        return out * 0.22f;
    }

private:
    double sr = 48000.0;
    float feedback = 0.84f;
    float damping  = 0.45f;
    std::vector<float> combBuf[8];
    std::vector<float> allpassBuf[4];
    float combStore[8] = {};
    int combIdx[8] = {};
    int allpassIdx[4] = {};
};
