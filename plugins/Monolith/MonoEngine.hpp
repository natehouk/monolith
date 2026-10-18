// Monolith — original subtractive synthesizer engine (mono or 10-voice poly).
// Clean-room implementation: written from public, generic synthesis techniques
// (PolyBLEP oscillators, TPT state-variable filter, one-pole envelope segments,
// Paul Kellet's "economy" pink-noise filter). No third-party product code,
// assets or disassembly were used. License: MIT (see LICENSE).

#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace mono {

static constexpr float kPi = 3.14159265358979f;
inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }

enum Param : uint32_t {
    kShape = 0,     // 0..1  sine -> triangle -> saw -> pulse
    kPulseWidth,    // 0.05..0.95; with a PWM source it sets modulation depth
    kOctave,        // -2..+2
    kFine,          // -100..+100 cents
    kSubLevel,      // 0..1   square one octave down
    kNoiseLevel,    // 0..1
    kGlide,         // 0..2 s
    kLegato,        // 0/1   mono: don't retrigger envelopes on overlapping notes
    kFilterMode,    // 0 LP, 1 BP, 2 HP
    kCutoff,        // 20..20000 Hz
    kResonance,     // 0..1 (self-oscillates near 1)
    kDrive,         // 0..1
    kKeyTrack,      // 0..1
    kFltEnvAmt,     // -1..1 (+/-6 octaves)
    kFltVelAmt,     // 0..1
    kFltAttack, kFltDecay, kFltSustain, kFltRelease,   // "mod envelope"
    kAmpAttack, kAmpDecay, kAmpSustain, kAmpRelease,
    kAmpVelAmt,     // 0..1
    kLfoRate,       // 0.05..20 Hz (free mode)
    kLfoPitch,      // 0..1  vibrato depth, 24 * x^2 semitones
    kLfoCutoff,     // 0..1 (+/-2 octaves)
    kBendRange,     // 0..12 semitones
    kVolume,        // -36..+6 dB
    // ---- added in 1.3 (appended so existing automation and saved sessions keep their indices)
    kVoiceMode,     // 0 Mono, 1 Poly (10 voices)
    kHold,          // 0..5 s  envelope keeps sustaining this long after key-up
    kPwmSource,     // 0 Off, 1 LFO, 2 Mod Env, 3 Mod Env inverted
    kNoiseType,     // 0 White, 1 Pink
    kPitchEnvAmt,   // -1..1  mod envelope -> pitch, +/-24 semitones
    kLfoWave,       // 0 Sine, 1 Triangle, 2 Square
    kLfoBalance,    // 0..1  rising/falling ramp balance (0.5 symmetric)
    kLfoSync,       // 0/1   lock to host tempo and song position
    kLfoSyncRate,   // 0..10 index into kSyncBars
    kOscLevel,      // 0..1  main oscillator level (0 = noise/sub only)
    kParamCount
};

static constexpr int kMaxVoices = 10;
static constexpr int kNumSyncRates = 11;
inline float syncBars(int i) {
    static const float t[kNumSyncRates] = { 16, 8, 4, 2, 1, 1.f/2, 1.f/4, 1.f/8, 1.f/16, 1.f/32, 1.f/64 };
    return t[i < 0 ? 0 : (i >= kNumSyncRates ? kNumSyncRates - 1 : i)];
}

struct ADSR {
    enum Stage { Idle, Attack, Decay, Sustain, Release } stage = Idle;
    float value = 0.f, s = 0.7f, sr = 48000.f;
    float coefA = 0, coefD = 0, coefR = 0;
    static float coef(float t, float sr) { return 1.f - std::exp(-1.f / std::max(t * sr, 1.f)); }
    void set(float A, float D, float S, float R) {
        s = S; coefA = coef(A / 1.5f, sr); coefD = coef(D / 4.f, sr); coefR = coef(R / 4.f, sr);
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

// Topology-preserving-transform state-variable filter (Zavalishin), with a
// soft-limited integrator state for bounded, musical self-oscillation.
struct SVF {
    float ic1 = 0, ic2 = 0;
    void reset() { ic1 = ic2 = 0; }
    inline void process(float x, float g, float k, float& lp, float& bp, float& hp) {
        const float a1 = 1.f / (1.f + g * (g + k));
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + g * a1 * v3;
        const float v2 = ic2 + g * v1;
        ic1 = 2.f * v1 - ic1;
        ic1 = ic1 / std::sqrt(1.f + 0.08f * ic1 * ic1);
        ic2 = 2.f * v2 - ic2;
        lp = v2; bp = v1; hp = x - k * v1 - v2;
    }
};

inline float polyBlep(float t, float dt) {
    if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
    if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
    return 0.f;
}

struct Voice {
    int note = -1;
    bool keyDown = false;      // key physically held (poly)
    bool sustained = false;    // key released while sustain pedal down
    int holdLeft = -1;         // samples until envelopes release (-1 = not counting)
    float phase = 0, subPhase = 0, triState = 0;
    float curNote = 60, targetNote = 60, vel = 1;
    uint32_t age = 0;
    SVF filt; ADSR fEnv, aEnv;
    bool active() const { return aEnv.stage != ADSR::Idle; }
    void release() { fEnv.gate(false); aEnv.gate(false); holdLeft = -1; }
    void kill() { aEnv.stage = fEnv.stage = ADSR::Idle; aEnv.value = fEnv.value = 0; filt.reset();
                  note = -1; keyDown = sustained = false; holdLeft = -1; triState = 0; }
};

class Engine {
public:
    float p[kParamCount];

    Engine() {
        const float def[kParamCount] = {
            0.66f, 0.5f, 0, 0, 0, 0, 0.0f, 1,
            0, 1800.f, 0.35f, 0.15f, 0.5f, 0.35f, 0.3f,
            0.005f, 0.35f, 0.2f, 0.25f,
            0.003f, 0.5f, 0.85f, 0.2f, 0.4f,
            5.f, 0, 0, 2, -6.f,
            // 1.3
            0, 0, 0, 0, 0, 0, 0.5f, 0, 4, 1.f };
        std::copy(def, def + kParamCount, p);
    }

    void setSampleRate(float sr) {
        fs = sr;
        for (Voice& v : voices) { v.fEnv.sr = sr; v.aEnv.sr = sr; }
        smoothCoef = 1.f - std::exp(-1.f / (0.004f * sr));
        lfoSmooth = 1.f - std::exp(-1.f / (0.0015f * sr));
        reset();
    }
    void reset() {
        for (Voice& v : voices) v.kill();
        held = 0; lfoPhase = 0; lfoOut = 0; dcX = dcY = 0; pb0 = pb1 = pb2 = 0;
        sustainDown = false; lastPolyNote = -1;
    }

    // Host transport, once per block. barPos = position in bars at block start.
    void setTransport(bool valid, bool playing, double barPos, double bpm, double beatsPerBar) {
        tValid = valid; tPlaying = playing; tBarPos = barPos;
        tBpm = bpm > 1 ? bpm : 120.0; tBeatsPerBar = beatsPerBar > 0 ? beatsPerBar : 4.0;
    }

    // ---------------------------------------------------------------- MIDI
    void noteOn(int note, int vel) {
        checkMode();
        if (poly()) polyNoteOn(note, vel); else monoNoteOn(note, vel);
    }
    void noteOff(int note) {
        if (poly()) {
            for (Voice& v : voices)
                if (v.note == note && v.keyDown) { v.keyDown = false; keyUp(v); }
        } else {
            int j = 0;
            for (int i = 0; i < held; ++i) if (stack[i] != note) stack[j++] = stack[i];
            const bool removed = j != held;
            held = j;
            if (!removed) return;
            Voice& v = voices[0];
            if (held > 0) { v.targetNote = (float)stack[held - 1]; v.note = stack[held - 1]; }   // last-note priority
            else keyUp(v);
        }
    }
    void sustainPedal(bool down) {
        sustainDown = down;
        if (!down)
            for (Voice& v : voices)
                if (v.sustained) { v.sustained = false; startHold(v); }
    }
    void allNotesOff() {
        held = 0; sustainDown = false;
        for (Voice& v : voices) { v.keyDown = v.sustained = false; v.release(); }
    }
    void allSoundOff() { for (Voice& v : voices) v.kill(); held = 0; }
    void pitchBend(int v14) { bend = (v14 - 8192) / 8192.f; }
    void modWheel(int v) { modWheelAmt = v / 127.f; }

    int activeVoices() const { int n = 0; for (const Voice& v : voices) n += v.active(); return n; }

    // -------------------------------------------------------------- render
    void render(float* outL, float* outR, uint32_t n) {
        checkMode();
        for (Voice& v : voices) {
            v.fEnv.set(p[kFltAttack], p[kFltDecay], p[kFltSustain], p[kFltRelease]);
            v.aEnv.set(p[kAmpAttack], p[kAmpDecay], p[kAmpSustain], p[kAmpRelease]);
        }
        const float glideCoef = p[kGlide] > 0.0005f ? 1.f - std::exp(-1.f / (p[kGlide] * 0.25f * fs)) : 1.f;
        const int mode = (int)(p[kFilterMode] + 0.5f);
        const float vibDepth = 24.f * p[kLfoPitch] * p[kLfoPitch] + 0.5f * modWheelAmt;
        const int pwmSrc = (int)(p[kPwmSource] + 0.5f);
        const bool pink = p[kNoiseType] > 0.5f;
        const int lfoWave = (int)(p[kLfoWave] + 0.5f);
        const float bal = clampf(p[kLfoBalance], 0.02f, 0.98f);
        const float voiceGain = poly() ? 0.35f : 0.5f;

        // LFO increment (cycles per sample), free or tempo-synced
        const bool sync = p[kLfoSync] > 0.5f;
        const double cycleBars = syncBars((int)(p[kLfoSyncRate] + 0.5f));
        const double barsPerSample = tBpm / (60.0 * fs * tBeatsPerBar);
        const double lfoInc = sync ? barsPerSample / cycleBars : p[kLfoRate] / fs;
        if (sync && tValid && tPlaying) {               // lock phase to song position
            double ph = tBarPos / cycleBars; ph -= std::floor(ph);
            lfoPhase = ph;
        }

        for (uint32_t i = 0; i < n; ++i) {
            // smoothed globals ("no zipper noise")
            sCut   += (p[kCutoff] - sCut) * smoothCoef;
            sShape += (p[kShape] - sShape) * smoothCoef;
            sPW    += (p[kPulseWidth] - sPW) * smoothCoef;
            sReso  += (p[kResonance] - sReso) * smoothCoef;
            sDrive += (p[kDrive] - sDrive) * smoothCoef;
            sGain  += (std::pow(10.f, p[kVolume] / 20.f) - sGain) * smoothCoef;
            sOsc   += (p[kOscLevel] - sOsc) * smoothCoef;
            sSub   += (p[kSubLevel] - sSub) * smoothCoef;
            sNoise += (p[kNoiseLevel] - sNoise) * smoothCoef;

            const float k = 2.f - 2.04f * std::pow(clampf(sReso, 0.f, 1.f), 0.6f);
            const float drive = 1.f + 8.f * sDrive * sDrive;
            const float driveNorm = 1.2f / std::sqrt(drive);

            // LFO
            lfoPhase += lfoInc; if (lfoPhase >= 1.0) lfoPhase -= std::floor(lfoPhase);
            const float lfoRaw = lfoShape((float)lfoPhase, lfoWave, bal);
            lfoOut += (lfoRaw - lfoOut) * (lfoWave == 2 ? lfoSmooth : 1.f);
            const float lfo = lfoOut;

            // noise (shared by all voices)
            rng = rng * 1664525u + 1013904223u;
            const float white = (int32_t)rng * (1.f / 2147483648.f);
            float noise = white;
            if (pink) {
                pb0 = 0.99765f * pb0 + white * 0.0990460f;
                pb1 = 0.96300f * pb1 + white * 0.2965164f;
                pb2 = 0.57000f * pb2 + white * 1.0526913f;
                noise = (pb0 + pb1 + pb2 + white * 0.1848f) * 0.30f;
            }

            float sum = 0.f;
            for (Voice& v : voices) {
                if (!v.active()) continue;
                if (v.holdLeft > 0 && --v.holdLeft == 0) v.release();

                v.curNote += (v.targetNote - v.curNote) * glideCoef;
                const float fe = v.fEnv.process();
                const float a = v.aEnv.process();

                const float note = v.curNote + 12.f * p[kOctave] + p[kFine] * 0.01f
                                 + bend * p[kBendRange] + lfo * vibDepth
                                 + p[kPitchEnvAmt] * 24.f * fe;
                const float freq = 440.f * std::pow(2.f, (note - 69.f) / 12.f);
                const float dt = clampf(freq / fs, 1e-6f, 0.45f);

                float pw = sPW;
                if (pwmSrc != 0) {
                    const float depth = clampf((sPW - 0.05f) / 0.9f, 0.f, 1.f);
                    const float m = pwmSrc == 1 ? 0.5f + 0.5f * lfo : (pwmSrc == 2 ? fe : 1.f - fe);
                    pw = 0.5f - 0.45f * depth * m;
                }

                const float osc = oscillator(v, dt, clampf(pw, 0.05f, 0.95f));

                v.subPhase += dt * 0.5f; if (v.subPhase >= 1.f) v.subPhase -= 1.f;
                float sub = v.subPhase < 0.5f ? 1.f : -1.f;
                sub += polyBlep(v.subPhase, dt * 0.5f);
                float t2 = v.subPhase + 0.5f; if (t2 >= 1.f) t2 -= 1.f;
                sub -= polyBlep(t2, dt * 0.5f);

                const float x = sOsc * osc + sSub * sub + sNoise * noise;

                const float velMod = 1.f - p[kFltVelAmt] * (1.f - v.vel);
                const float oct = std::log2(std::max(sCut, 20.f) / 440.f)
                                + p[kKeyTrack] * (note - 69.f) / 12.f
                                + p[kFltEnvAmt] * 6.f * fe * velMod
                                + p[kLfoCutoff] * 2.f * lfo;
                const float fc = clampf(440.f * std::pow(2.f, oct), 16.f, 0.45f * fs);
                const float g = std::tan(kPi * fc / fs);

                float lp, bp, hp;
                v.filt.process(std::tanh(x * drive) * driveNorm, g, k, lp, bp, hp);
                float y = mode == 0 ? lp : (mode == 1 ? bp * std::min(1.f, std::fabs(k) + 0.35f) : hp);

                const float ampVel = 1.f - p[kAmpVelAmt] * (1.f - v.vel);
                sum += y * a * ampVel;
                if (!v.active()) { v.filt.reset(); v.note = -1; }
            }

            float y = sum * sGain * voiceGain;
            float o = y - dcX + 0.9995f * dcY; dcX = y; dcY = o;
            const float ao = std::fabs(o);                       // soft ceiling at 0 dBFS
            if (ao > 0.9f) o = std::copysign(0.9f + 0.1f * std::tanh((ao - 0.9f) * 10.f), o);
            outL[i] = outR[i] = o;
        }
    }

    static float lfoShape(float t, int wave, float bal) {
        if (wave == 2) return t < bal ? 1.f : -1.f;                       // square, balance = duty
        const float u = t < bal ? 0.5f * t / bal : 0.5f + 0.5f * (t - bal) / (1.f - bal);  // warped phase
        if (wave == 1) return u < 0.5f ? -1.f + 4.f * u : 3.f - 4.f * u;  // triangle (ramps)
        return -std::cos(2.f * kPi * u);                                   // sine
    }

private:
    bool poly() const { return p[kVoiceMode] > 0.5f; }

    void checkMode() {
        const bool pm = poly();
        if (pm != lastPoly) {          // switching modes: release everything cleanly
            lastPoly = pm; held = 0;
            for (Voice& v : voices) { v.keyDown = v.sustained = false; v.release(); }
        }
    }

    void startHold(Voice& v) {
        const int h = (int)(p[kHold] * fs);
        if (h > 0) v.holdLeft = h; else v.release();
    }
    void keyUp(Voice& v) {
        if (sustainDown) v.sustained = true; else startHold(v);
    }
    void gateOn(Voice& v) {
        v.holdLeft = -1; v.sustained = false;
        v.fEnv.gate(true); v.aEnv.gate(true);
    }

    void monoNoteOn(int note, int vel) {
        const bool wasHeld = held > 0;
        if (held < 16) stack[held++] = note;
        Voice& v = voices[0];
        const bool wasActive = v.active();
        v.note = note; v.targetNote = (float)note;
        if (!wasHeld || p[kLegato] < 0.5f) {
            if (!wasActive) v.curNote = (float)note;               // no glide from silence
            v.vel = vel / 127.f;
            gateOn(v);
        } else {
            v.holdLeft = -1; v.sustained = false;
        }
        if (p[kGlide] <= 0.0005f) v.curNote = v.targetNote;
    }

    void polyNoteOn(int note, int vel) {
        Voice* v = nullptr;
        for (Voice& x : voices) if (x.active() && x.note == note) { v = &x; break; }   // same key: reuse
        if (!v) for (Voice& x : voices) if (!x.active()) { v = &x; break; }
        if (!v) {                                     // steal: quietest released voice, else oldest
            float best = 2.f;
            for (Voice& x : voices)
                if (!x.keyDown && !x.sustained && x.aEnv.value < best) { best = x.aEnv.value; v = &x; }
            if (!v) { v = &voices[0]; for (Voice& x : voices) if (x.age < v->age) v = &x; }
        }
        const bool fresh = !v->active() || v->note != note;
        v->note = note; v->targetNote = (float)note;
        if (fresh) v->curNote = (p[kGlide] > 0.0005f && lastPolyNote >= 0) ? (float)lastPolyNote : (float)note;
        v->vel = vel / 127.f; v->keyDown = true; v->age = ++ageCounter;
        gateOn(*v);
        lastPolyNote = note;
    }

    float oscillator(Voice& v, float dt, float pw) {
        v.phase += dt; if (v.phase >= 1.f) v.phase -= 1.f;
        const float t = v.phase;
        const float sine = std::sin(2.f * kPi * t);
        const float saw = 2.f * t - 1.f - polyBlep(t, dt);
        float sq = t < pw ? 1.f : -1.f;
        sq += polyBlep(t, dt);
        float t2 = t - pw; if (t2 < 0.f) t2 += 1.f;
        sq -= polyBlep(t2, dt);
        float sq50 = t < 0.5f ? 1.f : -1.f;                 // triangle = leaky-integrated square
        sq50 += polyBlep(t, dt);
        float t3 = t + 0.5f; if (t3 >= 1.f) t3 -= 1.f;
        sq50 -= polyBlep(t3, dt);
        v.triState = dt * 4.f * sq50 + (1.f - 0.5f * dt) * v.triState;
        const float tri = v.triState;
        const float m = clampf(sShape, 0.f, 1.f) * 3.f;    // 0 sine, 1/3 tri, 2/3 saw, 1 pulse
        if (m < 1.f) return sine + (tri - sine) * m;
        if (m < 2.f) return tri + (saw - tri) * (m - 1.f);
        return saw + (sq - saw) * (m - 2.f);
    }

    Voice voices[kMaxVoices];
    float fs = 48000.f, smoothCoef = 0.01f, lfoSmooth = 0.01f;
    double lfoPhase = 0; float lfoOut = 0;
    float bend = 0, modWheelAmt = 0;
    float sCut = 1800.f, sShape = 0.66f, sPW = 0.5f, sReso = 0.35f, sDrive = 0.15f, sGain = 0.5f;
    float sOsc = 1.f, sSub = 0.f, sNoise = 0.f;
    float dcX = 0, dcY = 0, pb0 = 0, pb1 = 0, pb2 = 0;
    uint32_t rng = 22222, ageCounter = 0;
    int stack[16] = {}; int held = 0;
    int lastPolyNote = -1;
    bool sustainDown = false, lastPoly = false;
    bool tValid = false, tPlaying = false;
    double tBarPos = 0, tBpm = 120, tBeatsPerBar = 4;
};

} // namespace mono
