// Shared parameter metadata (DSP + UI). License: ISC.
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
};

} // namespace mono
