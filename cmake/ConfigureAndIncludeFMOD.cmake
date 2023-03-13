add_library(FMOD SHARED IMPORTED)
set_target_properties(FMOD PROPERTIES FOLDER "Dependencies")


set(FMOD_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/deps/fmod/include")
set(FMOD_LIB_DIR "${PROJECT_SOURCE_DIR}/deps/fmod/lib")
set_property(TARGET FMOD PROPERTY IMPORTED_LOCATION "${FMOD_LIB_DIR}/x64/fmod.dll")
set_property(TARGET FMOD PROPERTY IMPORTED_IMPLIB "${FMOD_LIB_DIR}/x64/fmod_vc.lib")
# file(GLOB_RECURSE HEADER_FILES ${FMOD_INCLUDE_DIR}/*.h)
# set_property(TARGET FMOD PROPERTY IMPORTED_IMPLIB ${FMOD_LIB_DIR})

target_include_directories(FMOD INTERFACE ${FMOD_INCLUDE_DIR})
# include_directories(FMOD ${FMOD_INCLUDE_DIR})
# target_link_libraries(FMOD INTERFACE ${FMOD_LIB_DIR})
# target_link_directories(FMOD INTERFACE ${FMOD_LIB_DIR})
# find_library(FMOD SHARED IMPORTED ${FMOD_LIB_DIR})