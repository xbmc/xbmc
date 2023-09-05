#.rst:
# FindGIF
# -------
# Finds the libgif library
#
# This will define the following target:
#
#   GIF::GIF   - The libgif library

if(NOT TARGET GIF::GIF)
  find_library(GIF_LIBRARY NAMES gif NO_CACHE)
  find_path(GIF_INCLUDE_DIR gif_lib.h NO_CACHE)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(GIF
                                    REQUIRED_VARS GIF_LIBRARY GIF_INCLUDE_DIR)

  if(GIF_FOUND)
    add_library(GIF::GIF UNKNOWN IMPORTED)
    set_target_properties(GIF::GIF PROPERTIES
                                   IMPORTED_LOCATION "${GIF_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${GIF_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS HAVE_LIBGIF=1)
  endif()
endif()
