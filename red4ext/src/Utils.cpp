#include "stdafx.hpp"
#include "Utils.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

void Utils::CreateLogger()
{
    auto rootDir = GetRootDir();
    auto red4extDir = rootDir / L"red4ext";
    auto logsDir = red4extDir / L"logs";
    auto logFile = logsDir / "flight_control.log";

    auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile.string(), true);

    spdlog::sinks_init_list sinks = {console, file};

    auto logger = std::make_shared<spdlog::logger>("", sinks);
    spdlog::set_default_logger(logger);

    logger->flush_on(spdlog::level::trace);
    spdlog::set_level(spdlog::level::trace);
}

std::filesystem::path Utils::GetRootDir()
{
    constexpr auto pathLength = MAX_PATH + 1;

    // Try to get the executable path until we can fit the length of the path.
    std::wstring filename;
    do
    {
        filename.resize(filename.size() + pathLength, '\0');

        auto length = GetModuleFileName(nullptr, filename.data(), static_cast<uint32_t>(filename.size()));
        if (length > 0)
        {
            // Resize it to the real, std::filesystem::path" will use the string's length instead of recounting it.
            filename.resize(length);
        }
    } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);

    auto rootDir = std::filesystem::path(filename)
                       .parent_path()  // Resolve to "x64" directory.
                       .parent_path()  // Resolve to "bin" directory.
                       .parent_path(); // Resolve to game root directory.

    return rootDir;
}

std::wstring Utils::ToWString(const char* aText)
{
    auto length = strlen(aText);

    std::wstring result(L"", length);
    mbstowcs(result.data(), aText, length);

    return result;
}
