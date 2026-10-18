// Monolith — original monophonic synthesizer engine.
// Clean-room implementation: written from public, generic synthesis techniques
// (PolyBLEP oscillators, TPT state-variable filter, linear-segment ADSR).
// No third-party product code, assets or disassembly were used.
// License: ISC (see LICENSE).

#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace mono {

static constexpr float kPi = 3.14159265358979f;
inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }

enum Param : uint32_t {
    kShape = 0,     // 0..1  sine -> triangle -> saw -> square
    kPulseWidth,    // 0.05..0.95 (square region)
    kOctave,        // -2..+2
    kFine,          // -100..+100 cents
    kSubLevel,      // 0..1   square one octave down
    kNoiseLevel,    // 0..1
    kGlide,         // 0..2 s
    kLegato,        // 0/1   don't retrigger envelopes on overlapping notes
    kFilterMode,    // 0 LP, 1 BP, 2 HP
    kCutoff,        // 20..20000 Hz
    kResonance,     // 0..1 (self-oscillates near 1)
    kDrive,         // 0..1
    kKeyTrack,      // 0..1
    kFltEnvAmt,     // -1..1 (+/-6 octaves)
    kFltVelAmt,     // 0..1
    kFltAttack, kFltDecay, kFltSustain, kFltRelease,
    kAmpAttack, kAmpDecay, kAmpSustain, kAmpRelease,
    kAmpVelAmt,     // 0..1
    kLfoRate,       // 0.05..20 Hz
    kLfoPitch,      // 0..1 semitone vibrato depth (x2)
    kLfoCutoff,     // 0..1 (+/-2 octaves)
    kBendRange,     // 0..12 semitones
    kVolume,        // -36..+6 dB
    kParamCount
};

struct ADSR {
    enum Stage { Idle, Attack, Decay, Sustain, Release } stage = Idle;
    float value = 0.f, a = 0.01f, d = 0.2f, s = 0.7f, r = 0.3f, sr = 48000.f;
    // Exponential-ish segments via one-pole approach toward overshoot targets.
    float coefA = 0, coefD = 0, coefR = 0;
    static float coef(float t, float sr) { return 1.f - std::exp(-1.f / std::max(t * sr, 1.f)); }
    void set(float A, float D, float S, float R) {
        a = A; d = D; s = S; r = R;
        coefA = coef(a / 1.5f, sr); coefD = coef(d / 4.f, sr); coefR = coef(r / 4.f, sr);
    }
    void gate(bool on) { stage = on ? Attack : (stage == Idle ? Idle : Release); }
    float process() {
        switch (stage) {
        case Attack:
            value += (1.2f - value) * coefA;
            if (value >= 1.f) { value = 1.f; stage = Decay; }
            break;
        case Decay:
            value += (s - value) * coefD;
            if (std::fabs(value - s) < 1e-4f) { value = s; stage = Sustain; }
            break;
        case Sustain: value = s; break;
        case Release:
            value += (0.f - value) * coefR;
            if (value < 1e-5f) { value = 0.f; stage = Idle; }
            break;
        case Idle: value = 0.f; break;
        }
        return value;
    }
};

// Topology-preserving-transform state-variable filter (Zavalishin), with
// soft saturation in the feedback path for musical self-oscillation.
struct SVF {
    float ic1 = 0, ic2 = 0;
    void reset() { ic1 = ic2 = 0; }
    inline void process(float x, float g, float k, float& lp, float& bp, float& hp) {
        const float a1 = 1.f / (1.f + g * (g + k));
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + g * a1 * v3;
        const float v2 = ic2 + g * v1;
        ic1 = 2.f * v1 - ic1;
        ic1 = ic1 / std::sqrt(1.f + 0.08f * ic1 * ic1); // soft limit: bounded self-oscillation
        ic2 = 2.f * v2 - ic2;
        lp = v2; bp = v1; hp = x - k * v1 - v2;
    }
};

inline float polyBlep(float t, float dt) {
    if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
    if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
    return 0.f;
}

class Engine {
public:
    float p[kParamCount];

    Engine() {
        const float def[kParamCount] = {
            0.66f, 0.5f, 0, 0, 0, 0, 0.0f, 1,
            0, 1800.f, 0.35f, 0.15f, 0.5f, 0.35f, 0.3f,
            0.005f, 0.35f, 0.2f, 0.25f,
            0.003f, 0.5f, 0.85f, 0.2f, 0.4f,
            5.f, 0, 0, 2, -6.f };
        std::copy(def, def + kParamCount, p);
    }

    void setSampleRate(float sr) {
        fs = sr; fEnv.sr = sr; aEnv.sr = sr;
        smoothCoef = 1.f - std::exp(-1.f / (0.004f * sr));
        reset();
    }
    void reset() {
        filt.reset(); fEnv.stage = aEnv.stage = ADSR::Idle; fEnv.value = aEnv.value = 0;
        held = 0; phase = subPhase = lfoPhase = 0; triState = 0; dcX = dcY = 0;
    }

    void noteOn(int note, int vel) {
        const bool wasHeld = held > 0;
        if (held < 16) stack[held++] = note;
        velocity = vel / 127.f;
        targetNote = (float)note;
        if (!wasHeld || p[kLegato] < 0.5f) {
            if (!wasHeld && aEnv.stage == ADSR::Idle) curNote = targetNote; // no glide from silence
            fEnv.gate(true); aEnv.gate(true);
        }
        if (p[kGlide] <= 0.0005f) curNote = targetNote;
    }
    void noteOff(int note) {
        int j = 0;
        for (int i = 0; i < held; ++i) if (stack[i] != note) stack[j++] = stack[i];
        held = j;
        if (held > 0) targetNote = (float)stack[held - 1];      // last-note priority
        else { fEnv.gate(false); aEnv.gate(false); }
    }
    void allNotesOff() { held = 0; fEnv.gate(false); aEnv.gate(false); }
    void pitchBend(int v14) { bend = (v14 - 8192) / 8192.f; }
    void modWheel(int v) { modWheelAmt = v / 127.f; }

    void render(float* outL, float* outR, uint32_t n) {
        fEnv.set(p[kFltAttack], p[kFltDecay], p[kFltSustain], p[kFltRelease]);
        aEnv.set(p[kAmpAttack], p[kAmpDecay], p[kAmpSustain], p[kAmpRelease]);
        const float glideCoef = p[kGlide] > 0.0005f ? 1.f - std::exp(-1.f / (p[kGlide] * 0.25f * fs)) : 1.f;
        const float k = 2.f - 2.04f * std::pow(p[kResonance], 0.6f);
        const float drive = 1.f + 8.f * p[kDrive] * p[kDrive];
        const float gain = std::pow(10.f, p[kVolume] / 20.f);
        const int mode = (int)(p[kFilterMode] + 0.5f);
        const float lfoInc = p[kLfoRate] / fs;
        const float vibDepth = 2.f * p[kLfoPitch] + 0.5f * modWheelAmt;
        const float ampVel = 1.f - p[kAmpVelAmt] * (1.f - velocity);

        for (uint32_t i = 0; i < n; ++i) {
            // smooth continuous params
            sCut += (p[kCutoff] - sCut) * smoothCoef;
            sShape += (p[kShape] - sShape) * smoothCoef;
            sPW += (p[kPulseWidth] - sPW) * smoothCoef;

            curNote += (targetNote - curNote) * glideCoef;

            lfoPhase += lfoInc; if (lfoPhase >= 1.f) lfoPhase -= 1.f;
            const float lfo = std::sin(2.f * kPi * lfoPhase);

            const float note = curNote + 12.f * p[kOctave] + p[kFine] * 0.01f
                             + bend * p[kBendRange] + lfo * vibDepth;
            const float freq = 440.f * std::pow(2.f, (note - 69.f) / 12.f);
            const float dt = std::min(freq / fs, 0.45f);

            float osc = oscillator(dt) ;

            // sub oscillator: square one octave down
            subPhase += dt * 0.5f; if (subPhase >= 1.f) subPhase -= 1.f;
            float sub = subPhase < 0.5f ? 1.f : -1.f;
            sub += polyBlep(subPhase, dt * 0.5f);
            float t2 = subPhase + 0.5f; if (t2 >= 1.f) t2 -= 1.f;
            sub -= polyBlep(t2, dt * 0.5f);

            rng = rng * 1664525u + 1013904223u;
            const float noise = (int32_t)rng * (1.f / 2147483648.f);

            float x = osc + p[kSubLevel] * sub + p[kNoiseLevel] * noise;

            // filter cutoff modulation in octaves
            const float fe = fEnv.process();
            const float velMod = 1.f - p[kFltVelAmt] * (1.f - velocity);
            float oct = std::log2(std::max(sCut, 20.f) / 440.f)
                      + p[kKeyTrack] * (note - 69.f) / 12.f
                      + p[kFltEnvAmt] * 6.f * fe * velMod
                      + p[kLfoCutoff] * 2.f * lfo;
            float fc = 440.f * std::pow(2.f, oct);
            fc = clampf(fc, 16.f, 0.45f * fs);
            const float g = std::tan(kPi * fc / fs);

            float lp, bp, hp;
            filt.process(std::tanh(x * drive) / std::sqrt(drive) * 1.2f, g, k, lp, bp, hp);
            float y = mode == 0 ? lp : (mode == 1 ? bp * std::min(1.f, std::fabs(k) + 0.35f) : hp);

            const float a = aEnv.process();
            y *= a * ampVel * gain * 0.5f;

            // DC blocker
            float o = y - dcX + 0.9995f * dcY; dcX = y; dcY = o;
            // soft-knee safety limiter above -0.9 dBFS (ceiling 0 dBFS)
            const float ao = std::fabs(o);
            if (ao > 0.9f) o = std::copysign(0.9f + 0.1f * std::tanh((ao - 0.9f) * 10.f), o);
            outL[i] = outR[i] = o;
        }
    }

private:
    float oscillator(float dt) {
        phase += dt; if (phase >= 1.f) phase -= 1.f;
        const float t = phase;
        // band-limited shapes
        float sine = std::sin(2.f * kPi * t);
        float saw = 2.f * t - 1.f - polyBlep(t, dt);
        const float pw = clampf(sPW, 0.05f, 0.95f);
        float sq = t < pw ? 1.f : -1.f;
        sq += polyBlep(t, dt);
        float t2 = t - pw; if (t2 < 0.f) t2 += 1.f;
        sq -= polyBlep(t2, dt);
        // triangle by leaky integration of a 50% square
        float sq50 = t < 0.5f ? 1.f : -1.f;
        sq50 += polyBlep(t, dt);
        float t3 = t + 0.5f; if (t3 >= 1.f) t3 -= 1.f;
        sq50 -= polyBlep(t3, dt);
        triState = dt * 4.f * sq50 + (1.f - 0.5f * dt) * triState;
        float tri = triState;

        // morph: 0 sine, 1/3 tri, 2/3 saw, 1 square
        const float m = clampf(sShape, 0.f, 1.f) * 3.f;
        if (m < 1.f) return sine + (tri - sine) * m;
        if (m < 2.f) return tri + (saw - tri) * (m - 1.f);
        return saw + (sq - saw) * (m - 2.f);
    }

    float fs = 48000.f, smoothCoef = 0.01f;
    float phase = 0, subPhase = 0, lfoPhase = 0, triState = 0;
    float curNote = 60, targetNote = 60, velocity = 1, bend = 0, modWheelAmt = 0;
    float sCut = 1800.f, sShape = 0.66f, sPW = 0.5f;
    float dcX = 0, dcY = 0;
    uint32_t rng = 22222;
    int stack[16] = {}; int held = 0;
    SVF filt; ADSR fEnv, aEnv;
};

} // namespace mono
