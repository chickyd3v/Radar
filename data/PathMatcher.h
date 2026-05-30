#pragma once

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace RadarData {

inline std::string NormalizeAreaKey(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

inline bool AreaKeysEqual(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i]))
            != std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    }
    return true;
}

inline std::string NormalizePathLower(std::string_view path) {
    std::string out(path);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

// Game updates may use .tdtx, .tdbx, .tdb, .ot — match on path stem only.
inline std::string CanonicalTerrainPath(std::string_view path) {
    std::string s = NormalizePathLower(path);
    const auto colon = s.find(':');
    if (colon != std::string::npos) s.resize(colon);
    static const char* kExts[] = {".tdtx", ".tdbx", ".tdb", ".ot", ".idle"};
    for (const char* ext : kExts) {
        const size_t elen = strlen(ext);
        if (s.size() >= elen && s.compare(s.size() - elen, elen, ext) == 0) {
            s.resize(s.size() - elen);
            break;
        }
    }
    return s;
}

struct CompiledPattern {
    std::string normalized;
    bool        hasWildcard = false;
};

inline CompiledPattern CompilePattern(std::string_view path) {
    CompiledPattern p;
    p.normalized = CanonicalTerrainPath(path);
    p.hasWildcard = p.normalized.find('*') != std::string::npos;
    return p;
}

inline bool MatchPattern(const CompiledPattern& pattern, std::string_view candidateRaw) {
    if (pattern.normalized.empty()) return false;
    const std::string candidate = CanonicalTerrainPath(candidateRaw);
    if (!pattern.hasWildcard) {
        return candidate == pattern.normalized;
    }
    size_t pi = 0, ci = 0;
    const auto& pat = pattern.normalized;
    while (pi < pat.size() && ci < candidate.size()) {
        if (pat[pi] == '*') {
            ++pi;
            if (pi >= pat.size()) return true;
            const size_t next = candidate.find(pat[pi], ci);
            if (next == std::string::npos) return false;
            ci = next;
            continue;
        }
        if (pat[pi] != candidate[ci]) return false;
        ++pi;
        ++ci;
    }
    while (pi < pat.size() && pat[pi] == '*') ++pi;
    return pi >= pat.size() && ci >= candidate.size();
}

struct PatternMatcherSet {
    std::vector<CompiledPattern> patterns;
    void Add(std::string_view path) { patterns.push_back(CompilePattern(path)); }
    bool MatchesAny(std::string_view candidate) const {
        for (const auto& p : patterns)
            if (MatchPattern(p, candidate)) return true;
        return false;
    }
};

} // namespace RadarData
