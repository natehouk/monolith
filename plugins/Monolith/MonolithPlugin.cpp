// Monolith — DPF plugin wrapper. License: ISC.
#include "DistrhoPlugin.hpp"
#include "ParamDefs.hpp"
#include "Presets.hpp"

START_NAMESPACE_DISTRHO

using namespace mono;

class MonolithPlugin : public Plugin {
public:
    MonolithPlugin() : Plugin(kParamCount, kNumPresets, 0) {
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
    uint32_t getVersion() const override { return d_version(1, 2, 0); }

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

    void initProgramName(uint32_t i, String& name) override { name = presets()[i].name; }
    void loadProgram(uint32_t i) override { std::copy(presets()[i].v, presets()[i].v + kParamCount, engine.p); }

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
