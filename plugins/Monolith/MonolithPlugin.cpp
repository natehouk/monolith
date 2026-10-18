// Monolith — DPF plugin wrapper. License: ISC.
#include "DistrhoPlugin.hpp"
#include "ParamDefs.hpp"

START_NAMESPACE_DISTRHO

using namespace mono;

// Factory presets: {paramIndex, value} overrides on top of defaults.
struct Preset { const char* name; float v[kParamCount]; };
static Preset kPresets[] = {
    {"Init",        {}},
    {"Acid Line",   {}},
    {"Fat Bass",    {}},
    {"Soft Lead",   {}},
    {"Pluck",       {}},
    {"Sub Drone",   {}},
};
static constexpr uint32_t kNumPresets = sizeof(kPresets) / sizeof(kPresets[0]);

static void buildPresets() {
    static bool done = false; if (done) return; done = true;
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
}

class MonolithPlugin : public Plugin {
public:
    MonolithPlugin() : Plugin(kParamCount, kNumPresets, 0) {
        buildPresets();
        engine.setSampleRate((float)getSampleRate());
    }

protected:
    const char* getLabel() const override { return "Monolith"; }
    const char* getDescription() const override {
        return "Monophonic subtractive synthesizer: morphing oscillator, sub & noise, "
               "resonant multimode filter, two envelopes, glide and LFO.";
    }
    const char* getMaker() const override { return "Epsilon Audio"; }
    const char* getHomePage() const override { return "https://epsilonrecords.com"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 1, 0); }

    void initParameter(uint32_t i, Parameter& prm) override {
        const ParamDef& d = kDefs[i];
        prm.hints = kParameterIsAutomatable
                  | ((d.hints & kHintInteger) ? kParameterIsInteger : 0)
                  | ((d.hints & kHintBoolean) ? kParameterIsBoolean : 0)
                  | ((d.hints & kHintLog) ? kParameterIsLogarithmic : 0);
        prm.name = d.name; prm.symbol = d.sym; prm.unit = d.unit;
        prm.ranges.min = d.min; prm.ranges.max = d.max;
        Engine def; prm.ranges.def = def.p[i];
        if (i == kFilterMode) {
            ParameterEnumerationValue* ev = new ParameterEnumerationValue[3];
            ev[0].value = 0; ev[0].label = "Low Pass";
            ev[1].value = 1; ev[1].label = "Band Pass";
            ev[2].value = 2; ev[2].label = "High Pass";
            prm.enumValues.count = 3; prm.enumValues.restrictedMode = true; prm.enumValues.values = ev;
        }
    }

    void initProgramName(uint32_t i, String& name) override { name = kPresets[i].name; }
    void loadProgram(uint32_t i) override { std::copy(kPresets[i].v, kPresets[i].v + kParamCount, engine.p); }

    float getParameterValue(uint32_t i) const override { return engine.p[i]; }
    void setParameterValue(uint32_t i, float v) override { engine.p[i] = v; }

    void activate() override { engine.setSampleRate((float)getSampleRate()); }
    void sampleRateChanged(double sr) override { engine.setSampleRate((float)sr); }

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* ev, uint32_t evCount) override {
        uint32_t pos = 0;
        for (uint32_t e = 0; e <= evCount; ++e) {
            const uint32_t until = (e < evCount) ? std::min(ev[e].frame, frames) : frames;
            if (until > pos) { engine.render(outputs[0] + pos, outputs[1] + pos, until - pos); pos = until; }
            if (e == evCount) break;
            if (ev[e].size > 3) continue;
            const uint8_t* d = ev[e].data;
            const uint8_t st = d[0] & 0xF0;
            if (st == 0x90 && d[2] > 0) engine.noteOn(d[1], d[2]);
            else if (st == 0x80 || (st == 0x90 && d[2] == 0)) engine.noteOff(d[1]);
            else if (st == 0xE0) engine.pitchBend(d[1] | (d[2] << 7));
            else if (st == 0xB0) {
                if (d[1] == 1) engine.modWheel(d[2]);
                else if (d[1] == 120 || d[1] == 123) engine.allNotesOff();
            }
        }
    }

private:
    Engine engine;
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonolithPlugin)
};

Plugin* createPlugin() { return new MonolithPlugin(); }

END_NAMESPACE_DISTRHO
