#.rst:
# FindBluray
# ----------
# Finds the libbluray library
#
# This will define the following target:
#
#   Bluray::Bluray   - The libbluray library

if(NOT TARGET Bluray::Bluray)
  if(PKG_CONFIG_FOUND)
    if(Bluray_FIND_VERSION_EXACT)
      set(Bluray_FIND_SPEC "=${Bluray_FIND_VERSION_COMPLETE}")
    else()
      set(Bluray_FIND_SPEC ">=${Bluray_FIND_VERSION_COMPLETE}")
    endif()
    pkg_check_modules(PC_BLURAY libbluray${Bluray_FIND_SPEC} QUIET)
    set(BLURAY_VERSION ${PC_BLURAY_VERSION})
  endif()

  find_path(BLURAY_INCLUDE_DIR NAMES libbluray/bluray.h
                               HINTS ${DEPENDS_PATH}/include ${PC_BLURAY_INCLUDEDIR}
                               ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                               NO_CACHE)

  if(NOT BLURAY_VERSION AND EXISTS ${BLURAY_INCLUDE_DIR}/libbluray/bluray-version.h)
    file(STRINGS ${BLURAY_INCLUDE_DIR}/libbluray/bluray-version.h _bluray_version_str
         REGEX "#define[ \t]BLURAY_VERSION_STRING[ \t][\"]?[0-9.]+[\"]?")
    string(REGEX REPLACE "^.*BLURAY_VERSION_STRING[ \t][\"]?([0-9.]+).*$" "\\1" BLURAY_VERSION ${_bluray_version_str})
    unset(_bluray_version_str)
  endif()

  find_library(BLURAY_LIBRARY NAMES bluray libbluray
                              HINTS ${DEPENDS_PATH}/lib ${PC_BLURAY_LIBDIR}
                              ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                              NO_CACHE)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Bluray
                                    REQUIRED_VARS BLURAY_LIBRARY BLURAY_INCLUDE_DIR BLURAY_VERSION
                                    VERSION_VAR BLURAY_VERSION)

  if(BLURAY_FOUND)
    add_library(Bluray::Bluray UNKNOWN IMPORTED)
    set_target_properties(Bluray::Bluray PROPERTIES
                                         IMPORTED_LOCATION "${BLURAY_LIBRARY}"
                                         INTERFACE_INCLUDE_DIRECTORIES "${BLURAY_INCLUDE_DIR}")

    if(${BLURAY_LIBRARY} MATCHES ".+\.a$" AND PC_BLURAY_STATIC_LIBRARIES)
      set_target_properties(Bluray::Bluray PROPERTIES
                                           INTERFACE_LINK_LIBRARIES ${PC_BLURAY_STATIC_LIBRARIES})
    endif()

    if(NOT CORE_PLATFORM_NAME_LC STREQUAL windowsstore)
      set_target_properties(Bluray::Bluray PROPERTIES
                                           INTERFACE_COMPILE_DEFINITIONS "HAVE_LIBBLURAY_BDJ=1")
    endif()

    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Bluray::Bluray)
  endif()
endif()
