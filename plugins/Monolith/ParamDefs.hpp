// Shared parameter metadata (DSP + UI). License: MIT.
#pragma once
#include "MonoEngine.hpp"
#include <cstdint>

namespace mono {

static constexpr uint32_t kHintInteger = 1, kHintBoolean = 2, kHintLog = 4;

struct ParamDef { const char* name; const char* sym; const char* unit; float min, max; uint32_t hints; };

static const ParamDef kDefs[kParamCount] = {
    {"Osc Shape",       "shape",     "",    0.f,   1.f,     0},
    {"Pulse Width",     "pw",        "",    0.05f, 0.95f,   0},
    {"Octave",          "octave",    "oct", -2.f,  2.f,     kHintInteger},
    {"Fine Tune",       "fine",      "ct",  -100.f,100.f,   0},
    {"Sub Osc",         "sub",       "",    0.f,   1.f,     0},
    {"Noise",           "noise",     "",    0.f,   1.f,     0},
    {"Glide",           "glide",     "s",   0.f,   2.f,     0},
    {"Legato",          "legato",    "",    0.f,   1.f,     kHintBoolean | kHintInteger},
    {"Filter Mode",     "fmode",     "",    0.f,   2.f,     kHintInteger},
    {"Cutoff",          "cutoff",    "Hz",  20.f,  20000.f, kHintLog},
    {"Resonance",       "reso",      "",    0.f,   1.f,     0},
    {"Drive",           "drive",     "",    0.f,   1.f,     0},
    {"Key Track",       "keytrack",  "",    0.f,   1.f,     0},
    {"Filter Env Amt",  "fenvamt",   "",    -1.f,  1.f,     0},
    {"Filter Vel Amt",  "fvelamt",   "",    0.f,   1.f,     0},
    {"Filter Attack",   "fatk",      "s",   0.f,   5.f,     0},
    {"Filter Decay",    "fdec",      "s",   0.001f,5.f,     0},
    {"Filter Sustain",  "fsus",      "",    0.f,   1.f,     0},
    {"Filter Release",  "frel",      "s",   0.001f,5.f,     0},
    {"Amp Attack",      "aatk",      "s",   0.f,   5.f,     0},
    {"Amp Decay",       "adec",      "s",   0.001f,5.f,     0},
    {"Amp Sustain",     "asus",      "",    0.f,   1.f,     0},
    {"Amp Release",     "arel",      "s",   0.001f,5.f,     0},
    {"Amp Vel Amt",     "avelamt",   "",    0.f,   1.f,     0},
    {"LFO Rate",        "lforate",   "Hz",  0.05f, 20.f,    kHintLog},
    {"Vibrato",         "lfopitch",  "",    0.f,   1.f,     0},
    {"LFO > Cutoff",    "lfocut",    "",    0.f,   1.f,     0},
    {"Bend Range",      "bend",      "st",  0.f,   12.f,    kHintInteger},
    {"Volume",          "volume",    "dB",  -36.f, 6.f,     0},
    // 1.3
    {"Voice Mode",      "voicemode", "",    0.f,   1.f,     kHintInteger},
    {"Hold",            "hold",      "s",   0.f,   5.f,     0},
    {"PWM Source",      "pwmsrc",    "",    0.f,   3.f,     kHintInteger},
    {"Noise Type",      "noisetype", "",    0.f,   1.f,     kHintInteger},
    {"Pitch Env Amt",   "penvamt",   "",    -1.f,  1.f,     0},
    {"LFO Wave",        "lfowave",   "",    0.f,   2.f,     kHintInteger},
    {"LFO Balance",     "lfobal",    "",    0.f,   1.f,     0},
    {"LFO Sync",        "lfosync",   "",    0.f,   1.f,     kHintBoolean | kHintInteger},
    {"LFO Sync Rate",   "lfosyncrt", "",    0.f,   10.f,    kHintInteger},
    {"Osc Level",       "osclevel",  "",    0.f,   1.f,     0},
};

// Labels for stepped parameters (shared by host enum lists and the UI).
inline const char* const* enumLabels(uint32_t p, int& count) {
    static const char* const fmode[] = { "Low Pass", "Band Pass", "High Pass" };
    static const char* const vmode[] = { "Mono", "Poly" };
    static const char* const pwm[]   = { "Off", "LFO", "Mod Env", "Mod Env Inv" };
    static const char* const noise[] = { "White", "Pink" };
    static const char* const wave[]  = { "Sine", "Triangle", "Square" };
    static const char* const rate[]  = { "16 bars", "8 bars", "4 bars", "2 bars", "1 bar",
                                         "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
    switch (p) {
    case kFilterMode:  count = 3;  return fmode;
    case kVoiceMode:   count = 2;  return vmode;
    case kPwmSource:   count = 4;  return pwm;
    case kNoiseType:   count = 2;  return noise;
    case kLfoWave:     count = 3;  return wave;
    case kLfoSyncRate: count = 11; return rate;
    default:           count = 0;  return nullptr;
    }
}

} // namespace mono
