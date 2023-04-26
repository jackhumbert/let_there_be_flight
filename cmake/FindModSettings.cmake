message(STATUS "Configuring Mod Settings")
list(APPEND CMAKE_MESSAGE_INDENT "  ")
add_subdirectory(${PROJECT_SOURCE_DIR}/deps/mod_settings)
# not sure how the library would work yet
# set(ModSettings_ROOT ${PROJECT_SOURCE_DIR}/deps/mod_settings)
# find_library(MOD_SETTINGS ModSettings)
list(POP_BACK CMAKE_MESSAGE_INDENT)