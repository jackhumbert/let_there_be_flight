add_library(let_there_be_flight.dll SHARED)

set_target_properties(let_there_be_flight.dll PROPERTIES OUTPUT_NAME let_there_be_flight)
set_target_properties(let_there_be_flight.dll PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(let_there_be_flight.dll PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

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
    RED4ext.SDK
    spdlog
    Codeware
    PhysX3
)

add_custom_command(
  TARGET let_there_be_flight.dll POST_BUILD
  DEPENDS let_there_be_flight.dll
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
#  ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/let_there_be_flight.dll
  ${CMAKE_CURRENT_BINARY_DIR}/let_there_be_flight.dll
  ${LTBF_GAME_DIR}/red4ext/plugins/let_there_be_flight/let_there_be_flight.dll
  COMMENT "let_there_be_flight.dll -> ${LTBF_GAME_DIR}/red4ext/plugins/let_there_be_flight/let_there_be_flight.dll")
