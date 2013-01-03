# - Find the native FriBiDI includes and library
#
#
# This module defines
#  FRIBIDI_INCLUDE_DIR, where to find fribidi.h, etc.
#  FRIBIDI_LIBRARIES, the libraries to link against to use FriBiDi.
#  PNG_DEFINITIONS - You should ADD_DEFINITONS(${PNG_DEFINITIONS}) before compiling code that includes png library files.
#  FRIBIDI_FOUND, If false, do not try to use PNG.
# also defined, but not for general use are
#  FRIBIDI_LIBRARY, where to find the FriBiDi library.

include(CheckSymbolExists)

SET(FRIBIDI_FOUND "NO")

# Set variable in temp var, otherwise FIND_PATH might fail
# unset isn't present in the required version of cmake.
FIND_PATH(xFRIBIDI_INCLUDE_DIR fribidi.h
  /usr/local/include/fribidi
  /usr/include/fribidi
  )
set(FRIBIDI_INCLUDE_DIR ${xFRIBIDI_INCLUDE_DIR})

SET(FRIBIDI_NAMES ${FRIBIDI_NAMES} fribidi libfribidi)
FIND_LIBRARY(FRIBIDI_LIBRARY
  NAMES ${FRIBIDI_NAMES}
  PATHS /usr/lib /usr/local/lib
  )

IF (FRIBIDI_LIBRARY AND FRIBIDI_INCLUDE_DIR)
  SET(CMAKE_REQUIRED_INCLUDES ${FRIBIDI_INCLUDE_DIR})
  SET(CMAKE_REQUIRED_LIBRARIES ${FRIBIDI_LIBRARY})
  CHECK_SYMBOL_EXISTS(fribidi_utf8_to_unicode fribidi.h FOUND_fribidi_utf8_to_unicode)
  CHECK_SYMBOL_EXISTS(fribidi_charset_to_unicode fribidi.h FOUND_fribidi_charset_to_unicode)
  if(FOUND_fribidi_charset_to_unicode)
    SET(FRIBIDI_LIBRARIES ${FRIBIDI_LIBRARY})
    SET(FRIBIDI_FOUND "YES")
  elseif(FOUND_fribidi_utf8_to_unicode)
    SET(FRIBIDI_LIBRARIES ${FRIBIDI_LIBRARY})
    SET(FRIBIDI_FOUND "YES")
    add_definitions(-DOLD_FRIBIDI)
    MESSAGE(STATUS "Legacy FriBiDi: ${FRIBIDI_LIBRARY}")
  else()
    SET(FRIBIDI_LIBRARIES "NOTFOUND")
    SET(FRIBIDI_INCLUDE_DIR "NOTFOUND")
    SET(FRIBIDI_FOUND "NO")
  endif()
ENDIF (FRIBIDI_LIBRARY AND FRIBIDI_INCLUDE_DIR)

IF (FRIBIDI_FOUND)

  IF (NOT FRIBIDI_FIND_QUIETLY)
    MESSAGE(STATUS "Using FriBiDi: ${FRIBIDI_LIBRARY}")
  ENDIF (NOT FRIBIDI_FIND_QUIETLY)
ELSE (FRIBIDI_FOUND)
  IF (FRIBIDI_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find FriBiDi library")
  ENDIF (FRIBIDI_FIND_REQUIRED)
ENDIF (FRIBIDI_FOUND)
