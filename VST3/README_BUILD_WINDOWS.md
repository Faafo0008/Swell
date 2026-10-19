# Building the Windows VST3

A `.vst3` binary is locked to the operating system it was built on, so the
Windows binary has to be built on Windows (yours, or a free cloud Windows
machine). You have two easy routes — and the **JSFX version in `../JSFX`
works in REAPER right now** while you decide.

## Route A — Free cloud build, nothing to install (recommended)

1. Create a free account at github.com (if you don't have one).
2. Create a new repository and upload this entire `ReverseReverb-OneClick`
   folder (drag-and-drop on the website works, or use GitHub Desktop).
   Make sure the hidden `.github` folder is included — GitHub Desktop
   includes it automatically.
3. GitHub automatically runs the included workflow
   (`.github/workflows/build-windows.yml`): it starts a Windows machine,
   downloads JUCE, and compiles the VST3. Takes ~5–10 minutes.
4. In your repo: **Actions → latest run → Artifacts →**
   download `ReverseReverb-Windows-VST3`, unzip it.
5. Copy the `Reverse Reverb One-Click.vst3` folder to
   `C:\Program Files\Common Files\VST3\`
6. In REAPER: Options → Preferences → Plug-ins → VST → Re-scan →
   *Clear cache and re-scan VST paths for all plug-ins*.
7. Add it from the FX browser: search **Reverse Reverb One-Click** (VST3i/Fx → Reverb).

## Route B — Build on your own PC

1. Install **Visual Studio 2022** (free Community edition) and tick
   *Desktop development with C++*.
2. Install **CMake** from cmake.org (tick "Add CMake to PATH"), or simply run
   the script from a *Developer Command Prompt for VS 2022*.
3. Double-click **`build_windows.bat`** in this folder. First run downloads
   JUCE automatically. At the end it offers to copy the `.vst3` into
   `C:\Program Files\Common Files\VST3\` for you.
4. Rescan in REAPER as in step 6 above.

## Notes

* The project builds a VST3 **and** a Standalone app you can use to test
  outside REAPER (`build/.../Standalone/`).
* Source layout: `Source/DSP/` is the pure audio engine (no JUCE needed,
  unit-testable — see `Tools/TestHarness.cpp`), `PluginProcessor.*` is the
  audio/host glue, `PluginEditor.*` is the UI with the big 1-click button.
* Licence note: this project is MIT-licensed. JUCE itself is dual-licensed
  (free GPLv3 for open-source projects / free tier for small closed-source
  projects under the JUCE licence terms) — check juce.com if you plan to
  sell a closed-source version commercially at scale.
