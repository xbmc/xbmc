# FindEpollShim
# -------------
# Finds the epoll-shim library
#
# This will define the following variables::
#
# EPOLLSHIM_FOUND       - the system has epoll-shim
# EPOLLSHIM_INCLUDE_DIR - the epoll-shim include directory
# EPOLLSHIM_LIBRARY     - the epoll-shim library

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_EPOLLSHIM epoll-shim QUIET)
endif()

find_path(EPOLLSHIM_INCLUDE_DIR NAMES sys/epoll.h
                                HINTS ${PC_EPOLLSHIM_INCLUDE_DIRS})
find_library(EPOLLSHIM_LIBRARY NAMES epoll-shim
                               HINTS ${PC_EPOLLSHIM_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EpollShim
                                  REQUIRED_VARS EPOLLSHIM_LIBRARY EPOLLSHIM_INCLUDE_DIR)

if(EPOLLSHIM_FOUND)
  set(EPOLLSHIM_INCLUDE_DIRS ${EPOLLSHIM_INCLUDE_DIR})
  set(EPOLLSHIM_LIBRARIES ${EPOLLSHIM_LIBRARY})
endif()

mark_as_advanced(EPOLLSHIM_INCLUDE_DIR EPOLLSHIM_LIBRARY)
