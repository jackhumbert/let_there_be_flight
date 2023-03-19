# Experimental generation of all addresses in red4ext.sdk

set(ZOLTAN_ALL_SIGNATURES "${PROJECT_BINARY_DIR}/zoltan/Signatures.hpp")
set(ZOLTAN_ALL_ADDRESSES "${PROJECT_BINARY_DIR}/zoltan/Addresses.hpp")

file(GLOB_RECURSE RED4EXT_ZOLTAN_HEADERS ${PROJECT_SOURCE_DIR}/deps/red4ext.sdk/include/RED4ext/*.hpp)
list(FILTER RED4EXT_ZOLTAN_HEADERS EXCLUDE REGEX ".*-inl.hpp")
list(FILTER RED4EXT_ZOLTAN_HEADERS EXCLUDE REGEX "VFTEnum.hpp")

file(WRITE ${ZOLTAN_ALL_ADDRESSES} "")
foreach(RED4EXT_ZOLTAN_HEADER ${RED4EXT_ZOLTAN_HEADERS})
  file(RELATIVE_PATH IN_FILE_RELATIVE ${PROJECT_SOURCE_DIR}/deps/red4ext.sdk/include/ ${RED4EXT_ZOLTAN_HEADER})
  get_filename_component(FOLDER ${IN_FILE_RELATIVE} DIRECTORY)
  get_filename_component(FILE ${IN_FILE_RELATIVE} NAME_WE)
  file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/zoltan/${FOLDER}")
  message(STATUS "${IN_FILE_RELATIVE}")
  execute_process(
    COMMAND ${LTBF_TOOLS_DIR}/zoltan-clang.exe "${RED4EXT_ZOLTAN_HEADER}" "${CYBERPUNK_2077_GAME_DIR}/bin/x64/Cyberpunk2077.exe" -f "std=c++20" -f "I${PROJECT_SOURCE_DIR}/deps/red4ext.sdk/include" --c-output "${PROJECT_BINARY_DIR}/zoltan/${FOLDER}/${FILE}-addr.hpp")
  file(APPEND ${ZOLTAN_ALL_ADDRESSES} "#include <${FOLDER}/${FILE}-addr.hpp>\n")
endforeach()

#execute_process(
#  COMMAND ${LTBF_TOOLS_DIR}/zoltan-clang.exe "${ZOLTAN_ALL_SIGNATURES}" "${CYBERPUNK_2077_GAME_DIR}/bin/x64/Cyberpunk2077.exe" -f "std=c++20" -f "I${PROJECT_SOURCE_DIR}/deps/red4ext.sdk/include" --c-output "${ZOLTAN_ALL_ADDRESSES}")