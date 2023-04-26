add_library(PhysX3 SHARED IMPORTED)
set_target_properties(PhysX3 PROPERTIES FOLDER "Dependencies")

set(PhysX3_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/deps/physx")
set(PhysX3_LIB_DIR "${PROJECT_SOURCE_DIR}/deps/physx")
set(PhysX3_GAME_DLL "${CYBERPUNK_2077_GAME_DIR}/bin/x64/PhysX3_x64.dll")

set_property(TARGET PhysX3 PROPERTY IMPORTED_LOCATION "${PhysX3_GAME_DLL}")
set_property(TARGET PhysX3 PROPERTY IMPORTED_IMPLIB "${PhysX3_LIB_DIR}/PhysX3_x64.lib")

target_include_directories(PhysX3 INTERFACE "${PhysX3_INCLUDE_DIR}")
target_include_directories(PhysX3 INTERFACE "${PROJECT_SOURCE_DIR}/deps/red4ext.sdk/include")

