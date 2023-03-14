# uses these variables:
#  COMMENT_SLUG
#  HEADER_FILE
#  PACKED_FILE
#  GLOB_EXT
#  SEARCH_FOLDER

file(GLOB_RECURSE FILES ${SEARCH_FOLDER}/*.${GLOB_EXT})

string(TIMESTAMP CURRENT_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)

file(STRINGS "${HEADER_FILE}" HEADER)
string(CONFIGURE "${HEADER}" CONFIGURED_HEADER)

file(WRITE ${PACKED_FILE} "")
foreach(_LINE ${CONFIGURED_HEADER})
  file(APPEND ${PACKED_FILE} "${COMMENT_SLUG} ${_LINE}\n")
endforeach()
file(APPEND ${PACKED_FILE} "\n${COMMENT_SLUG} This file was automatically generated on ${CURRENT_TIMESTAMP}\n\n")

function(add_file IN_FILE OUT_FILE)
  file(READ ${IN_FILE} CONTENTS)
  file(RELATIVE_PATH IN_FILE_RELATIVE ${SEARCH_FOLDER} ${IN_FILE})
  file(APPEND ${OUT_FILE} "${COMMENT_SLUG} ${IN_FILE_RELATIVE}\n\n")
  message(STATUS "${IN_FILE_RELATIVE}")
  file(APPEND ${OUT_FILE} "${CONTENTS}\n\n")
endfunction()

foreach(_FILE ${FILES})
  add_file(${_FILE} ${PACKED_FILE})
endforeach()