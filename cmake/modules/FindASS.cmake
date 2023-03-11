#.rst:
# FindASS
# -------
# Finds the ASS library
#
# This will define the following variables::
#
# ASS_FOUND - system has ASS
# ASS_INCLUDE_DIRS - the ASS include directory
# ASS_LIBRARIES - the ASS libraries
#
# and the following imported targets::
#
#   ASS::ASS   - The ASS library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_ASS libass QUIET)

  # Darwin platforms have lib options like -framework CoreServices. pkgconfig return of
  # PC_ASS_LDFLAGS splits this into a list -framework;CoreServices, and when passed to linker
  # This then treats them as individual flags and appends -l to CoreServices. eg -framework;-lCoreServices
  # This causes failures, as -lCoreServices isnt a lib that can be found.
  # This just formats the list data to append frameworks (eg "-framework CoreServices")
  if(PC_ASS_LDFLAGS AND "-framework" IN_LIST PC_ASS_LDFLAGS)
    set(_framework_command OFF)
    foreach(flag ${PC_ASS_LDFLAGS})
      if(flag STREQUAL "-framework")
        set(_framework_command ON)
        continue()
      elseif(_framework_command)
        list(APPEND ASS_LDFLAGS "-framework ${flag}")
        set(_framework_command OFF)
      else()
        list(APPEND ASS_LDFLAGS ${flag})
      endif()
    endforeach()
    unset(_framework_command)
  else()
    set(ASS_LDFLAGS ${PC_ASS_LDFLAGS})
  endif()
endif()

find_path(ASS_INCLUDE_DIR NAMES ass/ass.h
                          PATHS ${PC_ASS_INCLUDEDIR})
find_library(ASS_LIBRARY NAMES ass libass
                         PATHS ${PC_ASS_LIBDIR})

set(ASS_VERSION ${PC_ASS_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ASS
                                  REQUIRED_VARS ASS_LIBRARY ASS_INCLUDE_DIR
                                  VERSION_VAR ASS_VERSION)

if(ASS_FOUND)
  set(ASS_LIBRARIES ${ASS_LIBRARY} ${ASS_LDFLAGS})
  set(ASS_INCLUDE_DIRS ${ASS_INCLUDE_DIR})

  if(NOT TARGET ASS::ASS)
    add_library(ASS::ASS UNKNOWN IMPORTED)
    set_target_properties(ASS::ASS PROPERTIES
                                   IMPORTED_LOCATION "${ASS_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${ASS_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(ASS_INCLUDE_DIR ASS_LIBRARY)
