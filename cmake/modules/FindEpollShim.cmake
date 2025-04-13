# FindEpollShim
# -------------
# Finds the epoll-shim library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::EpollShim   - The epoll-shim library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_package(PkgConfig ${SEARCH_QUIET})

  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_EPOLLSHIM epoll-shim ${SEARCH_QUIET})
  endif()

  find_path(EPOLLSHIM_INCLUDE_DIR NAMES sys/epoll.h
                                  HINTS ${PC_EPOLLSHIM_INCLUDE_DIRS})
  find_library(EPOLLSHIM_LIBRARY NAMES epoll-shim
                                 HINTS ${PC_EPOLLSHIM_LIBDIR})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(EpollShim
                                    REQUIRED_VARS EPOLLSHIM_LIBRARY EPOLLSHIM_INCLUDE_DIR)

  if(EPOLLSHIM_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     IMPORTED_LOCATION "${EPOLLSHIM_LIBRARY}"
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${EPOLLSHIM_INCLUDE_DIR}")
  endif()
endif()
