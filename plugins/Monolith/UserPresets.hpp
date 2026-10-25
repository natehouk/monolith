// User preset files: one plain-text file per preset ("symbol=value" lines), same format on every OS.
// License: MIT.
#pragma once
#include "ParamDefs.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

namespace mono {

inline std::string userPresetDir() {
    std::string base;
#if defined(_WIN32)
    const char* a = std::getenv("APPDATA");
    base = std::string(a ? a : ".") + "\\Epsilon Audio\\Monolith\\Presets";
#elif defined(__APPLE__)
    const char* h = std::getenv("HOME");
    base = std::string(h ? h : ".") + "/Library/Audio/Presets/Epsilon Audio/Monolith";
#else
    const char* x = std::getenv("XDG_CONFIG_HOME");
    const char* h = std::getenv("HOME");
    base = x ? std::string(x) + "/epsilon-audio/monolith/presets" : std::string(h ? h : ".") + "/.config/epsilon-audio/monolith/presets";
#endif
    return base;
}

inline void makeDirs(const std::string& path) {
    std::string cur;
    for (size_t i = 0; i < path.size(); ++i) {
        cur += path[i];
        if ((path[i] == '/' || path[i] == '\\' || i + 1 == path.size()) && cur.size() > 1) {
#ifdef _WIN32
            _mkdir(cur.c_str());
#else
            mkdir(cur.c_str(), 0755);
#endif
        }
    }
}

inline std::string sanitizeName(const std::string& in) {
    std::string s;
    for (char c : in) if (c != '/' && c != '\\' && c != ':' && c != '*' && c != '?' && c != '"' && c != '<' && c != '>' && c != '|') s += c;
    while (!s.empty() && (s.back() == ' ' || s.back() == '.')) s.pop_back();
    while (!s.empty() && s.front() == ' ') s.erase(s.begin());
    return s.substr(0, 48);
}

static const char* const kPresetExt = ".monolith";

inline std::vector<std::string> listUserPresets() {
    std::vector<std::string> out;
    DIR* d = opendir(userPresetDir().c_str());
    if (!d) return out;
    const size_t el = std::strlen(kPresetExt);
    while (dirent* e = readdir(d)) {
        std::string n = e->d_name;
        if (n.size() > el && n.compare(n.size() - el, el, kPresetExt) == 0) out.push_back(n.substr(0, n.size() - el));
    }
    closedir(d);
    std::sort(out.begin(), out.end(), [](const std::string& a, const std::string& b) {
        std::string x = a, y = b;
        for (char& c : x) c = (char)std::tolower((unsigned char)c);
        for (char& c : y) c = (char)std::tolower((unsigned char)c);
        return x < y;
    });
    return out;
}

inline bool saveUserPreset(const std::string& rawName, const float* values) {
    const std::string name = sanitizeName(rawName);
    if (name.empty()) return false;
    const std::string dir = userPresetDir();
    makeDirs(dir);
    FILE* f = std::fopen((dir + "/" + name + kPresetExt).c_str(), "wb");
    if (!f) return false;
    std::fprintf(f, "# Monolith preset\nformat=1\n");
    for (uint32_t i = 0; i < kParamCount; ++i) std::fprintf(f, "%s=%.6g\n", paramDef(i).sym, values[i]);
    std::fclose(f);
    return true;
}

// Missing keys keep their defaults, so presets from older versions still load.
inline bool loadUserPreset(const std::string& name, float* values) {
    FILE* f = std::fopen((userPresetDir() + "/" + name + kPresetExt).c_str(), "rb");
    if (!f) return false;
    Engine def;
    for (uint32_t i = 0; i < kParamCount; ++i) values[i] = def.p[i];
    char line[256];
    while (std::fgets(line, sizeof line, f)) {
        char* eq = std::strchr(line, '=');
        if (!eq || line[0] == '#') continue;
        *eq = 0;
        const float v = (float)std::atof(eq + 1);
        for (uint32_t i = 0; i < kParamCount; ++i)
            if (std::strcmp(paramDef(i).sym, line) == 0) { values[i] = clampf(v, paramDef(i).min, paramDef(i).max); break; }
    }
    std::fclose(f);
    return true;
}

inline bool deleteUserPreset(const std::string& name) {
    return std::remove((userPresetDir() + "/" + name + kPresetExt).c_str()) == 0;
}

} // namespace mono
