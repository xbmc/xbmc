#.rst:
# FindDetours
# --------
# Finds the Detours library
#
# This will define the following target:
#
#   windows::Detours - The Detours library

if(NOT TARGET windows::Detours)
  find_path(DETOURS_INCLUDE_DIR NAMES detours.h
                                NO_CACHE)

  find_library(DETOURS_LIBRARY_RELEASE NAMES detours
                                       NO_CACHE)
  find_library(DETOURS_LIBRARY_DEBUG NAMES detoursd
                                     NO_CACHE)

  include(SelectLibraryConfigurations)
  select_library_configurations(DETOURS)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Detours
                                    REQUIRED_VARS DETOURS_LIBRARY DETOURS_INCLUDE_DIR)

  if(DETOURS_FOUND)
    add_library(windows::Detours UNKNOWN IMPORTED)
    set_target_properties(windows::Detours PROPERTIES
                                           INTERFACE_INCLUDE_DIRECTORIES "${DETOURS_INCLUDE_DIR}"
                                           IMPORTED_LOCATION "${DETOURS_LIBRARY_RELEASE}")
    if(DETOURS_LIBRARY_DEBUG)
      set_target_properties(windows::Detours PROPERTIES
                                             IMPORTED_LOCATION_DEBUG "${DETOURS_LIBRARY_DEBUG}")
    endif()
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP windows::Detours)
  endif()
endif()
