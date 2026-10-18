// Factory presets, shared by the DSP (host program list) and the UI selector. License: ISC.
#pragma once
#include "MonoEngine.hpp"
#include <algorithm>

namespace mono {

struct Preset { const char* name; float v[kParamCount]; };
static constexpr uint32_t kNumPresets = 6;

inline const Preset* presets() {
    static Preset kPresets[kNumPresets] = {
        {"Init", {}}, {"Acid Line", {}}, {"Fat Bass", {}},
        {"Soft Lead", {}}, {"Pluck", {}}, {"Sub Drone", {}},
    };
    static bool done = false;
    if (done) return kPresets;
    done = true;
    Engine e;
    for (auto& pr : kPresets) std::copy(e.p, e.p + kParamCount, pr.v);
    auto& acid = kPresets[1].v;
    acid[kShape] = 0.66f; acid[kGlide] = 0.08f; acid[kLegato] = 1; acid[kCutoff] = 320.f;
    acid[kResonance] = 0.82f; acid[kDrive] = 0.45f; acid[kFltEnvAmt] = 0.6f; acid[kFltDecay] = 0.22f;
    acid[kFltSustain] = 0.f; acid[kOctave] = -1; acid[kAmpSustain] = 1.f; acid[kAmpRelease] = 0.05f;
    auto& bass = kPresets[2].v;
    bass[kShape] = 0.8f; bass[kPulseWidth] = 0.35f; bass[kSubLevel] = 0.7f; bass[kOctave] = -1;
    bass[kCutoff] = 600.f; bass[kResonance] = 0.2f; bass[kDrive] = 0.3f; bass[kFltEnvAmt] = 0.4f;
    bass[kFltDecay] = 0.3f; bass[kFltSustain] = 0.15f; bass[kAmpRelease] = 0.08f;
    auto& lead = kPresets[3].v;
    lead[kShape] = 0.5f; lead[kGlide] = 0.12f; lead[kLegato] = 1; lead[kCutoff] = 2500.f;
    lead[kResonance] = 0.3f; lead[kFltEnvAmt] = 0.2f; lead[kAmpAttack] = 0.03f; lead[kAmpRelease] = 0.4f;
    lead[kLfoRate] = 5.5f; lead[kLfoPitch] = 0.12f;
    auto& pluck = kPresets[4].v;
    pluck[kShape] = 1.f; pluck[kPulseWidth] = 0.2f; pluck[kCutoff] = 250.f; pluck[kResonance] = 0.5f;
    pluck[kFltEnvAmt] = 0.75f; pluck[kFltDecay] = 0.15f; pluck[kFltSustain] = 0.f; pluck[kFltVelAmt] = 0.6f;
    pluck[kAmpDecay] = 0.4f; pluck[kAmpSustain] = 0.f; pluck[kAmpRelease] = 0.3f;
    auto& drone = kPresets[5].v;
    drone[kShape] = 0.f; drone[kSubLevel] = 1.f; drone[kOctave] = -1; drone[kNoiseLevel] = 0.04f;
    drone[kCutoff] = 900.f; drone[kResonance] = 0.6f; drone[kFltEnvAmt] = 0.f; drone[kAmpAttack] = 1.2f;
    drone[kAmpRelease] = 2.5f; drone[kLfoRate] = 0.2f; drone[kLfoCutoff] = 0.5f;
    return kPresets;
}

} // namespace mono
