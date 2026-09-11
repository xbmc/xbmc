# Downloads ARCHIVE_URL to ARCHIVE_DEST, retrying failures that ExternalProject's own
# download step gives up on: it retries only connection-level curl errors (6 7 8 15 28
# 35), so a response dropped or refused mid-transfer fails the dependency build.
#
# BUILD_DEP_TARGET runs this as a step ahead of the download step, which then finds the
# file with a matching hash and skips the network.
#
# -DARCHIVE_URL=<url> -DARCHIVE_DEST=<path> -DARCHIVE_HASH=<ALGO>=<hex>
# CMAKE_TLS_* and CMAKE_NETRC* apply as they do to file(DOWNLOAD) when passed with -D.

if(NOT ARCHIVE_URL OR NOT ARCHIVE_DEST OR NOT ARCHIVE_HASH)
  message(FATAL_ERROR "DownloadWithRetry.cmake needs ARCHIVE_URL, ARCHIVE_DEST and ARCHIVE_HASH")
endif()
if(NOT ARCHIVE_HASH MATCHES "^([A-Za-z0-9_]+)=([0-9A-Fa-f]+)$")
  message(FATAL_ERROR "DownloadWithRetry.cmake: ARCHIVE_HASH must be <ALGO>=<hex>, got '${ARCHIVE_HASH}'")
endif()
set(_algo ${CMAKE_MATCH_1})
string(TOLOWER "${CMAKE_MATCH_2}" _expected)

if(EXISTS "${ARCHIVE_DEST}")
  file(${_algo} "${ARCHIVE_DEST}" _actual)
  if(_actual STREQUAL _expected)
    return()
  endif()
  file(REMOVE "${ARCHIVE_DEST}")
endif()

set(_attempts 5)
foreach(_attempt RANGE 1 ${_attempts})
  file(DOWNLOAD "${ARCHIVE_URL}" "${ARCHIVE_DEST}" INACTIVITY_TIMEOUT 60 STATUS _status)
  list(GET _status 0 _code)
  list(GET _status 1 _error)

  if(_code EQUAL 0)
    file(${_algo} "${ARCHIVE_DEST}" _actual)
    if(_actual STREQUAL _expected)
      return()
    endif()
    # A complete transfer with the wrong hash is a wrong pin, not a network fault
    file(REMOVE "${ARCHIVE_DEST}")
    message(FATAL_ERROR "${ARCHIVE_URL}: ${_algo} mismatch, expected ${_expected}, got ${_actual}")
  endif()

  file(REMOVE "${ARCHIVE_DEST}")
  if(_attempt EQUAL _attempts)
    message(FATAL_ERROR "Failed to download ${ARCHIVE_URL} after ${_attempts} attempts: ${_error}")
  endif()
  math(EXPR _delay "${_attempt} * 10")
  message(STATUS "Download of ${ARCHIVE_URL} failed (${_error}), attempt ${_attempt}/${_attempts}, retrying in ${_delay}s")
  execute_process(COMMAND ${CMAKE_COMMAND} -E sleep ${_delay})
endforeach()
