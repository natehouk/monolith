# Monolith 1.1 — monophonic synthesizer (VST3 / VST2 / CLAP)

Original mono synth with the same class of feature set as FabFilter's first product, FabFilter One (2002). It is a **clean-room implementation**. No FabFilter code, binaries, assets, presets, UI artwork or product name were used or referenced. The DSP is built entirely from published, textbook techniques:

- PolyBLEP band-limited oscillators
- Zavalishin TPT state-variable filter with a soft-limited state for bounded self-oscillation
- One-pole envelope segments

## Signal path

```
Osc (sine ▸ tri ▸ saw ▸ pulse morph) ─┐
Sub (square, −1 oct) ─────────────────┼─▶ Drive ─▶ SVF (LP/BP/HP) ─▶ Amp ─▶ DC block ─▶ Limiter ─▶ Out (stereo)
Noise ────────────────────────────────┘          ▲                   ▲
               cutoff mod: Filter Env, Key Track, Velocity, LFO     Amp Env × Velocity
Pitch: glide, octave, fine tune, pitch bend, LFO vibrato, mod wheel (CC1 → vibrato)
```

## Parameters (29, all automatable)

| Section | Parameters |
|---|---|
| Oscillator | Osc Shape (continuous morph), Pulse Width, Octave (±2), Fine Tune (±100 ct), Sub Osc, Noise |
| Voice | Glide (0–2 s), Legato (last-note priority, no env retrigger), Bend Range (0–12 st) |
| Filter | Mode (Low/Band/High Pass), Cutoff (20 Hz–20 kHz), Resonance (self-oscillates at max), Drive, Key Track, Filter Env Amt (±6 oct), Filter Vel Amt |
| Envelopes | Filter A/D/S/R, Amp A/D/S/R, Amp Vel Amt |
| LFO | Rate (0.05–20 Hz), Vibrato, LFO > Cutoff |
| Output | Volume (−36…+6 dB), with a built-in soft limiter that caps output at 0 dBFS |

**Factory presets:** Init, Acid Line, Fat Bass, Soft Lead, Pluck, Sub Drone.

**MIDI:** note on/off, velocity, pitch bend, CC1 (mod wheel → vibrato), CC120/123 (all notes off).

## Install

| Platform | Copy | To |
|---|---|---|
| Windows x64 | `windows-x64/Monolith.vst3` (folder) | `C:\Program Files\Common Files\VST3\` |
| Windows x64 | `windows-x64/Monolith.clap` | `C:\Program Files\Common Files\CLAP\` |
| Linux x64 | `linux-x64/Monolith.vst3` | `~/.vst3/` |
| Linux x64 | `linux-x64/Monolith.clap` | `~/.clap/` |
| macOS (universal) | `Monolith.vst3` / `Monolith.component` / `Monolith.clap` | `~/Library/Audio/Plug-Ins/VST3`, `/Components`, `/CLAP` |

VST2 (`Monolith-vst.dll` / `.so`) is also included for older hosts.

## Interface (new in 1.1)

![Monolith UI](screenshot.png)

The interface is an original vector design that scales on HiDPI screens. It has:

- a live oscillator scope
- a filter response curve that follows cutoff, resonance and mode
- drawings of both envelopes and the LFO

| Action | Result |
|---|---|
| Drag up/down | Adjust a knob |
| Hold Shift while dragging | Fine adjustment |
| Mouse wheel | Step the value |
| Double-click | Reset to default |
| Hover | Show the current value |

Time knobs use a square-law curve, so short envelope times get more of the knob's travel.

**Windows note:** the binaries are unsigned. They are statically linked, so no runtime install is needed.

## macOS build

The macOS binaries are built by GitHub Actions (`.github/workflows/build.yml`) on a `macos-14` runner. The build is universal (Apple Silicon + Intel) and produces VST3, AU and CLAP. The bundles are ad-hoc signed.

To get them, push this folder to a GitHub repo. Download the `Monolith-macos-universal` artifact from the Actions run, or push a `v1.1` tag to get a GitHub Release with all three platforms.

Without notarization, macOS may quarantine the downloaded plugin. Clear the flag with:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/*/Monolith*
```

## Build from source

The plugin is built on [DPF](https://github.com/DISTRHO/DPF) (ISC license):

```
git clone --recursive https://github.com/DISTRHO/DPF.git dpf
cd plugins/Monolith && make            # native build: Linux / macOS (macOS also gives AU)
make WINDOWS=true CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++   # Windows cross-build
```

## Verification done

- Steinberg VST3 validator (official SDK): **47 tests passed, 0 failed** (Linux x64 build, with the GUI).
- GUI tested under a virtual display with scripted input: knob drag, filter-mode click, legato toggle and scroll-wheel steps all update the parameters.
- Fuzz test: 12,000 random parameter/MIDI blocks at 44.1, 48, 96 and 192 kHz. No NaN or Inf output, and the peak stayed ≤ 0 dBFS.
- Self-oscillation at max resonance is stable in all three filter modes.
- `demo-acid.wav`: an offline render of the engine running the Acid Line settings.

## Files

- `plugins/Monolith/MonoEngine.hpp`: all of the DSP, header-only, with no framework dependency
- `plugins/Monolith/MonolithPlugin.cpp`: plugin wrapper and presets
- `plugins/Monolith/ParamDefs.hpp`: parameter table shared by the DSP and the UI
- `plugins/Monolith/MonolithUI.cpp`: custom interface (NanoVG)

License: ISC.
