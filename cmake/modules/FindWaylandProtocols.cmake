# FindWaylandProtocols
# --------------------
# Find wayland-protocols
#
# This will define the following variables::
#
# WAYLAND_PROTOCOLS_DIR - directory containing the additional Wayland protocols
#                         from the wayland-protocols package
#
# This will define the following target:
#
#   ${APP_NAME_LC}::WaylandProtocols   - The Wayland protocols

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

   find_package(PkgConfig ${SEARCH_QUIET})
   pkg_check_modules(PC_WAYLAND_PROTOCOLS wayland-protocols ${SEARCH_QUIET})
   if(PC_WAYLAND_PROTOCOLS_FOUND)
     pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
   endif()

   # Promote to cache variables so all code can access it
   set(WAYLAND_PROTOCOLS_DIR ${WAYLAND_PROTOCOLS_DIR} CACHE INTERNAL "")

   if(NOT VERBOSE_FIND)
      set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
    endif()

   include(FindPackageHandleStandardArgs)
   find_package_handle_standard_args(WaylandProtocols
     REQUIRED_VARS
       PC_WAYLAND_PROTOCOLS_FOUND
       WAYLAND_PROTOCOLS_DIR
     VERSION_VAR
       PC_WAYLAND_PROTOCOLS_VERSION)

   if(PC_WAYLAND_PROTOCOLS_FOUND)
       add_library(${CMAKE_FIND_PACKAGE_NAME} INTERFACE)
       add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS ${CMAKE_FIND_PACKAGE_NAME})

       set_target_properties(${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                        VERSION PC_WAYLAND_PROTOCOLS_VERSION)

       if(PC_WAYLAND_PROTOCOLS_VERSION VERSION_GREATER_EQUAL 1.41)
          set_target_properties(${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                           INTERFACE_COMPILE_DEFINITIONS HAS_WAYLAND_COLOR_MANAGEMENT)
       endif()
   endif()

endif()
