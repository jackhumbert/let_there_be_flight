# Experimental generation of all addresses in red4ext.sdk

set(ZOLTAN_ALL_SIGNATURES "${PROJECT_BINARY_DIR}/zoltan/Signatures.hpp")
#set(ZOLTAN_ALL_ADDRESSES "${PROJECT_BINARY_DIR}/zoltan/Addresses.hpp")
set(ZOLTAN_ALL_ADDRESSES "${PROJECT_SOURCE_DIR}/deps/red4ext.sdk/include/RED4ext/Addresses-Found.hpp")

execute_process(
  # ${LTBF_TOOLS_DIR}/zoltan-clang.exe 
  COMMAND "C:/Users/Jack/Documents/cyberpunk/zoltan/target/debug/zoltan-clang.exe"
  # COMMAND ${LTBF_TOOLS_DIR}/zoltan-clang.exe 
  "${ZOLTAN_ALL_SIGNATURES}" "${CYBERPUNK_2077_GAME_DIR}/bin/x64/Cyberpunk2077.exe" -f "std=c++20" -f "I${PROJECT_SOURCE_DIR}/deps/red4ext.sdk/include" --c-output "${ZOLTAN_ALL_ADDRESSES}")