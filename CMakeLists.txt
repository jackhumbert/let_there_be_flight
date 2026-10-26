cmake_minimum_required(VERSION 3.24)
# third-party submodules (pugixml, detours, spdlog) still declare <3.5 minimums; CMake 4 refuses those without this
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/cyberpunk_cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/red4ext.sdk/cmake")

include(ConfigureVersionFromGit)
configure_version_from_git()

project(let_there_be_flight VERSION ${GIT_VERSION_MAJOR}.${GIT_VERSION_MINOR}.${GIT_VERSION_PATCH} LANGUAGES NONE)

include(CyberpunkMod)

configure_mod(
  NAME "Let There Be Flight"
  SLUG ${PROJECT_NAME}
  PREFIX LTBF
  AUTHOR "Jack Humbert"
  URL "https://github.com/jackhumbert/let_there_be_flight"
  LICENSE "Licensed under the MIT license. See the license.md in the root project for details."
)
set(LOAD_TWEAKS_FROM_RED4EXT ON)
set(LOAD_ARCHIVES_FROM_RED4EXT ON)
set(LOAD_INPUTS_FROM_RED4EXT ON)
set(LOAD_REDSCRIPT_FROM_RED4EXT ON)

find_program(ZOLTAN_CLANG_EXE NAMES zoltan-clang.exe PATHS "${MOD_TOOLS_DIR}" CACHE)
find_program(REDSCRIPT_CLI_EXE NAMES redscript-cli.exe PATHS "${MOD_TOOLS_DIR}" CACHE)
find_program(CYBERPUNK_2077_EXE NAMES Cyberpunk2077.exe PATHS "${CYBERPUNK_2077_GAME_DIR}/bin/x64" CACHE DOC "Cyberpunk2077.exe Executable File")

set(MOD_GAME_DIR_FMOD_FOLDER "${MOD_GAME_DIR}/red4ext/plugins/${MOD_SLUG}")
set(LTBF_SOURCE_FMOD_DIR "${PROJECT_SOURCE_DIR}/src/fmod_studio/Build/Desktop")
set(ZOLTAN_CLANG_EXE "C:/Users/Jack/Documents/cyberpunk/zoltan/target/release/zoltan-clang.exe")

# FMOD

file(GLOB LTBF_SOURCE_FMOD_FILES ${LTBF_SOURCE_FMOD_DIR}/*)
foreach(_FILE ${LTBF_SOURCE_FMOD_FILES})
  file(RELATIVE_PATH _TEMPFILE ${LTBF_SOURCE_FMOD_DIR} ${_FILE})
  configure_mod_file(/src/fmod_studio/Build/Desktop/${_TEMPFILE} red4ext/plugins/${MOD_SLUG}/${_TEMPFILE})
endforeach()

# Autopilot nav map, baked offline by tools/heightmap/ from the game's collision. Too big for git, so it is a
# release asset (tag navmap-<game version>) fetched once into build/downloads. After a rebake: upload the new
# navmap.bin under a new tag and update the URL and hash here. LTBF_NAVMAP_FILE points at a local bake instead.

set(LTBF_NAVMAP_URL "https://github.com/jackhumbert/let_there_be_flight/releases/download/navmap-2.50/navmap.bin")
set(LTBF_NAVMAP_SHA256 "1583a1ba2fcb3eb7f7ae1f8c83d454c03a58c4edddf0c607c8c5ad4c2cd6d04f")
set(LTBF_NAVMAP_FILE "" CACHE FILEPATH "Local navmap.bin to ship instead of the release asset")
if(LTBF_NAVMAP_FILE)
  set(_NAVMAP "${LTBF_NAVMAP_FILE}")
else()
  string(SUBSTRING ${LTBF_NAVMAP_SHA256} 0 12 _NAVMAP_ID)
  set(_NAVMAP "${MOD_BINARY_DIR}/downloads/navmap-${_NAVMAP_ID}.bin")
  if(NOT EXISTS "${_NAVMAP}")
    message(STATUS "Downloading ${LTBF_NAVMAP_URL}")
    file(DOWNLOAD ${LTBF_NAVMAP_URL} "${_NAVMAP}.part" STATUS _NAVMAP_STATUS)
    list(GET _NAVMAP_STATUS 0 _NAVMAP_ERROR)
    if(_NAVMAP_ERROR EQUAL 0)
      file(SHA256 "${_NAVMAP}.part" _NAVMAP_GOT)
      if(_NAVMAP_GOT STREQUAL LTBF_NAVMAP_SHA256)
        file(RENAME "${_NAVMAP}.part" "${_NAVMAP}")
      else()
        set(_NAVMAP_STATUS "1;SHA256 ${_NAVMAP_GOT}, expected ${LTBF_NAVMAP_SHA256}")
      endif()
    endif()
    file(REMOVE "${_NAVMAP}.part")
  endif()
endif()
if(EXISTS "${_NAVMAP}")
  configure_mod_file("${_NAVMAP}" red4ext/plugins/${MOD_SLUG}/navmap.bin)
elseif(CMAKE_CI_BUILD)
  message(FATAL_ERROR "navmap.bin download failed: ${_NAVMAP_STATUS}")
else()
  message(WARNING "navmap.bin download failed (${_NAVMAP_STATUS}); the autopilot will be disabled")
endif()

# Landing pads found in the world sectors and checked against the nav map (tools/heightmap/pads.py, small, in git).
configure_mod_file(src/navigation/helipads.tsv red4ext/plugins/${MOD_SLUG}/helipads.tsv)

configure_red4ext(src/red4ext)
# configure_red4ext_addresses(Addresses.hpp)
set(MOD_NATIVE_TWEAKS ON)
set(MOD_YAML_TWEAKS ON)
configure_tweaks(src/tweaks)
# Tweaks always go through TweakXL. tools/tweaks_to_json.py can write them as JSON for the game's built-in
# TweakDB loader (2.5+), but that loader crashes on new typed flats, so the JSON is not shipped: the built-in
# loader would pick up anything under a "tweaks" folder in the plugin directory.
configure_archives(src/wolvenkit/packed/archive/pc/mod/let_there_be_flight.archive)
# One config, installed twice: .archive.xl is read by ArchiveXL when it is installed, .archive.json by the
# game's built-in loader (2.5+, via GameModFolders.hpp) when it is not. JSON is valid YAML, so ArchiveXL reads it too.
configure_mod_file(src/archive_xl/${MOD_SLUG}.archive.json red4ext/plugins/${MOD_SLUG}/${MOD_SLUG}.archive.json)
configure_mod_file(src/archive_xl/${MOD_SLUG}.archive.json red4ext/plugins/${MOD_SLUG}/${MOD_SLUG}.archive.xl)

find_package(Detours)
find_package(FMOD)
find_package(Spdlog)
find_package(RedLib)
find_package(PhysX3)
find_package(ModSettings)
find_package(InputLoader)
find_package(ArchiveXL)
list(TRANSFORM MOD_REQUIREMENTS REPLACE "^(ArchiveXL .*)$" "\\1 (optional on game 2.5+)")
find_package(TweakXL)

target_link_libraries(let_there_be_flight.dll
  PUBLIC
    Detours
    FMOD
    spdlog
    RedLib
    PhysX3
    input_loader.dll
    ArchiveXL
    TweakXL
)

# configure_inputs(src/input_loader/let_there_be_flight.xml)
configure_mod_file(src/input_loader/${MOD_SLUG}.xml red4ext/plugins/${MOD_SLUG}/inputs.xml)
configure_redscript(src/redscript)

target_link_libraries(${MOD_SLUG}.packed.reds PUBLIC mod_settings.packed.reds)

configure_folder_file(readme.md)
configure_folder_file(license.md)

list(APPEND MOD_REQUIREMENTS "Input Loader v0.2.3+")
list(APPEND MOD_REQUIREMENTS "Mod Settings v0.2.23+ (optional)")

configure_uninstall()
configure_release(${MOD_SLUG}_${MOD_VERSION_STR}.zip)
configure_install()