// Monolith — DPF plugin wrapper: parameters, programs, MIDI, MIDI Learn, host transport. License: MIT.
#include "DistrhoPlugin.hpp"
#include "ParamDefs.hpp"
#include "Presets.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

START_NAMESPACE_DISTRHO

using namespace mono;

class MonolithPlugin : public Plugin {
public:
    MonolithPlugin() : Plugin(kNumParams, kNumPresets, 2) {
        for (int& m : ccMap) m = -1;
        engine.setSampleRate((float)getSampleRate());
    }

protected:
    const char* getLabel() const override { return "Monolith"; }
    const char* getDescription() const override {
        return "Subtractive synthesizer, mono or 10-voice poly: morphing oscillator with PWM, "
               "sub and white/pink noise, resonant multimode filter, two envelopes with hold, "
               "glide and a tempo-syncable LFO.";
    }
    const char* getMaker() const override { return "Epsilon Audio"; }
    const char* getHomePage() const override { return "https://epsilonrecords.com"; }
    const char* getLicense() const override { return "MIT"; }
    uint32_t getVersion() const override { return d_version(2, 1, 0); }

    void initParameter(uint32_t i, Parameter& prm) override {
        const ParamDef& d = paramDef(i);
        prm.name = d.name; prm.symbol = d.sym; prm.unit = d.unit;
        prm.ranges.min = d.min; prm.ranges.max = d.max;
        if (d.hints & kHintOutput) { prm.hints = kParameterIsOutput; prm.ranges.def = 0; return; }
        prm.hints = kParameterIsAutomatable
                  | ((d.hints & kHintInteger) ? kParameterIsInteger : 0)
                  | ((d.hints & kHintBoolean) ? kParameterIsBoolean : 0)
                  | ((d.hints & kHintLog) ? kParameterIsLogarithmic : 0);
        Engine def; prm.ranges.def = def.p[i];
        int n = 0;
        if (const char* const* labels = enumLabels(i, n)) {
            ParameterEnumerationValue* ev = new ParameterEnumerationValue[n];
            for (int k = 0; k < n; ++k) { ev[k].value = (float)k; ev[k].label = labels[k]; }
            prm.enumValues.count = (uint8_t)n; prm.enumValues.restrictedMode = true; prm.enumValues.values = ev;
        }
    }

    // state: "midimap" is saved with the session; "cmd" carries one-shot UI requests
    void initState(uint32_t index, State& state) override {
        if (index == 0) { state.key = "midimap"; state.defaultValue = ""; state.label = "MIDI Learn map"; state.hints = kStateIsHostReadable; }
        else            { state.key = "cmd";     state.defaultValue = ""; state.label = "UI command";     state.hints = kStateIsOnlyForDSP; }
    }
    String getState(const char* key) const override {
        if (std::strcmp(key, "midimap") == 0) return String(serializeMap().c_str());
        return String();
    }
    void setState(const char* key, const char* value) override {
        if (std::strcmp(key, "midimap") == 0) { parseMap(value); return; }
        if (std::strcmp(key, "cmd") == 0 && value && *value) {
            if (std::strncmp(value, "learn:", 6) == 0)      learnTarget = std::atoi(value + 6);
            else if (std::strcmp(value, "cancel") == 0)     learnTarget = -1;
            else if (std::strncmp(value, "unmap:", 6) == 0) { const int p = std::atoi(value + 6); for (int& m : ccMap) if (m == p) m = -1; }
            else if (std::strcmp(value, "clear") == 0)      { for (int& m : ccMap) m = -1; }
        }
    }

    void initProgramName(uint32_t i, String& name) override { name = presets()[i].name; }
    void loadProgram(uint32_t i) override { std::copy(presets()[i].v, presets()[i].v + kParamCount, engine.p); }

    float getParameterValue(uint32_t i) const override { return i == kOutLearn ? learnEcho : engine.p[i]; }
    void setParameterValue(uint32_t i, float v) override { if (i < kParamCount) engine.p[i] = v; }

    void activate() override { engine.setSampleRate((float)getSampleRate()); }
    void sampleRateChanged(double sr) override { engine.setSampleRate((float)sr); }

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* ev, uint32_t evCount) override {
        const TimePosition& tp = getTimePosition();
        if (tp.bbt.valid) {
            const double bpb = tp.bbt.beatsPerBar > 0 ? tp.bbt.beatsPerBar : 4.0;
            const double tpb = tp.bbt.ticksPerBeat > 0 ? tp.bbt.ticksPerBeat : 1920.0;
            const double barPos = (tp.bbt.bar - 1) + ((tp.bbt.beat - 1) + tp.bbt.tick / tpb) / bpb;
            engine.setTransport(true, tp.playing, barPos, tp.bbt.beatsPerMinute, bpb);
        } else {
            engine.setTransport(false, false, 0.0, 120.0, 4.0);
        }

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
                const int cc = d[1] & 0x7F, v = d[2] & 0x7F;
                if (learnTarget >= 0 && learnTarget < (int)kParamCount && cc != 64 && cc < 120) {   // MIDI Learn: bind this controller
                    for (int& m : ccMap) if (m == learnTarget) m = -1;
                    ccMap[cc] = learnTarget;
                    learnSerial = (learnSerial + 1) % 8;
                    learnEcho = (float)(learnSerial * 10000 + learnTarget * 128 + cc + 1);
                    learnTarget = -1;
                }
                const int prm = ccMap[cc];
                if (prm >= 0) {
                    const float x = fromNorm((uint32_t)prm, v / 127.f);
                    engine.p[prm] = x;
                    requestParameterValueChange((uint32_t)prm, x);
                } else if (cc == 1) engine.modWheel(v);
                if (cc == 64) engine.sustainPedal(v >= 64);
                else if (cc == 120) engine.allSoundOff();
                else if (cc == 123) engine.allNotesOff();
            }
        }
    }

private:
    std::string serializeMap() const {
        std::string s; char b[24];
        for (int cc = 0; cc < 128; ++cc) if (ccMap[cc] >= 0) { std::snprintf(b, sizeof b, "%d:%d;", cc, ccMap[cc]); s += b; }
        return s;
    }
    void parseMap(const char* v) {
        for (int& m : ccMap) m = -1;
        if (!v) return;
        const char* q = v;
        while (*q) {
            int cc = -1, prm = -1;
            if (std::sscanf(q, "%d:%d", &cc, &prm) == 2 && cc >= 0 && cc < 128 && prm >= 0 && prm < (int)kParamCount) ccMap[cc] = prm;
            const char* semi = std::strchr(q, ';');
            if (!semi) break;
            q = semi + 1;
        }
    }

    Engine engine;
    int ccMap[128];
    int learnTarget = -1, learnSerial = 0;
    float learnEcho = 0;
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonolithPlugin)
};

Plugin* createPlugin() { return new MonolithPlugin(); }

END_NAMESPACE_DISTRHO
