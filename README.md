# Reverse Reverb — 1-Click Swell

**The classic studio reverse-reverb trick, as a single plugin with a single button.**

Instead of reversing your audio, printing a reverb, reversing it back, trimming
and lining it all up by hand, you just drop this on a track and press play.
The reverb now *swells into* every note, word or drum hit — that haunting
"whoosh before the vocal" you hear on everything from shoegaze to modern pop.

Built for **REAPER on Windows 64-bit**, in two flavours with **identical DSP**
(the C++ engine in `VST3/Source/DSP/` is the single source of truth;
the JSFX is a line-by-line port of it):

| Version | Use it when… | Where |
|---|---|---|
| **JSFX** (REAPER-native) | You want it working **in the next 60 seconds**, no compiling | `JSFX/` |
| **VST3** (JUCE source) | You want it in any DAW, with the professional GUI: live swell waveform, presets, meters + custom knobs | `VST3/` |

Demo audio rendered by the actual DSP is in `Demo/` — listen to
`demo_reverse_reverb.wav` vs `demo_dry.wav` and hear the swell arrive *before*
each note (measurement proof in `Demo/TEST_RESULTS.txt`). The GUI preview in
`Preview/GUI_VST3_Preview.png` is drawn from the editor code, and its waveform
uses real dry/wet peak taps exported by the test harness to
`Demo/scope_peaks.csv` — the same data the plug-in display consumes live.

---

## Quick start (JSFX — recommended first)

1. REAPER → **Options → Show REAPER resource path in explorer/finder**
2. Copy `JSFX/ReverseReverb_OneClick.jsfx` into the **Effects** folder there
3. In REAPER's FX browser search **Reverse Reverb** (press F5 if needed), add it
4. Leave **1-CLICK SWELL = ON** (or click the big purple button), press play ✅

Full details: `JSFX/INSTALL.txt`.

## Quick start (VST3)

A `.vst3` binary must be built on Windows, so pick one:

* **No installs:** upload this folder to GitHub — the included workflow builds
  the Windows `.vst3` in the cloud for free, you download it from the
  Actions tab. **Or**
* **On your PC:** install Visual Studio 2022 (free, C++ workload) + CMake,
  then double-click `VST3/build_windows.bat`.

Then copy `Reverse Reverb One-Click.vst3` to `C:\Program Files\Common Files\VST3\`
and rescan in REAPER. Step-by-step: `VST3/README_BUILD_WINDOWS.md`.

---

## Controls (kept deliberately simple)

| Control | What it does | Start here |
|---|---|---|
| **1-CLICK SWELL** | The whole point. On = instant reverse-reverb with sensible defaults | ON |
| **Swell Length** | How long before the sound the swell starts (= lookahead) | Vocals 0.8–1.6 s · Snare 0.3–0.6 s · Pads 2–3 s |
| **Amount** | Strength/level of the reversed swell | 90 % |
| **Decay** | Reverb size/density inside the swell | 70 % |
| **Tone** | Dark ↔ bright swell | 60 % |
| **Mix** | Dry/wet blend (equal-power). 100 % = swell only | 50 % insert · 100 % on a send |

**VST3 presets (also 1-click):** *Vocal* (1.2 s, balanced) · *Snare Pop*
(0.45 s, bright, poky) · *Guitar Wash* (1.8 s, dark, wide) · *Synth Pad*
(2.6 s, deep, washy).

**Best-practice routing for vocals:** put it on its own track ("Swell") at
Mix 100 %, send the vocal to it, and ride the Swell fader so the effect only
blooms in gaps and phrase starts. To print it permanently, freeze/render that track.

### VST3 interface (v1.1)

* **Swell waveform display** — a scrolling, seconds-wide peak display fed by
  separate post-gain taps inside the engine at 240 columns/second. The
  **violet shape is the wet reverse-reverb**, the **pale waveform is the
  delayed dry**, so you can watch each swell crescendo into its transient.
  The time window auto-scales with Swell Length (~1.2–4.3 s), with dB and
  time grids, a "now" marker, a waiting-for-audio hint, and a clear
  bypassed overlay when the swell is off.
* **Header status chips** — live lookahead time, PDC latency in samples, and
  host sample rate, so the cause of the (intentional, compensated) latency
  is always visible.
* **Swell engine panel** — the big lit 1-click switch with status LED and
  engaged/bypassed readout, 2×2 preset grid, and segmented **IN/OUT peak
  meters** with peak-hold markers and dB readouts.
* **Custom knobs** — machined-look rotary controls with tick rings, value
  arcs and value boxes; hover any control for a tooltip. The editor is
  resizable (900×630 – 1240×820, default 940×630).

## How it works

The old tape trick was: reverse the tape → record a 100 %-wet reverb → reverse
back, so every reverb tail runs *backwards into* its source sound. This plugin
does exactly that continuously, in real time:

1. Audio is captured in overlapping blocks (block length = Swell Length, 50 % overlap).
2. Each block is **reversed → Freeverb-style tank (8 combs + 4 allpasses) → reversed back**.
3. Blocks are Hann-windowed and overlap-added, sample-aligned with the dry
   signal delayed by exactly one block.
4. That block of latency is reported to the host, so REAPER's plug-in delay
   compensation keeps your whole mix in time — you only *hear* the future
   arriving early, which is precisely the effect.

Verified by measurement (`Demo/TEST_RESULTS.txt`): in the second before a note
the dry signal is silent (RMS 0.000000) while the processed signal crescendos
0.001 → 0.008 → 0.061 → 0.158 into the note. **Pass: the swell precedes the sound.**

## Good to know / troubleshooting

* **Don't monitor live input through it.** It must see the future, so it adds
  Swell-Length latency by design. It's a mixing effect; disable it while recording the track.
* **Timing feels off?** Make sure REAPER's PDC is on (it is by default). Changing
  Swell Length re-reports latency and REAPER re-syncs automatically; the audio
  briefly re-buffers while you drag — normal for lookahead effects.
* **CPU:** the block math happens twice per swell, so it's light — roughly a
  small reverb. REAPER's anticipative FX processing smooths it further on playback.
* **Swell too washy on busy parts?** Shorten Swell Length, darken Tone, or use
  the send-track method and automate the swell fader down during dense phrases.
* **Nothing before the very first sound of the project?** The first block needs
  to fill once; leave a beat of run-in, as you would with any lookahead effect.

## Files

```
ReverseReverb-OneClick/
├─ README.md                     ← you are here
├─ JSFX/
│  ├─ ReverseReverb_OneClick.jsfx   ← copy this into REAPER's Effects folder
│  └─ INSTALL.txt
├─ VST3/
│  ├─ CMakeLists.txt             ← JUCE project (JUCE auto-downloaded on first build)
│  ├─ build_windows.bat          ← double-click Windows builder
│  ├─ README_BUILD_WINDOWS.md
│  ├─ Source/
│  │  ├─ PluginProcessor.*       ← host/audio glue + scope/meter taps for the GUI
│  │  ├─ PluginEditor.*          ← professional GUI: swell waveform, meters, presets, custom knobs
│  │  └─ DSP/
│  │     ├─ ReverseReverbEngine.h   ← the reverse-reverb algorithm (JUCE-free, tested)
│  │     └─ FreeverbTank.h
│  └─ Tools/TestHarness.cpp      ← renders Demo/ WAVs + scope_peaks.csv with the real DSP
├─ Preview/  (GUI_VST3_Preview.png, GUI_JSFX_Preview.png, make_gui_preview.py)
├─ Demo/  (demo_dry.wav, demo_reverse_reverb.wav, scope_peaks.csv, TEST_RESULTS.txt)
└─ .github/workflows/build-windows.yml   ← free cloud Windows build
```

## Licence

MIT (see `LICENSE.txt`) — use it, share it, modify it. The VST3 project uses
JUCE; JUCE has its own licence terms if you distribute closed-source builds
commercially at scale (see the note in `VST3/README_BUILD_WINDOWS.md`).

*Tip for your first test: solo a lead vocal, JSFX version, defaults, and listen
to the start of any phrase after a pause. That ghostly inhale of reverb rising
into the first word is the whole trick — now it's one click.*
