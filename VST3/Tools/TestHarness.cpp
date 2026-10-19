// TestHarness.cpp — renders the demo WAVs using the ACTUAL plugin DSP
// (ReverseReverbEngine.h) and prints proof that energy arrives BEFORE each note.
//
// Build:  g++ -O2 -std=c++17 -I../Source/DSP TestHarness.cpp -o testharness
// Run:    ./testharness demo_dry.wav demo_reverse_reverb.wav [scope_peaks.csv]
// The optional CSV contains the same 240-columns/sec dry/wet peak taps the
// plugin GUI's swell waveform display consumes (used for the GUI preview).

#include <cstdio>
#include <cstdint>
#include <cmath>
#include <vector>
#include <cstring>
#include "ReverseReverbEngine.h"

static void writeWav16(const char* path, const std::vector<float>& L,
                       const std::vector<float>& R, int sr)
{
    FILE* f = fopen(path, "wb");
    if (!f) { printf("cannot write %s\n", path); return; }
    const int n = (int) L.size();
    const int dataBytes = n * 2 * 2;
    auto u32 = [&](uint32_t v){ fwrite(&v, 4, 1, f); };
    auto u16 = [&](uint16_t v){ fwrite(&v, 2, 1, f); };
    fwrite("RIFF", 1, 4, f); u32(36 + dataBytes); fwrite("WAVEfmt ", 1, 8, f);
    u32(16); u16(1); u16(2); u32(sr); u32(sr * 4); u16(4); u16(16);
    fwrite("data", 1, 4, f); u32(dataBytes);
    for (int i = 0; i < n; ++i)
        for (int ch = 0; ch < 2; ++ch)
        {
            float x = std::clamp(ch == 0 ? L[i] : R[i], -1.0f, 1.0f);
            int16_t s = (int16_t) std::lround(x * 32767.0f);
            fwrite(&s, 2, 1, f);
        }
    fclose(f);
    printf("wrote %s\n", path);
}

static double rms(const std::vector<float>& v, int a, int b)
{
    double s = 0; int c = 0;
    for (int i = std::max(0, a); i < std::min(b, (int) v.size()); ++i) { s += (double)v[i]*v[i]; ++c; }
    return c ? std::sqrt(s / c) : 0.0;
}

int main(int argc, char** argv)
{
    const int sr = 48000;
    const double dur = 10.0;
    const int N = (int)(dur * sr);
    std::vector<float> inL(N, 0.0f), inR(N, 0.0f);

    // Two clean "pluck/vocal-like" phrases with silence before them,
    // so a PRE-swell is easy to hear and measure. Events (input time): 2.0s and 5.5s.
    auto addPluck = [&](double t0, double freq, double len, float amp)
    {
        int a = (int)(t0 * sr), b = std::min(N, a + (int)(len * sr));
        for (int i = a; i < b; ++i)
        {
            double t = (i - a) / (double) sr;
            double env = std::exp(-3.0 * t) * (1.0 - std::exp(-220.0 * t));
            double v = amp * env * (std::sin(6.2831853*freq*t)
                                  + 0.35*std::sin(6.2831853*freq*2.0*t)
                                  + 0.15*std::sin(6.2831853*freq*3.01*t));
            inL[i] += (float)v; inR[i] += (float)(v * 0.92);
        }
    };
    addPluck(2.0, 220.0, 1.2, 0.70f);
    addPluck(2.7, 277.0, 1.0, 0.50f);
    addPluck(5.5, 330.0, 1.2, 0.70f);
    addPluck(6.2, 440.0, 1.0, 0.50f);

    ReverseReverbEngine eng;
    eng.prepare(sr);
    ReverseReverbParameters p; // defaults = Vocal preset
    p.enabled = true; p.swellSeconds = 1.2f; p.amount = 0.90f;
    p.decay = 0.70f;  p.tone = 0.60f;        p.mix = 0.50f;
    eng.setParameters(p);
    const int lat = eng.getLatencySamples();
    printf("latency = %d samples (%.2f sec) -> dry notes appear in output at 3.20s and 6.70s\n",
           lat, lat / (double) sr);

    std::vector<float> outL(N, 0.0f), outR(N, 0.0f);
    std::vector<float> scopeDry, scopeWet;
    {
        const int samplesPerColumn = sr / 240; // matches the GUI scope rate
        float dryAcc = 0.0f, wetAcc = 0.0f;
        int counter = 0;
        for (int i = 0; i < N; ++i)
        {
            eng.processSample(inL[i], inR[i], outL[i], outR[i]);
            dryAcc = std::max(dryAcc, eng.getLastDryComponentPeak());
            wetAcc = std::max(wetAcc, eng.getLastWetComponentPeak());
            if (++counter >= samplesPerColumn)
            {
                scopeDry.push_back(dryAcc);
                scopeWet.push_back(wetAcc);
                dryAcc = 0.0f; wetAcc = 0.0f; counter = 0;
            }
        }
    }

    // Dry demo = same input delayed by the same latency, so A/B is time-aligned.
    std::vector<float> dryL(N, 0.0f), dryR(N, 0.0f);
    for (int i = lat; i < N; ++i) { dryL[i] = inL[i-lat] * 0.9f; dryR[i] = inR[i-lat] * 0.9f; }

    float peak = 0; for (float v : outL) peak = std::max(peak, std::abs(v));
    printf("output peak = %.3f\n", peak);

    // PROOF: RMS in the 1 second BEFORE the first (delayed) note, dry vs processed.
    const double noteOut = 2.0 + lat / (double) sr;
    printf("\nRMS before first note (note itself at %.2fs):\n", noteOut);
    for (double t = noteOut - 1.2; t < noteOut + 0.4; t += 0.2)
    {
        int a = (int)(t * sr), b = (int)((t + 0.2) * sr);
        printf("  t=%4.1fs  dry=%.5f   processed=%.5f %s\n", t, rms(dryL, a, b), rms(outL, a, b),
               (t + 0.2 <= noteOut && rms(outL,a,b) > 0.005) ? " <-- PRE-SWELL (dry is still silent)" : "");
    }
    double preDry = rms(dryL, (int)((noteOut-1.0)*sr), (int)(noteOut*sr));
    double preWet = rms(outL, (int)((noteOut-1.0)*sr), (int)(noteOut*sr));
    printf("\nRESULT: 1s before note: dry RMS=%.6f, processed RMS=%.6f -> %s\n",
           preDry, preWet, preWet > 0.01 && preDry < 0.001 ? "PASS: swell precedes the note"
                                                             : "CHECK: swell weak/absent");

    const char* dryPath = argc > 1 ? argv[1] : "demo_dry.wav";
    const char* wetPath = argc > 2 ? argv[2] : "demo_reverse_reverb.wav";
    writeWav16(dryPath, dryL, dryR, sr);
    writeWav16(wetPath, outL, outR, sr);

    if (argc > 3)
    {
        if (FILE* f = fopen(argv[3], "w"))
        {
            fprintf(f, "time_s,dry_peak,wet_peak\n");
            for (size_t i = 0; i < scopeDry.size(); ++i)
                fprintf(f, "%.4f,%.6f,%.6f\n", (double) i / 240.0, scopeDry[i], scopeWet[i]);
            fclose(f);
            printf("wrote %s (%zu scope columns)\n", argv[3], scopeDry.size());
        }
    }
    return 0;
}
