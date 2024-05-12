#.rst:
# FindSmbClient
# -------------
# Finds the SMB Client library
#
# This following imported target will be defined::
#
#   ${APP_NAME_LC}::SmbClient   - The SmbClient library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  if(WIN32 OR WINDOWS_STORE)
    # UWP doesnt have native smb support. It receives it from an addon.
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_COMPILE_DEFINITIONS HAS_FILESYSTEM_SMB)
    endif()
  else()

    find_package(PkgConfig)

    if(PKG_CONFIG_FOUND)
      pkg_check_modules(SMBCLIENT smbclient QUIET)

      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET SMBCLIENT_LINK_LIBRARIES 0 SMBCLIENT_LIBRARY)

      # Add link libraries for static lib usage
      if(${SMBCLIENT_LIBRARY} MATCHES ".+\.a$" AND SMBCLIENT_LINK_LIBRARIES)
        # Remove duplicates
        list(REMOVE_DUPLICATES SMBCLIENT_LINK_LIBRARIES)

        # Remove own library
        list(FILTER SMBCLIENT_LINK_LIBRARIES EXCLUDE REGEX ".*smbclient.*\.a$")
        set(PC_SMBCLIENT_LINK_LIBRARIES ${SMBCLIENT_LINK_LIBRARIES})
      endif()

      # pkgconfig sets SMBCLIENT_INCLUDEDIR, map this to our "standard" variable name
      set(SMBCLIENT_INCLUDE_DIR ${SMBCLIENT_INCLUDEDIR})
      set(SMBCLIENT_VERSION ${PC_SMBCLIENT_VERSION})
    else()
      find_path(SMBCLIENT_INCLUDE_DIR NAMES libsmbclient.h)
      find_library(SMBCLIENT_LIBRARY NAMES smbclient)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(SmbClient
                                      REQUIRED_VARS SMBCLIENT_LIBRARY SMBCLIENT_INCLUDE_DIR
                                      VERSION_VAR SMBCLIENT_VERSION)

    if(SMBCLIENT_FOUND)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_LOCATION "${SMBCLIENT_LIBRARY}"
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${SMBCLIENT_INCLUDE_DIR}"
                                                                       INTERFACE_COMPILE_DEFINITIONS HAS_FILESYSTEM_SMB)

      # Add link libraries for static lib usage found from pkg-config
      if(PC_SMBCLIENT_LINK_LIBRARIES)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         INTERFACE_LINK_LIBRARIES "${PC_SMBCLIENT_LINK_LIBRARIES}")
      endif()
    endif()
  endif()
endif()
