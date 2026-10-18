// Monolith — custom vector UI (DPF/NanoVG). Original design. License: MIT.
#include "DistrhoUI.hpp"
#include "ParamDefs.hpp"
#include "Presets.hpp"
#include "ResizeHandle.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <string>
#include <vector>

START_NAMESPACE_DISTRHO

using namespace mono;
using DGL_NAMESPACE::ResizeHandle;

static constexpr float W = 1060.f, H = 600.f;

// palette: basalt + one warm accent
#define C_BG        Color(0x0f, 0x10, 0x12)
#define C_PANEL     Color(0x18, 0x1a, 0x1d)
#define C_PANEL_HI  Color(0x22, 0x25, 0x29)
#define C_LINE      Color(0x2c, 0x30, 0x35)
#define C_TEXT      Color(0xe8, 0xe4, 0xdc)
#define C_DIM       Color(0x84, 0x88, 0x8e)
#define C_FAINT     Color(0x4a, 0x4e, 0x54)
#define C_ACCENT    Color(0xff, 0xa8, 0x3e)
#define C_ACCENT_A(a) Color(0xff, 0xa8, 0x3e, (a) / 255.f)

enum WType { Knob, Toggle, Segment };
struct Ctl { WType type; uint32_t param; float x, y, r; const char* label; };   // Segment: r = total width
struct Panel { float x, y, w, h; const char* title; };

static const Panel kPanels[] = {
    { 16,  64, 330, 262, "OSCILLATOR" },
    { 358, 64, 400, 262, "FILTER" },
    { 770, 64, 274, 262, "VOICE" },
    { 16, 338, 330, 246, "MOD ENVELOPE" },
    { 358,338, 400, 246, "AMP ENVELOPE" },
    { 770,338, 274, 246, "LFO" },
};

static const Ctl kCtls[] = {
    // oscillator
    { Segment, kPwmSource, 181, 172, 290, "" },
    { Knob, kShape,       60, 228, 22, "SHAPE" },
    { Knob, kPulseWidth, 128, 228, 17, "PW" },
    { Knob, kPitchEnvAmt,196, 228, 17, "PITCH ENV" },
    { Knob, kOctave,     258, 228, 15, "OCTAVE" },
    { Knob, kFine,       312, 228, 15, "FINE" },
    { Knob, kOscLevel,    60, 292, 14, "OSC" },
    { Knob, kSubLevel,   128, 292, 14, "SUB" },
    { Knob, kNoiseLevel, 196, 292, 14, "NOISE" },
    { Segment, kNoiseType, 284, 292, 96, "" },
    // filter
    { Segment, kFilterMode, 558, 172, 336, "" },
    { Knob, kCutoff,    410, 248, 28, "CUTOFF" },
    { Knob, kResonance, 486, 248, 20, "RESO" },
    { Knob, kDrive,     548, 248, 16, "DRIVE" },
    { Knob, kFltEnvAmt, 600, 248, 16, "ENV" },
    { Knob, kKeyTrack,  652, 248, 16, "KEY" },
    { Knob, kFltVelAmt, 704, 248, 16, "VEL" },
    // voice
    { Segment, kVoiceMode, 907, 102, 200, "" },
    { Knob, kVolume,    850, 182, 28, "VOLUME" },
    { Knob, kGlide,     962, 182, 20, "GLIDE" },
    { Knob, kBendRange, 850, 268, 15, "BEND" },
    { Toggle, kLegato,  962, 268, 0,  "LEGATO" },
    // mod envelope
    { Knob, kFltAttack,  76, 530, 17, "A" },
    { Knob, kFltDecay,  146, 530, 17, "D" },
    { Knob, kFltSustain,216, 530, 17, "S" },
    { Knob, kFltRelease,286, 530, 17, "R" },
    // amp envelope
    { Knob, kAmpAttack, 400, 530, 17, "A" },
    { Knob, kAmpDecay,  462, 530, 17, "D" },
    { Knob, kAmpSustain,524, 530, 17, "S" },
    { Knob, kAmpRelease,586, 530, 17, "R" },
    { Knob, kHold,      652, 530, 17, "HOLD" },
    { Knob, kAmpVelAmt, 714, 530, 17, "VEL" },
    // lfo
    { Segment, kLfoWave, 868, 452, 156, "" },
    { Toggle, kLfoSync, 1000, 452, 0, "SYNC" },
    { Knob, kLfoRate,    812, 530, 20, "RATE" },
    { Knob, kLfoBalance, 878, 530, 16, "BALANCE" },
    { Knob, kLfoPitch,   940, 530, 16, "VIBRATO" },
    { Knob, kLfoCutoff, 1002, 530, 16, "> CUTOFF" },
};
static constexpr int kNumCtls = sizeof(kCtls) / sizeof(kCtls[0]);

// short labels for segment switches
static const char* const* segLabels(uint32_t p, int& n) {
    static const char* const pwm[]   = { "PW FIXED", "PWM LFO", "PWM ENV", "PWM -ENV" };
    static const char* const noise[] = { "WHITE", "PINK" };
    static const char* const fmode[] = { "LOW PASS", "BAND PASS", "HIGH PASS" };
    static const char* const vmode[] = { "MONO", "POLY  10" };
    static const char* const wave[]  = { "SINE", "TRI", "SQUARE" };
    switch (p) {
    case kPwmSource:  n = 4; return pwm;
    case kNoiseType:  n = 2; return noise;
    case kFilterMode: n = 3; return fmode;
    case kVoiceMode:  n = 2; return vmode;
    case kLfoWave:    n = 3; return wave;
    default:          n = 0; return nullptr;
    }
}

static bool isTime(uint32_t p) {
    return p == kGlide || p == kHold
        || (p >= kFltAttack && p <= kAmpRelease && p != kFltSustain && p != kAmpSustain);
}

// header preset selector:  [◀][ name ▾ ][▶]
static constexpr float PS_X = 519.f, PS_Y = 20.f, PS_W = 272.f, PS_H = 28.f, PS_ARROW = 30.f;
// preset menu grid
static constexpr float PM_COLW = 150.f, PM_ROW = 24.f, PM_HEAD = 24.f, PM_PAD = 8.f;

class MonolithUI : public UI {
public:
    MonolithUI() : UI((uint)W, (uint)H), resizeHandle(this) {
        loadSharedResources();
        Engine def;
        for (uint32_t i = 0; i < kParamCount; ++i) { val[i] = def.p[i]; defv[i] = def.p[i]; }
        const double sf = getScaleFactor();
        if (sf != 1.0) setSize((uint)(W * sf), (uint)(H * sf));
        setGeometryConstraints((uint)(W * 0.7 * sf), (uint)(H * 0.7 * sf), true);
        if (isResizable()) resizeHandle.hide();
    }

protected:
    float uiScale() const { return (float)getWidth() / W; }

    // ---- value mapping -----------------------------------------------------
    static float toNorm(uint32_t p, float v) {
        const ParamDef& d = kDefs[p];
        if (d.hints & kHintLog) return std::log(v / d.min) / std::log(d.max / d.min);
        float n = (v - d.min) / (d.max - d.min);
        if (isTime(p)) n = std::sqrt(std::max(n, 0.f));
        return n;
    }
    static float fromNorm(uint32_t p, float n) {
        const ParamDef& d = kDefs[p];
        n = std::min(1.f, std::max(0.f, n));
        float v;
        if (d.hints & kHintLog) v = d.min * std::pow(d.max / d.min, n);
        else { if (isTime(p)) n = n * n; v = d.min + n * (d.max - d.min); }
        if (d.hints & kHintInteger) v = std::round(v);
        return v;
    }
    void setValue(uint32_t p, float v) {
        const ParamDef& d = kDefs[p];
        v = std::min(d.max, std::max(d.min, v));
        if (v == val[p]) return;
        val[p] = v; setParameterValue(p, v); repaint();
    }
    void commit(uint32_t p, float v) { editParameter(p, true); setValue(p, v); editParameter(p, false); }

    void parameterChanged(uint32_t i, float v) override {
        if (i < kParamCount && val[i] != v) { val[i] = v; repaint(); }
    }

    // the LFO rate knob shows the sync division when sync is on
    uint32_t effParam(const Ctl& c) const {
        if (c.param == kLfoRate && val[kLfoSync] > 0.5f) return kLfoSyncRate;
        return c.param;
    }
    const char* effLabel(const Ctl& c) const {
        if (c.param == kPulseWidth && val[kPwmSource] > 0.5f) return "PWM DEPTH";
        return c.label;
    }

    // ---- formatting --------------------------------------------------------
    void fmt(uint32_t p, char* b, size_t n) const {
        const float v = val[p];
        int ne = 0;
        if (const char* const* l = enumLabels(p, ne)) { snprintf(b, n, "%s", l[std::max(0, std::min(ne - 1, (int)std::round(v)))]); return; }
        switch (p) {
        case kCutoff: if (v >= 1000) snprintf(b, n, "%.2f kHz", v / 1000); else snprintf(b, n, "%.0f Hz", v); return;
        case kOctave: snprintf(b, n, "%+d oct", (int)v); return;
        case kFine: snprintf(b, n, "%+.0f ct", v); return;
        case kBendRange: snprintf(b, n, "%d st", (int)v); return;
        case kVolume: snprintf(b, n, "%+.1f dB", v); return;
        case kLfoRate: snprintf(b, n, v < 1 ? "%.2f Hz" : "%.1f Hz", v); return;
        case kFltEnvAmt: snprintf(b, n, "%+.0f%%", v * 100); return;
        case kPitchEnvAmt: snprintf(b, n, "%+.1f st", v * 24); return;
        case kLfoPitch: snprintf(b, n, "%.2f st", 24 * v * v); return;
        case kShape: {
            static const char* names[] = { "SINE", "TRI", "SAW", "PULSE" };
            const float m = v * 3.f; const int i = (int)std::round(m);
            if (std::fabs(m - i) < 0.06f) snprintf(b, n, "%s", names[i]);
            else snprintf(b, n, "%.0f%%", v * 100);
            return; }
        default: break;
        }
        if (isTime(p)) { if (v < 1) snprintf(b, n, "%.0f ms", v * 1000); else snprintf(b, n, "%.2f s", v); return; }
        snprintf(b, n, "%.0f%%", v * 100);
    }

    // typed entry, in the same units the value display uses
    bool parse(uint32_t p, const std::string& in, float& out) const {
        std::string s; for (char ch : in) if (!std::isspace((unsigned char)ch)) s += (char)std::tolower((unsigned char)ch);
        if (s.empty()) return false;
        int ne = 0;
        if (const char* const* l = enumLabels(p, ne)) {
            for (int i = 0; i < ne; ++i) {
                std::string lab; for (const char* c = l[i]; *c; ++c) if (*c != ' ') lab += (char)std::tolower((unsigned char)*c);
                if (lab.compare(0, s.size(), s) == 0) { out = (float)i; return true; }
            }
            return false;
        }
        if (p == kShape) {
            static const char* names[] = { "sine", "tri", "saw", "pulse" };
            for (int i = 0; i < 4; ++i) if (std::string(names[i]).compare(0, s.size(), s) == 0) { out = i / 3.f; return true; }
        }
        char* end = nullptr;
        double x = std::strtod(s.c_str(), &end);
        if (end == s.c_str()) return false;
        std::string suf(end);
        if (suf == "k" || suf == "khz") x *= 1000.0;
        if (p == kCutoff || p == kLfoRate || p == kVolume || p == kOctave || p == kBendRange || p == kFine) { out = (float)x; return true; }
        if (isTime(p)) { out = (float)(suf == "ms" ? x / 1000.0 : x); return true; }
        if (p == kPitchEnvAmt) { out = (float)(x / 24.0); return true; }
        if (p == kLfoPitch) { out = (float)std::sqrt(std::max(0.0, x / 24.0)); return true; }
        out = (float)(x / 100.0);   // percentages
        return true;
    }

    // ---- drawing -----------------------------------------------------------
    void onNanoDisplay() override {
        const float s = uiScale();
        save();
        scale(s, s);
        beginPath(); rect(0, 0, W, H); fillColor(C_BG); fill();

        fontFace(NANOVG_DEJAVU_SANS_TTF);
        fontSize(22); textLetterSpacing(9); fillColor(C_TEXT);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE); text(44, 34, "MONOLITH", nullptr);
        fontSize(10); textLetterSpacing(2.5f); fillColor(C_DIM);
        text(252, 35, "SUBTRACTIVE SYNTHESIZER", nullptr);
        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); text(W - 22, 35, "EPSILON AUDIO  ·  1.3", nullptr);
        beginPath(); rect(20, 21, 10, 26); fillColor(C_ACCENT); fill();

        for (const Panel& p : kPanels) drawPanel(p);

        drawOscScope(36, 92, 290, 60);
        drawFilterPlot(378, 92, 360, 60);
        drawEnv(36, 366, 290, 112, kFltAttack, C_ACCENT);
        drawEnv(378, 366, 360, 112, kAmpAttack, C_TEXT);
        drawLfo(790, 366, 234, 60);

        for (int i = 0; i < kNumCtls; ++i) drawCtl(kCtls[i], i == active || i == hover);
        drawPresetBar();
        if (editing >= 0) drawEditBox();
        if (menuOpen) drawPresetMenu();
        restore();
    }

    void drawPanel(const Panel& p) {
        beginPath(); roundedRect(p.x, p.y, p.w, p.h, 6); fillColor(C_PANEL); fill();
        strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(9.5f); textLetterSpacing(2.2f); fillColor(C_DIM);
        textAlign(ALIGN_LEFT | ALIGN_TOP); text(p.x + 14, p.y + 10, p.title, nullptr);
    }

    void displayBox(float x, float y, float w, float h) {
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(Color(0x0b, 0x0c, 0x0d)); fill();
        strokeColor(C_LINE); strokeWidth(1); stroke();
        beginPath();
        for (int i = 1; i < 4; ++i) { moveTo(x + w * i / 4, y + 2); lineTo(x + w * i / 4, y + h - 2); }
        moveTo(x + 2, y + h / 2); lineTo(x + w - 2, y + h / 2);
        strokeColor(Color(255, 255, 255, 10 / 255.f)); stroke();
    }

    static float morph(float t, float shape, float pw) {
        const float sine = std::sin(2 * 3.14159265f * t);
        const float tri = t < 0.25f ? 4 * t : (t < 0.75f ? 2 - 4 * t : 4 * t - 4);
        const float saw = 2 * t - 1;
        const float sq = t < pw ? 1.f : -1.f;
        const float m = shape * 3;
        if (m < 1) return sine + (tri - sine) * m;
        if (m < 2) return tri + (saw - tri) * (m - 1);
        return saw + (sq - saw) * (m - 2);
    }

    void drawOscScope(float x, float y, float w, float h) {
        displayBox(x, y, w, h);
        const int N = 260; const float cy = y + h / 2, amp = h * 0.36f;
        const float pw = val[kPwmSource] > 0.5f ? 0.5f : val[kPulseWidth];
        uint32_t r = 12345;
        const float norm = std::max(1.f, val[kOscLevel] + val[kSubLevel] + val[kNoiseLevel] * 0.6f);
        beginPath();
        for (int i = 0; i <= N; ++i) {
            const float t = std::fmod(2.f * i / N, 1.f);
            float s = val[kOscLevel] * morph(t, val[kShape], pw);
            const float ts = (float)i / N; s += val[kSubLevel] * (ts < 0.5f ? 1.f : -1.f);
            r = r * 1664525u + 1013904223u; s += val[kNoiseLevel] * 0.6f * ((int32_t)r * (1.f / 2147483648.f));
            s /= norm;
            const float px = x + 6 + (w - 12) * i / N, py = cy - s * amp;
            if (i == 0) moveTo(px, py); else lineTo(px, py);
        }
        strokeColor(C_ACCENT); strokeWidth(1.6f); lineJoin(ROUND); stroke();
    }

    void drawFilterPlot(float x, float y, float w, float h) {
        displayBox(x, y, w, h);
        const float k = std::max(0.06f, 2.f - 2.04f * std::pow(val[kResonance], 0.6f));
        const int mode = (int)(val[kFilterMode] + 0.5f);
        const float fc = val[kCutoff];
        const int N = 180;
        std::vector<float> ys(N + 1);
        for (int i = 0; i <= N; ++i) {
            const float f = 20.f * std::pow(1000.f, (float)i / N);
            const float wn = f / fc, re = 1 - wn * wn, im = k * wn;
            const float den = std::sqrt(re * re + im * im);
            const float mag = mode == 0 ? 1.f / den : (mode == 1 ? wn / den * std::min(1.f, k + 0.35f) : wn * wn / den);
            float db = 20 * std::log10(std::max(mag, 1e-4f));
            db = std::min(23.f, std::max(-29.f, db));
            ys[i] = y + h * (1 - (db + 30.f) / 54.f);
        }
        beginPath(); moveTo(x + 2, y + h - 1);
        for (int i = 0; i <= N; ++i) lineTo(x + 2 + (w - 4) * i / N, ys[i]);
        lineTo(x + w - 2, y + h - 1); closePath();
        fillPaint(linearGradient(x, y, x, y + h, C_ACCENT_A(70), C_ACCENT_A(0))); fill();
        beginPath();
        for (int i = 0; i <= N; ++i) { const float px = x + 2 + (w - 4) * i / N; if (!i) moveTo(px, ys[i]); else lineTo(px, ys[i]); }
        strokeColor(C_ACCENT); strokeWidth(1.6f); stroke();
        const float cx = x + 2 + (w - 4) * std::log(fc / 20.f) / std::log(1000.f);
        beginPath(); moveTo(cx, y + 3); lineTo(cx, y + h - 3); strokeColor(Color(255, 255, 255, 40 / 255.f)); stroke();
    }

    void drawEnv(float x, float y, float w, float h, uint32_t base, Color col) {
        displayBox(x, y, w, h);
        const float a = val[base], d = val[base + 1], s = val[base + 2], r = val[base + 3];
        const float hold = 0.6f + val[kHold];
        const float total = std::max(a + d + hold + r, 0.5f);
        const float sx = (w - 16) / total;
        const float x0 = x + 8, yb = y + h - 8, yt = y + 8, hh = yb - yt;
        if (val[kHold] > 0.001f) {           // shade the hold region (after key-up)
            const float hx0 = x0 + (a + d + 0.6f) * sx, hx1 = x0 + (a + d + hold) * sx;
            beginPath(); rect(hx0, yt, hx1 - hx0, hh); fillColor(C_ACCENT_A(22)); fill();
        }
        beginPath(); moveTo(x0, yb);
        lineTo(x0 + a * sx, yt);
        for (int i = 1; i <= 24; ++i) {
            const float t = (float)i / 24, lvl = s + (1 - s) * std::exp(-4 * t);
            lineTo(x0 + (a + d * t) * sx, yb - hh * lvl);
        }
        lineTo(x0 + (a + d + hold) * sx, yb - hh * s);
        for (int i = 1; i <= 24; ++i) {
            const float t = (float)i / 24, lvl = s * std::exp(-4 * t) * (1 - t * 0.02f);
            lineTo(x0 + (a + d + hold + r * t) * sx, yb - hh * lvl);
        }
        strokeColor(col); strokeWidth(1.6f); lineJoin(ROUND); stroke();
    }

    void drawLfo(float x, float y, float w, float h) {
        displayBox(x, y, w, h);
        const int wave = (int)(val[kLfoWave] + 0.5f);
        const float bal = std::min(0.98f, std::max(0.02f, val[kLfoBalance]));
        const float cycles = val[kLfoSync] > 0.5f ? 3.f : std::max(0.5f, std::min(10.f, val[kLfoRate] * 0.6f));
        beginPath();
        for (int i = 0; i <= 200; ++i) {
            const float t = (float)i / 200, ph = std::fmod(t * cycles, 1.f);
            const float py = y + h / 2 - Engine::lfoShape(ph, wave, bal) * h * 0.34f;
            if (!i) moveTo(x + 4 + (w - 8) * t, py); else lineTo(x + 4 + (w - 8) * t, py);
        }
        strokeColor(C_ACCENT); strokeWidth(1.5f); stroke();
        if (val[kLfoSync] > 0.5f) {
            fontSize(8); textLetterSpacing(1.5f); textAlign(ALIGN_RIGHT | ALIGN_TOP); fillColor(C_DIM);
            char b[24]; fmt(kLfoSyncRate, b, sizeof b); text(x + w - 6, y + 4, b, nullptr);
        }
    }

    void drawCtl(const Ctl& c, bool hot) {
        char buf[32];
        if (c.type == Knob) {
            const uint32_t p = effParam(c);
            const float n = toNorm(p, val[p]);
            const float a0 = 0.75f * 3.14159265f, a1 = 2.25f * 3.14159265f;
            const bool bipolar = kDefs[p].min < 0 && p != kVolume;
            const float ang = a0 + (a1 - a0) * n;
            beginPath(); arc(c.x, c.y, c.r, a0, a1, CW);
            strokeColor(C_LINE); strokeWidth(c.r > 20 ? 4.f : 3.f); lineCap(ROUND); stroke();
            const float from = bipolar ? (a0 + a1) / 2 : a0;
            if (std::fabs(ang - from) > 0.001f) {
                beginPath(); arc(c.x, c.y, c.r, std::min(from, ang), std::max(from, ang), CW);
                strokeColor(C_ACCENT); stroke();
            }
            const float cr = c.r - (c.r > 20 ? 8 : 6);
            beginPath(); circle(c.x, c.y, cr);
            fillPaint(radialGradient(c.x - cr * 0.3f, c.y - cr * 0.4f, 0, cr * 1.4f,
                                     hot ? Color(0x3a, 0x3e, 0x44) : C_PANEL_HI, Color(0x14, 0x15, 0x17)));
            fill(); strokeColor(Color(255, 255, 255, 18 / 255.f)); strokeWidth(1); stroke();
            beginPath();
            moveTo(c.x + std::cos(ang) * cr * 0.35f, c.y + std::sin(ang) * cr * 0.35f);
            lineTo(c.x + std::cos(ang) * cr * 0.9f, c.y + std::sin(ang) * cr * 0.9f);
            strokeColor(C_TEXT); strokeWidth(2); lineCap(ROUND); stroke();
            fontSize(9); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_TOP);
            if (hot && editing < 0) { fmt(p, buf, sizeof buf); fillColor(C_ACCENT); text(c.x, c.y + c.r + 6, buf, nullptr); }
            else { fillColor(C_DIM); text(c.x, c.y + c.r + 6, effLabel(c), nullptr); }
        } else if (c.type == Toggle) {
            const bool on = val[c.param] > 0.5f;
            const bool dim = c.param == kLegato && val[kVoiceMode] > 0.5f;    // legato only applies in mono
            const float w = 40, h = 20, x = c.x - w / 2, y = c.y - h / 2;
            beginPath(); roundedRect(x, y, w, h, h / 2); fillColor(on && !dim ? C_ACCENT_A(60) : C_PANEL_HI); fill();
            strokeColor(on && !dim ? C_ACCENT : C_LINE); strokeWidth(1); stroke();
            beginPath(); circle(on ? x + w - h / 2 : x + h / 2, c.y, 7); fillColor(dim ? C_FAINT : (on ? C_ACCENT : C_DIM)); fill();
            fontSize(9); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_TOP);
            fillColor(dim ? C_FAINT : C_DIM); text(c.x, c.y + 16, c.label, nullptr);
        } else {
            int n = 0; const char* const* names = segLabels(c.param, n);
            const int cur = (int)(val[c.param] + 0.5f);
            const float sw = c.r / n, sh = 22, x0 = c.x - c.r / 2;
            for (int i = 0; i < n; ++i) {
                const float x = x0 + sw * i;
                beginPath(); roundedRect(x + 1, c.y - sh / 2, sw - 2, sh, 4);
                fillColor(i == cur ? C_ACCENT : C_PANEL_HI); fill();
                fontSize(9); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
                fillColor(i == cur ? C_BG : C_DIM); text(x + sw / 2, c.y + 0.5f, names[i], nullptr);
            }
        }
    }

    void drawEditBox() {
        const Ctl& c = kCtls[editing];
        const float w = 84, h = 20, x = c.x - w / 2, y = c.y + c.r + 3;
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(Color(0x0b, 0x0c, 0x0d)); fill();
        strokeColor(C_ACCENT); strokeWidth(1); stroke();
        fontSize(11); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT);
        const std::string shown = editBuf + "|";
        text(c.x, y + h / 2 + 0.5f, shown.c_str(), nullptr);
    }

    // ---- presets ------------------------------------------------------------
    bool matches(uint32_t idx) const {
        const float* v = presets()[idx].v;
        for (uint32_t i = 0; i < kParamCount; ++i) {
            const float tol = 1e-4f * std::max(1.f, std::fabs(v[i]));
            if (std::fabs(val[i] - v[i]) > tol) return false;
        }
        return true;
    }
    int exactMatch() const {
        for (uint32_t i = 0; i < kNumPresets; ++i) if (matches(i)) return (int)i;
        return -1;
    }
    void applyPreset(int idx) {
        idx = (idx % (int)kNumPresets + (int)kNumPresets) % (int)kNumPresets;
        const float* v = presets()[idx].v;
        for (uint32_t i = 0; i < kParamCount; ++i) if (val[i] != v[i]) commit(i, v[i]);
        curPreset = idx; repaint();
    }
    void programLoaded(uint32_t index) override { curPreset = (int)index; repaint(); }
    int stepBase() const { return curPreset >= 0 ? curPreset : std::max(0, exactMatch()); }

    void drawPresetBar() {
        const float x = PS_X, y = PS_Y, w = PS_W, h = PS_H, a = PS_ARROW;
        beginPath(); roundedRect(x, y, w, h, 5); fillColor(C_PANEL); fill();
        strokeColor(menuOpen ? C_ACCENT : C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(x + a, y + 5); lineTo(x + a, y + h - 5);
        moveTo(x + w - a, y + 5); lineTo(x + w - a, y + h - 5);
        strokeColor(C_LINE); stroke();
        for (int side = 0; side < 2; ++side) {
            const bool hot = presetHover == (side == 0 ? 0 : 2);
            const float cx = side == 0 ? x + a / 2 : x + w - a / 2, cy = y + h / 2, d = side == 0 ? -1.f : 1.f;
            beginPath(); moveTo(cx - 3 * d, cy - 5); lineTo(cx + 3 * d, cy); lineTo(cx - 3 * d, cy + 5); closePath();
            fillColor(hot ? C_ACCENT : C_DIM); fill();
        }
        char buf[64];
        int shown = curPreset; bool modified = false;
        if (shown < 0 || !matches((uint32_t)shown)) {
            const int m = exactMatch();
            if (m >= 0) shown = m; else modified = true;
        }
        if (shown < 0) snprintf(buf, sizeof buf, "Custom");
        else snprintf(buf, sizeof buf, modified ? "%s *" : "%s", presets()[shown].name);
        fontSize(12); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        fillColor(presetHover == 1 || menuOpen ? C_TEXT : Color(0xd0, 0xcc, 0xc4));
        text(x + w / 2 - 6, y + h / 2 + 0.5f, buf, nullptr);
        const float cx = x + w - a - 14, cy = y + h / 2;
        beginPath(); moveTo(cx - 4, cy - 2); lineTo(cx + 4, cy - 2); lineTo(cx, cy + 3); closePath();
        fillColor(C_DIM); fill();
    }

    // menu geometry: Init row on top, then one column per category
    struct MenuRect { float x, y, w, h; };
    void menuLayout(float& mx, float& my, float& mw, float& mh, std::vector<MenuRect>& items) const {
        mw = PM_COLW * kNumCategories + 2 * PM_PAD;
        mx = std::min(W - mw - 10, std::max(10.f, PS_X + PS_W / 2 - mw / 2));
        my = PS_Y + PS_H + 4;
        items.assign(kNumPresets, MenuRect{0, 0, 0, 0});
        int maxRows = 0;
        int rowsIn[kNumCategories] = {};
        for (uint32_t i = 0; i < kNumPresets; ++i) {
            const char* cat = presets()[i].category;
            if (std::strcmp(cat, "Init") == 0) { items[i] = { mx + PM_PAD, my + PM_PAD, PM_COLW - 4, PM_ROW }; continue; }
            for (int c = 0; c < kNumCategories; ++c)
                if (std::strcmp(cat, kCategories[c]) == 0) {
                    const int r = rowsIn[c]++;
                    items[i] = { mx + PM_PAD + c * PM_COLW, my + PM_PAD + PM_ROW + 6 + PM_HEAD + r * PM_ROW, PM_COLW - 4, PM_ROW };
                    maxRows = std::max(maxRows, rowsIn[c]);
                }
        }
        mh = PM_PAD + PM_ROW + 6 + PM_HEAD + maxRows * PM_ROW + PM_PAD;
    }

    void drawPresetMenu() {
        float mx, my, mw, mh; std::vector<MenuRect> items; menuLayout(mx, my, mw, mh, items);
        beginPath(); rect(mx - 12, my - 4, mw + 24, mh + 20);
        fillPaint(boxGradient(mx, my + 2, mw, mh, 6, 14, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(mx, my, mw, mh, 6); fillColor(C_PANEL_HI); fill();
        strokeColor(C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(mx + PM_PAD, my + PM_PAD + PM_ROW + 3); lineTo(mx + mw - PM_PAD, my + PM_PAD + PM_ROW + 3);
        strokeColor(C_LINE); stroke();
        fontSize(8.5f); textLetterSpacing(2.f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_ACCENT);
        for (int c = 0; c < kNumCategories; ++c) {
            std::string up; for (const char* ch = kCategories[c]; *ch; ++ch) up += (char)std::toupper((unsigned char)*ch);
            text(mx + PM_PAD + c * PM_COLW + 12, my + PM_PAD + PM_ROW + 6 + PM_HEAD / 2, up.c_str(), nullptr);
        }
        const int cur = curPreset >= 0 ? curPreset : exactMatch();
        for (uint32_t i = 0; i < kNumPresets; ++i) {
            const MenuRect& r = items[i];
            if ((int)i == menuHover) { beginPath(); roundedRect(r.x, r.y, r.w, r.h, 4); fillColor(C_ACCENT_A(40)); fill(); }
            if ((int)i == cur) { beginPath(); circle(r.x + 6, r.y + r.h / 2, 2.5f); fillColor(C_ACCENT); fill(); }
            fontSize(11.5f); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
            fillColor((int)i == menuHover || (int)i == cur ? C_TEXT : C_DIM);
            text(r.x + 12, r.y + r.h / 2 + 0.5f, presets()[i].name, nullptr);
        }
    }
    int menuHit(float x, float y) const {
        float mx, my, mw, mh; std::vector<MenuRect> items; menuLayout(mx, my, mw, mh, items);
        for (uint32_t i = 0; i < kNumPresets; ++i) {
            const MenuRect& r = items[i];
            if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) return (int)i;
        }
        return -1;
    }
    int presetBarHit(float x, float y) const {
        if (y < PS_Y || y > PS_Y + PS_H || x < PS_X || x > PS_X + PS_W) return -1;
        if (x < PS_X + PS_ARROW) return 0;
        if (x > PS_X + PS_W - PS_ARROW) return 2;
        return 1;
    }

    // ---- interaction ------------------------------------------------------
    int hit(float x, float y, int* seg = nullptr) const {
        for (int i = 0; i < kNumCtls; ++i) {
            const Ctl& c = kCtls[i];
            if (c.type == Knob) {
                if ((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y) <= (c.r + 6) * (c.r + 6)) return i;
            } else if (c.type == Toggle) {
                if (std::fabs(x - c.x) <= 24 && std::fabs(y - c.y) <= 14) return i;
            } else {
                int n = 0; segLabels(c.param, n);
                const float x0 = c.x - c.r / 2;
                if (x >= x0 && x < x0 + c.r && std::fabs(y - c.y) <= 12) { if (seg) *seg = (int)((x - x0) / (c.r / n)); return i; }
            }
        }
        return -1;
    }

    void endEdit(bool apply) {
        if (editing < 0) return;
        const uint32_t p = effParam(kCtls[editing]);
        float v;
        if (apply && parse(p, editBuf, v)) commit(p, v);
        editing = -1; editBuf.clear(); repaint();
    }

    bool onKeyboard(const KeyboardEvent& ev) override {
        if (editing < 0 || !ev.press) return false;
        if (ev.key == kKeyEnter) { endEdit(true); return true; }
        if (ev.key == kKeyEscape) { endEdit(false); return true; }
        if (ev.key == kKeyBackspace || ev.key == kKeyDelete) { if (!editBuf.empty()) editBuf.pop_back(); repaint(); return true; }
        if (ev.key >= 32 && ev.key < 127 && editBuf.size() < 12) { editBuf += (char)ev.key; repaint(); return true; }
        return true;
    }

    bool onMouse(const MouseEvent& ev) override {
        const float s = uiScale();
        const float x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (ev.button != 1) return false;
        if (ev.press && editing >= 0) endEdit(true);
        if (ev.press && menuOpen) {
            const int m = menuHit(x, y);
            menuOpen = false; menuHover = -1;
            if (m >= 0) applyPreset(m);
            repaint();
            return true;
        }
        if (ev.press) {
            const int pb = presetBarHit(x, y);
            if (pb == 0) { applyPreset(stepBase() - 1); return true; }
            if (pb == 2) { applyPreset(stepBase() + 1); return true; }
            if (pb == 1) { menuOpen = true; menuHover = -1; repaint(); return true; }

            int seg = 0; const int i = hit(x, y, &seg);
            if (i < 0) return false;
            const Ctl& c = kCtls[i];
            const uint32_t p = effParam(c);
            if (c.type == Toggle) { commit(p, val[p] > 0.5f ? 0.f : 1.f); return true; }
            if (c.type == Segment) { commit(p, (float)seg); return true; }
            if (ev.mod & (kModifierControl | kModifierSuper)) { commit(p, defv[p]); return true; }   // Cmd/Ctrl-click: reset
            if (i == lastClick && ev.time - lastClickTime < 350) {                                  // double-click: type a value
                lastClick = -1; editing = i;
                char b[32]; fmt(p, b, sizeof b); editBuf = b;
                const size_t sp = editBuf.find(' '); if (sp != std::string::npos) editBuf.resize(sp);
                if (!editBuf.empty() && editBuf.back() == '%') editBuf.pop_back();
                if (p == kCutoff && val[p] >= 1000) { char kb[16]; snprintf(kb, sizeof kb, "%.2fk", val[p] / 1000); editBuf = kb; }
                if (isTime(p) && val[p] < 1) { char tb[16]; snprintf(tb, sizeof tb, "%.0fms", val[p] * 1000); editBuf = tb; }
                repaint(); return true;
            }
            lastClick = i; lastClickTime = ev.time;
            active = i; dragY = y; dragNorm = toNorm(p, val[p]);
            editParameter(p, true); repaint();
            return true;
        } else if (active >= 0) {
            editParameter(effParam(kCtls[active]), false); active = -1; repaint(); return true;
        }
        return false;
    }

    bool onMotion(const MotionEvent& ev) override {
        const float s = uiScale();
        const float x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (active >= 0) {
            const uint32_t p = effParam(kCtls[active]);
            float sens = (ev.mod & kModifierShift) ? 1500.f : 220.f;
            if (kDefs[p].hints & kHintInteger) sens = std::max(60.f, 18.f * (kDefs[p].max - kDefs[p].min));
            dragNorm += (dragY - y) / sens; dragY = y;
            dragNorm = std::min(1.f, std::max(0.f, dragNorm));
            setValue(p, fromNorm(p, dragNorm));
            return true;
        }
        if (menuOpen) {
            const int m = menuHit(x, y);
            if (m != menuHover) { menuHover = m; repaint(); }
            return true;
        }
        const int pb = presetBarHit(x, y);
        if (pb != presetHover) { presetHover = pb; repaint(); }
        const int h = pb >= 0 ? -1 : hit(x, y);
        if (h != hover) { hover = h; repaint(); }
        return false;
    }

    bool onScroll(const ScrollEvent& ev) override {
        const float s = uiScale();
        const float sx = ev.pos.getX() / s, sy = ev.pos.getY() / s;
        if (menuOpen) return true;
        if (presetBarHit(sx, sy) >= 0) { applyPreset(stepBase() + (ev.delta.getY() > 0 ? -1 : 1)); return true; }
        const int i = hit(sx, sy);
        if (i < 0 || kCtls[i].type != Knob) return false;
        const uint32_t p = effParam(kCtls[i]);
        float step = (ev.mod & kModifierShift) ? 0.004f : 0.025f;
        if (kDefs[p].hints & kHintInteger) step = 1.f / (kDefs[p].max - kDefs[p].min);
        commit(p, fromNorm(p, toNorm(p, val[p]) + (ev.delta.getY() > 0 ? step : -step)));
        return true;
    }

private:
    float val[kParamCount], defv[kParamCount];
    int active = -1, hover = -1, lastClick = -1;
    uint lastClickTime = 0;
    float dragY = 0, dragNorm = 0;
    int curPreset = 0, presetHover = -1, menuHover = -1;
    bool menuOpen = false;
    int editing = -1;
    std::string editBuf;
    ResizeHandle resizeHandle;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonolithUI)
};

UI* createUI() { return new MonolithUI(); }

END_NAMESPACE_DISTRHO
