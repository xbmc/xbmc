# This script provides helper functions for FindModules

# Parse and set variables from VERSION dependency file
# Arguments:
#   module_name name of the library (currently must match tools/depends/target/${module_name})
# On return:
#   ARCHIVE will be set to parent scope
#   MODULENAME_VER will be set to parent scope (eg FFMPEG_VER, DAV1D_VER)
#   MODULENAME_BASE_URL will be set to parent scope if exists in VERSION file (eg FFMPEG_BASE_URL)
function(get_archive_name module_name)
  string(TOUPPER ${module_name} UPPER_MODULE_NAME)

  # Dependency path
  set(MODULE_PATH "${CMAKE_SOURCE_DIR}/tools/depends/target/${module_name}")
  if(NOT EXISTS "${MODULE_PATH}/${UPPER_MODULE_NAME}-VERSION")
    MESSAGE(FATAL_ERROR "${UPPER_MODULE_NAME}-VERSION does not exist at ${MODULE_PATH}.")
  else()
    set(${UPPER_MODULE_NAME}_FILE "${MODULE_PATH}/${UPPER_MODULE_NAME}-VERSION")
  endif()

  file(STRINGS ${${UPPER_MODULE_NAME}_FILE} ${UPPER_MODULE_NAME}_LNAME REGEX "^[ \t]*LIBNAME=")
  file(STRINGS ${${UPPER_MODULE_NAME}_FILE} ${UPPER_MODULE_NAME}_VER REGEX "^[ \t]*VERSION=")
  file(STRINGS ${${UPPER_MODULE_NAME}_FILE} ${UPPER_MODULE_NAME}_ARCHIVE REGEX "^[ \t]*ARCHIVE=")
  file(STRINGS ${${UPPER_MODULE_NAME}_FILE} ${UPPER_MODULE_NAME}_BASE_URL REGEX "^[ \t]*BASE_URL=")

  string(REGEX REPLACE ".*LIBNAME=([^ \t]*).*" "\\1" ${UPPER_MODULE_NAME}_LNAME "${${UPPER_MODULE_NAME}_LNAME}")
  string(REGEX REPLACE ".*VERSION=([^ \t]*).*" "\\1" ${UPPER_MODULE_NAME}_VER "${${UPPER_MODULE_NAME}_VER}")
  string(REGEX REPLACE ".*ARCHIVE=([^ \t]*).*" "\\1" ${UPPER_MODULE_NAME}_ARCHIVE "${${UPPER_MODULE_NAME}_ARCHIVE}")
  string(REGEX REPLACE ".*BASE_URL=([^ \t]*).*" "\\1" ${UPPER_MODULE_NAME}_BASE_URL "${${UPPER_MODULE_NAME}_BASE_URL}")

  string(REGEX REPLACE "\\$\\(LIBNAME\\)" "${${UPPER_MODULE_NAME}_LNAME}" ${UPPER_MODULE_NAME}_ARCHIVE "${${UPPER_MODULE_NAME}_ARCHIVE}")
  string(REGEX REPLACE "\\$\\(VERSION\\)" "${${UPPER_MODULE_NAME}_VER}" ${UPPER_MODULE_NAME}_ARCHIVE "${${UPPER_MODULE_NAME}_ARCHIVE}")

  set(ARCHIVE ${${UPPER_MODULE_NAME}_ARCHIVE} PARENT_SCOPE)
  set(${UPPER_MODULE_NAME}_VER ${${UPPER_MODULE_NAME}_VER} PARENT_SCOPE)
  if (${UPPER_MODULE_NAME}_BASE_URL)
    set(${UPPER_MODULE_NAME}_BASE_URL ${${UPPER_MODULE_NAME}_BASE_URL} PARENT_SCOPE)
  endif()
endfunction()
