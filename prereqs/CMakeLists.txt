add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/prereqs.redscripts
  COMMAND ${LTBF_TOOLS_DIR}/redscript-cli.exe
  ARGS compile -s "${REDSCRIPT_PREREQS_DIR}/r6/scripts" -b "${CYBERPUNK_2077_BACKUP_REDSCRIPT}" -o ${PROJECT_BINARY_DIR}/prereqs.redscripts
  COMMENT "Compiling redscript prereqs"
  )

add_custom_target(redscript_prereqs DEPENDS ${PROJECT_BINARY_DIR}/prereqs.redscripts)

set_target_properties(redscript_prereqs PROPERTIES FOLDER "Dependencies")