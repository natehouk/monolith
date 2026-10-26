// Monolith — hover tips and the in-plugin help pages. License: MIT.
#pragma once
#include "ParamDefs.hpp"

namespace mono {

static const char* const kManualURL = "https://github.com/natehouk/monolith/blob/main/docs/MANUAL.md";

inline const char* paramTip(uint32_t p) {
    switch (p) {
    case kShape: return "Morphs the oscillator smoothly from sine through triangle and saw to pulse. In-between settings blend the neighbouring shapes. Band-limited, so it never aliases.";
    case kPulseWidth: return "The pulse width of the pulse shape (and the square part of the morph). With a PWM source switched on, this knob sets how far the source sweeps the width instead.";
    case kOctave: return "Shifts the oscillator by whole octaves, two down to two up.";
    case kFine: return "Fine tuning, up to a semitone either way (in cents).";
    case kSubLevel: return "A square wave one octave below the oscillator, for weight under bass and leads.";
    case kNoiseLevel: return "Mixes in noise. Choose white (bright hiss) or pink (darker, even across octaves) with the switch beside it. Turn OSC down for pure noise sounds.";
    case kOscLevel: return "The level of the main oscillator. Turn it down to hear only the sub and noise.";
    case kGlide: return "Slides the pitch from the previous note to the new one. Works in mono and in poly, where new notes glide from the last note played.";
    case kLegato: return "Mono only: when you play a note while another is held, the envelopes don't restart, so the line flows. With glide, only overlapping notes glide.";
    case kFilterMode: return "LOW PASS keeps the lows and removes the highs, BAND PASS keeps a band around the cutoff, HIGH PASS removes the lows. 12 dB per octave.";
    case kCutoff: return "Where the filter starts working, from 20 Hz to 20 kHz. The envelope, LFO, key tracking and velocity all move it from here.";
    case kResonance: return "Emphasises the frequencies around the cutoff. Near the top the filter rings and sings on its own (self-oscillation), which you can play as a sine wave with KEY at 100%.";
    case kDrive: return "Drives the filter input for warmth and grit.";
    case kFltEnvAmt: return "How far the mod envelope moves the cutoff: up to six octaves. Negative amounts sweep it down instead of up.";
    case kKeyTrack: return "Makes the cutoff follow the keyboard, so higher notes stay as bright as low ones. 100% tracks the pitch exactly.";
    case kFltVelAmt: return "Harder playing opens the filter further.";
    case kFltAttack: return "Mod envelope attack: how long it takes to rise when a note starts. The mod envelope moves the cutoff, the pitch (PITCH ENV) and the pulse width (PWM ENV).";
    case kFltDecay: return "Mod envelope decay: how long it takes to fall to the sustain level.";
    case kFltSustain: return "Mod envelope sustain: the level it holds while the key is down.";
    case kFltRelease: return "Mod envelope release: how long it takes to fall to zero after the key is released (and the hold time has passed).";
    case kAmpAttack: return "Amp envelope attack: how long the sound takes to fade in.";
    case kAmpDecay: return "Amp envelope decay: how long it takes to fall to the sustain level.";
    case kAmpSustain: return "Amp envelope sustain: the level while the key is held.";
    case kAmpRelease: return "Amp envelope release: how long the sound takes to fade out after the key is released (and the hold time has passed).";
    case kAmpVelAmt: return "How much velocity changes the volume. 0%: every note is equally loud.";
    case kHold: return "After you release a key, both envelopes keep sustaining for this long before they release. The next note can overlap the previous one. The hold region is shaded in the envelope displays.";
    case kLfoRate: return "LFO speed, 0.05 to 20 Hz. With SYNC on, this knob picks a note length (16 bars to 1/64) instead, locked to the host tempo and song position.";
    case kLfoPitch: return "Vibrato: how far the LFO moves the pitch, up to 24 semitones. The curve is gentle at the bottom, so small settings stay subtle. The mod wheel adds vibrato too.";
    case kLfoCutoff: return "How far the LFO moves the filter cutoff, up to two octaves either way.";
    case kBendRange: return "How far the pitch-bend wheel bends, up to 12 semitones.";
    case kVolume: return "Output level.";
    case kVoiceMode: return "MONO plays one note at a time, the last one you pressed (with optional legato). POLY plays up to 10 notes; when you play an 11th, the oldest note is taken over.";
    case kPwmSource: return "What moves the pulse width: nothing (PW FIXED), the LFO, the mod envelope, or the mod envelope upside down. With a source on, the PW knob sets the depth.";
    case kNoiseType: return "WHITE noise is bright hiss; PINK noise is darker, with equal energy in every octave.";
    case kPitchEnvAmt: return "How far the mod envelope bends the pitch, up to 24 semitones. Negative amounts bend down. Great for kicks, zaps and plucks.";
    case kLfoWave: return "The LFO's shape: sine, triangle or square. BALANCE skews it.";
    case kLfoBalance: return "For sine and triangle, skews the shape from a falling ramp through symmetric (50%) to a rising ramp. For square, sets the duty cycle.";
    case kLfoSync: return "Locks the LFO to the host tempo and song position, so it lines up with the beat every time you play. The RATE knob then picks a note length.";
    case kLfoSyncRate: return "The LFO cycle length when synced, from 16 bars to a 64th note.";
    default: return "";
    }
}

enum TipTarget { kTipUndo, kTipRedo, kTipLearn, kTipHelp, kTipA, kTipB, kTipCopy, kTipPrev, kTipPresets, kTipNext,
                 kTipScope, kTipFilter, kTipModEnv, kTipAmpEnv, kTipLfo, kNumTipTargets };
inline const char* targetTitle(int t) {
    static const char* const n[kNumTipTargets] = { "Undo", "Redo", "MIDI Learn", "Help", "Setting A", "Setting B", "Copy A/B",
                                                   "Previous preset", "Presets", "Next preset", "Oscillator", "Filter response",
                                                   "Mod envelope", "Amp envelope", "LFO" };
    return n[t];
}
inline const char* targetTip(int t) {
    switch (t) {
    case kTipUndo: return "Steps back through your edits, including preset loads and A/B switches. Cmd/Ctrl-Z.";
    case kTipRedo: return "Re-applies an edit you undid. Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y.";
    case kTipLearn: return "Click, then click any control and move a knob or fader on your MIDI controller to assign it. Right-click a control to unassign. Assignments are saved with your project.";
    case kTipHelp: return "Opens the help pages: the oscillator, filter, envelopes, LFO, voices, MIDI and shortcuts.";
    case kTipA: case kTipB: return "Monolith keeps two complete sounds, A and B. Click the other letter to switch and compare.";
    case kTipCopy: return "Copies the current sound to the other slot.";
    case kTipPrev: return "Load the previous preset. The mouse wheel over the preset name also steps through presets.";
    case kTipPresets: return "Open the preset browser: factory sounds in five categories, your own sounds, and SAVE CURRENT AS. A * after the name means you have changed it. In the browser, type to search names and categories, and click a star to keep a preset in Favourites.";
    case kTipNext: return "Load the next preset.";
    case kTipScope: return "A picture of the oscillator mix: shape, pulse width, sub and noise.";
    case kTipFilter: return "The filter's response at the current cutoff, resonance and mode. The line marks the cutoff.";
    case kTipModEnv: return "The mod envelope: attack, decay, sustain, release. The shaded part is the hold time after key-up.";
    case kTipAmpEnv: return "The amp envelope: attack, decay, sustain, release. The shaded part is the hold time after key-up.";
    case kTipLfo: return "The LFO's shape and balance. With sync on, the note length is shown in the corner.";
    default: return "";
    }
}

struct HelpPage { const char* title; const char* body; };
static const HelpPage kHelpPages[] = {
    { "Overview",
      "Monolith is a subtractive synthesizer: an oscillator makes a bright, rich tone, a filter shapes it, and envelopes and an LFO bring it to life.\n\n"
      "1. Pick a preset, or start from Init.\n"
      "2. Choose the oscillator SHAPE and mix in SUB and NOISE.\n"
      "3. Set the filter CUTOFF and RESONANCE, and how much the mod envelope (ENV) sweeps it.\n"
      "4. Shape the volume with the amp envelope.\n\n"
      "Hover over anything for a moment to see what it does." },
    { "Oscillator",
      "SHAPE morphs from sine through triangle and saw to pulse. PW sets the pulse width; PWM switches let the LFO or the mod envelope sweep it, and PW then sets the depth.\n\n"
      "PITCH ENV bends the pitch with the mod envelope (negative bends down). OCTAVE and FINE tune it.\n\n"
      "The mixer has OSC, SUB (a square an octave down) and NOISE (white or pink)." },
    { "Filter",
      "A 12 dB/oct filter: LOW PASS, BAND PASS or HIGH PASS. CUTOFF sets where it works, RESONANCE emphasises it (and makes it sing on its own near the top), DRIVE adds grit.\n\n"
      "The cutoff moves with the mod envelope (ENV, either direction), the keyboard (KEY), velocity (VEL) and the LFO (> CUTOFF)." },
    { "Envelopes & LFO",
      "The MOD ENVELOPE moves the cutoff, pitch and pulse width. The AMP ENVELOPE shapes the volume; VEL sets how much velocity changes it.\n\n"
      "HOLD keeps both envelopes sustaining for a while after you let go, so notes overlap.\n\n"
      "The LFO is a sine, triangle or square; BALANCE skews it. RATE is free (Hz) or, with SYNC, a note length locked to the host. It drives VIBRATO, > CUTOFF and PWM." },
    { "Voices",
      "MONO plays the last note you pressed; LEGATO stops the envelopes restarting on overlapping notes. POLY plays up to 10 notes.\n\n"
      "GLIDE slides between notes in both modes. BEND sets the pitch-bend range. The mod wheel adds vibrato, and the sustain pedal holds notes." },
    { "MIDI & shortcuts",
      "MIDI LEARN: click LEARN, click a control, move a controller. Right-click unassigns. CLEAR ALL removes every assignment.\n\n"
      "A/B holds two complete sounds; COPY copies the current one to the other slot. Every edit is an undo step (Cmd/Ctrl-Z, Cmd/Ctrl-Shift-Z).\n\n"
      "Double-click a knob to type a value; Cmd/Ctrl-click resets it; Shift-drag is fine adjustment; grab the rim of a large knob to turn it in a circle. Esc closes this window." },
};
static constexpr int kNumHelpPages = sizeof(kHelpPages) / sizeof(kHelpPages[0]);

} // namespace mono
