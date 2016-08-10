#.rst:
# FindSSH
# -------
# Finds the SSH library
#
# This will will define the following variables::
#
# SSH_FOUND - system has SSH
# SSH_INCLUDE_DIRS - the SSH include directory
# SSH_LIBRARIES - the SSH libraries
# SSH_DEFINITIONS - the SSH definitions
#
# and the following imported targets::
#
#   SSH::SSH   - The SSH library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_SSH libssh QUIET)
endif()

find_path(SSH_INCLUDE_DIR NAMES libssh/libssh.h
                          PATHS ${PC_SSH_INCLUDEDIR})
find_library(SSH_LIBRARY NAMES ssh
                         PATHS ${PC_SSH_LIBDIR})

set(SSH_VERSION ${PC_SSH_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SSH
                                  REQUIRED_VARS SSH_LIBRARY SSH_INCLUDE_DIR
                                  VERSION_VAR SSH_VERSION)

if(SSH_FOUND)
  set(SSH_LIBRARIES ${SSH_LIBRARY})
  set(SSH_INCLUDE_DIRS ${SSH_INCLUDE_DIR})
  set(SSH_DEFINITIONS -DHAVE_LIBSSH=1)

  if(NOT TARGET SSH::SSH)
    add_library(SSH::SSH UNKNOWN IMPORTED)
    set_target_properties(SSH::SSH PROPERTIES
                                   IMPORTED_LOCATION "${SSH_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${SSH_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS HAVE_LIBSSH=1)
  endif()
endif()

mark_as_advanced(SSH_INCLUDE_DIR SSH_LIBRARY)
