add_library(let_there_be_flight.dll SHARED)

set_target_properties(let_there_be_flight.dll PROPERTIES OUTPUT_NAME let_there_be_flight)

configure_file(Version.hpp.in "${CMAKE_CURRENT_SOURCE_DIR}/Version.hpp" @ONLY)

file(GLOB_RECURSE HEADER_FILES *.hpp)
file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE RC_FILES *.rc)

source_group(_CMake REGULAR_EXPRESSION cmake_pch.*)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES} ${RC_FILES})

target_include_directories(let_there_be_flight.dll PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(let_there_be_flight.dll PRIVATE ${HEADER_FILES} ${SOURCE_FILES} ${RC_FILES})

target_precompile_headers(let_there_be_flight.dll PUBLIC stdafx.hpp)
target_link_libraries(let_there_be_flight.dll
  PUBLIC
    Detours
    FMOD
    RED4ext::SDK
    spdlog
)