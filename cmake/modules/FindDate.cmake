#.rst:
# FindDate
# -------
# Finds the DATE library
#
# This will define the following variables::
#
# DATE_FOUND - system has DATE
# DATE_INCLUDE_DIRS - the DATE include directory
# DATE_LIBRARIES - the DATE libraries

set(DATE_VERSION "3.0.0")

find_library(DATE_LIBRARY NAMES date-tz libdate-tz)
find_path(DATE_INCLUDE_DIR NAMES date/date.h)

if(ENABLE_INTERNAL_DATE)
  include(ExternalProject)

  # allow user to override the download URL with a local tarball
  # needed for offline build envs
  if(DATE_URL)
    get_filename_component(DATE_URL "${DATE_URL}" ABSOLUTE)
  else()
    set(DATE_URL http://mirrors.kodi.tv/build-deps/sources/date-v${DATE_VERSION}.tar.gz)
  endif()

  if(VERBOSE)
    message(STATUS "DATE_URL: ${DATE_URL}")
  endif()

  set(DATE_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/libdate-tz.a)
  set(DATE_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)

  externalproject_add(Date
                      URL ${DATE_URL}
                      DOWNLOAD_NAME date-${DATE_VERSION}.tar.gz
                      DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download
                      PREFIX ${CORE_BUILD_DIR}/date
                      INSTALL_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DCMAKE_CXX_STANDARD=14 -DUSE_SYSTEM_TZ_DB=OFF -DMANUAL_TZ_DB=ON -DUSE_TZ_DB_IN_DOT=OFF -DBUILD_SHARED_LIBS=OFF -DDISABLE_STRING_VIEW=ON -DBUILD_TZ_LIB=ON -DCMAKE_INSTALL_LIBDIR=lib
                      BUILD_BYPRODUCTS ${DATE_LIBRARY})

  externalproject_add_step(Date install_headers
                          COMMAND ${CMAKE_COMMAND} -E copy include/date/iso_week.h ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include/date/iso_week.h
                          WORKING_DIRECTORY <SOURCE_DIR>
                          DEPENDEES install)

  set_target_properties(Date PROPERTIES FOLDER "External Projects")
  list(APPEND GLOBAL_TARGET_DEPS Date)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Date
                                  REQUIRED_VARS DATE_LIBRARY DATE_INCLUDE_DIR
                                  VERSION_VAR DATE_VERSION)

if(DATE_FOUND)
  set(DATE_INCLUDE_DIRS ${DATE_INCLUDE_DIR})
  set(DATE_LIBRARIES ${DATE_LIBRARY})
endif()

mark_as_advanced(DATE_INCLUDE_DIR DATE_LIBRARY)

# not sure where this should go so it lives here for now

set(TZDATA_VERSION "2020a")

# allow user to override the download URL with a local tarball
# needed for offline build envs
if(TZDATA_URL)
  get_filename_component(TZDATA_URL "${TZDATA_URL}" ABSOLUTE)
else()
  set(TZDATA_URL http://mirrors.kodi.tv/build-deps/sources/tzdata${TZDATA_VERSION}.tar.gz)
endif()

if(VERBOSE)
  message(STATUS "TZDATA_URL: ${TZDATA_URL}")
endif()

externalproject_add(tzdata
                    URL ${TZDATA_URL}
                    DOWNLOAD_NAME tzdata-${TZDATA_VERSION}.tar.gz
                    DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download
                    PREFIX ${CORE_BUILD_DIR}/tzdata
                    INSTALL_DIR ${CMAKE_BINARY_DIR}/addons/resource.timezone/resources/tzdata
                    CONFIGURE_COMMAND ""
                    BUILD_COMMAND ""
                    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR> <INSTALL_DIR>
                    BUILD_IN_SOURCE ON)

list(APPEND GLOBAL_TARGET_DEPS tzdata)
