#include "Telemetry.hpp"
#include "Utils/Utils.hpp"
#include "stdafx.hpp"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <map>
#include <mutex>
#include <string>

namespace {
struct File {
  FILE *file;
  uint32_t rows;
};
std::mutex lock;
std::map<int32_t, File> files;
int32_t nextFile = 1;
constexpr uint32_t kFlushEvery = 120; // rows (about 2 s): a crash keeps all but the last moments
} // namespace

int32_t FlightTelemetry::Begin(RED4ext::CString aName, RED4ext::CString aHeader) {
  auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm local{};
  localtime_s(&local, &now);
  char stamp[32];
  std::strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", &local);
  auto dir = Utils::GetRootDir() / "red4ext" / "logs";
  std::error_code error;
  std::filesystem::create_directories(dir, error);
  auto path = dir / (std::string("ltbf_") + aName.c_str() + "_" + stamp + ".csv");
  FILE *file = nullptr;
  if (_wfopen_s(&file, path.c_str(), L"w") != 0 || !file) {
    spdlog::warn("[Telemetry] cannot open {}", path.string());
    return 0;
  }
  std::setvbuf(file, nullptr, _IOFBF, 1 << 16);
  std::fputs(aHeader.c_str(), file);
  std::fputc('\n', file);
  std::lock_guard guard(lock);
  int32_t id = nextFile++;
  files[id] = {file, 0};
  spdlog::info("[Telemetry] writing {}", path.string());
  return id;
}

bool FlightTelemetry::IsOpen(int32_t aFile) {
  std::lock_guard guard(lock);
  return files.contains(aFile);
}

void FlightTelemetry::Row(int32_t aFile, RED4ext::DynArray<float> aValues) {
  std::lock_guard guard(lock);
  auto it = files.find(aFile);
  if (it == files.end()) {
    return;
  }
  for (uint32_t i = 0; i < aValues.size; ++i) {
    // 9 digits round-trip a float: 6 cut a wall clock (WallClock) to a tenth of a second
    std::fprintf(it->second.file, i == 0 ? "%.9g" : ",%.9g", aValues[i]);
  }
  std::fputc('\n', it->second.file);
  if (++it->second.rows % kFlushEvery == 0) {
    std::fflush(it->second.file);
  }
}

// Seconds since midnight UTC (to a few milliseconds as a float), to line rows up with a recording
// made outside the game (a test's video starts at a wall-clock time).
float FlightTelemetry::WallClock() {
  auto now = std::chrono::system_clock::now().time_since_epoch();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count() % 86400000;
  return float(ms) / 1000.0f;
}

void FlightTelemetry::End(int32_t aFile) {
  std::lock_guard guard(lock);
  auto it = files.find(aFile);
  if (it != files.end()) {
    std::fclose(it->second.file);
    spdlog::info("[Telemetry] {} rows written", it->second.rows);
    files.erase(it);
  }
}
