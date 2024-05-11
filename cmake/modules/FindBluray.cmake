#.rst:
# FindBluray
# ----------
# Finds the libbluray library
#
# This will define the following target:
#
#   Bluray::Bluray   - The libbluray library

if(NOT TARGET Bluray::Bluray)

  find_package(PkgConfig)

  # We only rely on pkgconfig for non windows platforms
  if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
    if(Bluray_FIND_VERSION_EXACT)
      set(Bluray_FIND_SPEC "=${Bluray_FIND_VERSION_COMPLETE}")
    else()
      set(Bluray_FIND_SPEC ">=${Bluray_FIND_VERSION_COMPLETE}")
    endif()
    pkg_check_modules(BLURAY libbluray${Bluray_FIND_SPEC} QUIET)

    # First item is the full path of the library file found
    # pkg_check_modules does not populate a variable of the found library explicitly
    list(GET BLURAY_LINK_LIBRARIES 0 BLURAY_LIBRARY)
  else()
    # todo: for windows use find_package CONFIG call potentially

    find_path(BLURAY_INCLUDEDIR NAMES libbluray/bluray.h
                                HINTS ${DEPENDS_PATH}/include
                                ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                                NO_CACHE)

    find_library(BLURAY_LIBRARY NAMES bluray libbluray
                                HINTS ${DEPENDS_PATH}/lib
                                ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                                NO_CACHE)
  endif()

  if(NOT BLURAY_VERSION AND EXISTS ${BLURAY_INCLUDEDIR}/libbluray/bluray-version.h)
    file(STRINGS ${BLURAY_INCLUDEDIR}/libbluray/bluray-version.h _bluray_version_str
         REGEX "#define[ \t]BLURAY_VERSION_STRING[ \t][\"]?[0-9.]+[\"]?")
    string(REGEX REPLACE "^.*BLURAY_VERSION_STRING[ \t][\"]?([0-9.]+).*$" "\\1" BLURAY_VERSION ${_bluray_version_str})
    unset(_bluray_version_str)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Bluray
                                    REQUIRED_VARS BLURAY_LIBRARY BLURAY_INCLUDEDIR BLURAY_VERSION
                                    VERSION_VAR BLURAY_VERSION)

  if(BLURAY_FOUND)
    add_library(Bluray::Bluray UNKNOWN IMPORTED)
    set_target_properties(Bluray::Bluray PROPERTIES
                                         IMPORTED_LOCATION "${BLURAY_LIBRARY}"
                                         INTERFACE_COMPILE_DEFINITIONS "HAVE_LIBBLURAY=1"
                                         INTERFACE_INCLUDE_DIRECTORIES "${BLURAY_INCLUDEDIR}")

    # Add link libraries for static lib usage
    if(${BLURAY_LIBRARY} MATCHES ".+\.a$" AND BLURAY_LINK_LIBRARIES)
      # Remove duplicates
      list(REMOVE_DUPLICATES BLURAY_LINK_LIBRARIES)

      # Remove own library - eg libbluray.a
      list(FILTER BLURAY_LINK_LIBRARIES EXCLUDE REGEX ".*bluray.*\.a$")

      set_target_properties(Bluray::Bluray PROPERTIES
                                           INTERFACE_LINK_LIBRARIES "${BLURAY_LINK_LIBRARIES}")
    endif()

    if(NOT CORE_PLATFORM_NAME_LC STREQUAL windowsstore)
      set_property(TARGET Bluray::Bluray APPEND PROPERTY
                                                INTERFACE_COMPILE_DEFINITIONS "HAVE_LIBBLURAY_BDJ")
    endif()

    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Bluray::Bluray)
  endif()
endif()
