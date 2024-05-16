#.rst:
# FindMicroHttpd
# --------------
# Finds the MicroHttpd library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::MicroHttpd   - The microhttpd library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  find_package(PkgConfig)

  if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWS_STORE))
    if(MicroHttpd_FIND_VERSION)
      if(MicroHttpd_FIND_VERSION_EXACT)
        set(MicroHttpd_FIND_SPEC "=${MicroHttpd_FIND_VERSION_COMPLETE}")
      else()
        set(MicroHttpd_FIND_SPEC ">=${MicroHttpd_FIND_VERSION_COMPLETE}")
      endif()
    endif()

    pkg_check_modules(MICROHTTPD libmicrohttpd${MicroHttpd_FIND_SPEC} QUIET)

    # First item is the full path of the library file found
    # pkg_check_modules does not populate a variable of the found library explicitly
    list(GET MICROHTTPD_LINK_LIBRARIES 0 MICROHTTPD_LIBRARY)

    # Add link libraries for static lib usage
    if(${MICROHTTPD_LIBRARY} MATCHES ".+\.a$" AND MICROHTTPD_LINK_LIBRARIES)
      # Remove duplicates
      list(REMOVE_DUPLICATES MICROHTTPD_LINK_LIBRARIES)

      # Remove own library - eg libmicrohttpd.a
      list(FILTER MICROHTTPD_LINK_LIBRARIES EXCLUDE REGEX ".*microhttpd.*\.a$")
      set(PC_MICROHTTPD_LINK_LIBRARIES ${MICROHTTPD_LINK_LIBRARIES})
    endif()

    # pkgconfig sets MICROHTTPD_INCLUDEDIR, map this to our "standard" variable name
    set(MICROHTTPD_INCLUDE_DIR ${MICROHTTPD_INCLUDEDIR})
  else()

    find_path(MICROHTTPD_INCLUDE_DIR NAMES microhttpd.h
                                     HINTS ${DEPENDS_PATH}/include
                                     ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

    find_library(MICROHTTPD_LIBRARY NAMES microhttpd libmicrohttpd
                                    HINTS ${DEPENDS_PATH}/lib
                                    ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(MicroHttpd
                                    REQUIRED_VARS MICROHTTPD_LIBRARY MICROHTTPD_INCLUDE_DIR
                                    VERSION_VAR MICROHTTPD_VERSION)

  if(MICROHTTPD_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${MICROHTTPD_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${MICROHTTPD_INCLUDE_DIR}"
                                                                     INTERFACE_COMPILE_DEFINITIONS "HAS_WEB_SERVER;HAS_WEB_INTERFACE")

      # Add link libraries for static lib usage found from pkg-config
      if(PC_MICROHTTPD_LINK_LIBRARIES)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         INTERFACE_LINK_LIBRARIES "${PC_MICROHTTPD_LINK_LIBRARIES}")
      endif()

    if(${MICROHTTPD_LIBRARY} MATCHES ".+\.a$" AND PC_MICROHTTPD_STATIC_LIBRARIES)
      list(APPEND MICROHTTPD_LIBRARIES ${PC_MICROHTTPD_STATIC_LIBRARIES})
    endif()
  endif()
endif()
