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

set(LTBF_GAME_DIR "${PROJECT_SOURCE_DIR}/game_dir")
set(LTBF_TOOLS_DIR "${PROJECT_SOURCE_DIR}/tools")
set(LTBF_CYBERPUNK_DIR "C:/Program Files (x86)/Steam/steamapps/common/Cyberpunk 2077")
set(LTBF_REDSCRIPT_DIR "${PROJECT_SOURCE_DIR}/src/redscript")

set(REDSCRIPT_PREREQS_DIR "${PROJECT_SOURCE_DIR}/prereqs")
set(REDSCRIPT_ORIGINAL_REDSCRIPTS_BK "${LTBF_CYBERPUNK_DIR}/r6/cache/final.redscripts.bk")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

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

#  _DEBUG
#  _WINDLL
#  _CRT_SECURE_NO_WARNINGS
)

# experimenting with VS compile/link flags

# debug
# /JMC /permissive- /ifcOutput "x64\Debug\" /GS /W3 /Zc:wchar_t /ZI /Gm- /Od /sdl /Fd"x64\Debug\vc143.pdb" /Zc:inline /fp:precise /D "_DEBUG" /D "_CRT_SECURE_NO_WARNINGS" /D "_WINDLL" /D "_UNICODE" /D "UNICODE" /errorReport:prompt /WX- /Zc:forScope /RTC1 /std:c17 /Gd /MDd /std:c++20 /FC /Fa"x64\Debug\" /EHsc /nologo /Fo"x64\Debug\" /Fp"x64\Debug\flight_control.pch" /diagnostics:column

# release
# /permissive- /ifcOutput "x64\Release\" /GS /GL /W3 /Gy /Zc:wchar_t /Zi /Gm- /O2 /sdl /Fd"x64\Release\vc143.pdb" /Zc:inline /fp:precise /D "NDEBUG" /D "_CRT_SECURE_NO_WARNINGS" /D "_WINDLL" /D "_UNICODE" /D "UNICODE" /errorReport:prompt /WX- /Zc:forScope /std:c17 /Gd /Oi /MD /std:c++20 /FC /Fa"x64\Release\" /EHsc /nologo /Fo"x64\Release\" /Fp"x64\Release\flight_control.pch" /diagnostics:column

#add_compile_options(
#  # Enable correct reporting of C++ version, see https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus.
##  $<$<BOOL:MSVC>:/Zc:__cplusplus>
#  /JMC
#  /permissive-
#  /Gs
#  /W3
#  /Zc:wchar_t
#  # auto sets /Gy apparently
#  #  /ZI
#  /Gm-
#  /Od
#  /sdl
#  /Zc:inline
#  /fp:precise
#  /WX-
#  /Zc:forScope
#  /RTC1
#  /Gd
#  /MDd
#  /FC
#  /nologo
#  /diagnostics:column
#  /Gy
#)

# debug
# /OUT:"src\red4ext\build\debug\bin\flight_control.dll" /MANIFEST /NXCOMPAT /PDB:"src\red4ext\build\debug\bin\flight_control.pdb" /DYNAMICBASE "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" "deps\fmod\lib\x64\fmodstudioL_vc.lib" "deps\fmod\lib\x64\fmodL_vc.lib" /IMPLIB:"src\red4ext\build\debug\bin\flight_control.lib" /DEBUG /DLL /MACHINE:X64 /INCREMENTAL /PGD:"src\red4ext\build\debug\bin\flight_control.pgd" /SUBSYSTEM:WINDOWS /MANIFESTUAC:NO /ManifestFile:"x64\Debug\flight_control.dll.intermediate.manifest" /LTCGOUT:"x64\Debug\flight_control.iobj" /ERRORREPORT:PROMPT /ILK:"x64\Debug\flight_control.ilk" /NOLOGO /TLBID:1

# release
# /OUT:"src\red4ext\build\release\bin\flight_control.dll" /MANIFEST /LTCG:incremental /NXCOMPAT /PDB:"src\red4ext\build\release\bin\flight_control.pdb" /DYNAMICBASE "hid.lib" "deps\fmod\lib\x64\fmodstudio_vc.lib" "deps\fmod\lib\x64\fmod_vc.lib" "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" /IMPLIB:"src\red4ext\build\release\bin\flight_control.lib" /DEBUG /DLL /MACHINE:X64 /OPT:REF /INCREMENTAL:NO /PGD:"src\red4ext\build\release\bin\flight_control.pgd" /SUBSYSTEM:WINDOWS /MANIFESTUAC:NO /ManifestFile:"x64\Release\flight_control.dll.intermediate.manifest" /LTCGOUT:"x64\Release\flight_control.iobj" /OPT:ICF /ERRORREPORT:PROMPT /ILK:"x64\Release\flight_control.ilk" /NOLOGO /TLBID:1

#add_link_options(
#  /MANIFEST
#  /NXCOMPAT
##  /DEBUG
#  /DLL
##  /MACHINE:X64
##  /INCREMENTAL
#  /NOLOGO
#  /TLBID:1
#  /OPT:REF
##  /OPT:ICF
##  /INCREMENTAL:NO
#)

include(ConfigureAndIncludeDetours)
include(ConfigureAndIncludeFMOD)
include(ConfigureAndIncludeRED4extSdk)
include(ConfigureAndIncludeSpdlog)

add_subdirectory(src/red4ext)
add_subdirectory(prereqs)
add_subdirectory(src/redscript)

add_custom_target(let_there_be_flight
  DEPENDS let_there_be_flight.dll redscript)