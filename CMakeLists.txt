cmake_minimum_required(VERSION 3.24)

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

# FMOD

file(GLOB LTBF_SOURCE_FMOD_FILES ${LTBF_SOURCE_FMOD_DIR}/*)
foreach(_FILE ${LTBF_SOURCE_FMOD_FILES})
  file(RELATIVE_PATH _TEMPFILE ${LTBF_SOURCE_FMOD_DIR} ${_FILE})
  configure_mod_file(/src/fmod_studio/Build/Desktop/${_TEMPFILE} red4ext/plugins/${MOD_SLUG}/${_TEMPFILE})
endforeach()

configure_red4ext(src/red4ext)
configure_red4ext_addresses(Addresses.hpp)

configure_tweaks(src/tweaks)
configure_archives(src/wolvenkit/packed/mods/let_there_be_flight/archives/let_there_be_flight.archive src/archive_xl/let_there_be_flight.archive.xl)

find_package(Detours)
find_package(FMOD)
find_package(Spdlog)
find_package(RedLib)
find_package(PhysX3)
find_package(ModSettings)
find_package(InputLoader)
find_package(ArchiveXL)
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

# configure_uninstall()
configure_release(${MOD_SLUG}_${MOD_VERSION_STR}.zip)
configure_install()