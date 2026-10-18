// Factory presets, shared by the DSP (host program list) and the UI selector.
// Each preset lists only the parameters it changes from the engine defaults.
// License: MIT.
#pragma once
#include "MonoEngine.hpp"
#include <algorithm>

namespace mono {

struct PV { uint32_t id; float v; };
struct PresetDef { const char* name; const char* category; const PV* pv; };
struct Preset { const char* name; const char* category; float v[kParamCount]; };

static const char* const kCategories[] = { "Bass", "Lead", "Poly", "Pluck & Keys", "FX & Drums" };
static constexpr int kNumCategories = 5;

#define END { kParamCount, 0 }
namespace pv {
static const PV init[] = { END };

// ---------------------------------------------------------------- Bass (mono)
static const PV acid[] = { {kShape,.66f},{kGlide,.08f},{kLegato,1},{kCutoff,320},{kResonance,.82f},{kDrive,.45f},
    {kFltEnvAmt,.6f},{kFltDecay,.22f},{kFltSustain,0},{kOctave,-1},{kAmpSustain,1},{kAmpRelease,.05f}, END };
static const PV fatBass[] = { {kShape,.8f},{kPulseWidth,.35f},{kSubLevel,.7f},{kOctave,-1},{kCutoff,600},{kResonance,.2f},
    {kDrive,.3f},{kFltEnvAmt,.4f},{kFltDecay,.3f},{kFltSustain,.15f},{kAmpRelease,.08f}, END };
static const PV subDrone[] = { {kShape,0},{kSubLevel,1},{kOctave,-1},{kNoiseLevel,.04f},{kCutoff,900},{kResonance,.6f},
    {kFltEnvAmt,0},{kAmpAttack,1.2f},{kAmpRelease,2.5f},{kLfoRate,.2f},{kLfoCutoff,.5f}, END };
static const PV rubber[] = { {kShape,.9f},{kPulseWidth,.4f},{kOctave,-1},{kCutoff,380},{kResonance,.55f},{kFltEnvAmt,.5f},
    {kFltDecay,.18f},{kFltSustain,.1f},{kDrive,.35f},{kAmpRelease,.06f},{kGlide,.05f}, END };
static const PV deepSine[] = { {kShape,0},{kOctave,-1},{kSubLevel,.3f},{kCutoff,900},{kResonance,.1f},{kFltEnvAmt,0},
    {kAmpAttack,.002f},{kAmpDecay,.8f},{kAmpSustain,.7f},{kAmpRelease,.1f},{kDrive,.25f}, END };
static const PV reese[] = { {kShape,.66f},{kSubLevel,.5f},{kOctave,-1},{kCutoff,700},{kResonance,.3f},{kFltEnvAmt,.1f},
    {kLfoRate,.3f},{kLfoCutoff,.25f},{kLfoPitch,.05f},{kLfoWave,1},{kDrive,.4f},{kAmpRelease,.1f}, END };
static const PV pluckBass[] = { {kShape,.7f},{kOctave,-1},{kCutoff,200},{kResonance,.45f},{kFltEnvAmt,.7f},{kFltAttack,0},
    {kFltDecay,.12f},{kFltSustain,0},{kAmpDecay,.35f},{kAmpSustain,.25f},{kAmpRelease,.08f},{kFltVelAmt,.5f},{kLegato,0}, END };
static const PV squareSub[] = { {kShape,1},{kPulseWidth,.5f},{kOctave,-2},{kCutoff,1200},{kResonance,.15f},{kFltEnvAmt,.1f},
    {kSubLevel,.4f},{kDrive,.4f},{kAmpRelease,.08f}, END };
static const PV wobble[] = { {kShape,.66f},{kOctave,-1},{kSubLevel,.5f},{kCutoff,300},{kResonance,.6f},{kLfoCutoff,.8f},
    {kLfoSync,1},{kLfoSyncRate,7},{kLfoWave,1},{kFltEnvAmt,0},{kDrive,.5f},{kAmpRelease,.08f}, END };

// ---------------------------------------------------------------- Lead (mono)
static const PV softLead[] = { {kShape,.5f},{kGlide,.12f},{kLegato,1},{kCutoff,2500},{kResonance,.3f},{kFltEnvAmt,.2f},
    {kAmpAttack,.03f},{kAmpRelease,.4f},{kLfoRate,5.5f},{kLfoPitch,.12f}, END };
static const PV screamer[] = { {kShape,.66f},{kCutoff,3500},{kResonance,.7f},{kDrive,.8f},{kFltEnvAmt,.25f},{kGlide,.08f},
    {kLfoPitch,.1f},{kLfoRate,6},{kAmpAttack,.01f}, END };
static const PV squareLead[] = { {kShape,1},{kPulseWidth,.7f},{kPwmSource,1},{kLfoRate,4},{kLfoWave,1},{kCutoff,2800},
    {kResonance,.25f},{kGlide,.06f},{kAmpAttack,.01f},{kAmpRelease,.25f}, END };
static const PV whistle[] = { {kShape,0},{kOctave,1},{kCutoff,6000},{kResonance,.1f},{kFltEnvAmt,0},{kLfoPitch,.12f},
    {kLfoRate,5.5f},{kAmpAttack,.06f},{kAmpRelease,.4f},{kGlide,.15f},{kNoiseLevel,.03f},{kNoiseType,1}, END };
static const PV portaLead[] = { {kShape,.5f},{kGlide,.3f},{kLegato,1},{kCutoff,2000},{kResonance,.4f},{kFltEnvAmt,.3f},
    {kAmpRelease,.3f}, END };
static const PV thinPulse[] = { {kShape,1},{kPulseWidth,.85f},{kPwmSource,3},{kFltAttack,.3f},{kFltDecay,.8f},{kFltSustain,.3f},
    {kCutoff,4000},{kResonance,.2f},{kFltEnvAmt,.1f},{kAmpRelease,.3f}, END };
static const PV sweepLead[] = { {kCutoff,500},{kResonance,.85f},{kFltEnvAmt,.7f},{kFltAttack,.15f},{kFltDecay,1.2f},
    {kFltSustain,.3f},{kDrive,.3f},{kGlide,.05f}, END };
static const PV chip[] = { {kShape,1},{kPulseWidth,.25f},{kCutoff,20000},{kResonance,0},{kFltEnvAmt,0},{kDrive,0},
    {kAmpAttack,0},{kAmpDecay,.2f},{kAmpSustain,.7f},{kAmpRelease,.05f},{kOctave,1},{kLegato,0}, END };

// ---------------------------------------------------------------- Poly
static const PV warmPad[] = { {kVoiceMode,1},{kShape,.55f},{kCutoff,1200},{kResonance,.2f},{kFltEnvAmt,.2f},{kFltAttack,1.2f},
    {kFltDecay,2},{kFltSustain,.6f},{kFltRelease,1.5f},{kAmpAttack,.8f},{kAmpDecay,1},{kAmpSustain,.9f},{kAmpRelease,1.8f},
    {kLfoCutoff,.15f},{kLfoRate,.3f},{kLfoWave,1}, END };
static const PV strings[] = { {kVoiceMode,1},{kShape,.66f},{kCutoff,2400},{kResonance,.1f},{kFltEnvAmt,.1f},{kAmpAttack,.35f},
    {kAmpRelease,.9f},{kLfoPitch,.06f},{kLfoRate,5}, END };
static const PV pwmPad[] = { {kVoiceMode,1},{kShape,1},{kPulseWidth,.75f},{kPwmSource,1},{kLfoRate,.6f},{kLfoWave,1},
    {kCutoff,1800},{kResonance,.15f},{kFltEnvAmt,.1f},{kAmpAttack,.6f},{kAmpSustain,.9f},{kAmpRelease,1.4f}, END };
static const PV glassPad[] = { {kVoiceMode,1},{kShape,.33f},{kOctave,1},{kCutoff,5000},{kResonance,.3f},{kFltEnvAmt,0},
    {kAmpAttack,1},{kAmpRelease,2.5f},{kHold,.5f},{kNoiseLevel,.03f},{kNoiseType,1}, END };
static const PV brass[] = { {kVoiceMode,1},{kShape,.66f},{kCutoff,600},{kResonance,.2f},{kFltEnvAmt,.55f},{kFltAttack,.06f},
    {kFltDecay,.4f},{kFltSustain,.4f},{kAmpAttack,.03f},{kAmpSustain,.85f},{kAmpRelease,.25f},{kFltVelAmt,.5f}, END };
static const PV darkChoir[] = { {kVoiceMode,1},{kShape,.4f},{kCutoff,800},{kResonance,.45f},{kNoiseLevel,.08f},{kNoiseType,1},
    {kFltEnvAmt,.1f},{kAmpAttack,1.5f},{kAmpRelease,2},{kLfoCutoff,.1f},{kLfoRate,.2f}, END };
static const PV polySweep[] = { {kVoiceMode,1},{kCutoff,300},{kResonance,.6f},{kFltEnvAmt,.8f},{kFltAttack,2.5f},{kFltDecay,3},
    {kFltSustain,.5f},{kFltRelease,2},{kAmpAttack,.4f},{kAmpRelease,2}, END };
static const PV glideChords[] = { {kVoiceMode,1},{kGlide,.2f},{kShape,.6f},{kCutoff,2000},{kFltEnvAmt,.15f},{kAmpAttack,.2f},
    {kAmpRelease,1}, END };

// ---------------------------------------------------------------- Pluck & Keys (poly)
static const PV pluck[] = { {kVoiceMode,1},{kLegato,0},{kShape,1},{kPulseWidth,.2f},{kCutoff,250},{kResonance,.5f},
    {kFltEnvAmt,.75f},{kFltDecay,.15f},{kFltSustain,0},{kFltVelAmt,.6f},{kAmpDecay,.4f},{kAmpSustain,0},{kAmpRelease,.3f}, END };
static const PV bellKeys[] = { {kVoiceMode,1},{kShape,.2f},{kOctave,1},{kCutoff,3000},{kResonance,.5f},{kFltEnvAmt,.4f},
    {kFltDecay,.5f},{kFltSustain,0},{kAmpDecay,1.6f},{kAmpSustain,0},{kAmpRelease,1.2f},{kHold,.3f}, END };
static const PV plucked[] = { {kVoiceMode,1},{kShape,.9f},{kPulseWidth,.3f},{kCutoff,1500},{kFltEnvAmt,.6f},{kFltDecay,.25f},
    {kFltSustain,.05f},{kAmpDecay,.9f},{kAmpSustain,0},{kAmpRelease,.3f},{kFltVelAmt,.6f}, END };
static const PV funkKeys[] = { {kVoiceMode,1},{kShape,1},{kPulseWidth,.2f},{kFilterMode,1},{kCutoff,1400},{kResonance,.5f},
    {kFltEnvAmt,.4f},{kFltDecay,.15f},{kAmpDecay,.4f},{kAmpSustain,.1f},{kAmpRelease,.08f}, END };
static const PV mallet[] = { {kVoiceMode,1},{kShape,.1f},{kCutoff,2500},{kResonance,.3f},{kFltEnvAmt,0},{kAmpAttack,0},
    {kAmpDecay,.45f},{kAmpSustain,0},{kAmpRelease,.45f},{kPitchEnvAmt,.05f},{kFltAttack,0},{kFltDecay,.05f},{kFltSustain,0}, END };
static const PV organ[] = { {kVoiceMode,1},{kShape,.33f},{kSubLevel,.5f},{kCutoff,4000},{kResonance,0},{kFltEnvAmt,0},
    {kAmpAttack,.005f},{kAmpSustain,1},{kAmpRelease,.06f},{kLfoPitch,.05f},{kLfoRate,6.5f}, END };
static const PV kalimba[] = { {kVoiceMode,1},{kShape,0},{kOctave,1},{kCutoff,5000},{kFltEnvAmt,0},{kAmpDecay,.6f},
    {kAmpSustain,0},{kAmpRelease,.5f},{kPitchEnvAmt,.08f},{kFltAttack,0},{kFltDecay,.03f},{kFltSustain,0}, END };
static const PV stab[] = { {kVoiceMode,1},{kShape,.66f},{kCutoff,900},{kResonance,.4f},{kFltEnvAmt,.65f},{kFltDecay,.2f},
    {kFltSustain,.1f},{kAmpDecay,.3f},{kAmpSustain,.2f},{kAmpRelease,.2f},{kDrive,.3f}, END };

// ---------------------------------------------------------------- FX & Drums
static const PV kick[] = { {kVolume,-1},{kShape,0},{kOctave,-1},{kPitchEnvAmt,.9f},{kFltAttack,0},{kFltDecay,.08f},{kFltSustain,0},
    {kFltRelease,.05f},{kCutoff,20000},{kResonance,0},{kFltEnvAmt,0},{kKeyTrack,0},{kAmpAttack,0},{kAmpDecay,.35f},
    {kAmpSustain,0},{kAmpRelease,.1f},{kDrive,.4f},{kLegato,0}, END };
static const PV snare[] = { {kVolume,-1},{kShape,.33f},{kNoiseLevel,1},{kCutoff,6000},{kResonance,.2f},{kFltEnvAmt,0},{kKeyTrack,0},
    {kPitchEnvAmt,.4f},{kFltAttack,0},{kFltDecay,.05f},{kFltSustain,0},{kAmpAttack,0},{kAmpDecay,.18f},{kAmpSustain,0},
    {kAmpRelease,.12f},{kLegato,0}, END };
static const PV hihat[] = { {kVolume,-1},{kOscLevel,0},{kNoiseLevel,1},{kFilterMode,2},{kCutoff,7000},{kResonance,.3f},{kFltEnvAmt,0},
    {kKeyTrack,0},{kAmpAttack,0},{kAmpDecay,.06f},{kAmpSustain,0},{kAmpRelease,.05f},{kLegato,0}, END };
static const PV laser[] = { {kPitchEnvAmt,1},{kFltAttack,0},{kFltDecay,.25f},{kFltSustain,0},{kOctave,1},{kShape,.66f},
    {kAmpDecay,.3f},{kAmpSustain,0},{kAmpRelease,.1f},{kFltEnvAmt,0},{kLegato,0}, END };
static const PV siren[] = { {kShape,1},{kLfoWave,1},{kLfoRate,.4f},{kLfoPitch,.55f},{kCutoff,3000},{kFltEnvAmt,0},
    {kAmpRelease,.3f}, END };
static const PV wind[] = { {kVolume,-1},{kOscLevel,0},{kNoiseLevel,1},{kNoiseType,1},{kFilterMode,1},{kCutoff,800},{kResonance,.7f},
    {kLfoCutoff,.6f},{kLfoRate,.15f},{kLfoWave,0},{kFltEnvAmt,0},{kKeyTrack,0},{kAmpAttack,1},{kAmpRelease,2}, END };
static const PV thunder[] = { {kVolume,-1},{kOscLevel,0},{kNoiseLevel,1},{kNoiseType,1},{kCutoff,200},{kResonance,.3f},{kKeyTrack,0},
    {kAmpAttack,.3f},{kAmpDecay,3},{kAmpSustain,0},{kAmpRelease,3},{kDrive,.6f},{kFltEnvAmt,.4f},{kFltDecay,2},{kFltSustain,0}, END };
static const PV bleeps[] = { {kShape,1},{kOctave,1},{kLfoWave,2},{kLfoSync,1},{kLfoSyncRate,8},{kLfoPitch,.6f},{kCutoff,3000},
    {kFltEnvAmt,0},{kAmpRelease,.15f}, END };
} // namespace pv
#undef END

static const PresetDef kPresetDefs[] = {
    {"Init",            "Init",          pv::init},
    {"Acid Line",       "Bass",          pv::acid},
    {"Fat Bass",        "Bass",          pv::fatBass},
    {"Sub Drone",       "Bass",          pv::subDrone},
    {"Rubber Bass",     "Bass",          pv::rubber},
    {"Deep Sine",       "Bass",          pv::deepSine},
    {"Reese",           "Bass",          pv::reese},
    {"Pluck Bass",      "Bass",          pv::pluckBass},
    {"Square Sub",      "Bass",          pv::squareSub},
    {"Wobble 1/8",      "Bass",          pv::wobble},
    {"Soft Lead",       "Lead",          pv::softLead},
    {"Screamer",        "Lead",          pv::screamer},
    {"PWM Lead",        "Lead",          pv::squareLead},
    {"Whistle",         "Lead",          pv::whistle},
    {"Portamento",      "Lead",          pv::portaLead},
    {"Thin Pulse",      "Lead",          pv::thinPulse},
    {"Sweep Lead",      "Lead",          pv::sweepLead},
    {"Chiptune",        "Lead",          pv::chip},
    {"Warm Pad",        "Poly",          pv::warmPad},
    {"Strings",         "Poly",          pv::strings},
    {"PWM Pad",         "Poly",          pv::pwmPad},
    {"Glass Pad",       "Poly",          pv::glassPad},
    {"Brass",           "Poly",          pv::brass},
    {"Dark Choir",      "Poly",          pv::darkChoir},
    {"Slow Sweep",      "Poly",          pv::polySweep},
    {"Gliding Chords",  "Poly",          pv::glideChords},
    {"Pluck",           "Pluck & Keys",  pv::pluck},
    {"Bell Keys",       "Pluck & Keys",  pv::bellKeys},
    {"Plucked Keys",    "Pluck & Keys",  pv::plucked},
    {"Funk Keys",       "Pluck & Keys",  pv::funkKeys},
    {"Mallet",          "Pluck & Keys",  pv::mallet},
    {"Tone Organ",      "Pluck & Keys",  pv::organ},
    {"Kalimba",         "Pluck & Keys",  pv::kalimba},
    {"House Stab",      "Pluck & Keys",  pv::stab},
    {"Kick",            "FX & Drums",    pv::kick},
    {"Snare",           "FX & Drums",    pv::snare},
    {"Hi-Hat",          "FX & Drums",    pv::hihat},
    {"Laser",           "FX & Drums",    pv::laser},
    {"Siren",           "FX & Drums",    pv::siren},
    {"Wind",            "FX & Drums",    pv::wind},
    {"Thunder",         "FX & Drums",    pv::thunder},
    {"Bleeps 1/16",     "FX & Drums",    pv::bleeps},
};
static constexpr uint32_t kNumPresets = sizeof(kPresetDefs) / sizeof(kPresetDefs[0]);

inline const Preset* presets() {
    static Preset table[kNumPresets];
    static bool done = false;
    if (done) return table;
    done = true;
    Engine e;
    for (uint32_t i = 0; i < kNumPresets; ++i) {
        table[i].name = kPresetDefs[i].name;
        table[i].category = kPresetDefs[i].category;
        std::copy(e.p, e.p + kParamCount, table[i].v);
        for (const PV* x = kPresetDefs[i].pv; x->id != kParamCount; ++x) table[i].v[x->id] = x->v;
    }
    return table;
}

} // namespace mono
