// Monolith — custom vector UI (DPF/NanoVG). Original design. License: MIT.
#include "DistrhoUI.hpp"
#include "ParamDefs.hpp"
#include "Presets.hpp"
#include "UserPresets.hpp"
#include "Help.hpp"
#include "ResizeHandle.hpp"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <string>
#include <vector>

#if defined(_WIN32)
extern "C" __declspec(dllimport) void* __stdcall ShellExecuteA(void*, const char*, const char*, const char*, const char*, int);
#endif

START_NAMESPACE_DISTRHO

static void openURL(const char* url) {
#if defined(_WIN32)
    ShellExecuteA(nullptr, "open", url, nullptr, nullptr, 1);
#elif defined(__APPLE__)
    const std::string c = std::string("open '") + url + "' &"; if (std::system(c.c_str())) {}
#else
    const std::string c = std::string("xdg-open '") + url + "' >/dev/null 2>&1 &"; if (std::system(c.c_str())) {}
#endif
}
static bool gTipsOn = true;
static double nowSec() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

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
#define C_USER      Color(0x8c, 0xc8, 0xe8)
#define C_DISPLAY   Color(0x0b, 0x0c, 0x0d)
#define C_FAV       Color(0xff, 0xc8, 0x5a)       // favourite star
#define C_ACC       C_ACCENT
static Color withAlpha(const Color& c, float a) { Color r(c); r.alpha = a; return r; }

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

// display areas (for hover tips)
struct Area { float x, y, w, h; int tip; };
static const Area kAreas[] = {
    { 36, 92, 290, 60, kTipScope }, { 378, 92, 360, 60, kTipFilter },
    { 36, 366, 290, 112, kTipModEnv }, { 378, 366, 360, 112, kTipAmpEnv }, { 790, 366, 234, 60, kTipLfo },
};

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

// header:  logo · preset bar · A B COPY · UNDO REDO · LEARN · HELP
static constexpr float PS_X = 410.f, PS_Y = 20.f, PS_W = 232.f, PS_H = 28.f, PS_ARROW = 30.f;
static constexpr float A_X = 652.f, AB_W = 30.f, COPY_X = 714.f, COPY_W = 46.f;
static constexpr float UNDO_X = 770.f, REDO_X = 822.f, UR_W = 48.f, LEARN_X = 880.f, LEARN_W = 70.f, HELP_X = 960.f, HELP_W = 50.f;
// preset menu grid
static constexpr float PM_COLW = 146.f, PM_ROW = 24.f, PM_HEAD = 24.f, PM_PAD = 8.f;
// learn-mode "clear all" button (top right of the voice panel)
static constexpr float CLR_X = 940.f, CLR_Y = 70.f, CLR_W = 92.f, CLR_H = 18.f;

class MonolithUI : public UI {
public:
    MonolithUI() : UI((uint)W, (uint)H), resizeHandle(this) {
        loadSharedResources();
        Engine def;
        for (uint32_t i = 0; i < kParamCount; ++i) { val[i] = def.p[i]; defv[i] = def.p[i]; }
        val[kOutLearn] = defv[kOutLearn] = 0;
        for (int& m : paramCC) m = -1;
        const double sf = getScaleFactor();
        if (sf != 1.0) setSize((uint)(W * sf), (uint)(H * sf));
        setGeometryConstraints((uint)(W * 0.7 * sf), (uint)(H * 0.7 * sf), true);
        if (isResizable()) resizeHandle.hide();
        userPresets = listUserPresets();
        std::copy(val, val + kParamCount, before);
    }

protected:
    float uiScale() const { return (float)getWidth() / W; }

    void setValue(uint32_t p, float v) {
        const ParamDef& d = paramDef(p);
        v = clampf(v, d.min, d.max);
        if (d.hints & kHintInteger) v = std::round(v);
        if (v == val[p]) return;
        val[p] = v; setParameterValue(p, v); repaint();
    }
    void commit(uint32_t p, float v) { editParameter(p, true); setValue(p, v); editParameter(p, false); }

    void parameterChanged(uint32_t i, float v) override {
        if (i == kOutLearn) {
            if (v != lastLearnEcho && v > 0) {
                const int code = (int)v % 10000 - 1, prm = code / 128, cc = code % 128;
                for (int& m : paramCC) if (m == cc) m = -1;
                if (prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
                learnWaiting = -1;
            }
            lastLearnEcho = v; repaint(); return;
        }
        if (i < kParamCount && val[i] != v) { val[i] = v; if (!gesture) before[i] = v; repaint(); }
    }
    void stateChanged(const char* key, const char* value) override {
        if (std::strcmp(key, "midimap") != 0) return;
        for (int& m : paramCC) m = -1;
        const char* q = value;
        while (q && *q) {
            int cc = -1, prm = -1;
            if (std::sscanf(q, "%d:%d", &cc, &prm) == 2 && prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
            const char* s = std::strchr(q, ';'); if (!s) break; q = s + 1;
        }
        repaint();
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
    bool dimmed(const Ctl& c) const { return c.param == kLegato && val[kVoiceMode] > 0.5f; }   // legato only applies in mono

    // ---- undo / redo / A-B ---------------------------------------------------
    typedef std::vector<float> Snap;
    Snap snapshot() const { Snap s(val, val + kParamCount); s.push_back((float)abSlot); return s; }
    static bool differs(const float* a, const float* b) { for (uint32_t i = 0; i < kParamCount; ++i) if (a[i] != b[i]) return true; return false; }
    void beginGesture() { gesture = true; std::copy(val, val + kParamCount, before); }
    void endGesture() {
        gesture = false;
        if (differs(before, val)) {
            Snap u(before, before + kParamCount); u.push_back((float)abSlot);
            undoStack.push_back(u); if (undoStack.size() > 200) undoStack.erase(undoStack.begin());
            redoStack.clear();
        }
        std::copy(val, val + kParamCount, before); repaint();
    }
    void applyAll(const float* s) { for (uint32_t i = 0; i < kParamCount; ++i) if (val[i] != s[i]) commit(i, s[i]); }
    void restoreSnap(const Snap& s) {
        if (s.size() > kParamCount && (int)s[kParamCount] != abSlot) { abStore[abSlot] = snapshot(); abSlot = (int)s[kParamCount]; }
        applyAll(s.data()); std::copy(val, val + kParamCount, before);
    }
    void undo() { if (undoStack.empty()) return; redoStack.push_back(snapshot()); restoreSnap(undoStack.back()); undoStack.pop_back(); flash("Undo"); }
    void redo() { if (redoStack.empty()) return; undoStack.push_back(snapshot()); restoreSnap(redoStack.back()); redoStack.pop_back(); flash("Redo"); }
    void switchAB(int to) {
        if (to == abSlot) return;
        abStore[abSlot] = snapshot(); if (abStore[to].empty()) abStore[to] = snapshot();
        beginGesture(); applyAll(abStore[to].data()); endGesture();
        abSlot = to; flash(to == 0 ? "Sound A" : "Sound B");
    }
    void copyAB() { abStore[1 - abSlot] = snapshot(); flash(abSlot == 0 ? "Copied A to B" : "Copied B to A"); }

    // ---- formatting --------------------------------------------------------
    void fmt(uint32_t p, char* b, size_t n) const {
        const float v = val[p];
        int ne = 0;
        if (const char* const* l = enumLabels(p, ne)) { snprintf(b, n, "%s", l[std::max(0, std::min(ne - 1, (int)std::round(v)))]); return; }
        if (paramDef(p).hints & kHintBoolean) { snprintf(b, n, "%s", v > 0.5f ? "On" : "Off"); return; }
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
        drawHeader();
        for (const Panel& p : kPanels) drawPanel(p);

        drawOscScope(36, 92, 290, 60);
        drawFilterPlot(378, 92, 360, 60);
        drawEnv(36, 366, 290, 112, kFltAttack, C_ACCENT);
        drawEnv(378, 366, 360, 112, kAmpAttack, C_TEXT);
        drawLfo(790, 366, 234, 60);

        for (int i = 0; i < kNumCtls; ++i) drawCtl(kCtls[i], i == active || i == hover);
        if (editing >= 0) drawEditBox();
        drawStatus();
        if (menuOpen) drawPresetMenu();
        if (saving) drawSaveBox();
        if (helpOpen) drawHelp();
        else if (tipShown && gTipsOn) drawTooltip();
        restore();
    }

    void drawHeaderButton(float x, float w, const char* label, bool on, bool hot, bool disabled) {
        beginPath(); roundedRect(x, PS_Y, w, PS_H, 5); fillColor(on ? C_ACCENT_A(50) : C_PANEL); fill();
        strokeColor(on ? C_ACCENT : (hot && !disabled ? C_DIM : C_LINE)); strokeWidth(1); stroke();
        fontSize(9); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(disabled ? C_FAINT : (on ? C_TEXT : C_DIM));
        text(x + w / 2, PS_Y + PS_H / 2 + 0.5f, label, nullptr);
    }
    void drawHeader() {
        beginPath(); rect(20, 21, 10, 26); fillColor(C_ACCENT); fill();
        fontSize(22); textLetterSpacing(9); fillColor(C_TEXT);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE); text(44, 34, "MONOLITH", nullptr);
        fontSize(9.5f); textLetterSpacing(2.5f); fillColor(C_DIM); text(252, 29, "SUBTRACTIVE SYNTH", nullptr);
        fontSize(7); textLetterSpacing(1.1f); fillColor(C_FAINT); text(252, 41, "EPSILON AUDIO 2.1", nullptr);
        drawPresetBar();
        for (int k = 0; k < 2; ++k) {
            const float x = A_X + k * AB_W; const bool on = abSlot == k, hot = hoverHeader == 6 + k;
            beginPath(); roundedRect(x, PS_Y, AB_W - 2, PS_H, 5); fillColor(on ? C_ACCENT_A(60) : C_PANEL); fill();
            strokeColor(on ? C_ACCENT : (hot ? C_DIM : C_LINE)); strokeWidth(1); stroke();
            fontSize(10); textLetterSpacing(0); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM);
            text(x + (AB_W - 2) / 2, PS_Y + PS_H / 2 + 0.5f, k == 0 ? "A" : "B", nullptr);
        }
        drawHeaderButton(COPY_X, COPY_W, "COPY", false, hoverHeader == 8, false);
        drawHeaderButton(UNDO_X, UR_W, "UNDO", false, hoverHeader == 3, undoStack.empty());
        drawHeaderButton(REDO_X, UR_W, "REDO", false, hoverHeader == 4, redoStack.empty());
        drawHeaderButton(LEARN_X, LEARN_W, learnMode ? "LEARNING" : "LEARN", learnMode, hoverHeader == 1, false);
        drawHeaderButton(HELP_X, HELP_W, "HELP", helpOpen, hoverHeader == 5, false);
    }

    void drawPanel(const Panel& p) {
        beginPath(); roundedRect(p.x, p.y, p.w, p.h, 6); fillColor(C_PANEL); fill();
        strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(9.5f); textLetterSpacing(2.2f); fillColor(C_DIM);
        textAlign(ALIGN_LEFT | ALIGN_TOP); text(p.x + 14, p.y + 10, p.title, nullptr);
    }

    void displayBox(float x, float y, float w, float h) {
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_DISPLAY); fill();
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
        const uint32_t p = effParam(c);
        if (c.type == Knob) {
            const float n = toNorm(p, val[p]);
            const float a0 = 0.75f * 3.14159265f, a1 = 2.25f * 3.14159265f;
            const bool bipolar = paramDef(p).min < 0 && p != kVolume;
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
            const bool on = val[c.param] > 0.5f, dim = dimmed(c);
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
        if (learnMode) {
            const int cc = paramCC[p]; const bool waiting = learnWaiting == (int)p;
            if (cc >= 0 || waiting) {
                const float by = c.type == Knob ? c.y - c.r - 10 : c.y - 19;
                snprintf(buf, sizeof buf, waiting ? "?" : "CC%d", cc);
                fontSize(8.5f); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
                const float bw = waiting ? 16 : 34;
                beginPath(); roundedRect(c.x - bw / 2, by - 7, bw, 14, 3); fillColor(waiting ? C_ACCENT : C_PANEL_HI); fill(); strokeColor(C_ACCENT); strokeWidth(1); stroke();
                fillColor(waiting ? C_BG : C_ACCENT); text(c.x, by + 0.5f, buf, nullptr);
            }
        }
    }

    void drawEditBox() {
        const Ctl& c = kCtls[editing];
        const float w = 84, h = 20, x = c.x - w / 2, y = c.y + c.r + 3;
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_DISPLAY); fill();
        strokeColor(C_ACCENT); strokeWidth(1); stroke();
        fontSize(11); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT);
        const std::string shown = editBuf + "|";
        text(c.x, y + h / 2 + 0.5f, shown.c_str(), nullptr);
    }

    // a pill under the header for short messages; in learn mode it explains what to do
    void drawStatus() {
        std::string msg;
        if (flashing()) msg = statusText;
        else if (learnMode) msg = learnWaiting >= 0 ? std::string("Move a MIDI controller to assign: ") + paramDef((uint32_t)learnWaiting).name
                                                   : "MIDI Learn: click a control, then move a controller";
        if (!msg.empty()) {                         // sits in the gap between the header and the panels
            fontSize(9); textLetterSpacing(0.6f);
            Rectangle<float> rb; const float tw = textBounds(0, 0, msg.c_str(), nullptr, rb);
            const float w = tw + 24, x = W / 2 - w / 2 + 20, y = 49;
            beginPath(); roundedRect(x, y, w, 15, 7.5f); fillColor(C_PANEL_HI); fill(); strokeColor(C_ACCENT); strokeWidth(1); stroke();
            textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_ACCENT); text(x + w / 2, y + 8, msg.c_str(), nullptr);
        }
        if (learnMode && learnWaiting < 0) {
            beginPath(); roundedRect(CLR_X, CLR_Y, CLR_W, CLR_H, 4); fillColor(hoverClear ? C_ACCENT_A(50) : C_PANEL_HI); fill(); strokeColor(C_ACCENT); strokeWidth(1); stroke();
            fontSize(8); textLetterSpacing(1.4f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(CLR_X + CLR_W / 2, CLR_Y + CLR_H / 2 + 0.5f, "CLEAR ALL", nullptr);
        }
    }
    void flash(const std::string& s) { statusText = s; flashUntil = nowSec() + 1.4; repaint(); }

    // ---- presets ------------------------------------------------------------
    bool matchesValues(const float* v) const {
        for (uint32_t i = 0; i < kParamCount; ++i) if (std::fabs(val[i] - v[i]) > 1e-4f * std::max(1.f, std::fabs(v[i]))) return false;
        return true;
    }
    int exactMatch() const { for (uint32_t i = 0; i < kNumPresets; ++i) if (matchesValues(presets()[i].v)) return (int)i; return -1; }
    void applyValues(const float* v) { beginGesture(); applyAll(v); endGesture(); }
    void applyFactory(int idx) { idx = (idx % (int)kNumPresets + (int)kNumPresets) % (int)kNumPresets; applyValues(presets()[idx].v); curFactory = idx; curUser.clear(); repaint(); }
    void applyUser(const std::string& name) {
        float v[kParamCount]; std::copy(defv, defv + kParamCount, v);
        if (!loadUserPreset(name, v)) return;
        applyValues(v); std::copy(v, v + kParamCount, userVals); curUser = name; curFactory = -1; repaint();
    }
    void programLoaded(uint32_t index) override { curFactory = (int)index; curUser.clear(); repaint(); }
    void step(int d) {
        const int nf = (int)kNumPresets, nu = (int)userPresets.size(), total = nf + nu;
        int cur = curFactory;
        if (!curUser.empty()) { for (int i = 0; i < nu; ++i) if (userPresets[i] == curUser) cur = nf + i; }
        else if (cur < 0) cur = std::max(0, exactMatch());
        const int next = ((cur + d) % total + total) % total;
        if (next < nf) applyFactory(next); else applyUser(userPresets[next - nf]);
    }

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
        std::string name; bool modified = false;
        if (!curUser.empty()) { name = curUser; modified = !matchesValues(userVals); }
        else {
            int shown = curFactory;
            if (shown < 0 || !matchesValues(presets()[shown].v)) { const int m = exactMatch(); if (m >= 0) shown = m; else modified = true; }
            name = shown >= 0 ? presets()[shown].name : "Custom";
        }
        if (modified) name += " *";
        fontSize(12); textLetterSpacing(1.0f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        fillColor(presetHover == 1 || menuOpen ? C_TEXT : Color(0xd0, 0xcc, 0xc4));
        text(x + w / 2 - 6, y + h / 2 + 0.5f, name.c_str(), nullptr);
        const float cx = x + w - a - 14, cy = y + h / 2;
        beginPath(); moveTo(cx - 4, cy - 2); lineTo(cx + 4, cy - 2); lineTo(cx, cy + 3); closePath();
        fillColor(C_DIM); fill();
    }

    // menu geometry: Init and SAVE on top, then one column per category plus User
    // preset browser: categories, user presets and favourites in columns; typing searches names and categories.
    // kind: 0 factory, 1 user, 2 save button, 3 search field, 4 favourite (idx into favs)
    struct MRect { float x, y, w, h; int kind; int idx; };
    static std::string lower(const std::string& s) { std::string r(s); for (char& c : r) c = (char)std::tolower((unsigned char)c); return r; }
    std::string favKey(int kind, int idx) const { return kind == 0 ? std::string("f:") + presets()[idx].name : (kind == 1 ? "u:" + userPresets[idx] : (idx < (int)favs.size() ? favs[idx] : std::string())); }
    bool isFav(const std::string& k) const { return std::find(favs.begin(), favs.end(), k) != favs.end(); }
    void toggleFav(const std::string& k) {
        if (k.empty()) return;
        auto it = std::find(favs.begin(), favs.end(), k); if (it != favs.end()) favs.erase(it); else favs.push_back(k);
        saveFavourites(favs); repaint();
    }
    static constexpr int kMenuCols = kNumCategories + 2, kMenuRows = 16;
    void menuLayout(float& mx, float& my, float& mw, float& mh, std::vector<MRect>& items) const {
        mw = PM_COLW * kMenuCols + 2 * PM_PAD; mx = std::min(W - mw - 8, std::max(8.f, PS_X + PS_W / 2 - mw / 2)); my = PS_Y + PS_H + 4;
        items.clear();
        const float top = my + PM_PAD + PM_ROW + 6 + PM_HEAD;
        int maxRows = 1;
        items.push_back({ mx + PM_PAD + PM_COLW, my + PM_PAD, 300, PM_ROW, 3, 0 });
        items.push_back({ mx + mw - PM_PAD - 150, my + PM_PAD, 150, PM_ROW, 2, 0 });
        if (!query.empty()) {
            const std::string q = lower(query); int k = 0;
            auto place = [&](int kind, int idx) { const int col = k / kMenuRows, row = k % kMenuRows; if (col >= kMenuCols) return; items.push_back({ mx + PM_PAD + col * PM_COLW, top + row * PM_ROW, PM_COLW - 4, PM_ROW, kind, idx }); maxRows = std::max(maxRows, row + 1); ++k; };
            for (uint32_t i = 0; i < kNumPresets; ++i) if (lower(presets()[i].name).find(q) != std::string::npos || lower(presets()[i].category).find(q) != std::string::npos) place(0, (int)i);
            for (int u = 0; u < (int)userPresets.size(); ++u) if (lower(userPresets[u]).find(q) != std::string::npos || std::string("user").find(q) != std::string::npos) place(1, u);
        } else {
            int rows[kNumCategories] = {};
            for (uint32_t i = 0; i < kNumPresets; ++i) {
                const char* cat = presets()[i].category;
                if (std::strcmp(cat, "Init") == 0) { items.push_back({ mx + PM_PAD, my + PM_PAD, PM_COLW - 4, PM_ROW, 0, (int)i }); continue; }
                for (int c = 0; c < kNumCategories; ++c)
                    if (std::strcmp(cat, kCategories[c]) == 0) { items.push_back({ mx + PM_PAD + c * PM_COLW, top + rows[c] * PM_ROW, PM_COLW - 4, PM_ROW, 0, (int)i }); maxRows = std::max(maxRows, ++rows[c]); }
            }
            for (int u = 0; u < (int)userPresets.size() && u < kMenuRows; ++u) items.push_back({ mx + PM_PAD + kNumCategories * PM_COLW, top + u * PM_ROW, PM_COLW - 4, PM_ROW, 1, u });
            maxRows = std::max(maxRows, std::min(kMenuRows, (int)userPresets.size()));
            for (int f = 0; f < (int)favs.size() && f < kMenuRows; ++f) items.push_back({ mx + PM_PAD + (kNumCategories + 1) * PM_COLW, top + f * PM_ROW, PM_COLW - 4, PM_ROW, 4, f });
            maxRows = std::max(maxRows, std::min(kMenuRows, (int)favs.size()));
        }
        mh = PM_PAD + PM_ROW + 6 + PM_HEAD + maxRows * PM_ROW + PM_PAD;
    }
    std::string itemLabel(const MRect& r) const {
        if (r.kind == 0) return presets()[r.idx].name;
        if (r.kind == 1) return userPresets[r.idx];
        if (r.kind == 4 && r.idx < (int)favs.size()) return favs[r.idx].substr(2);
        return "";
    }
    void drawPresetMenu() {
        float mx, my, mw, mh; std::vector<MRect> items; menuLayout(mx, my, mw, mh, items);
        beginPath(); rect(mx - 12, my - 4, mw + 24, mh + 20); fillPaint(boxGradient(mx, my + 2, mw, mh, 6, 14, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(mx, my, mw, mh, 6); fillColor(C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(mx + PM_PAD, my + PM_PAD + PM_ROW + 3); lineTo(mx + mw - PM_PAD, my + PM_PAD + PM_ROW + 3); strokeColor(C_LINE); stroke();
        fontSize(8.5f); textLetterSpacing(1.8f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        const float hy = my + PM_PAD + PM_ROW + 6 + PM_HEAD / 2;
        if (query.empty()) {
            for (int c = 0; c < kMenuCols; ++c) {
                std::string up = c < kNumCategories ? kCategories[c] : (c == kNumCategories ? "User" : "Favourites"); for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
                fillColor(c < kNumCategories ? C_ACC : C_FAV); text(mx + PM_PAD + c * PM_COLW + 12, hy, up.c_str(), nullptr);
            }
            fontSize(10); textLetterSpacing(0.4f); fillColor(C_FAINT);
            if (userPresets.empty()) text(mx + PM_PAD + kNumCategories * PM_COLW + 12, hy + PM_HEAD / 2 + PM_ROW / 2, "(none saved yet)", nullptr);
            if (favs.empty()) text(mx + PM_PAD + (kNumCategories + 1) * PM_COLW + 12, hy + PM_HEAD / 2 + PM_ROW / 2, "(click a star)", nullptr);
        } else {
            int found = 0; for (const MRect& r : items) if (r.kind == 0 || r.kind == 1) ++found;
            char b[64]; snprintf(b, sizeof b, found ? "%d MATCHING PRESET%s" : "NO PRESET MATCHES", found, found == 1 ? "" : "S"); fillColor(C_ACC); text(mx + PM_PAD + 12, hy, b, nullptr);
        }
        for (int i = 0; i < (int)items.size(); ++i) {
            const MRect& r = items[i]; const bool hot = i == menuHover;
            if (r.kind == 2) {
                beginPath(); roundedRect(r.x, r.y + 1, r.w, r.h - 2, 4); fillColor(hot ? withAlpha(C_ACC, 0.25f) : C_PANEL); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
                fontSize(9.5f); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(r.x + r.w / 2, r.y + r.h / 2 + 0.5f, "SAVE CURRENT AS...", nullptr);
                continue;
            }
            if (r.kind == 3) {             // search field
                beginPath(); roundedRect(r.x, r.y + 1, r.w, r.h - 2, 4); fillColor(C_DISPLAY); fill(); strokeColor(query.empty() ? C_LINE : C_ACC); strokeWidth(1); stroke();
                beginPath(); circle(r.x + 12, r.y + r.h / 2, 4); moveTo(r.x + 15, r.y + r.h / 2 + 3); lineTo(r.x + 18, r.y + r.h / 2 + 6); strokeColor(C_DIM); strokeWidth(1.3f); stroke();
                fontSize(11); textLetterSpacing(0.3f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(query.empty() ? C_FAINT : C_TEXT);
                text(r.x + 26, r.y + r.h / 2 + 0.5f, query.empty() ? "Type to search names and categories" : (query + "|").c_str(), nullptr);
                continue;
            }
            const std::string key = favKey(r.kind, r.idx);
            const bool isCur = r.kind == 0 ? (curUser.empty() && r.idx == curFactory) : (r.kind == 1 ? curUser == userPresets[r.idx] : false);
            const bool del = r.kind == 1 && pendingDelete == r.idx;
            if (hot || del) { beginPath(); roundedRect(r.x, r.y, r.w, r.h, 4); fillColor(del ? Color(0xd0, 0x50, 0x40, 0.35f) : withAlpha(C_ACC, 0.18f)); fill(); }
            if (isCur) { beginPath(); circle(r.x + 6, r.y + r.h / 2, 2.5f); fillColor(C_ACC); fill(); }
            fontSize(11); textLetterSpacing(0.3f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hot || isCur || del ? C_TEXT : C_DIM);
            const std::string label = del ? "Delete? click again" : itemLabel(r);
            text(r.x + 12, r.y + r.h / 2 + 0.5f, label.c_str(), nullptr);
            // the favourite star
            const bool fav = isFav(key);
            if (fav || hot) drawStar(r.x + r.w - 10, r.y + r.h / 2, 5.f, fav);
        }
    }
    void drawStar(float cx, float cy, float rr, bool filled) {
        beginPath();
        for (int k = 0; k < 10; ++k) { const float a = -3.14159265f / 2 + k * 3.14159265f / 5, q = k % 2 ? rr * 0.45f : rr; if (!k) moveTo(cx + q * std::cos(a), cy + q * std::sin(a)); else lineTo(cx + q * std::cos(a), cy + q * std::sin(a)); }
        closePath();
        if (filled) { fillColor(C_FAV); fill(); } else { strokeColor(C_DIM); strokeWidth(1); stroke(); }
    }
    int menuHit(float x, float y) const { float mx, my, mw, mh; std::vector<MRect> it; menuLayout(mx, my, mw, mh, it);
        for (int i = 0; i < (int)it.size(); ++i) if (x >= it[i].x && x < it[i].x + it[i].w && y >= it[i].y && y < it[i].y + it[i].h) return i;
        return -1; }
    MRect menuItem(int i) const { float mx, my, mw, mh; std::vector<MRect> it; menuLayout(mx, my, mw, mh, it); return it[i]; }
    void openMenu() { userPresets = listUserPresets(); favs = loadFavourites(); query.clear(); menuOpen = true; menuHover = -1; pendingDelete = -1; repaint(); }
    void applyFavourite(int idx) {
        if (idx >= (int)favs.size()) return;
        const std::string& k = favs[idx];
        if (k.compare(0, 2, "f:") == 0) { for (uint32_t i = 0; i < kNumPresets; ++i) if (k.substr(2) == presets()[i].name) { applyFactory((int)i); return; } }
        else applyUser(k.substr(2));
    }
    int presetBarHit(float x, float y) const {
        if (y < PS_Y || y > PS_Y + PS_H || x < PS_X || x > PS_X + PS_W) return -1;
        if (x < PS_X + PS_ARROW) return 0;
        if (x > PS_X + PS_W - PS_ARROW) return 2;
        return 1;
    }
    void drawSaveBox() {
        const float w = 360, h = 96, x = W / 2 - w / 2, y = 140;
        beginPath(); rect(0, 0, W, H); fillColor(Color(0, 0, 0, 0.45f)); fill();
        beginPath(); roundedRect(x, y, w, h, 8); fillColor(C_PANEL_HI); fill(); strokeColor(C_ACCENT); strokeWidth(1); stroke();
        fontSize(9.5f); textLetterSpacing(2.f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM); text(x + 16, y + 14, "SAVE PRESET AS", nullptr);
        beginPath(); roundedRect(x + 16, y + 36, w - 32, 26, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); stroke();
        fontSize(13); textLetterSpacing(0.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT); text(x + 26, y + 49.5f, (saveName + "|").c_str(), nullptr);
        fontSize(8.5f); textLetterSpacing(1.2f); fillColor(C_FAINT); textAlign(ALIGN_RIGHT | ALIGN_TOP);
        text(x + w - 16, y + 72, saveError ? "COULD NOT SAVE - CHECK THE NAME" : "ENTER TO SAVE   ·   ESC TO CANCEL", nullptr);
    }

    // ---- interaction ------------------------------------------------------
    static bool inRect(float x, float y, float rx, float ry, float rw, float rh) { return x >= rx && x < rx + rw && y >= ry && y < ry + rh; }
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
    int headerHit(float x, float y) const {
        if (y < PS_Y || y > PS_Y + PS_H) return 0;
        if (x >= LEARN_X && x <= LEARN_X + LEARN_W) return 1;
        if (x >= UNDO_X && x <= UNDO_X + UR_W) return 3;
        if (x >= REDO_X && x <= REDO_X + UR_W) return 4;
        if (x >= HELP_X && x <= HELP_X + HELP_W) return 5;
        if (x >= A_X && x < A_X + AB_W) return 6;
        if (x >= A_X + AB_W && x < A_X + 2 * AB_W) return 7;
        if (x >= COPY_X && x <= COPY_X + COPY_W) return 8;
        return 0;
    }
    bool clearHit(float x, float y) const { return learnMode && learnWaiting < 0 && inRect(x, y, CLR_X, CLR_Y, CLR_W, CLR_H); }
    int areaHit(float x, float y) const { for (const Area& a : kAreas) if (inRect(x, y, a.x, a.y, a.w, a.h)) return a.tip; return -1; }

    void setLearn(bool on) { learnMode = on; learnWaiting = -1; if (!on) setState("cmd", "cancel"); repaint(); }
    void endEdit(bool apply) {
        if (editing < 0) return;
        const uint32_t p = effParam(kCtls[editing]);
        float v;
        if (apply && parse(p, editBuf, v)) { beginGesture(); commit(p, v); endGesture(); }
        editing = -1; editBuf.clear(); repaint();
    }
    static float angleNorm(const Ctl& c, float x, float y) {
        const float a = std::atan2(y - c.y, x - c.x), a0 = 0.75f * 3.14159265f;
        float rel = a - a0; while (rel < 0) rel += 2 * 3.14159265f;
        if (rel > 1.5f * 3.14159265f) rel = rel > 1.75f * 3.14159265f ? 0.f : 1.5f * 3.14159265f;
        return rel / (1.5f * 3.14159265f);
    }

    bool onKeyboard(const KeyboardEvent& ev) override {
        if (!ev.press) return false;
        std::string* buf = saving ? &saveName : (editing >= 0 ? &editBuf : nullptr);
        if (!buf && menuOpen) {                              // typing in the preset browser searches
            if (ev.key == kKeyEscape) { if (!query.empty()) query.clear(); else menuOpen = false; }
            else if (ev.key == kKeyBackspace || ev.key == kKeyDelete) { if (!query.empty()) query.pop_back(); }
            else if (ev.key >= 32 && ev.key < 127 && query.size() < 32) query += (char)ev.key;
            else return false;
            menuHover = -1; pendingDelete = -1; repaint(); return true;
        }
        if (!buf) {
            const bool cmd = (ev.mod & (kModifierControl | kModifierSuper)) != 0;
            if (cmd && (ev.key == 'z' || ev.key == 'Z')) { if (ev.mod & kModifierShift) redo(); else undo(); return true; }
            if (cmd && (ev.key == 'y' || ev.key == 'Y')) { redo(); return true; }
            if (ev.key == kKeyEscape) { if (helpOpen) helpOpen = false; else if (menuOpen) menuOpen = false; else if (learnMode) setLearn(false); repaint(); return true; }
            return false;
        }
        if (ev.key == kKeyEnter) {
            if (saving) {
                if (saveUserPreset(saveName, val)) { userPresets = listUserPresets(); curUser = sanitizeName(saveName); curFactory = -1; std::copy(val, val + kParamCount, userVals); saving = false; saveError = false; flash("Saved"); }
                else saveError = true;
            } else endEdit(true);
            repaint(); return true;
        }
        if (ev.key == kKeyEscape) { if (saving) saving = false; else endEdit(false); repaint(); return true; }
        if (ev.key == kKeyBackspace || ev.key == kKeyDelete) { if (!buf->empty()) buf->pop_back(); repaint(); return true; }
        if (ev.key >= 32 && ev.key < 127 && buf->size() < (saving ? 40u : 12u)) {
            char ch = (char)ev.key;
            if ((ev.mod & kModifierShift) && std::isalpha((unsigned char)ch)) ch = (char)std::toupper((unsigned char)ch);   // keys arrive unshifted
            *buf += ch; repaint(); return true;
        }
        return true;
    }

    bool onMouse(const MouseEvent& ev) override {
        const float s = uiScale();
        const float x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (ev.press) { flashUntil = 0; tipShown = false; tipSuppressed = tipKey; }
        if (helpOpen) { if (ev.press && ev.button == 1) helpClick(x, y); return true; }
        if (ev.button == 2 && ev.press) {                  // right-click (DPF: 1 left, 2 right, 3 middle)
            if (menuOpen) { const int m = menuHit(x, y); if (m >= 0 && menuItem(m).kind == 1) { pendingDelete = menuItem(m).idx; repaint(); } return true; }
            if (learnMode) { const int i = hit(x, y); if (i >= 0) { const uint32_t p = effParam(kCtls[i]); char b[24]; snprintf(b, sizeof b, "unmap:%u", p); setState("cmd", b); paramCC[p] = -1; repaint(); } return true; }
            return false;
        }
        if (ev.button != 1) return false;
        if (!ev.press) {
            if (active >= 0) { editParameter(effParam(kCtls[active]), false); active = -1; endGesture(); return true; }
            return false;
        }
        if (editing >= 0) endEdit(true);
        if (menuOpen) {
            const int m = menuHit(x, y);
            if (m >= 0) {
                const MRect r = menuItem(m);
                if (r.kind == 3) return true;                       // the search field: just type
                if ((r.kind == 0 || r.kind == 1 || r.kind == 4) && x >= r.x + r.w - 20) { toggleFav(favKey(r.kind, r.idx)); return true; }
                if (r.kind == 2) { menuOpen = false; saving = true; saveError = false; saveName = curUser; }
                else if (r.kind == 1 && pendingDelete == r.idx) { deleteUserPreset(userPresets[r.idx]); if (curUser == userPresets[r.idx]) curUser.clear(); userPresets = listUserPresets(); pendingDelete = -1; repaint(); return true; }
                else { menuOpen = false; if (r.kind == 0) applyFactory(r.idx); else if (r.kind == 1) applyUser(userPresets[r.idx]); else applyFavourite(r.idx); }
            } else menuOpen = false;
            pendingDelete = -1; menuHover = -1; repaint(); return true;
        }
        const int hh = headerHit(x, y);
        if (hh == 1) { setLearn(!learnMode); return true; }
        if (hh == 3) { undo(); return true; }
        if (hh == 4) { redo(); return true; }
        if (hh == 5) { helpOpen = true; repaint(); return true; }
        if (hh == 6 || hh == 7) { switchAB(hh - 6); return true; }
        if (hh == 8) { copyAB(); return true; }
        const int pb = presetBarHit(x, y);
        if (pb == 0) { step(-1); return true; }
        if (pb == 2) { step(1); return true; }
        if (pb == 1) { openMenu(); return true; }
        if (clearHit(x, y)) { setState("cmd", "clear"); for (int& m : paramCC) m = -1; flash("All MIDI assignments cleared"); return true; }

        int seg = 0; const int i = hit(x, y, &seg);
        if (i < 0) return false;
        const Ctl& c = kCtls[i];
        const uint32_t p = effParam(c);
        if (learnMode) { learnWaiting = (int)p; char b[24]; snprintf(b, sizeof b, "learn:%u", p); setState("cmd", b); repaint(); return true; }
        if (c.type == Toggle) { beginGesture(); commit(p, val[p] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
        if (c.type == Segment) { beginGesture(); commit(p, (float)seg); endGesture(); return true; }
        if (ev.mod & (kModifierControl | kModifierSuper)) { beginGesture(); commit(p, defv[p]); endGesture(); return true; }   // Cmd/Ctrl-click: reset
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
        beginGesture();
        active = i; dragY = y; dragNorm = toNorm(p, val[p]);
        const float dist = std::sqrt((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y));
        circular = c.r > 19 && dist > c.r * 0.62f && !(paramDef(p).hints & kHintInteger);    // grab the rim of a large knob to turn it in a circle
        editParameter(p, true);
        if (circular) setValue(p, fromNorm(p, angleNorm(c, x, y)));
        repaint();
        return true;
    }

    bool onMotion(const MotionEvent& ev) override {
        const float s = uiScale();
        const float x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (helpOpen) { const int h = helpHit(x, y); if (h != helpHover) { helpHover = h; repaint(); } return true; }
        if (active >= 0) {
            const Ctl& c = kCtls[active]; const uint32_t p = effParam(c);
            if (circular) { setValue(p, fromNorm(p, angleNorm(c, x, y))); return true; }
            float sens = (ev.mod & kModifierShift) ? 1500.f : 220.f;
            if (paramDef(p).hints & kHintInteger) sens = std::max(60.f, 18.f * (paramDef(p).max - paramDef(p).min));
            dragNorm += (dragY - y) / sens; dragY = y;
            dragNorm = clampf(dragNorm, 0.f, 1.f);
            setValue(p, fromNorm(p, dragNorm));
            return true;
        }
        if (menuOpen) {
            const int m = menuHit(x, y);
            if (m != menuHover) { menuHover = m; repaint(); }
            return true;
        }
        const int hh = headerHit(x, y); if (hh != hoverHeader) { hoverHeader = hh; repaint(); }
        const int pb = presetBarHit(x, y);
        if (pb != presetHover) { presetHover = pb; repaint(); }
        const bool hc = clearHit(x, y); if (hc != hoverClear) { hoverClear = hc; repaint(); }
        const int h = pb >= 0 ? -1 : hit(x, y);
        if (h != hover) { hover = h; repaint(); }
        const int ar = h < 0 ? areaHit(x, y) : -1;
        int key = -1;
        if (h >= 0) key = h; else if (hh > 0) key = 1000 + hh; else if (pb >= 0) key = 2000 + pb; else if (ar >= 0) key = 3000 + ar;
        tipX = x; tipY = y;
        if (key != tipKey) { const bool was = tipShown; tipKey = key; tipSince = nowSec(); tipShown = false; if (key != tipSuppressed) tipSuppressed = -2; if (was) repaint(); }
        return false;
    }

    bool onScroll(const ScrollEvent& ev) override {
        const float s = uiScale();
        const float sx = ev.pos.getX() / s, sy = ev.pos.getY() / s;
        if (menuOpen || saving || helpOpen) return true;
        if (presetBarHit(sx, sy) >= 0) { step(ev.delta.getY() > 0 ? -1 : 1); return true; }
        const int i = hit(sx, sy);
        if (i < 0 || kCtls[i].type != Knob) return false;
        const uint32_t p = effParam(kCtls[i]);
        float st = (ev.mod & kModifierShift) ? 0.004f : 0.025f;
        if (paramDef(p).hints & kHintInteger) st = 1.f / (paramDef(p).max - paramDef(p).min);
        beginGesture(); commit(p, fromNorm(p, toNorm(p, val[p]) + (ev.delta.getY() > 0 ? st : -st))); endGesture();
        return true;
    }

    bool flashing() const { return nowSec() < flashUntil; }
    void uiIdle() override {
        if (flashing() != flashShown) { flashShown = flashing(); repaint(); }
        if (gTipsOn && !tipShown && tipKey >= 0 && tipKey != tipSuppressed && active < 0 && !menuOpen && !saving && !helpOpen && editing < 0
            && nowSec() - tipSince > 0.6) { tipShown = true; repaint(); }
    }

    // ---- hover tips ---------------------------------------------------------------
    bool tipContent(std::string& title, std::string& value, std::string& body) const {
        if (tipKey < 0) return false;
        if (tipKey < 1000) {
            const Ctl& c = kCtls[tipKey]; const uint32_t p = effParam(c); char b[40]; fmt(p, b, sizeof b);
            title = paramDef(p).name; value = b; body = paramTip(p);
            if (c.type == Knob) body += "\n\nDrag up/down (Shift for fine). Double-click to type a value, Cmd/Ctrl-click to reset.";
            if (dimmed(c)) body += "\n\n(Not used in Poly mode.)";
            if (paramCC[p] >= 0) { char cc[32]; snprintf(cc, sizeof cc, "   ·   CC%d", paramCC[p]); value += cc; }
            return true;
        }
        int t;
        if (tipKey >= 3000) t = tipKey - 3000;
        else if (tipKey >= 2000) t = kTipPrev + (tipKey - 2000);
        else { const int hh = tipKey - 1000;
            t = hh == 1 ? kTipLearn : hh == 3 ? kTipUndo : hh == 4 ? kTipRedo : hh == 5 ? kTipHelp : hh == 6 ? kTipA : hh == 7 ? kTipB : kTipCopy; }
        title = targetTitle(t); value.clear(); body = targetTip(t);
        if (t == kTipUndo || t == kTipRedo) { const size_t n = t == kTipUndo ? undoStack.size() : redoStack.size(); char b[32]; snprintf(b, sizeof b, "%d step%s", (int)n, n == 1 ? "" : "s"); value = b; }
        return true;
    }
    void drawTooltip() {
        std::string title, value, body;
        if (!tipContent(title, value, body)) return;
        const float w = 320, pad = 12;
        fontSize(11); textLetterSpacing(0.2f); textLineHeight(1.3f);
        float bb[4]; textBoxBounds(0, 0, w - 2 * pad, body.c_str(), nullptr, bb);
        const float h = pad + 16 + 8 + (bb[3] - bb[1]) + pad;
        float x = tipX + 16, y = tipY + 20;
        if (x + w > W - 8) x = tipX - w - 12;
        if (y + h > H - 8) y = tipY - h - 12;
        x = clampf(x, 8, W - w - 8); y = clampf(y, 8, H - h - 8);
        beginPath(); rect(x - 10, y - 4, w + 20, h + 18); fillPaint(boxGradient(x, y + 3, w, h, 6, 14, Color(0, 0, 0, 0.55f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(x, y, w, h, 6); fillColor(Color(0x1e, 0x20, 0x23)); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(10); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_TEXT);
        std::string up = title; for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
        text(x + pad, y + pad + 1, up.c_str(), nullptr);
        if (!value.empty()) { fontSize(10.5f); textLetterSpacing(0.4f); textAlign(ALIGN_RIGHT | ALIGN_TOP); fillColor(C_ACCENT); text(x + w - pad, y + pad + 1, value.c_str(), nullptr); }
        fontSize(11); textLetterSpacing(0.2f); textLineHeight(1.3f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM);
        textBox(x + pad, y + pad + 24, w - 2 * pad, body.c_str(), nullptr);
        textLineHeight(1.f);
    }

    // ---- help window ---------------------------------------------------------------
    static constexpr float HX = 150, HY = 40, HW = 760, HH = 520, TAB_W = 180, TAB_H = 34;
    int helpHit(float x, float y) const {
        if (x < HX || x > HX + HW || y < HY || y > HY + HH) return -2;
        for (int i = 0; i < kNumHelpPages; ++i) if (x >= HX + 12 && x < HX + 12 + TAB_W && y >= HY + 56 + i * TAB_H && y < HY + 56 + (i + 1) * TAB_H) return i;
        const float by = HY + HH - 44;
        if (y >= by && y < by + 28) {
            if (x >= HX + 12 && x < HX + 12 + TAB_W) return 100;
            if (x >= HX + HW - 330 && x < HX + HW - 140) return 101;
            if (x >= HX + HW - 124 && x < HX + HW - 16) return 102;
        }
        if (x >= HX + HW - 40 && y < HY + 40) return 102;
        return -1;
    }
    void helpClick(float x, float y) {
        const int h = helpHit(x, y);
        if (h >= 0 && h < kNumHelpPages) helpPage = h;
        else if (h == 100) gTipsOn = !gTipsOn;
        else if (h == 101) openURL(kManualURL);
        else if (h == 102 || h == -2) helpOpen = false;
        repaint();
    }
    void drawHelp() {
        beginPath(); rect(0, 0, W, H); fillColor(Color(0, 0, 0, 0.55f)); fill();
        beginPath(); roundedRect(HX, HY, HW, HH, 10); fillColor(C_PANEL); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(13); textLetterSpacing(4.f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT); text(HX + 22, HY + 28, "MONOLITH HELP", nullptr);
        const float cx = HX + HW - 24, cy = HY + 26;
        beginPath(); moveTo(cx - 6, cy - 6); lineTo(cx + 6, cy + 6); moveTo(cx + 6, cy - 6); lineTo(cx - 6, cy + 6); strokeColor(helpHover == 102 ? C_TEXT : C_DIM); strokeWidth(1.6f); stroke();
        for (int i = 0; i < kNumHelpPages; ++i) {
            const float ty = HY + 56 + i * TAB_H;
            if (i == helpPage || i == helpHover) { beginPath(); roundedRect(HX + 12, ty + 2, TAB_W, TAB_H - 4, 5); fillColor(i == helpPage ? C_ACCENT_A(40) : C_PANEL_HI); fill(); }
            if (i == helpPage) { beginPath(); roundedRect(HX + 12, ty + 8, 3, TAB_H - 16, 1.5f); fillColor(C_ACCENT); fill(); }
            fontSize(11.5f); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(i == helpPage ? C_TEXT : C_DIM);
            text(HX + 28, ty + TAB_H / 2 + 0.5f, kHelpPages[i].title, nullptr);
        }
        beginPath(); moveTo(HX + TAB_W + 26, HY + 56); lineTo(HX + TAB_W + 26, HY + HH - 56); strokeColor(C_LINE); strokeWidth(1); stroke();
        const float bx = HX + TAB_W + 46, bw = HW - TAB_W - 70;
        fontSize(15); textLetterSpacing(0.8f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_ACCENT); text(bx, HY + 60, kHelpPages[helpPage].title, nullptr);
        fontSize(12); textLetterSpacing(0.2f); textLineHeight(1.38f); fillColor(Color(0xcc, 0xc8, 0xc0));
        textBox(bx, HY + 92, bw, kHelpPages[helpPage].body, nullptr);
        textLineHeight(1.f);
        const float by = HY + HH - 44;
        beginPath(); moveTo(HX + 12, by - 10); lineTo(HX + HW - 12, by - 10); strokeColor(C_LINE); stroke();
        const float tw = 34, th = 18, tx = HX + 16, tyy = by + 14 - th / 2;
        beginPath(); roundedRect(tx, tyy, tw, th, th / 2); fillColor(gTipsOn ? C_ACCENT_A(60) : C_PANEL_HI); fill(); strokeColor(gTipsOn ? C_ACCENT : C_LINE); stroke();
        beginPath(); circle(gTipsOn ? tx + tw - th / 2 : tx + th / 2, by + 14, 6); fillColor(gTipsOn ? C_ACCENT : C_DIM); fill();
        fontSize(10); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(helpHover == 100 ? C_TEXT : C_DIM); text(tx + tw + 10, by + 14.5f, "HOVER TIPS", nullptr);
        auto btn = [&](float x0, float w0, const char* label, bool hot, bool primary) {
            beginPath(); roundedRect(x0, by, w0, 28, 5); fillColor(primary ? (hot ? C_ACCENT_A(80) : C_ACCENT_A(40)) : (hot ? C_PANEL_HI : C_PANEL)); fill();
            strokeColor(primary ? C_ACCENT : C_LINE); strokeWidth(1); stroke();
            fontSize(9.5f); textLetterSpacing(1.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(x0 + w0 / 2, by + 14.5f, label, nullptr);
        };
        btn(HX + HW - 330, 190, "FULL MANUAL (WEB)", helpHover == 101, false);
        btn(HX + HW - 124, 108, "CLOSE", helpHover == 102, true);
    }

private:
    float val[kNumParams], defv[kNumParams], userVals[kParamCount] = {}, before[kParamCount];
    int paramCC[kParamCount];
    int active = -1, hover = -1, lastClick = -1, editing = -1;
    uint lastClickTime = 0;
    float dragY = 0, dragNorm = 0; bool circular = false, gesture = false;
    std::vector<Snap> undoStack, redoStack;
    Snap abStore[2]; int abSlot = 0;
    std::string editBuf, statusText; double flashUntil = 0; bool flashShown = false;
    int curFactory = 0, presetHover = -1, menuHover = -1, pendingDelete = -1, hoverHeader = 0;
    std::string curUser, query; std::vector<std::string> userPresets, favs;
    bool menuOpen = false, saving = false, saveError = false; std::string saveName;
    bool learnMode = false, hoverClear = false; int learnWaiting = -1; float lastLearnEcho = 0;
    int tipKey = -1, tipSuppressed = -2; double tipSince = 0; bool tipShown = false; float tipX = 0, tipY = 0;
    bool helpOpen = false; int helpPage = 0, helpHover = -1;
    ResizeHandle resizeHandle;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonolithUI)
};

UI* createUI() { return new MonolithUI(); }

END_NAMESPACE_DISTRHO
