#.rst:
# FindFstrcmp
# --------
# Finds the fstrcmp library
#
# This will define the following target:
#
#   fstrcmp::fstrcmp   - The fstrcmp library
#

if(NOT TARGET fstrcmp::fstrcmp)
  if(ENABLE_INTERNAL_FSTRCMP)
    find_program(LIBTOOL libtool REQUIRED)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC fstrcmp)

    SETUP_BUILD_VARS()

    find_program(AUTORECONF autoreconf REQUIRED)

    set(CONFIGURE_COMMAND ${AUTORECONF} -vif
                  COMMAND ./configure --prefix ${DEPENDS_PATH})
    set(BUILD_COMMAND make lib/libfstrcmp.la)
    set(BUILD_IN_SOURCE 1)
    set(INSTALL_COMMAND make install-libdir install-include)

    BUILD_DEP_TARGET()
  else()
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
      pkg_check_modules(PC_FSTRCMP fstrcmp QUIET)
    endif()

    find_path(FSTRCMP_INCLUDE_DIR NAMES fstrcmp.h
                                  HINTS ${DEPENDS_PATH}/include ${PC_FSTRCMP_INCLUDEDIR}
                                  ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                                  NO_CACHE)

    find_library(FSTRCMP_LIBRARY NAMES fstrcmp
                                 HINTS ${DEPENDS_PATH}/lib ${PC_FSTRCMP_LIBDIR}
                                 ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                                 NO_CACHE)

    set(FSTRCMP_VER ${PC_FSTRCMP_VERSION})
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Fstrcmp
                                    REQUIRED_VARS FSTRCMP_LIBRARY FSTRCMP_INCLUDE_DIR
                                    VERSION_VAR FSTRCMP_VER)

  add_library(fstrcmp::fstrcmp UNKNOWN IMPORTED)
  set_target_properties(fstrcmp::fstrcmp PROPERTIES
                                         IMPORTED_LOCATION "${FSTRCMP_LIBRARY}"
                                         INTERFACE_INCLUDE_DIRECTORIES "${FSTRCMP_INCLUDE_DIR}")

  if(TARGET fstrcmp)
    add_dependencies(fstrcmp::fstrcmp fstrcmp)
  endif()

  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP fstrcmp::fstrcmp)
endif()
