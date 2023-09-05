#.rst:
# FindShairplay
# -------------
# Finds the Shairplay library
#
# This will define the following target:
#
#   Shairplay::Shairplay - The Shairplay library

if(NOT TARGET Shairplay::Shairplay)

  find_path(SHAIRPLAY_INCLUDE_DIR shairplay/raop.h NO_CACHE)

  include(FindPackageHandleStandardArgs)
  find_library(SHAIRPLAY_LIBRARY NAMES shairplay libshairplay
                                 NO_CACHE)

  if(SHAIRPLAY_INCLUDE_DIR AND SHAIRPLAY_LIBRARY)
    include(CheckCSourceCompiles)
    set(CMAKE_REQUIRED_INCLUDES ${SHAIRPLAY_INCLUDE_DIR})
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

  if(SHAIRPLAY_FOUND)
    add_library(Shairplay::Shairplay UNKNOWN IMPORTED)
    set_target_properties(Shairplay::Shairplay PROPERTIES
                                               IMPORTED_LOCATION "${SHAIRPLAY_LIBRARY}"
                                               INTERFACE_INCLUDE_DIRECTORIES "${SHAIRPLAY_INCLUDE_DIR}"
                                               INTERFACE_COMPILE_DEFINITIONS HAS_AIRTUNES=1)
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Shairplay::Shairplay)
  endif()
endif()
