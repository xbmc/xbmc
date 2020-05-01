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
  include(${WITH_KODI_DEPENDS}/packages/fstrcmp/package.cmake)
  add_depends_for_targets("HOST")

  add_custom_target(fstrcmp ALL DEPENDS fstrcmp-host)

  set(FSTRCMP_LIBRARY ${INSTALL_PREFIX_HOST}/lib/libfstrcmp.a)
  set(FSTRCMP_INCLUDE_DIR ${INSTALL_PREFIX_HOST}/include)

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
