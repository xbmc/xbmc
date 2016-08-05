#.rst:
# FindLzo2
# --------
# Finds the Lzo2 library
#
# This will will define the following variables::
#
# LZO2_FOUND - system has Lzo2
# LZO2_INCLUDE_DIRS - the Lzo2 include directory
# LZO2_LIBRARIES - the Lzo2 libraries
#
# and the following imported targets::
#
#   Lzo2::Lzo2   - The Lzo2 library

find_path(LZO2_INCLUDE_DIR NAMES lzo1x.h
                           PATH_SUFFIXES lzo)

find_library(LZO2_LIBRARY NAMES lzo2 liblzo2)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Lzo2
                                  REQUIRED_VARS LZO2_LIBRARY LZO2_INCLUDE_DIR)

if(LZO2_FOUND)
  set(LZO2_LIBRARIES ${LZO2_LIBRARY})
  set(LZO2_INCLUDE_DIRS ${LZO2_INCLUDE_DIR})

  if(NOT TARGET Lzo2::Lzo2)
    add_library(Lzo2::Lzo2 UNKNOWN IMPORTED)
    set_target_properties(Lzo2::Lzo2 PROPERTIES
                                     IMPORTED_LOCATION "${LZO2_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${LZO2_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(LZO2_INCLUDE_DIR LZO2_LIBRARY)
