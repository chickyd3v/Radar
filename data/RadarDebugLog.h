#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

namespace RadarData {

// #region agent log
inline void DebugSessionLog(const std::filesystem::path& workspaceRoot,
                            const char* hypothesisId, const char* location,
                            const std::string& message, const std::string& dataJson) {
    static std::mutex mu;
    std::lock_guard<std::mutex> lock(mu);
    const auto path = workspaceRoot / "debug-94900e.log";
    std::ofstream out(path, std::ios::app);
    if (!out.is_open()) return;
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
    out << "{\"sessionId\":\"94900e\",\"hypothesisId\":\"" << hypothesisId
        << "\",\"location\":\"" << location << "\",\"message\":\"" << message
        << "\",\"data\":" << dataJson << ",\"timestamp\":" << ms << "}\n";
}
// #endregion

} // namespace RadarData
