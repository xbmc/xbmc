#.rst:
# FindTagLib
# ----------
# Finds the TagLib library
#
# This will define the following variables::
#
# TAGLIB_FOUND - system has TagLib
# TAGLIB_INCLUDE_DIRS - the TagLib include directory
# TAGLIB_LIBRARIES - the TagLib libraries
#
# and the following imported targets::
#
#   TagLib::TagLib   - The TagLib library

if(ENABLE_INTERNAL_TAGLIB)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC taglib)

  SETUP_BUILD_VARS()

  set(TAGLIB_VERSION ${${MODULE}_VER})

  if(WIN32 OR WINDOWS_STORE)
    set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/001-cmake-pdb-debug.patch")
    generate_patchcommand("${patches}")
  endif()

  # Debug postfix only used for windows
  if(WIN32 OR WINDOWS_STORE)
    set(TAGLIB_DEBUG_POSTFIX "d")
  endif()

  set(CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
                 -DBUILD_BINDINGS=OFF)

  BUILD_DEP_TARGET()

else()

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

endif()

include(SelectLibraryConfigurations)
select_library_configurations(TAGLIB)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TagLib
                                  REQUIRED_VARS TAGLIB_LIBRARY TAGLIB_INCLUDE_DIR
                                  VERSION_VAR TAGLIB_VERSION)

if(TAGLIB_FOUND)
  set(TAGLIB_INCLUDE_DIRS ${TAGLIB_INCLUDE_DIR})
  set(TAGLIB_LIBRARIES ${TAGLIB_LIBRARY})

  # Workaround broken .pc file
  list(APPEND TAGLIB_LIBRARIES ${PC_TAGLIB_ZLIB_LIBRARIES})

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
    if(TARGET taglib)
      add_dependencies(TagLib::TagLib taglib)
    endif()
  endif()
  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP TagLib::TagLib)
endif()

mark_as_advanced(TAGLIB_INCLUDE_DIR TAGLIB_LIBRARY)
