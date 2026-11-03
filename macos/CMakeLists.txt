cmake_minimum_required(VERSION 3.21)
# R41. arm64 build of Let There Be Flight for the native macOS game (2.3.1). Pattern: cp2077-codeware/macos/CMakeLists.txt.
# Windows builds keep using the top-level CMakeLists.txt; nothing here is read by it.
project(let_there_be_flight VERSION 0.3.18 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64")

if(NOT APPLE)
    message(FATAL_ERROR "This target builds the macOS Let There Be Flight dylib")
endif()

option(LTBF_MAC_NO_AUDIO "Build without FMOD (FlightAudio becomes a logged no-op)" OFF)

set(LTBF_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(LTBF_SRC "${LTBF_DIR}/src/red4ext")
set(MAC_REPOS "${LTBF_DIR}/../.." CACHE PATH "the folder that holds the sibling checkouts")
set(RED4EXT_SDK_DIR "${MAC_REPOS}/RED4ext.SDK-macos" CACHE PATH "macOS RED4ext SDK checkout")
set(CODEWARE_DIR "${MAC_REPOS}/cp2077-codeware" CACHE PATH "macOS Codeware checkout (RedLib and the SDK rows are reused)")
set(FMOD_DIR "${LTBF_DIR}/../fmod-2.04.00" CACHE PATH "FMOD Engine 2.04 for macOS (core/ and studio/ with inc and lib); never committed")
set(SDK_INCLUDE "${RED4EXT_SDK_DIR}/include")

add_compile_definitions(RED4EXT_PLATFORM_MACOS RED4EXT_HEADER_ONLY)
add_compile_definitions(
    MOD_VERSION_STR="${PROJECT_VERSION}"
    MOD_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    MOD_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    MOD_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    MOD_PACKED_NATIVE_TWEAKS_FILENAME="let_there_be_flight.tweak"
    MOD_PACKED_YAML_TWEAKS_FILENAME="let_there_be_flight.yaml"
)
if(LTBF_MAC_NO_AUDIO)
    add_compile_definitions(LTBF_MAC_NO_AUDIO=1)
endif()

# Game symbols resolve at load time inside the game process (RED4ext, TweakXL's vault functions), as for Codeware.
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")

get_filename_component(_repo_root "${LTBF_DIR}" ABSOLUTE)
get_filename_component(_sdk_root "${RED4EXT_SDK_DIR}" ABSOLUTE)
get_filename_component(_cw_root "${CODEWARE_DIR}" ABSOLUTE)
add_compile_options("-ffile-prefix-map=${_repo_root}=." "-ffile-prefix-map=${_sdk_root}=RED4ext.SDK-macos"
                    "-ffile-prefix-map=${_cw_root}=cp2077-codeware" "-ffile-prefix-map=${CMAKE_BINARY_DIR}=build")

find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

# ---------------------------------------------------------------------------------------------------------------------------------
# Generated SDK overlay (build/sdk-overlay, first on the include path):
#  1. Relocation-inl.hpp: the Mac SDK table plus Codeware's MacSdkRows.inc (the SDK-core rows R40 found ALREADY) plus
#     macos/LtbfSdkRows.inc (this plugin's rows). An unknown hash goes to ltbf_mac_trap_for_hash (macos/MacShims.cpp):
#     a stub that logs once and returns 0. Nothing ever resolves to 0.
#  2. Relocation.hpp: the Mac SDK header plus the jackhumbert-fork additions LTBF's sources use (IHookable, UniRelocFunc).
set(SDK_OVERLAY "${CMAKE_CURRENT_BINARY_DIR}/sdk-overlay")
set(SDK_RELOC "${SDK_INCLUDE}/RED4ext/Relocation-inl.hpp")
set(SDK_RELOC_H "${SDK_INCLUDE}/RED4ext/Relocation.hpp")
set(CW_ROWS_INC "${CODEWARE_DIR}/macos/MacSdkRows.inc")
set(LTBF_ROWS_INC "${CMAKE_CURRENT_SOURCE_DIR}/LtbfSdkRows.inc")
file(READ "${SDK_RELOC}" SDK_RELOC_TEXT)
file(READ "${CW_ROWS_INC}" CW_ROWS_TEXT)
file(READ "${LTBF_ROWS_INC}" LTBF_ROWS_TEXT)
set(SDK_ROWS_MARKER "        { 4201325776, 0x109BAA8 },  // CGlobalFunction_ctor\n")
string(FIND "${SDK_RELOC_TEXT}" "${SDK_ROWS_MARKER}" SDK_ROWS_POS)
if(SDK_ROWS_POS EQUAL -1)
    message(FATAL_ERROR "the anchor row (CGlobalFunction_ctor) is not in ${SDK_RELOC}")
endif()
string(REPLACE "${SDK_ROWS_MARKER}" "${SDK_ROWS_MARKER}        // --- cp2077-codeware/macos/MacSdkRows.inc ---\n${CW_ROWS_TEXT}        // --- let_there_be_flight/macos/LtbfSdkRows.inc ---\n${LTBF_ROWS_TEXT}" SDK_RELOC_TEXT "${SDK_RELOC_TEXT}")
set(SDK_TAIL_START "    // Address not found or placeholder - return a non-zero sentinel value")
set(SDK_TAIL_END "    return 0;\n#else\n    if constexpr (Detail::AddressResolverOverride<uint32_t>::value)")
string(FIND "${SDK_RELOC_TEXT}" "${SDK_TAIL_START}" SDK_TAIL_A)
string(FIND "${SDK_RELOC_TEXT}" "${SDK_TAIL_END}" SDK_TAIL_B)
if(SDK_TAIL_A EQUAL -1 OR SDK_TAIL_B EQUAL -1)
    message(FATAL_ERROR "the unknown-hash tail of Resolve was not found in ${SDK_RELOC}")
endif()
string(SUBSTRING "${SDK_RELOC_TEXT}" 0 ${SDK_TAIL_A} SDK_HEAD)
math(EXPR SDK_REST_FROM "${SDK_TAIL_B} + 14")
string(SUBSTRING "${SDK_RELOC_TEXT}" ${SDK_REST_FROM} -1 SDK_REST)
set(SDK_RELOC_TEXT "${SDK_HEAD}    // LTBF R41: fail-safe trap. Logs the hash once, returns a stub that logs its first call and returns 0.\n    return ltbf_mac_trap_for_hash(aHash);\n${SDK_REST}")
set(SDK_DECL_MARKER "extern \"C\" uintptr_t tweakxl_class_vslot(uint32_t aByteOffset);\n")
string(FIND "${SDK_RELOC_TEXT}" "${SDK_DECL_MARKER}" SDK_DECL_POS)
if(SDK_DECL_POS EQUAL -1)
    message(FATAL_ERROR "the extern declaration anchor was not found in ${SDK_RELOC}")
endif()
string(REPLACE "${SDK_DECL_MARKER}" "${SDK_DECL_MARKER}extern \"C\" uintptr_t ltbf_mac_trap_for_hash(uint32_t aHash);\n" SDK_RELOC_TEXT "${SDK_RELOC_TEXT}")
file(WRITE "${SDK_OVERLAY}/RED4ext/Relocation-inl.hpp" "${SDK_RELOC_TEXT}")

file(READ "${SDK_RELOC_H}" SDK_RELOC_H_TEXT)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/sdk-overlay/RelocationForkAdditions.inc" FORK_RELOC_TEXT)
set(SDK_H_MARKER "} // namespace RED4ext\n\n#ifdef RED4EXT_HEADER_ONLY\n#include <RED4ext/Relocation-inl.hpp>")
string(FIND "${SDK_RELOC_H_TEXT}" "${SDK_H_MARKER}" SDK_H_POS)
if(SDK_H_POS EQUAL -1)
    message(FATAL_ERROR "the namespace end anchor was not found in ${SDK_RELOC_H}")
endif()
string(REPLACE "${SDK_H_MARKER}" "${FORK_RELOC_TEXT}\n${SDK_H_MARKER}" SDK_RELOC_H_TEXT "${SDK_RELOC_H_TEXT}")
file(WRITE "${SDK_OVERLAY}/RED4ext/Relocation.hpp" "${SDK_RELOC_H_TEXT}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${SDK_RELOC}" "${SDK_RELOC_H}" "${CW_ROWS_INC}" "${LTBF_ROWS_INC}"
             "${CMAKE_CURRENT_SOURCE_DIR}/sdk-overlay/RelocationForkAdditions.inc")

# 3. A few core SDK headers with the fork's small helpers inserted (IsOfClass, StaticArray::operator[], CRUID::Next,
#    Entity::GetComponent, ExecuteFunction(ScriptInstance, ...)): macos/gen_sdk_overlay.py.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/gen_sdk_overlay.py" "${SDK_INCLUDE}" "${SDK_OVERLAY}"
    RESULT_VARIABLE GEN_OVERLAY_RESULT
)
if(NOT GEN_OVERLAY_RESULT EQUAL 0)
    message(FATAL_ERROR "macos/gen_sdk_overlay.py failed (${GEN_OVERLAY_RESULT})")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/gen_sdk_overlay.py"
             "${SDK_INCLUDE}/RED4ext/ISerializable.hpp" "${SDK_INCLUDE}/RED4ext/ISerializable-inl.hpp"
             "${SDK_INCLUDE}/RED4ext/NativeTypes.hpp" "${SDK_INCLUDE}/RED4ext/Scripting/Utils.hpp"
             "${SDK_INCLUDE}/RED4ext/Scripting/Natives/entEntity.hpp" "${SDK_INCLUDE}/RED4ext/RTTITypes.hpp"
             "${SDK_INCLUDE}/RED4ext/Scripting/Natives/Vector3.hpp")

# ---------------------------------------------------------------------------------------------------------------------------------
file(GLOB_RECURSE LTBF_SOURCES CONFIGURE_DEPENDS "${LTBF_SRC}/*.cpp")
list(REMOVE_ITEM LTBF_SOURCES "${LTBF_SRC}/stdafx.cpp")
# Utils/FileStream.cpp is a Win32 file-handle BaseStream that nothing in the plugin uses (and the macOS SDK has no BaseStream).
list(REMOVE_ITEM LTBF_SOURCES "${LTBF_SRC}/Utils/FileStream.cpp")
set(LTBF_MAC_SOURCES
    MacShims.cpp
    MacRedLib.cpp
    MacVault.cpp
)

add_library(let_there_be_flight SHARED ${LTBF_SOURCES} ${LTBF_MAC_SOURCES})
set_target_properties(let_there_be_flight PROPERTIES OUTPUT_NAME "let_there_be_flight" PREFIX "" SUFFIX ".dylib")

target_include_directories(let_there_be_flight PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/support"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${SDK_OVERLAY}"
    "${CMAKE_CURRENT_SOURCE_DIR}/sdk-overlay"
    "${SDK_INCLUDE}"
    "${CODEWARE_DIR}/lib"
    "${CODEWARE_DIR}/vendor/nameof/include"
    "${LTBF_SRC}"
    "${LTBF_DIR}/deps/physx"
)
# Discovery aid while porting: the jackhumbert fork as the last include directory. The headers the build really uses are copied into
# macos/sdk-overlay (see README); the release build has this OFF.
option(LTBF_MAC_FORK_DISCOVERY "append deps/red4ext.sdk/include as the last include directory" OFF)
if(LTBF_MAC_FORK_DISCOVERY)
    target_include_directories(let_there_be_flight PRIVATE "${LTBF_DIR}/deps/red4ext.sdk/include")
endif()
if(LTBF_MAC_NO_AUDIO)
    # No FMOD: a no-op stand-in for the FMOD calls LTBF makes (macos/fmod-stub); nothing is linked.
    target_include_directories(let_there_be_flight PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/fmod-stub")
else()
    target_include_directories(let_there_be_flight PRIVATE "${FMOD_DIR}/core/inc" "${FMOD_DIR}/studio/inc")
    target_link_libraries(let_there_be_flight PRIVATE "${FMOD_DIR}/core/lib/libfmod.dylib" "${FMOD_DIR}/studio/lib/libfmodstudio.dylib")
endif()

target_link_libraries(let_there_be_flight PRIVATE spdlog::spdlog fmt::fmt)

# The Windows build (deps/cyberpunk_cmake configure_red4ext) uses src/red4ext/stdafx.hpp as the precompiled header; so does this one.
target_precompile_headers(let_there_be_flight PRIVATE "${LTBF_SRC}/stdafx.hpp")

target_compile_options(let_there_be_flight PRIVATE
    -fexceptions
    -fms-extensions
    -Wno-deprecated-declarations
    -Wno-microsoft-template
    -Wno-invalid-offsetof
    -fvisibility=hidden
    -fvisibility-inlines-hidden
)

# The plugin folder carries spdlog, fmt and the FMOD libraries next to the dylib: rewrite their install names to @loader_path,
# add @loader_path as an rpath (libfmodstudio asks for @rpath/libfmod.dylib) and re-sign ad hoc.
add_custom_command(TARGET let_there_be_flight POST_BUILD
    COMMAND /bin/bash "${CMAKE_CURRENT_SOURCE_DIR}/fix-install-names.sh" "$<TARGET_FILE:let_there_be_flight>"
    COMMAND /bin/bash "${CMAKE_CURRENT_SOURCE_DIR}/check-undefined.sh" "$<TARGET_FILE:let_there_be_flight>"
    VERBATIM
)

if(LTBF_MAC_NO_AUDIO)
    message(STATUS "Let There Be Flight ${PROJECT_VERSION} configured for macOS (audio: off)")
else()
    message(STATUS "Let There Be Flight ${PROJECT_VERSION} configured for macOS (audio: on, FMOD ${FMOD_DIR})")
endif()
