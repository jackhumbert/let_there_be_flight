#pragma once
#include <filesystem>

namespace Utils
{
void CreateLogger();
std::filesystem::path GetRootDir();
std::wstring ToWString(const char* aText);
}
