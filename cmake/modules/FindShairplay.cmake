#.rst:
# FindShairplay
# -------------
# Finds the Shairplay library
#
# This will will define the following variables::
#
# SHAIRPLAY_FOUND - system has Shairplay
# SHAIRPLAY_INCLUDE_DIRS - the Shairplay include directory
# SHAIRPLAY_LIBRARIES - the Shairplay libraries
# SHAIRPLAY_DEFINITIONS - the Shairplay compile definitions
#
# and the following imported targets::
#
#   Shairplay::Shairplay   - The Shairplay library

find_path(SHAIRPLAY_INCLUDE_DIR shairplay/raop.h)

include(FindPackageHandleStandardArgs)
if(NOT WIN32)
  find_library(SHAIRPLAY_LIBRARY NAMES shairplay)

  if(SHAIRPLAY_INCLUDE_DIR AND SHAIRPLAY_LIBRARY)
    include(CheckCSourceCompiles)
    set(CMAKE_REQUIRED_INCLUDES ${SHAIRPLAY_INCLUDE_DIRS})
    set(CMAKE_REQUIRED_LIBRARIES ${SHAIRPLAY_LIBRARIES})
    check_c_source_compiles("#include <shairplay/raop.h>

                             int main()
                             {
                               struct raop_callbacks_s foo;
                               foo.cls;
                               return 0;
                             }
                            " HAVE_SHAIRPLAY_CALLBACK_CLS)
  endif()

  find_package_handle_standard_args(Shairplay
                                    REQUIRED_VARS SHAIRPLAY_LIBRARY SHAIRPLAY_INCLUDE_DIR HAVE_SHAIRPLAY_CALLBACK_CLS)
else()
  # Dynamically loaded DLL
  find_package_handle_standard_args(Shairplay
                                    REQUIRED_VARS SHAIRPLAY_INCLUDE_DIR)
endif()

if(SHAIRPLAY_FOUND)
  set(SHAIRPLAY_LIBRARIES ${SHAIRPLAY_LIBRARY})
  set(SHAIRPLAY_INCLUDE_DIRS ${SHAIRPLAY_INCLUDE_DIR})
  set(SHAIRPLAY_DEFINITIONS -DHAVE_LIBSHAIRPLAY=1)

  if(NOT TARGET Shairplay::Shairplay)
    add_library(Shairplay::Shairplay UNKNOWN IMPORTED)
    if(SHAIRPLAY_LIBRARY)
      set_target_properties(Shairplay::Shairplay PROPERTIES
                                                 IMPORTED_LOCATION "${SHAIRPLAY_LIBRARY}")
    endif()
    set_target_properties(Shairplay::Shairplay PROPERTIES
                                               INTERFACE_INCLUDE_DIRECTORIES "${SHAIRPLAY_INCLUDE_DIR}"
                                               INTERFACE_COMPILE_DEFINITIONS HAVE_LIBSHAIRPLAY=1)
  endif()
endif()

mark_as_advanced(SHAIRPLAY_INCLUDE_DIR SHAIRPLAY_LIBRARY)
