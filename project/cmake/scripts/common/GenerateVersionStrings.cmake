# This script parses version.txt and libKODI_guilib.h and sets variables
# used to construct dirs structure, file naming, API versions, etc.
#
# On return, the following variables are set:
# ** set from version.txt **
# APP_NAME - app name
# APP_NAME_LC - lowercased app name
# APP_NAME_UC - uppercased app name
# COMPANY_NAME - company name
# APP_VERSION_MAJOR - the app version major
# APP_VERSION_MINOR - the app version minor
# APP_VERSION_TAG - the app version tag
# APP_VERSION_TAG_LC - lowercased app version tag
# APP_VERSION - the app version (${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}-${APP_VERSION_TAG})
# APP_ADDON_API - the addon API version in the form of 16.9.702
# FILE_VERSION - file version in the form of 16,9,702, Windows only
#
# ** set from libKODI_guilib.h **
# guilib_version - current ADDONGUI API version
# guilib_version_min - minimal ADDONGUI API version

include(CMakeParseArguments)
include(${CORE_SOURCE_DIR}/project/cmake/scripts/common/Macros.cmake)
core_file_read_filtered(version_list ${CORE_SOURCE_DIR}/version.txt)
string(REPLACE " " ";" version_list "${version_list}")
cmake_parse_arguments(APP "" "VERSION_MAJOR;VERSION_MINOR;VERSION_TAG;VERSION_CODE;ADDON_API;APP_NAME;COMPANY_NAME" "" ${version_list})

set(APP_NAME ${APP_APP_NAME}) # inconsistency but APP_APP_NAME looks weird
string(TOLOWER ${APP_APP_NAME} APP_NAME_LC)
string(TOUPPER ${APP_APP_NAME} APP_NAME_UC)
set(COMPANY_NAME ${APP_COMPANY_NAME})
set(APP_VERSION ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR})
if(APP_VERSION_TAG)
  set(APP_VERSION ${APP_VERSION}-${APP_VERSION_TAG})
endif()
string(REPLACE "." "," FILE_VERSION ${APP_ADDON_API}.0)
string(TOLOWER ${APP_VERSION_TAG} APP_VERSION_TAG_LC)
file(STRINGS ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/libKODI_guilib.h guilib_version REGEX "^.*GUILIB_API_VERSION (.*)$")
string(REGEX REPLACE ".*\"(.*)\"" "\\1" guilib_version ${guilib_version})
file(STRINGS ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/libKODI_guilib.h guilib_version_min REGEX "^.*GUILIB_MIN_API_VERSION (.*)$")
string(REGEX REPLACE ".*\"(.*)\"" "\\1" guilib_version_min ${guilib_version_min})

# bail if we can't parse version.txt
if(NOT DEFINED APP_VERSION_MAJOR OR NOT DEFINED APP_VERSION_MINOR)
  message(FATAL_ERROR "Could not determine app version! Make sure that ${CORE_SOURCE_DIR}/version.txt exists")
endif()

# bail if we can't parse libKODI_guilib.h
if(NOT DEFINED guilib_version)
  message(FATAL_ERROR "Could not determine add-on API version! Make sure that ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi/libKODI_guilib.h exists")
endif()
