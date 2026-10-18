// Monolith — custom vector UI (DPF/NanoVG). Original design. License: ISC.
#include "DistrhoUI.hpp"
#include "ParamDefs.hpp"
#include <cmath>
#include <cstdio>
#include <vector>

START_NAMESPACE_DISTRHO

using namespace mono;

static constexpr float W = 980.f, H = 520.f;

// palette: basalt + one warm accent
#define C_BG        Color(0x0f, 0x10, 0x12)
#define C_PANEL     Color(0x18, 0x1a, 0x1d)
#define C_PANEL_HI  Color(0x22, 0x25, 0x29)
#define C_LINE      Color(0x2c, 0x30, 0x35)
#define C_TEXT      Color(0xe8, 0xe4, 0xdc)
#define C_DIM       Color(0x84, 0x88, 0x8e)
#define C_ACCENT    Color(0xff, 0xa8, 0x3e)
#define C_ACCENT_A(a) Color(0xff, 0xa8, 0x3e, (a) / 255.f)

enum WType { Knob, Toggle, Segment };

struct Ctl {
    WType type; uint32_t param; float x, y, r; const char* label;
};

struct Panel { float x, y, w, h; const char* title; };

static const Panel kPanels[] = {
    { 16,  64, 300, 236, "OSCILLATOR" },
    { 328, 64, 400, 236, "FILTER" },
    { 740, 64, 224, 236, "VOICE" },
    { 16, 312, 300, 192, "FILTER ENVELOPE" },
    { 328,312, 400, 192, "AMP ENVELOPE" },
    { 740,312, 224, 192, "LFO" },
};

static const Ctl kCtls[] = {
    // oscillator
    { Knob, kShape,      66, 196, 24, "SHAPE" },
    { Knob, kPulseWidth,166, 196, 20, "PW" },
    { Knob, kSubLevel,  266, 196, 20, "SUB" },
    { Knob, kOctave,     66, 262, 16, "OCTAVE" },
    { Knob, kFine,      166, 262, 16, "FINE" },
    { Knob, kNoiseLevel,266, 262, 16, "NOISE" },
    // filter
    { Segment, kFilterMode, 528, 176, 0, "" },
    { Knob, kCutoff,    382, 236, 30, "CUTOFF" },
    { Knob, kResonance, 458, 236, 22, "RESO" },
    { Knob, kDrive,     524, 236, 18, "DRIVE" },
    { Knob, kFltEnvAmt, 582, 236, 18, "ENV" },
    { Knob, kKeyTrack,  636, 236, 16, "KEY" },
    { Knob, kFltVelAmt, 686, 236, 16, "VEL" },
    // voice
    { Knob, kVolume,    852, 148, 30, "VOLUME" },
    { Knob, kGlide,     792, 240, 18, "GLIDE" },
    { Knob, kBendRange, 852, 240, 16, "BEND" },
    { Toggle, kLegato,  912, 240, 0,  "LEGATO" },
    // filter env
    { Knob, kFltAttack,  56, 456, 17, "A" },
    { Knob, kFltDecay,  126, 456, 17, "D" },
    { Knob, kFltSustain,196, 456, 17, "S" },
    { Knob, kFltRelease,266, 456, 17, "R" },
    // amp env
    { Knob, kAmpAttack, 378, 456, 17, "A" },
    { Knob, kAmpDecay,  448, 456, 17, "D" },
    { Knob, kAmpSustain,518, 456, 17, "S" },
    { Knob, kAmpRelease,588, 456, 17, "R" },
    { Knob, kAmpVelAmt, 672, 456, 17, "VEL" },
    // lfo
    { Knob, kLfoRate,   790, 400, 22, "RATE" },
    { Knob, kLfoPitch,  880, 400, 18, "VIBRATO" },
    { Knob, kLfoCutoff, 852, 466, 15, "> CUTOFF" },
};
static constexpr int kNumCtls = sizeof(kCtls) / sizeof(kCtls[0]);

static bool isTime(uint32_t p) {
    return p == kGlide || (p >= kFltAttack && p <= kAmpRelease && p != kFltSustain && p != kAmpSustain);
}

class MonolithUI : public UI {
public:
    MonolithUI() : UI((uint)W, (uint)H) {
        loadSharedResources();
        Engine def;
        for (uint32_t i = 0; i < kParamCount; ++i) { val[i] = def.p[i]; defv[i] = def.p[i]; }
        uiScale = (float)getScaleFactor();
        if (uiScale != 1.f) setSize((uint)(W * uiScale), (uint)(H * uiScale));
        setGeometryConstraints((uint)(W * uiScale), (uint)(H * uiScale), true);
    }

protected:
    // ---- value mapping -----------------------------------------------------
    float toNorm(uint32_t p, float v) const {
        const ParamDef& d = kDefs[p];
        if (d.hints & kHintLog) return std::log(v / d.min) / std::log(d.max / d.min);
        float n = (v - d.min) / (d.max - d.min);
        if (isTime(p)) n = std::sqrt(std::max(n, 0.f));
        return n;
    }
    float fromNorm(uint32_t p, float n) const {
        const ParamDef& d = kDefs[p];
        n = std::min(1.f, std::max(0.f, n));
        float v;
        if (d.hints & kHintLog) v = d.min * std::pow(d.max / d.min, n);
        else { if (isTime(p)) n = n * n; v = d.min + n * (d.max - d.min); }
        if (d.hints & kHintInteger) v = std::round(v);
        return v;
    }
    void setValue(uint32_t p, float v) {
        if (v == val[p]) return;
        val[p] = v; setParameterValue(p, v); repaint();
    }

    void parameterChanged(uint32_t i, float v) override {
        if (i < kParamCount && val[i] != v) { val[i] = v; repaint(); }
    }

    // ---- formatting --------------------------------------------------------
    void fmt(uint32_t p, char* b, size_t n) const {
        const float v = val[p];
        switch (p) {
        case kCutoff: if (v >= 1000) snprintf(b, n, "%.2f kHz", v / 1000); else snprintf(b, n, "%.0f Hz", v); return;
        case kOctave: snprintf(b, n, "%+d", (int)v); return;
        case kFine: snprintf(b, n, "%+.0f ct", v); return;
        case kBendRange: snprintf(b, n, "%d st", (int)v); return;
        case kVolume: snprintf(b, n, "%+.1f dB", v); return;
        case kLfoRate: snprintf(b, n, v < 1 ? "%.2f Hz" : "%.1f Hz", v); return;
        case kFltEnvAmt: snprintf(b, n, "%+.0f%%", v * 100); return;
        case kShape: {
            static const char* names[] = { "SINE", "TRI", "SAW", "PULSE" };
            const float m = v * 3.f; const int i = (int)std::round(m);
            if (std::fabs(m - i) < 0.06f) snprintf(b, n, "%s", names[i]);
            else snprintf(b, n, "%.0f%%", v * 100);
            return; }
        default: break;
        }
        if (isTime(p)) { if (v < 1) snprintf(b, n, "%.0f ms", v * 1000); else snprintf(b, n, "%.2f s", v); return; }
        if (p == kPulseWidth) { snprintf(b, n, "%.0f%%", v * 100); return; }
        snprintf(b, n, "%.0f%%", v * 100);
    }

    // ---- drawing -----------------------------------------------------------
    void onNanoDisplay() override {
        save();
        scale(scale_(), scale_());
        // background
        beginPath(); rect(0, 0, W, H); fillColor(C_BG); fill();

        // header
        fontFace(NANOVG_DEJAVU_SANS_TTF);
        fontSize(22); textLetterSpacing(9); fillColor(C_TEXT);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE); text(44, 34, "MONOLITH", nullptr);
        fontSize(10); textLetterSpacing(2.5f); fillColor(C_DIM);
        text(252, 35, "MONOPHONIC SYNTHESIZER", nullptr);
        textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); text(W - 22, 35, "EPSILON AUDIO  ·  1.1", nullptr);
        // accent bar: a single slab
        beginPath(); rect(20, 21, 10, 26); fillColor(C_ACCENT); fill();

        for (const Panel& p : kPanels) drawPanel(p);

        drawOscScope(36, 92, 260, 72);
        drawFilterPlot(348, 92, 360, 70);
        drawEnv(36, 340, 260, 72, kFltAttack, C_ACCENT_A(255));
        drawEnv(348, 340, 360, 72, kAmpAttack, C_TEXT);
        drawLfo(760, 340, 184, 30);

        for (int i = 0; i < kNumCtls; ++i) drawCtl(kCtls[i], i == active || i == hover);
        restore();
    }

    float scale_() const { return uiScale; }

    void drawPanel(const Panel& p) {
        beginPath(); roundedRect(p.x, p.y, p.w, p.h, 6); fillColor(C_PANEL); fill();
        strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(9.5f); textLetterSpacing(2.2f); fillColor(C_DIM);
        textAlign(ALIGN_LEFT | ALIGN_TOP); text(p.x + 14, p.y + 10, p.title, nullptr);
    }

    void displayBox(float x, float y, float w, float h) {
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(Color(0x0b, 0x0c, 0x0d)); fill();
        strokeColor(C_LINE); strokeWidth(1); stroke();
        // faint grid
        beginPath();
        for (int i = 1; i < 4; ++i) { moveTo(x + w * i / 4, y + 2); lineTo(x + w * i / 4, y + h - 2); }
        moveTo(x + 2, y + h / 2); lineTo(x + w - 2, y + h / 2);
        strokeColor(Color(255, 255, 255, (10) / 255.f)); stroke();
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
        const int N = 240; const float cy = y + h / 2, amp = h * 0.36f;
        beginPath();
        for (int i = 0; i <= N; ++i) {
            const float t = std::fmod(2.f * i / N, 1.f);
            float s = morph(t, val[kShape], val[kPulseWidth]);
            if (val[kSubLevel] > 0) {
                const float ts = (float)i / N; s += val[kSubLevel] * (ts < 0.5f ? 1.f : -1.f);
                s /= 1 + val[kSubLevel];
            }
            const float px = x + 6 + (w - 12) * i / N, py = cy - s * amp;
            if (i == 0) moveTo(px, py); else lineTo(px, py);
        }
        strokeColor(C_ACCENT); strokeWidth(1.8f); lineJoin(ROUND); stroke();
    }

    void drawFilterPlot(float x, float y, float w, float h) {
        displayBox(x, y, w, h);
        const float res = val[kResonance];
        const float k = std::max(0.06f, 2.f - 2.04f * std::pow(res, 0.6f));
        const int mode = (int)(val[kFilterMode] + 0.5f);
        const float fc = val[kCutoff];
        const int N = 180;
        auto yOf = [&](float db) { return y + h * (1 - (db + 30.f) / 54.f); };
        std::vector<float> ys(N + 1);
        for (int i = 0; i <= N; ++i) {
            const float f = 20.f * std::pow(1000.f, (float)i / N);
            const float wn = f / fc;  // s = j*wn
            const float re = 1 - wn * wn, im = k * wn;
            const float den = std::sqrt(re * re + im * im);
            float mag = mode == 0 ? 1.f / den : (mode == 1 ? wn / den * std::min(1.f, k + 0.35f) : wn * wn / den);
            float db = 20 * std::log10(std::max(mag, 1e-4f));
            db = std::min(23.f, std::max(-29.f, db));
            ys[i] = yOf(db);
        }
        // fill
        beginPath(); moveTo(x + 2, y + h - 1);
        for (int i = 0; i <= N; ++i) lineTo(x + 2 + (w - 4) * i / N, ys[i]);
        lineTo(x + w - 2, y + h - 1); closePath();
        fillPaint(linearGradient(x, y, x, y + h, C_ACCENT_A(70), C_ACCENT_A(0))); fill();
        beginPath();
        for (int i = 0; i <= N; ++i) { const float px = x + 2 + (w - 4) * i / N; if (!i) moveTo(px, ys[i]); else lineTo(px, ys[i]); }
        strokeColor(C_ACCENT); strokeWidth(1.8f); stroke();
        // cutoff marker
        const float cx = x + 2 + (w - 4) * std::log(fc / 20.f) / std::log(1000.f);
        beginPath(); moveTo(cx, y + 3); lineTo(cx, y + h - 3); strokeColor(Color(255, 255, 255, (40) / 255.f)); stroke();
    }

    void drawEnv(float x, float y, float w, float h, uint32_t base, Color col) {
        displayBox(x, y, w, h);
        const float a = val[base], d = val[base + 1], s = val[base + 2], r = val[base + 3];
        const float hold = 0.6f;
        const float total = std::max(a + d + hold + r, 0.5f);
        const float sx = (w - 16) / total;
        const float x0 = x + 8, yb = y + h - 8, yt = y + 8, hh = yb - yt;
        beginPath(); moveTo(x0, yb);
        lineTo(x0 + a * sx, yt);
        // decay curve
        for (int i = 1; i <= 24; ++i) {
            const float t = (float)i / 24; const float lvl = s + (1 - s) * std::exp(-4 * t);
            lineTo(x0 + (a + d * t) * sx, yb - hh * lvl);
        }
        lineTo(x0 + (a + d + hold) * sx, yb - hh * s);
        for (int i = 1; i <= 24; ++i) {
            const float t = (float)i / 24; const float lvl = s * std::exp(-4 * t) * (1 - t * 0.02f);
            lineTo(x0 + (a + d + hold + r * t) * sx, yb - hh * lvl);
        }
        strokeColor(col); strokeWidth(1.8f); lineJoin(ROUND); stroke();
    }

    void drawLfo(float x, float y, float w, float h) {
        displayBox(x, y, w, h);
        const float cycles = std::max(0.5f, std::min(12.f, val[kLfoRate] * 0.6f));
        beginPath();
        for (int i = 0; i <= 120; ++i) {
            const float t = (float)i / 120;
            const float py = y + h / 2 - std::sin(t * cycles * 2 * 3.14159265f) * h * 0.32f;
            if (!i) moveTo(x + 4 + (w - 8) * t, py); else lineTo(x + 4 + (w - 8) * t, py);
        }
        strokeColor(C_ACCENT); strokeWidth(1.5f); stroke();
    }

    void drawCtl(const Ctl& c, bool hot) {
        char buf[32];
        if (c.type == Knob) {
            const float n = toNorm(c.param, val[c.param]);
            const float a0 = 0.75f * 3.14159265f, a1 = 2.25f * 3.14159265f;
            const bool bipolar = kDefs[c.param].min < 0 && c.param != kVolume;
            const float ang = a0 + (a1 - a0) * n;
            // track
            beginPath(); arc(c.x, c.y, c.r, a0, a1, CW);
            strokeColor(C_LINE); strokeWidth(c.r > 20 ? 4.f : 3.f); lineCap(ROUND); stroke();
            // value arc
            const float from = bipolar ? (a0 + a1) / 2 : a0;
            if (std::fabs(ang - from) > 0.001f) {
                beginPath(); arc(c.x, c.y, c.r, std::min(from, ang), std::max(from, ang), CW);
                strokeColor(C_ACCENT); stroke();
            }
            // cap
            const float cr = c.r - (c.r > 20 ? 8 : 6);
            beginPath(); circle(c.x, c.y, cr);
            fillPaint(radialGradient(c.x - cr * 0.3f, c.y - cr * 0.4f, 0, cr * 1.4f,
                                     hot ? Color(0x3a, 0x3e, 0x44) : C_PANEL_HI, Color(0x14, 0x15, 0x17)));
            fill(); strokeColor(Color(255, 255, 255, (18) / 255.f)); strokeWidth(1); stroke();
            // pointer
            beginPath();
            moveTo(c.x + std::cos(ang) * cr * 0.35f, c.y + std::sin(ang) * cr * 0.35f);
            lineTo(c.x + std::cos(ang) * cr * 0.9f, c.y + std::sin(ang) * cr * 0.9f);
            strokeColor(C_TEXT); strokeWidth(2); lineCap(ROUND); stroke();
            // label + value
            fontSize(9); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_TOP);
            fillColor(hot ? C_TEXT : C_DIM);
            if (hot) { fmt(c.param, buf, sizeof buf); fillColor(C_ACCENT); text(c.x, c.y + c.r + 6, buf, nullptr); }
            else text(c.x, c.y + c.r + 6, c.label, nullptr);
        } else if (c.type == Toggle) {
            const bool on = val[c.param] > 0.5f;
            const float w = 40, h = 20, x = c.x - w / 2, y = c.y - h / 2;
            beginPath(); roundedRect(x, y, w, h, h / 2); fillColor(on ? C_ACCENT_A(60) : C_PANEL_HI); fill();
            strokeColor(on ? C_ACCENT : C_LINE); strokeWidth(1); stroke();
            beginPath(); circle(on ? x + w - h / 2 : x + h / 2, c.y, 7); fillColor(on ? C_ACCENT : C_DIM); fill();
            fontSize(9); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_TOP);
            fillColor(C_DIM); text(c.x, c.y + 24, c.label, nullptr);
        } else { // Segment: filter mode
            static const char* names[] = { "LOW PASS", "BAND PASS", "HIGH PASS" };
            const int cur = (int)(val[c.param] + 0.5f);
            const float sw = 112, sh = 22, x0 = c.x - sw * 1.5f;
            for (int i = 0; i < 3; ++i) {
                const float x = x0 + sw * i;
                beginPath(); roundedRect(x + 1, c.y - sh / 2, sw - 2, sh, 4);
                fillColor(i == cur ? C_ACCENT : C_PANEL_HI); fill();
                fontSize(9); textLetterSpacing(1.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
                fillColor(i == cur ? C_BG : C_DIM); text(x + sw / 2, c.y + 0.5f, names[i], nullptr);
            }
        }
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
                const float x0 = c.x - 168;
                if (x >= x0 && x < x0 + 336 && std::fabs(y - c.y) <= 12) { if (seg) *seg = (int)((x - x0) / 112); return i; }
            }
        }
        return -1;
    }

    bool onMouse(const MouseEvent& ev) override {
        const float x = ev.pos.getX() / uiScale, y = ev.pos.getY() / uiScale;
        if (ev.button != 1) return false;
        if (ev.press) {
            int seg = 0; const int i = hit(x, y, &seg);
            if (i < 0) return false;
            const Ctl& c = kCtls[i];
            if (c.type == Toggle) { editParameter(c.param, true); setValue(c.param, val[c.param] > 0.5f ? 0.f : 1.f); editParameter(c.param, false); return true; }
            if (c.type == Segment) { editParameter(c.param, true); setValue(c.param, (float)seg); editParameter(c.param, false); return true; }
            // knob: double-click resets
            if (i == lastClick && ev.time - lastClickTime < 320) {
                editParameter(c.param, true); setValue(c.param, defv[c.param]); editParameter(c.param, false);
                lastClick = -1; return true;
            }
            lastClick = i; lastClickTime = ev.time;
            active = i; dragY = y; dragNorm = toNorm(c.param, val[c.param]);
            editParameter(c.param, true); repaint();
            return true;
        } else if (active >= 0) {
            editParameter(kCtls[active].param, false); active = -1; repaint(); return true;
        }
        return false;
    }

    bool onMotion(const MotionEvent& ev) override {
        const float x = ev.pos.getX() / uiScale, y = ev.pos.getY() / uiScale;
        if (active >= 0) {
            const Ctl& c = kCtls[active];
            const float sens = (ev.mod & kModifierShift) ? 1500.f : 220.f;
            dragNorm += (dragY - y) / sens; dragY = y;
            dragNorm = std::min(1.f, std::max(0.f, dragNorm));
            setValue(c.param, fromNorm(c.param, dragNorm));
            return true;
        }
        const int h = hit(x, y);
        if (h != hover) { hover = h; repaint(); }
        return false;
    }

    bool onScroll(const ScrollEvent& ev) override {
        const int i = hit(ev.pos.getX() / uiScale, ev.pos.getY() / uiScale);
        if (i < 0 || kCtls[i].type != Knob) return false;
        const uint32_t p = kCtls[i].param;
        float step = (ev.mod & kModifierShift) ? 0.004f : 0.025f;
        if (kDefs[p].hints & kHintInteger) step = 1.f / (kDefs[p].max - kDefs[p].min);
        editParameter(p, true);
        setValue(p, fromNorm(p, toNorm(p, val[p]) + (ev.delta.getY() > 0 ? step : -step)));
        editParameter(p, false);
        return true;
    }

private:
    float val[kParamCount], defv[kParamCount];
    float uiScale = 1.f;
    int active = -1, hover = -1, lastClick = -1;
    uint lastClickTime = 0;
    float dragY = 0, dragNorm = 0;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonolithUI)
};

UI* createUI() { return new MonolithUI(); }

END_NAMESPACE_DISTRHO
