#.rst:
# FindDetours
# --------
# Finds the Detours library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Detours - The Detours library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  find_path(DETOURS_INCLUDE_DIR NAMES detours.h)

  find_library(DETOURS_LIBRARY_RELEASE NAMES detours
                                       ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})
  find_library(DETOURS_LIBRARY_DEBUG NAMES detoursd
                                     ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

  include(SelectLibraryConfigurations)
  select_library_configurations(DETOURS)
  unset(DETOURS_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Detours
                                    REQUIRED_VARS DETOURS_LIBRARY DETOURS_INCLUDE_DIR)

  if(DETOURS_FOUND)
    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${DETOURS_INCLUDE_DIR}")
    if(DETOURS_LIBRARY_RELEASE)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_CONFIGURATIONS RELEASE
                                                                       IMPORTED_LOCATION_RELEASE "${DETOURS_LIBRARY_RELEASE}")
    endif()
    if(DETOURS_LIBRARY_DEBUG)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       IMPORTED_LOCATION_DEBUG "${DETOURS_LIBRARY_DEBUG}")
      set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                            IMPORTED_CONFIGURATIONS DEBUG)
    endif()
  else()
    if(Detours_FIND_REQUIRED)
      message(FATAL_ERROR "Detour libraries were not found.")
    endif()
  endif()
endif()
