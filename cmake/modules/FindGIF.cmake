#.rst:
# FindGIF
# -------
# Finds the libgif library
#
# This will will define the following variables::
#
# GIF_FOUND - system has libgif
# GIF_INCLUDE_DIRS - the libgif include directory
# GIF_LIBRARIES - the libgif libraries
#
# and the following imported targets::
#
#   GIF::GIF   - The libgif library

find_path(GIF_INCLUDE_DIR gif_lib.h)

include(FindPackageHandleStandardArgs)
if(NOT WIN32)
  find_library(GIF_LIBRARY NAMES gif)
  find_package_handle_standard_args(GIF
                                    REQUIRED_VARS GIF_LIBRARY GIF_INCLUDE_DIR)
else()
  # Dynamically loaded DLL
  find_package_handle_standard_args(GIF
                                    REQUIRED_VARS GIF_INCLUDE_DIR)
endif()

if(GIF_FOUND)
  set(GIF_LIBRARIES ${GIF_LIBRARY})
  set(GIF_INCLUDE_DIRS ${GIF_INCLUDE_DIR})
  set(GIF_DEFINITIONS -DHAVE_LIBGIF=1)

  if(NOT TARGET GIF::GIF)
    add_library(GIF::GIF UNKNOWN IMPORTED)
    if(GIF_LIBRARY)
      set_target_properties(GIF::GIF PROPERTIES
                                     IMPORTED_LOCATION "${GIF_LIBRARY}")
    endif()
    set_target_properties(GIF::GIF PROPERTIES
                                   INTERFACE_INCLUDE_DIRECTORIES "${GIF_INCLUDE_DIR}"
                                   INTERFACE_COMPILE_DEFINITIONS HAVE_LIBGIF=1)
  endif()
endif()

mark_as_advanced(GIF_INCLUDE_DIR GIF_LIBRARY)
