cmake_minimum_required(VERSION 3.24)

include(CMakeDependentOption)

project(
  let_there_be_flight 
  VERSION 0.1.12 
  LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(ConfigureVersionFromGit)
include(ConfigureDefaultOutputDirectories)
include(TargetOutputDirectory)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set_property(GLOBAL PROPERTY CXX_FLAGS_DEBUG "/Gy")
set_property(GLOBAL PROPERTY LINKER_FLAGS_DEBUG "/OPT:REF /OPT:ICF /INCREMENTAL:NO")

#CMAKE_EXE_LINKER_FLAGS_DEBUG

configure_version_from_git()
configure_default_output_directories()

add_compile_definitions(
  # Support Windows 7 and above.
  WINVER=0x0601
  _WIN32_WINNT=0x0601

  # Exclude unnecessary APIs.
  WIN32_LEAN_AND_MEAN

  # Use Unicode charset.
  UNICODE
  _UNICODE
)

add_compile_options(
  # Enable correct reporting of C++ version, see https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus.
  $<$<BOOL:MSVC>:/Zc:__cplusplus>
)

# set_property(TARGET let_there_be_flight APPEND_STRING PROPERTY CMAKE_EXE_LINKER_FLAGS_DEBUG PROPERTY " /OPT")

include(ConfigureAndIncludeDetours)
include(ConfigureAndIncludeFMOD)
include(ConfigureAndIncludeRED4extSdk)
include(ConfigureAndIncludeSpdlog)

add_subdirectory(src/red4ext)