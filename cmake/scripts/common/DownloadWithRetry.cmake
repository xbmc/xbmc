# Downloads ARCHIVE_URL to ARCHIVE_DEST, retrying failures that ExternalProject's own
# download step gives up on: it retries only connection-level curl errors (6 7 8 15 28
# 35), so a response dropped or refused mid-transfer fails the dependency build.
#
# BUILD_DEP_TARGET runs this ahead of the download step using a build-local
# ARCHIVE_DEST. Only verified, complete downloads are published to ARCHIVE_CACHE.
#
# -DARCHIVE_URL=<url> -DARCHIVE_DEST=<path> -DARCHIVE_CACHE=<path> -DARCHIVE_HASH=<ALGO>=<hex>
# CMAKE_TLS_* and CMAKE_NETRC* apply as they do to file(DOWNLOAD) when passed with -D.

if(NOT ARCHIVE_URL OR NOT ARCHIVE_DEST OR NOT ARCHIVE_CACHE OR NOT ARCHIVE_HASH)
  message(FATAL_ERROR "DownloadWithRetry.cmake needs ARCHIVE_URL, ARCHIVE_DEST, ARCHIVE_CACHE and ARCHIVE_HASH")
endif()
if(NOT ARCHIVE_HASH MATCHES "^([A-Za-z0-9_]+)=([0-9A-Fa-f]+)$")
  message(FATAL_ERROR "DownloadWithRetry.cmake: ARCHIVE_HASH must be <ALGO>=<hex>, got '${ARCHIVE_HASH}'")
endif()
set(_algo ${CMAKE_MATCH_1})
string(TOLOWER "${CMAKE_MATCH_2}" _expected)

if(EXISTS "${ARCHIVE_DEST}" AND NOT IS_DIRECTORY "${ARCHIVE_DEST}")
  file(${_algo} "${ARCHIVE_DEST}" _actual)
  if(_actual STREQUAL _expected)
    return()
  endif()
endif()

get_filename_component(_dest_dir "${ARCHIVE_DEST}" DIRECTORY)
file(MAKE_DIRECTORY "${_dest_dir}")
file(LOCK "${ARCHIVE_CACHE}.lock" GUARD PROCESS TIMEOUT 1200)

# Other cache writers may ignore the lock. Verify a private copy, never the
# shared file that they can truncate or remove while it is being read.
if(EXISTS "${ARCHIVE_CACHE}" AND NOT IS_DIRECTORY "${ARCHIVE_CACHE}")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E copy "${ARCHIVE_CACHE}" "${ARCHIVE_DEST}"
                  RESULT_VARIABLE _copy_result ERROR_QUIET)
  if(_copy_result EQUAL 0)
    file(${_algo} "${ARCHIVE_DEST}" _actual)
    if(_actual STREQUAL _expected)
      return()
    endif()
  endif()
endif()

string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef _suffix)
set(_download "${ARCHIVE_CACHE}.${_suffix}.tmp")
set(_attempts 5)
foreach(_attempt RANGE 1 ${_attempts})
  file(DOWNLOAD "${ARCHIVE_URL}" "${_download}" INACTIVITY_TIMEOUT 60 STATUS _status)
  list(GET _status 0 _code)
  list(GET _status 1 _error)

  if(_code EQUAL 0)
    set(_error "missing download output")
    if(EXISTS "${_download}" AND NOT IS_DIRECTORY "${_download}")
      execute_process(COMMAND "${CMAKE_COMMAND}" -E copy "${_download}" "${ARCHIVE_DEST}"
                      RESULT_VARIABLE _copy_result ERROR_VARIABLE _error)
      if(_copy_result EQUAL 0)
        file(SIZE "${ARCHIVE_DEST}" _size)
        if(_size GREATER 0)
          file(${_algo} "${ARCHIVE_DEST}" _actual)
          if(_actual STREQUAL _expected)
            execute_process(COMMAND "${CMAKE_COMMAND}" -E rename "${_download}" "${ARCHIVE_CACHE}"
                            RESULT_VARIABLE _rename_result ERROR_QUIET)
            if(NOT _rename_result EQUAL 0)
              file(REMOVE "${_download}")
            endif()
            return()
          endif()
          # A complete transfer with the wrong hash is a wrong pin, not a network fault
          file(REMOVE "${_download}" "${ARCHIVE_DEST}")
          message(FATAL_ERROR "${ARCHIVE_URL}: ${_algo} mismatch, expected ${_expected}, got ${_actual}")
        endif()
        # An empty 200 is the server failing mid-request, so it is retried like a dropped connection
        set(_error "empty response")
      endif()
    endif()
  endif()

  file(REMOVE "${_download}" "${ARCHIVE_DEST}")
  if(_attempt EQUAL _attempts)
    message(FATAL_ERROR "Failed to download ${ARCHIVE_URL} after ${_attempts} attempts: ${_error}")
  endif()
  math(EXPR _delay "${_attempt} * 10")
  message(STATUS "Download of ${ARCHIVE_URL} failed (${_error}), attempt ${_attempt}/${_attempts}, retrying in ${_delay}s")
  execute_process(COMMAND ${CMAKE_COMMAND} -E sleep ${_delay})
endforeach()
