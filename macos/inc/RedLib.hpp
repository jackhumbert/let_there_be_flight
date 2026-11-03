#pragma once
// LTBF R41 (macOS): RedLib from the macOS Codeware fork (cp2077-codeware/lib/Red, which carries the Mac fixes: RawFunc returns,
// exact integer types). This is the include list of Codeware's macos/pch.hpp (RED4ext headers RedLib expects first), without
// Codeware's own Core and address library, then the same RedLib entry points as jackhumbert's red_lib RedLib.hpp.


#include <algorithm>
#include <codecvt>
#include <cwctype>
#include <mutex>
#include <shared_mutex>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <future>
#include <map>
#include <memory>
#include <ranges>
#include <regex>
#include <set>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <RED4ext/RED4ext.hpp>
#include "LtbfMacGenerated.hpp"

#include <RED4ext/JobQueue.hpp>
#include <RED4ext/Package.hpp>
#include <RED4ext/ResourceDepot.hpp>
#include <RED4ext/ResourceLoader.hpp>
#include <RED4ext/SystemUpdate.hpp>

#include <RED4ext/StringView.hpp>

#include <nameof.hpp>




#include "Red/Alias.hpp"
#include "Red/Engine.hpp"
#include "Red/Specializations.hpp"
#include "Red/TypeInfo.hpp"
#include "Red/Utils.hpp"

