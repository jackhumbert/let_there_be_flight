message(STATUS "Configuring Input Loader")
list(APPEND CMAKE_MESSAGE_INDENT "  ")
add_subdirectory(${PROJECT_SOURCE_DIR}/deps/input_loader)
list(POP_BACK CMAKE_MESSAGE_INDENT)