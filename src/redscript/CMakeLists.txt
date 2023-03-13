add_custom_target(redscript_lint
  DEPENDS redscript_prereqs
  COMMAND ${LTBF_TOOLS_DIR}/redscript-cli.exe lint -s ${PROJECT_SOURCE_DIR}/src/redscript -b ${PROJECT_BINARY_DIR}/prereqs.redscripts
  COMMENT "Linting redscript against pre-compiled prereqs")

#set_target_properties(redscript_lint PROPERTIES FOLDER "Dependencies")

add_custom_target(redscript_lint_game_dir
  DEPENDS redscript_prereqs
  COMMAND ${LTBF_TOOLS_DIR}/redscript-cli.exe lint -s ${LTBF_GAME_DIR}/r6/scripts -b ${PROJECT_BINARY_DIR}/prereqs.redscripts
  COMMENT "Linting game_dir redscript against pre-compiled prereqs")

#set_target_properties(redscript_lint_game_dir PROPERTIES FOLDER "Dependencies")

add_custom_target(redscript_lint_installed
DEPENDS redscript_prereqs
COMMAND ${LTBF_TOOLS_DIR}/redscript-cli.exe lint -s ${LTBF_CYBERPUNK_DIR}/r6/scripts -b ${REDSCRIPT_ORIGINAL_REDSCRIPTS_BK}
COMMENT "Linting installed redscript")

#set_target_properties(redscript_lint_installed PROPERTIES FOLDER "Dependencies")

set(LTBF_REDSCRIPT_PACKED_FILE "${LTBF_GAME_DIR}/r6/scripts/let_there_be_flight/let_there_be_flight.packed.reds")
set(LTBF_REDSCRIPT_MODULE_FILE "${LTBF_GAME_DIR}/r6/scripts/let_there_be_flight/let_there_be_flight.module.reds")

file(GLOB_RECURSE REDSCRIPT_FILES ${LTBF_REDSCRIPT_DIR}/*.reds)

add_custom_command(
  OUTPUT ${LTBF_REDSCRIPT_PACKED_FILE}
  DEPENDS redscript_lint ${REDSCRIPT_FILES}
  COMMAND ${CMAKE_COMMAND} -D LTBF_REDSCRIPT_PACKED_FILE=${LTBF_REDSCRIPT_PACKED_FILE} -D LTBF_REDSCRIPT_DIR=${LTBF_REDSCRIPT_DIR} -P ${CMAKE_MODULE_PATH}/PackRedscriptFiles.cmake
  COMMENT "Packing redscript files into one")

add_custom_command(
  OUTPUT ${LTBF_REDSCRIPT_MODULE_FILE}
  DEPENDS redscript_lint ${REDSCRIPT_FILES}
  COMMAND echo module LetThereBeFlight > ${LTBF_REDSCRIPT_MODULE_FILE}
  COMMENT "Creating LetThereBeFlight redscript module file")

add_custom_target(redscript DEPENDS ${LTBF_REDSCRIPT_PACKED_FILE} ${LTBF_REDSCRIPT_MODULE_FILE})

#set_target_properties(redscript PROPERTIES FOLDER "Dependencies")