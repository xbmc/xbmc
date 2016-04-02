# - Try to find libssh
# Once done this will define
#
# SSH_FOUND - system has libssh
# SSH_INCLUDE_DIRS - the libssh include directory
# SSH_LIBRARIES - The libssh libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (SSH libssh)
  list(APPEND SSH_INCLUDE_DIRS /usr/include)
else()
  find_path(SSH_INCLUDE_DIRS libssh/libssh.h)
  find_library(SSH_LIBRARIES ssh)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SSH DEFAULT_MSG SSH_INCLUDE_DIRS SSH_LIBRARIES)

list(APPEND SSH_DEFINITIONS -DHAVE_LIBSSH=1)

mark_as_advanced(SSH_INCLUDE_DIRS SSH_LIBRARIES SSH_DEFINITIONS)
