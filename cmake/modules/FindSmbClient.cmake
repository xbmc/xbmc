#.rst:
# FindSmbClient
# -------------
# Finds the SMB Client library
#
# This will define the following variables::
#
# SMBCLIENT_FOUND - system has SmbClient
# SMBCLIENT_INCLUDE_DIRS - the SmbClient include directory
# SMBCLIENT_LIBRARIES - the SmbClient libraries
# SMBCLIENT_DEFINITIONS - the SmbClient definitions
#
# and the following imported targets::
#
#   SmbClient::SmbClient   - The SmbClient library

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_SMBCLIENT smbclient QUIET)
endif()

find_path(SMBCLIENT_INCLUDE_DIR NAMES libsmbclient.h
                                HINTS ${PC_SMBCLIENT_INCLUDEDIR})
find_library(SMBCLIENT_LIBRARY NAMES smbclient
                               HINTS ${PC_SMBCLIENT_LIBDIR})

set(SMBCLIENT_VERSION ${PC_SMBCLIENT_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SmbClient
                                  REQUIRED_VARS SMBCLIENT_LIBRARY SMBCLIENT_INCLUDE_DIR
                                  VERSION_VAR SMBCLIENT_VERSION)

if(SMBCLIENT_FOUND)
  set(SMBCLIENT_LIBRARIES ${SMBCLIENT_LIBRARY})
  if(${SMBCLIENT_LIBRARY} MATCHES ".+\.a$" AND PC_SMBCLIENT_STATIC_LIBRARIES)
    list(APPEND SMBCLIENT_LIBRARIES ${PC_SMBCLIENT_STATIC_LIBRARIES})
  endif()
  set(SMBCLIENT_INCLUDE_DIRS ${SMBCLIENT_INCLUDE_DIR})
  set(SMBCLIENT_DEFINITIONS -DHAS_FILESYSTEM_SMB=1)

  if(NOT TARGET SmbClient::SmbClient)
    add_library(SmbClient::SmbClient UNKNOWN IMPORTED)
    set_target_properties(SmbClient::SmbClient PROPERTIES
                                   IMPORTED_LOCATION "${SMBCLIENT_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${SMBCLIENT_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS HAS_FILESYSTEM_SMB=1)
  endif()
endif()

mark_as_advanced(LIBSMBCLIENT_INCLUDE_DIR LIBSMBCLIENT_LIBRARY)
