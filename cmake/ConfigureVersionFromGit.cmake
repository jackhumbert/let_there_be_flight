function(configure_version_from_git)
  find_package(Git)

  __git_set_default_version_variables()

  if(GIT_FOUND)
    __git_check_if_valid_repository("${GIT_EXECUTABLE}")
    if(${GIT_IS_VALID_REPOSITORY})
      __git_get_latest_tag("${GIT_EXECUTABLE}")
      if(GIT_LATEST_TAG)
        # Split the version into parts.
        string(REGEX MATCHALL "([0-9]+).([0-9]+).([0-9]+)-?(.*)?" GIT_LATEST_TAG_MATCHES "${GIT_LATEST_TAG}")

        set(GIT_VERSION_MAJOR ${CMAKE_MATCH_1})
        set(GIT_VERSION_MINOR ${CMAKE_MATCH_2})
        set(GIT_VERSION_PATCH ${CMAKE_MATCH_3})
        set(GIT_VERSION_PRERELEASE ${CMAKE_MATCH_4})
      endif()

      __git_get_latest_commit("${GIT_EXECUTABLE}")
      if(GIT_COMMIT_SHA)
        list(PREPEND GIT_VERSION_METADATA ${GIT_COMMIT_SHA})

        if(GIT_LATEST_TAG)
          __git_get_commit_count_since_tag("${GIT_EXECUTABLE}" "${GIT_LATEST_TAG}")
          if(GIT_COMMIT_COUNT_SINCE_TAG GREATER 0)
            list(PREPEND GIT_VERSION_METADATA ${GIT_COMMIT_COUNT_SINCE_TAG})
          endif()
        endif()

        __git_get_branch("${GIT_EXECUTABLE}")
        list(PREPEND GIT_VERSION_METADATA "${GIT_BRANCH}")
      endif()
    endif()
  endif()

  # Create the version string containing metadata, if necessary.
  set(GIT_VERSION_STR_FULL "${GIT_VERSION_MAJOR}.${GIT_VERSION_MINOR}.${GIT_VERSION_PATCH}")

  if(GIT_VERSION_PRERELEASE)
    string(APPEND GIT_VERSION_STR_FULL "-${GIT_VERSION_PRERELEASE}")
  endif()

  if(NOT RED4EXT_IS_CI_RELEASE AND GIT_VERSION_METADATA)
    string(JOIN "." GIT_VERSION_METADATA ${GIT_VERSION_METADATA})
    string(APPEND GIT_VERSION_STR_FULL "+${GIT_VERSION_METADATA}")
  endif()

  # Set the variable so that they will be available in the parent scope.
  set(GIT_VERSION_MAJOR ${GIT_VERSION_MAJOR} PARENT_SCOPE)
  set(GIT_VERSION_MINOR ${GIT_VERSION_MINOR} PARENT_SCOPE)
  set(GIT_VERSION_PATCH ${GIT_VERSION_PATCH} PARENT_SCOPE)
  set(GIT_VERSION_PRERELEASE ${GIT_VERSION_PRERELEASE} PARENT_SCOPE)
  set(GIT_VERSION_STR_FULL ${GIT_VERSION_STR_FULL} PARENT_SCOPE)
endfunction()

function(__git_set_default_version_variables)
  set(GIT_VERSION_MAJOR 0 PARENT_SCOPE)
  set(GIT_VERSION_MINOR 0 PARENT_SCOPE)
  set(GIT_VERSION_PATCH 0 PARENT_SCOPE)

  string(TIMESTAMP TIMESTAMP "%Y%m%dT%H%M%SZ" UTC)
  list(APPEND GIT_VERSION_METADATA ${TIMESTAMP})

  set(GIT_VERSION_METADATA ${GIT_VERSION_METADATA} PARENT_SCOPE)
endfunction()

function(__git_check_if_valid_repository GIT_EXECUTABLE)
  execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --is-inside-work-tree
                  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                  OUTPUT_VARIABLE CMD_STDOUT
                  OUTPUT_STRIP_TRAILING_WHITESPACE
                  ERROR_QUIET
  )

  if(CMD_STDOUT STREQUAL "true")
    set(GIT_IS_VALID_REPOSITORY YES PARENT_SCOPE)
  else()
    set(GIT_IS_VALID_REPOSITORY NO PARENT_SCOPE)
  endif()
endfunction()

function(__git_get_latest_tag GIT_EXECUTABLE)
  execute_process(COMMAND "${GIT_EXECUTABLE}" tag --sort -version:refname
                  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                  OUTPUT_VARIABLE GIT_TAGS
                  OUTPUT_STRIP_TRAILING_WHITESPACE
                  COMMAND_ERROR_IS_FATAL ANY
  )

  # Create a list from the command's output.
  separate_arguments(GIT_TAGS NATIVE_COMMAND ${GIT_TAGS})
  list(LENGTH GIT_TAGS GIT_TAGS_LEN)

  if(GIT_TAGS_LEN GREATER 0)
    # Get the latest tag and make sure it doesn't contain spaces.
    list(GET GIT_TAGS 0 GIT_LATEST_TAG)
    string(STRIP "${GIT_LATEST_TAG}" GIT_LATEST_TAG)

    set(GIT_LATEST_TAG ${GIT_LATEST_TAG} PARENT_SCOPE)
  endif()
endfunction()

function(__git_get_latest_commit GIT_EXECUTABLE)
  execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
                  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                  OUTPUT_VARIABLE GIT_COMMIT_SHA
                  OUTPUT_STRIP_TRAILING_WHITESPACE
                  ERROR_QUIET
  )

  set(GIT_COMMIT_SHA ${GIT_COMMIT_SHA} PARENT_SCOPE)
endfunction()

function(__git_get_branch GIT_EXECUTABLE)
  execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --abbrev-ref HEAD
                  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                  OUTPUT_VARIABLE GIT_BRANCH
                  OUTPUT_STRIP_TRAILING_WHITESPACE
                  COMMAND_ERROR_IS_FATAL ANY
  )

  set(GIT_BRANCH ${GIT_BRANCH} PARENT_SCOPE)
endfunction()

function(__git_get_commit_count_since_tag GIT_EXECUTABLE GIT_TAG)
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-list --count ${GIT_TAG}..HEAD
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                    OUTPUT_VARIABLE GIT_COMMIT_COUNT_SINCE_TAG
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    COMMAND_ERROR_IS_FATAL ANY
    )

    set(GIT_COMMIT_COUNT_SINCE_TAG ${GIT_COMMIT_COUNT_SINCE_TAG} PARENT_SCOPE)
endfunction()
