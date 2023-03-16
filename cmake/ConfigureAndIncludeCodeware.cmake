add_library(Codeware INTERFACE)
set_target_properties(Codeware PROPERTIES FOLDER "Dependencies")

#set_property(TARGET Codeware PROPERTY IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/deps/codeware/build/windows/x64/debug/Codeware.dll")
#set_property(TARGET Codeware PROPERTY IMPORTED_IMPLIB "${PROJECT_SOURCE_DIR}/deps/codeware/build/windows/x64/debug/Codeware.lib")

target_include_directories(Codeware INTERFACE "${PROJECT_SOURCE_DIR}/deps/codeware/lib")
target_include_directories(Codeware INTERFACE "${PROJECT_SOURCE_DIR}/deps/codeware/vendor/nameof/include")