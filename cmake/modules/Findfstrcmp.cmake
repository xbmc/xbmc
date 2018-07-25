#.rst:
# Findfstrcmp
# --------
# Finds the fstrcmp library
#
# This will define the following variables::
#
# FSTRCMP_FOUND - system has libfstrcmp
# FSTRCMP_INCLUDE_DIRS - the libfstrcmp include directory
# FSTRCMP_LIBRARIES - the libfstrcmp libraries
#

if(ENABLE_INTERNAL_FSTRCMP)
  include(ExternalProject)
  file(STRINGS ${CMAKE_SOURCE_DIR}/tools/depends/target/libfstrcmp/Makefile VER)
  string(REGEX MATCH "VERSION=[^ ]*" FSTRCMP_VER "${VER}")
  list(GET FSTRCMP_VER 0 FSTRCMP_VER)
  string(SUBSTRING "${FSTRCMP_VER}" 8 -1 FSTRCMP_VER)

  # allow user to override the download URL with a local tarball
  # needed for offline build envs
  if(FSTRCMP_URL)
    get_filename_component(FSTRCMP_URL "${FSTRCMP_URL}" ABSOLUTE)
  else()
    set(FSTRCMP_URL http://mirrors.kodi.tv/build-deps/sources/fstrcmp-${FSTRCMP_VER}.tar.gz)
  endif()
  if(VERBOSE)
    message(STATUS "FSTRCMPURL: ${FSTRCMP_URL}")
  endif()

  set(FSTRCMP_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/libfstrcmp.a)
  set(FSTRCMP_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)
  externalproject_add(fstrcmp
                      URL ${FSTRCMP_URL}
                      DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/download
                      PREFIX ${CORE_BUILD_DIR}/fstrcmp
                      CONFIGURE_COMMAND autoreconf -vif && ./configure --prefix ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                      BUILD_BYPRODUCTS ${FSTRCMP_LIBRARY}
                      BUILD_IN_SOURCE 1)
  set_target_properties(fstrcmp PROPERTIES FOLDER "External Projects")

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(fstrcmp
                                    REQUIRED_VARS FSTRCMP_LIBRARY FSTRCMP_INCLUDE_DIR
                                    VERSION_VAR FSTRCMP_VER)

  set(FSTRCMP_LIBRARIES -Wl,-Bstatic ${FSTRCMP_LIBRARY} -Wl,-Bdynamic)
  set(FSTRCMP_INCLUDE_DIRS ${FSTRCMP_INCLUDE_DIR})
else()
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_FSTRCMP fstrcmp QUIET)
  endif()

  find_path(FSTRCMP_INCLUDE_DIR NAMES fstrcmp.h
                                 PATHS ${PC_FSTRCMP_INCLUDEDIR})

  find_library(FSTRCMP_LIBRARY NAMES fstrcmp
                                PATHS ${PC_FSTRCMP_LIBDIR})

  set(FSTRCMP_VERSION ${PC_FSTRCMP_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(fstrcmp
                                    REQUIRED_VARS FSTRCMP_LIBRARY FSTRCMP_INCLUDE_DIR
                                    VERSION_VAR FSTRCMP_VERSION)

  if(FSTRCMP_FOUND)
    set(FSTRCMP_INCLUDE_DIRS ${FSTRCMP_INCLUDE_DIR})
    set(FSTRCMP_LIBRARIES ${FSTRCMP_LIBRARY})
  endif()

  if(NOT TARGET fstrcmp)
    add_library(fstrcmp UNKNOWN IMPORTED)
    set_target_properties(fstrcmp PROPERTIES
                                  IMPORTED_LOCATION "${FSTRCMP_LIBRARY}"
                                  INTERFACE_INCLUDE_DIRECTORIES "${FSTRCMP_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(FSTRCMP_INCLUDE_DIR FSTRCMP_LIBRARY)
