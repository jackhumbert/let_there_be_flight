add_library(Detours STATIC)
set_target_properties(Detours PROPERTIES FOLDER "Dependencies")

set(DETOURS_SRC_DIR "${PROJECT_SOURCE_DIR}/deps/detours/src")
file(GLOB_RECURSE HEADER_FILES ${DETOURS_SRC_DIR}/*.h)
file(GLOB_RECURSE SOURCE_FILES ${DETOURS_SRC_DIR}/*.cpp)

# Remove "uimports.cpp" since it throws "detours.h version mismatch" error.
list(REMOVE_ITEM SOURCE_FILES ${DETOURS_SRC_DIR}/uimports.cpp)

target_include_directories(Detours PUBLIC ${DETOURS_SRC_DIR})
target_sources(Detours PRIVATE ${HEADER_FILES} ${SOURCE_FILES})
