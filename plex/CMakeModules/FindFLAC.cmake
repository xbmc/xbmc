# Finds FLAC library
#
#  FLAC_INCLUDE_DIR - where to find flac.h, etc.
#  FLAC_LIBRARIES   - List of libraries when using FLAC.
#  FLAC_FOUND       - True if FLAC found.
#
# Adapted from http://whispercast.org/trac/browser/trunk/cmake/FindFLAC.cmake
# Edited by Etienne Perot to add FLAC++ library.


if (FLAC_INCLUDE_DIR)
	# Already in cache, be silent
	set(FLAC_FIND_QUIETLY TRUE)
endif (FLAC_INCLUDE_DIR)

find_path(FLAC_INCLUDE_DIR FLAC/all.h
	/opt/local/include
	/usr/local/include
	/usr/include
)

set(FLAC_NAMES FLAC)
find_library(FLAC_LIBRARY
	NAMES ${FLAC_NAMES}
	PATHS /usr/lib /usr/local/lib /opt/local/lib
)

find_path(FLACPP_INCLUDE_DIR FLAC++/all.h
	/opt/local/include
	/usr/local/include
	/usr/include
)

set(FLACPP_NAMES FLAC++)
find_library(FLACPP_LIBRARY
	NAMES ${FLACPP_NAMES}
	PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (FLAC_INCLUDE_DIR AND FLAC_LIBRARY AND FLACPP_INCLUDE_DIR AND FLACPP_LIBRARY)
	set(FLAC_FOUND TRUE)
	set(FLAC_LIBRARIES ${FLAC_LIBRARY} ${FLACPP_LIBRARY})
else (FLAC_INCLUDE_DIR AND FLAC_LIBRARY AND FLACPP_INCLUDE_DIR AND FLACPP_LIBRARY)
	set(FLAC_FOUND FALSE)
	set(FLAC_LIBRARIES)
endif (FLAC_INCLUDE_DIR AND FLAC_LIBRARY AND FLACPP_INCLUDE_DIR AND FLACPP_LIBRARY)

if (FLAC_FOUND)
	if (NOT FLAC_FIND_QUIETLY)
		message(STATUS "Found FLAC: ${FLAC_LIBRARY}")
	endif (NOT FLAC_FIND_QUIETLY)
else (FLAC_FOUND)
	if (FLAC_FIND_REQUIRED)
		message(STATUS "Looked for FLAC libraries named ${FLAC_NAMES}.")
		message(STATUS "Include file detected: [${FLAC_INCLUDE_DIR}].")
		message(STATUS "Lib file detected: [${FLAC_LIBRARY}].")
		message(FATAL_ERROR "=========> Could NOT find FLAC library")
	endif (FLAC_FIND_REQUIRED)
endif (FLAC_FOUND)

mark_as_advanced(
	FLAC_LIBRARY
	FLAC_INCLUDE_DIR
	FLACPP_LIBRARY
	FLACPP_INCLUDE_DIR
)

if(FLACPP_LIBRARY AND FLACPP_INCLUDE_DIR)
  plex_get_soname(FLACPP_SONAME ${FLACPP_LIBRARY})
endif()

if(FLAC_LIBRARY AND FLAC_INCLUDE_DIR)
  plex_get_soname(FLAC_SONAME ${FLAC_LIBRARY})
endif()
