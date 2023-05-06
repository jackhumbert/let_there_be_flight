add_library(FMOD SHARED IMPORTED)
set_target_properties(FMOD PROPERTIES FOLDER "Dependencies")

set(FMOD_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/deps/fmod/include")
set(FMOD_LIB_DIR "${PROJECT_SOURCE_DIR}/deps/fmod/lib")
set(FMOD_GAME_DIR_DLL "${MOD_GAME_DIR_FMOD_FOLDER}/fmod.dll")
set(FMOD_STUDIO_GAME_DIR_DLL "${MOD_GAME_DIR_FMOD_FOLDER}/fmodstudio.dll")

set_property(TARGET FMOD PROPERTY IMPORTED_LOCATION "${FMOD_LIB_DIR}/x64/fmod.dll")
# set_property(TARGET FMOD PROPERTY IMPORTED_LOCATION_DEBUG "${FMOD_LIB_DIR}/x64/fmodL.dll")
set_property(TARGET FMOD PROPERTY IMPORTED_IMPLIB "${FMOD_LIB_DIR}/x64/fmod_vc.lib")
# set_property(TARGET FMOD PROPERTY IMPORTED_IMPLIB_DEBUG "${FMOD_LIB_DIR}/x64/fmodL_vc.lib")
# file(GLOB_RECURSE HEADER_FILES ${FMOD_INCLUDE_DIR}/*.h)
# set_property(TARGET FMOD PROPERTY IMPORTED_IMPLIB ${FMOD_LIB_DIR})

add_library(FMODStudio SHARED IMPORTED)
set_property(TARGET FMODStudio PROPERTY IMPORTED_LOCATION "${FMOD_LIB_DIR}/x64/fmodstudio.dll")
# set_property(TARGET FMODStudio PROPERTY IMPORTED_LOCATION_DEBUG "${FMOD_LIB_DIR}/x64/fmodstudioL.dll")
set_property(TARGET FMODStudio PROPERTY IMPORTED_IMPLIB "${FMOD_LIB_DIR}/x64/fmodstudio_vc.lib")
# set_property(TARGET FMODStudio PROPERTY IMPORTED_IMPLIB_DEBUG "${FMOD_LIB_DIR}/x64/fmodstudioL_vc.lib")

# add_custom_command(
#   OUTPUT ${FMOD_GAME_DIR_DLL}
#   DEPENDS ${FMOD_LIB_DIR}/x64/fmod.dll
#   COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FMOD_LIB_DIR}/x64/fmod.dll ${MOD_GAME_DIR_FMOD_FOLDER}
# )
configure_mod_file(deps/fmod/lib/x64/fmod.dll red4ext/plugins/${MOD_SLUG}/fmod.dll)
configure_mod_file(deps/fmod/lib/x64/fmodstudio.dll red4ext/plugins/${MOD_SLUG}/fmodstudio.dll)

# add_custom_target(fmod_dll DEPENDS ${FMOD_GAME_DIR_DLL})

# add_custom_command(
#   OUTPUT ${FMOD_STUDIO_GAME_DIR_DLL}
#   DEPENDS ${FMOD_LIB_DIR}/x64/fmodstudio.dll
#   COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FMOD_LIB_DIR}/x64/fmodstudio.dll ${FMOD_STUDIO_GAME_DIR_DLL}
# )

# add_custom_target(fmodstudio_dll DEPENDS ${FMOD_STUDIO_GAME_DIR_DLL})

target_link_libraries(FMOD INTERFACE FMODStudio)

target_include_directories(FMOD INTERFACE ${FMOD_INCLUDE_DIR})

# include_directories(FMOD ${FMOD_INCLUDE_DIR})
# target_link_libraries(FMOD INTERFACE ${FMOD_LIB_DIR})
# target_link_directories(FMOD INTERFACE ${FMOD_LIB_DIR})
# find_library(FMOD SHARED IMPORTED ${FMOD_LIB_DIR})
