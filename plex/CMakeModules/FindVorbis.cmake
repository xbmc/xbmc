# - Find vorbis
# Find the native vorbis includes and libraries
#
#  VORBIS_INCLUDE_DIR - where to find vorbis.h, etc.
#  VORBIS_LIBRARIES   - List of libraries when using vorbis(file).
#  VORBIS_FOUND       - True if vorbis found.

if(VORBIS_INCLUDE_DIR)
  # Already in cache, be silent
  set(VORBIS_FIND_QUIETLY TRUE)
endif(VORBIS_INCLUDE_DIR)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(_VORBIS vorbis)
  pkg_check_modules(_OGG ogg)
  pkg_check_modules(_VORBISENC vorbisenc)
endif (PKG_CONFIG_FOUND)

find_path(OGG_INCLUDE_DIR ogg/ogg.h HINTS ${_OGG_INCLUDEDIR})
find_path(VORBIS_INCLUDE_DIR vorbis/vorbisfile.h HINTS ${_VORBIS_INCLUDEDIR})
find_path(VORBISENC_INCLUDE_DIR vorbis/vorbisenc.h HINTS ${_VORBISENC_INCLUDEDIR})
# MSVC built ogg/vorbis may be named ogg_static and vorbis_static
find_library(OGG_LIBRARY NAMES ogg ogg_static HINTS ${_OGG_LIBDIR})
find_library(VORBIS_LIBRARY NAMES vorbis vorbis_static HINTS ${_VORBIS_LIBDIR})
find_library(VORBISENC_LIBRARY NAMES vorbisenc vorbisenc_static HINTS ${_VORBISENC_LIBDIR})
# Handle the QUIETLY and REQUIRED arguments and set VORBIS_FOUND
# to TRUE if all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VORBIS DEFAULT_MSG OGG_INCLUDE_DIR VORBIS_INCLUDE_DIR VORBISENC_INCLUDE_DIR OGG_LIBRARY VORBIS_LIBRARY VORBISENC_LIBRARY)
    
if(VORBIS_FOUND)
  set(VORBIS_LIBRARIES ${VORBISENC_LIBRARY} ${VORBIS_LIBRARY} ${OGG_LIBRARY})
  set(VORBIS_INCLUDE_DIRS ${VORBISENC_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR} ${OGG_INCLUDE_DIR})
  plex_get_soname(VORBIS_SONAME ${VORBIS_LIBRARY})
  plex_get_soname(VORBISENC_SONAME ${VORBISENC_LIBRARY})
  plex_get_soname(OGG_SONAME ${OGG_LIBRARY})
else(VORBIS_FOUND)
  set(VORBIS_LIBRARIES)
  set(VORBIS_INCLUDE_DIRS)
endif(VORBIS_FOUND)

mark_as_advanced(OGG_INCLUDE_DIR VORBIS_INCLUDE_DIR)
mark_as_advanced(OGG_LIBRARY VORBIS_LIBRARY VORBISENC_LIBRARY)

