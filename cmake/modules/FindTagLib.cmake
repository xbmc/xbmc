#.rst:
# FindTagLib
# ----------
# Finds the TagLib library
#
# This will will define the following variables::
#
# TAGLIB_FOUND - system has TagLib
# TAGLIB_INCLUDE_DIRS - the TagLib include directory
# TAGLIB_LIBRARIES - the TagLib libraries
#
# and the following imported targets::
#
#   TagLib::TagLib   - The TagLib library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_TAGLIB taglib>=1.9.0 QUIET)
endif()

find_path(TAGLIB_INCLUDE_DIR taglib/tag.h
                             PATHS ${PC_TAGLIB_INCLUDEDIR})
find_library(TAGLIB_LIBRARY_RELEASE NAMES tag
                                    PATHS ${PC_TAGLIB_LIBDIR})
find_library(TAGLIB_LIBRARY_DEBUG NAMES tagd
                                  PATHS ${PC_TAGLIB_LIBDIR})
set(TAGLIB_VERSION ${PC_TAGLIB_VERSION})

include(SelectLibraryConfigurations)
select_library_configurations(TAGLIB)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TagLib
                                  REQUIRED_VARS TAGLIB_LIBRARY TAGLIB_INCLUDE_DIR
                                  VERSION_VAR TAGLIB_VERSION)

if(TAGLIB_FOUND)
  set(TAGLIB_LIBRARIES ${TAGLIB_LIBRARY})

  # Workaround broken .pc file
  list(APPEND TAGLIB_LIBRARIES ${PC_TAGLIB_ZLIB_LIBRARIES})

  set(TAGLIB_INCLUDE_DIRS ${TAGLIB_INCLUDE_DIR})
  if(NOT TARGET TagLib::TagLib)
    add_library(TagLib::TagLib UNKNOWN IMPORTED)
    if(TAGLIB_LIBRARY_RELEASE)
      set_target_properties(TagLib::TagLib PROPERTIES
                                           IMPORTED_CONFIGURATIONS RELEASE
                                           IMPORTED_LOCATION "${TAGLIB_LIBRARY_RELEASE}")
    endif()
    if(TAGLIB_LIBRARY_DEBUG)
      set_target_properties(TagLib::TagLib PROPERTIES
                                           IMPORTED_CONFIGURATIONS DEBUG
                                           IMPORTED_LOCATION "${TAGLIB_LIBRARY_DEBUG}")
    endif()
    set_target_properties(TagLib::TagLib PROPERTIES
                                         INTERFACE_INCLUDE_DIRECTORIES "${TAGLIB_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(TAGLIB_INCLUDE_DIR TAGLIB_LIBRARY)
