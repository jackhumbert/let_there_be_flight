#include "Telemetry.hpp"
#include "Utils/Utils.hpp"
#include "stdafx.hpp"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <mutex>
#include <string>

namespace {
std::mutex lock;
FILE *file = nullptr;
uint32_t rows = 0;
constexpr uint32_t kFlushEvery = 120; // rows (about 2 s): a crash keeps all but the last moments
} // namespace

bool FlightTelemetry::Begin(RED4ext::CString aName, RED4ext::CString aHeader) {
  std::lock_guard guard(lock);
  if (file) {
    std::fclose(file);
    file = nullptr;
  }
  auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm local{};
  localtime_s(&local, &now);
  char stamp[32];
  std::strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", &local);
  auto dir = Utils::GetRootDir() / "red4ext" / "logs";
  std::error_code error;
  std::filesystem::create_directories(dir, error);
  auto path = dir / (std::string("ltbf_") + aName.c_str() + "_" + stamp + ".csv");
  if (_wfopen_s(&file, path.c_str(), L"w") != 0 || !file) {
    file = nullptr;
    spdlog::warn("[Telemetry] cannot open {}", path.string());
    return false;
  }
  std::setvbuf(file, nullptr, _IOFBF, 1 << 16);
  std::fputs(aHeader.c_str(), file);
  std::fputc('\n', file);
  rows = 0;
  spdlog::info("[Telemetry] writing {}", path.string());
  return true;
}

bool FlightTelemetry::IsOpen() {
  std::lock_guard guard(lock);
  return file != nullptr;
}

void FlightTelemetry::Row(RED4ext::DynArray<float> aValues) {
  std::lock_guard guard(lock);
  if (!file) {
    return;
  }
  for (uint32_t i = 0; i < aValues.size; ++i) {
    std::fprintf(file, i == 0 ? "%.6g" : ",%.6g", aValues[i]);
  }
  std::fputc('\n', file);
  if (++rows % kFlushEvery == 0) {
    std::fflush(file);
  }
}

void FlightTelemetry::End() {
  std::lock_guard guard(lock);
  if (file) {
    std::fclose(file);
    file = nullptr;
    spdlog::info("[Telemetry] {} rows written", rows);
  }
}
