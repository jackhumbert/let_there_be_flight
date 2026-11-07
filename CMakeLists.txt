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
set(ZOLTAN_CLANG_EXE "C:/Users/Jack/Documents/cyberpunk/zoltan/target/release/zoltan-clang.exe")

# Sound files LTBF plays itself on FMOD Core channels (Audio/Samples, docs/design/audio-native.md A.3)

file(GLOB LTBF_AUDIO_FILES ${PROJECT_SOURCE_DIR}/src/audio/*.wav)
foreach(_FILE ${LTBF_AUDIO_FILES})
  get_filename_component(_NAME ${_FILE} NAME)
  configure_mod_file(src/audio/${_NAME} red4ext/plugins/${MOD_SLUG}/audio/${_NAME})
endforeach()

# Autopilot nav map, baked offline by tools/navmap_bake/ from the game's collision. Too big for git, so it is a
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
# Embedded in the plugin DLL as resource NAVMAP (one file fewer to ship); OFF installs navmap.bin
# next to it instead, which the plugin also reads when the DLL has none (Main.cpp, LoadEmbeddedNavMap).
option(LTBF_NAVMAP_EMBED "Embed navmap.bin in the plugin DLL" ON)
if(EXISTS "${_NAVMAP}" AND LTBF_NAVMAP_EMBED)
  set(_NAVMAP_EMBEDDED ON)
  # a navmap.bin left in game_dir from before embedding would be installed and zipped into releases,
  # and outdated: the plugin reads it only when the DLL has none
  file(REMOVE "${MOD_GAME_DIR}/red4ext/plugins/${MOD_SLUG}/navmap.bin")
elseif(EXISTS "${_NAVMAP}")
  configure_mod_file("${_NAVMAP}" red4ext/plugins/${MOD_SLUG}/navmap.bin)
elseif(CMAKE_CI_BUILD)
  message(FATAL_ERROR "navmap.bin download failed: ${_NAVMAP_STATUS}")
else()
  message(WARNING "navmap.bin download failed (${_NAVMAP_STATUS}); the autopilot will be disabled")
endif()

# Landing pads found in the world sectors and checked against the nav map (tools/pads/pads.py, small, in git).
configure_mod_file(src/navigation/helipads.tsv red4ext/plugins/${MOD_SLUG}/helipads.tsv)
# Openings for the Autopilot's free roam to fly through, from the nav map's doorways (tools/navmap_bake/gates.py, in git).
configure_mod_file(src/navigation/gates.tsv red4ext/plugins/${MOD_SLUG}/gates.tsv)
# Invisible vehicle blockers the plugin moves out of the world as sectors load (tools/world/blockers.py export).
# Without the file the hook in Hooks/StreamingSector_OnPostLoad.cpp does nothing (A/B in docs/archive/autodrive-flight-plan.md).
configure_mod_file(src/navigation/blockers.txt red4ext/plugins/${MOD_SLUG}/blockers.txt)
configure_mod_file(src/navigation/hidden_nodes.txt red4ext/plugins/${MOD_SLUG}/hidden_nodes.txt)

configure_red4ext(src/red4ext)
if(_NAVMAP_EMBEDDED)
  file(TO_CMAKE_PATH "${_NAVMAP}" _NAVMAP_RC_PATH)
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/navmap.rc" "NAVMAP RCDATA \"${_NAVMAP_RC_PATH}\"\n")
  target_sources(${MOD_SLUG}.dll PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/navmap.rc")
  set_property(SOURCE "${CMAKE_CURRENT_BINARY_DIR}/navmap.rc" APPEND PROPERTY OBJECT_DEPENDS "${_NAVMAP}")
endif()
# configure_red4ext_addresses(Addresses.hpp)
set(MOD_NATIVE_TWEAKS ON)
set(MOD_YAML_TWEAKS ON)
configure_tweaks(src/tweaks)
# Tweaks always go through TweakXL: the game's built-in TweakDB loader (2.5+) crashes on new typed flats
# (docs/history.md, "tweaks_to_json.py removed").
# The archive is not in git: it is packed from src/wolvenkit/source at build time (tools/packaging/pack_archive.py) with a
# WolvenKit CLI and the game's Oodle dll (oo2ext_7_win64.dll: not redistributable, so CI gets it from a private repo,
# .github/workflows). It is written to src/wolvenkit/packed (gitignored), where game.py --archive-only finds it.
# With neither, an archive already there is shipped; with none at all the build has no archive (a warning).
set(LTBF_WOLVENKIT_CLI "$ENV{WOLVENKIT_CLI}" CACHE FILEPATH "WolvenKit.CLI.exe to pack the archive with (oo2ext_7_win64.dll next to it)")
set(LTBF_OODLE_DLL "$ENV{LTBF_OODLE_DLL}" CACHE FILEPATH "oo2ext_7_win64.dll to pack the archive with (copied next to the CLI)")
set(_ARCHIVE "${CMAKE_CURRENT_SOURCE_DIR}/src/wolvenkit/packed/archive/pc/mod/let_there_be_flight.archive")
if(LTBF_WOLVENKIT_CLI OR LTBF_OODLE_DLL)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  file(GLOB_RECURSE _ARCHIVE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wolvenkit/source/archive/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wolvenkit/source/raw/user/jackhumbert/localization/*.json.json")
  set(_PACK_ARGS --out "${_ARCHIVE}")
  if(LTBF_WOLVENKIT_CLI)
    list(APPEND _PACK_ARGS --cli "${LTBF_WOLVENKIT_CLI}")
  endif()
  if(LTBF_OODLE_DLL)
    list(APPEND _PACK_ARGS --oodle "${LTBF_OODLE_DLL}")
  endif()
  add_custom_command(
    OUTPUT "${_ARCHIVE}"
    DEPENDS ${_ARCHIVE_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/tools/packaging/pack_archive.py"
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/packaging/pack_archive.py" ${_PACK_ARGS}
    COMMENT "Packing let_there_be_flight.archive"
    VERBATIM)
  message(STATUS "Archive: packed at build time")
  configure_archives("${_ARCHIVE}")
elseif(EXISTS "${_ARCHIVE}")
  message(STATUS "Archive: the one already in src/wolvenkit/packed (no WolvenKit CLI or Oodle dll to repack it)")
  configure_archives("${_ARCHIVE}")
else()
  message(WARNING "No archive: set WOLVENKIT_CLI (with oo2ext_7_win64.dll next to it) or LTBF_OODLE_DLL to pack one")
endif()
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
# 1.28.3 fixed instanced mesh world node mutations (the proxy colliders' sector edits); the shared
# FindArchiveXL asks for 1.28.0. Keep in step with archiveXL_version in Main.cpp.
mod_require(ArchiveXL 1.28.3 NOTE "optional on game 2.5+")
find_package(TweakXL)

# Mod Settings' C++ API (its Variable struct; AddVariable is looked up at runtime: Flight/FlightSettingsLink.cpp)
target_include_directories(let_there_be_flight.dll PRIVATE deps/mod_settings/include)

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

# offline route planner on navmap.bin with the plugin's planner, for tuning and plotting routes, and whole
# Autopilot trips flown on it (tools/navplan, NAVPLAN_TRIP); not built by default
add_executable(navplan EXCLUDE_FROM_ALL
  tools/navplan/navplan.cpp
  tools/flightsim/Sim.cpp
  tools/flightsim/Colliders.cpp
  tools/flightsim/Trip.cpp
  src/red4ext/Flight/Stats.cpp
  src/red4ext/Flight/Force.cpp
  src/red4ext/Flight/Pid.cpp
  src/red4ext/Flight/Controls.cpp
  src/red4ext/Flight/Modes.cpp
  src/red4ext/Flight/DeveloperSettings.cpp
  src/red4ext/Navigation/World.cpp
  src/red4ext/Navigation/Plans.cpp
  src/red4ext/Navigation/Helipads.cpp
  src/red4ext/Navigation/Gates.cpp
  src/red4ext/Navigation/AutopilotParams.cpp
  src/red4ext/Navigation/Landing.cpp
  src/red4ext/Navigation/Aerobatics.cpp
  src/red4ext/Navigation/Autopilot.cpp
  src/red4ext/Navigation/AutopilotRoute.cpp
  src/red4ext/Navigation/AutopilotFreeRoam.cpp
  src/red4ext/Navigation/AutopilotDescent.cpp
  src/red4ext/Navigation/NavMap.cpp
  src/red4ext/Navigation/Planner.cpp
  src/red4ext/Navigation/PlannerCommon.cpp
  src/red4ext/Navigation/Line.cpp
  src/red4ext/Navigation/PlanAhead.cpp
  src/red4ext/Navigation/Doorways.cpp
  src/red4ext/Navigation/Vehicle.cpp
  src/red4ext/Navigation/Trajectory.cpp
  src/red4ext/Navigation/Route.cpp
  src/red4ext/Navigation/DroneAttitude.cpp
  src/red4ext/Navigation/Surge.cpp
  src/red4ext/Navigation/Tracker.cpp
  src/red4ext/ThirdParty/miniz/miniz_tinfl.cpp
)
target_include_directories(navplan PRIVATE src/red4ext)
target_compile_features(navplan PRIVATE cxx_std_20)
target_link_libraries(navplan PRIVATE spdlog)
# NAVPLAN_TRIP reads the fleet (tools/flightsim/vehicles.tsv), the pads and the gates (src/navigation)
target_compile_definitions(navplan PRIVATE LTBF_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

# Hover & Fly flown offline on the plugin's mode, input and force code, scenario by scenario
# (tools/flightsim, docs/design/hover-fly-feel.md); not built by default
add_executable(flightsim EXCLUDE_FROM_ALL
  tools/flightsim/flightsim.cpp
  tools/flightsim/Sim.cpp
  tools/flightsim/Colliders.cpp
  tools/flightsim/Scenarios.cpp
  tools/flightsim/TerrainRuns.cpp
  tools/flightsim/TerrainCheck.cpp
  tools/flightsim/Seams.cpp
  tools/flightsim/Replay.cpp
  src/red4ext/Flight/Stats.cpp
  src/red4ext/Flight/Force.cpp
  src/red4ext/Flight/Pid.cpp
  src/red4ext/Flight/Controls.cpp
  src/red4ext/Flight/Modes.cpp
)
target_include_directories(flightsim PRIVATE src/red4ext)
target_compile_features(flightsim PRIVATE cxx_std_20)
# the game's own PhysX for flightsim --physx-joint (tools/flightsim/PhysXWorld.hpp): NVIDIA's PhysX 3.4 headers, the
# game's DLLs loaded at run time from its bin/x64
set(LTBF_PHYSX34 "D:/cp2077_work/tools/physx34" CACHE PATH "PhysX-3.4 checkout (its PhysX_3.4/Include and PxShared/include)")
if(EXISTS "${LTBF_PHYSX34}/PhysX_3.4/Include/PxPhysicsAPI.h")
  target_sources(flightsim PRIVATE tools/flightsim/PhysXWorld.cpp tools/flightsim/PhysXCheck.cpp)
  target_include_directories(flightsim PRIVATE "${LTBF_PHYSX34}/PhysX_3.4/Include" "${LTBF_PHYSX34}/PxShared/include")
  target_compile_definitions(flightsim PRIVATE LTBF_PHYSX LTBF_GAME_BIN="${CYBERPUNK_2077_GAME_DIR}/bin/x64" NOMINMAX
                             WIN32_LEAN_AND_MEAN)
endif()

# offline run of the plugin's acoustics code over Autopilot telemetry, checked against
# tools/acoustics/flyby.py (docs/design/acoustics.md); not built by default
add_executable(acoustics EXCLUDE_FROM_ALL
  tools/acoustics/acoustics.cpp
  src/red4ext/Audio/Acoustics/Probe.cpp
  src/red4ext/Audio/Acoustics/Images.cpp
  src/red4ext/Audio/Acoustics/Reflections.cpp
  src/red4ext/Navigation/NavMap.cpp
  src/red4ext/ThirdParty/miniz/miniz_tinfl.cpp
)
target_include_directories(acoustics PRIVATE src/red4ext)
target_compile_features(acoustics PRIVATE cxx_std_20)
target_link_libraries(acoustics PRIVATE spdlog)

# the engine synth over a run's thruster parameters (docs/design/engine-synth.md); not built by default
add_executable(engine_synth EXCLUDE_FROM_ALL
  tools/acoustics/engine_synth.cpp
  src/red4ext/Audio/EngineSynth/Synth.cpp
  src/red4ext/Audio/EngineSynth/Sources.cpp
  src/red4ext/Audio/Acoustics/Probe.cpp
  src/red4ext/Audio/Acoustics/Images.cpp
  src/red4ext/Audio/Acoustics/Reflections.cpp
  src/red4ext/Navigation/NavMap.cpp
  src/red4ext/ThirdParty/miniz/miniz_tinfl.cpp
)
target_include_directories(engine_synth PRIVATE src/red4ext)
target_compile_features(engine_synth PRIVATE cxx_std_20)
target_link_libraries(engine_synth PRIVATE spdlog)

# Unit tests for the parts of the plugin that need no game (tests/): ctest --test-dir build, or run
# build/ltbf_tests.exe [substring]. The sources they cover are compiled in again here.
enable_testing()
add_executable(ltbf_tests
  tests/main.cpp
  tests/synth_tests.cpp
  tests/sources_tests.cpp
  tests/voices_tests.cpp
  tests/samples_tests.cpp
  tests/wind_tests.cpp
  tests/inputs_tests.cpp
  tests/images_tests.cpp
  tests/planes_tests.cpp
  tests/probe_tests.cpp
  tests/reflections_tests.cpp
  tests/materials_tests.cpp
  tests/occlusion_tests.cpp
  tests/buses_tests.cpp
  tests/vehicle_tests.cpp
  tests/route_tests.cpp
  tests/trajectory_tests.cpp
  tests/attitude_tests.cpp
  tests/surge_tests.cpp
  tests/tracker_tests.cpp
  tests/follow_tests.cpp
  tests/autopilot_fleet_tests.cpp
  tests/scriptmath_tests.cpp
  tests/stats_tests.cpp
  tests/force_tests.cpp
  tests/pid_tests.cpp
  tests/controls_tests.cpp
  tests/modes_tests.cpp
  tests/scrape_tests.cpp
  tests/thruster_tests.cpp
  tests/aerobatics_tests.cpp
  tests/cinematic_tests.cpp
  tests/feel_tests.cpp
  tests/aero_tests.cpp
  tests/colliders_tests.cpp
  tests/terrain_runs_tests.cpp
  tests/settings_tests.cpp
  tests/world_tests.cpp
  tests/plans_tests.cpp
  tests/autopilot_params_tests.cpp
  tests/landing_tests.cpp
  tests/autopilot_tests.cpp
  tests/autopilot_trip_tests.cpp
  tests/autopilot_shell_tests.cpp
  tests/wwise_tests.cpp
  tests/out_tests.cpp
  tools/navplan/follow.cpp
  tools/flightsim/Sim.cpp
  tools/flightsim/Colliders.cpp
  tools/flightsim/Scenarios.cpp
  tools/flightsim/TerrainRuns.cpp
  tools/flightsim/TerrainCheck.cpp
  tools/flightsim/Replay.cpp
  tools/flightsim/Trip.cpp
  src/red4ext/Audio/EngineSynth/Synth.cpp
  src/red4ext/Audio/EngineSynth/Sources.cpp
  src/red4ext/Audio/Buses.cpp
  src/red4ext/Audio/Voices/Pool.cpp
  src/red4ext/Audio/Samples/Events.cpp
  src/red4ext/Audio/Wind/Model.cpp
  src/red4ext/Audio/Inputs/Inputs.cpp
  src/red4ext/Audio/Acoustics/Images.cpp
  src/red4ext/Audio/Acoustics/Materials.cpp
  src/red4ext/Audio/Acoustics/Occlusion.cpp
  src/red4ext/Audio/Acoustics/Planes.cpp
  src/red4ext/Audio/Acoustics/Probe.cpp
  src/red4ext/Audio/Acoustics/Reflections.cpp
  src/red4ext/Audio/Wwise/Axes.cpp
  src/red4ext/Audio/Wwise/Bank.cpp
  src/red4ext/Audio/Wwise/Ids.cpp
  src/red4ext/Audio/Wwise/Pattern.cpp
  src/red4ext/Audio/Wwise/Relay.cpp
  src/red4ext/Audio/Out/Mix.cpp
  src/red4ext/Audio/Samples/Player.cpp
  src/red4ext/Navigation/Vehicle.cpp
  src/red4ext/Navigation/Route.cpp
  src/red4ext/Navigation/Trajectory.cpp
  src/red4ext/Navigation/DroneAttitude.cpp
  src/red4ext/Navigation/Surge.cpp
  src/red4ext/Navigation/Tracker.cpp
  src/red4ext/Navigation/Aerobatics.cpp
  src/red4ext/Camera/Cinematic.cpp
  src/red4ext/Navigation/NavMap.cpp
  src/red4ext/Navigation/World.cpp
  src/red4ext/Navigation/Plans.cpp
  src/red4ext/Navigation/AutopilotParams.cpp
  src/red4ext/Navigation/Landing.cpp
  src/red4ext/Navigation/Autopilot.cpp
  src/red4ext/Navigation/AutopilotRoute.cpp
  src/red4ext/Navigation/AutopilotFreeRoam.cpp
  src/red4ext/Navigation/AutopilotDescent.cpp
  src/red4ext/Navigation/AutopilotTelemetry.cpp
  src/red4ext/Navigation/Planner.cpp
  src/red4ext/Navigation/PlannerCommon.cpp
  src/red4ext/Navigation/Line.cpp
  src/red4ext/Navigation/PlanAhead.cpp
  src/red4ext/Navigation/Doorways.cpp
  src/red4ext/Flight/Stats.cpp
  src/red4ext/Flight/Force.cpp
  src/red4ext/Flight/Pid.cpp
  src/red4ext/Flight/Controls.cpp
  src/red4ext/Flight/Modes.cpp
  src/red4ext/Flight/Scrape.cpp
  src/red4ext/Flight/Thruster.cpp
  src/red4ext/Flight/Aero.cpp
  src/red4ext/Flight/DeveloperSettings.cpp
  src/red4ext/Flight/Settings.cpp
  src/red4ext/ThirdParty/miniz/miniz_tinfl.cpp
)
target_include_directories(ltbf_tests PRIVATE src/red4ext)
target_compile_features(ltbf_tests PRIVATE cxx_std_20)
target_link_libraries(ltbf_tests PRIVATE spdlog)
# feel_tests reads the fleet from tools/flightsim/vehicles.tsv
target_compile_definitions(ltbf_tests PRIVATE LTBF_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME ltbf_tests COMMAND ltbf_tests)
# every vanilla car and bike's rig has the bones and wheel slots its thrusters need (tools/flightsim/thruster_slots.py)
find_package(Python3 COMPONENTS Interpreter)
add_test(NAME thruster_slots COMMAND ${Python3_EXECUTABLE} -I "${CMAKE_CURRENT_SOURCE_DIR}/tools/flightsim/thruster_slots.py")

# configure_inputs(src/input_loader/let_there_be_flight.xml)
configure_mod_file(src/input_loader/${MOD_SLUG}.xml red4ext/plugins/${MOD_SLUG}/inputs.xml)
configure_redscript(src/redscript)

target_link_libraries(${MOD_SLUG}.packed.reds PUBLIC mod_settings.packed.reds)

configure_folder_file(readme.md)
configure_folder_file(license.md)

mod_require("Input Loader" 0.2.3)
mod_require("Mod Settings" 0.2.23 OPTIONAL)

configure_uninstall()
configure_release(${MOD_SLUG}_${MOD_VERSION_STR}.zip)
configure_install()