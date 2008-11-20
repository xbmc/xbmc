CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
##
# DEBUG
##
SET(CMAKE_VERBOSE_MAKEFILE ON)

IF(APPLE)
  SET(CMAKE_OSX_SYSROOT /Developer/SDKs/MacOSX10.4u.sdk)
ENDIF(APPLE)

##
# Things that change "often" (sources and dirs)
##
SET(DIRS guilib)

##
# Set up include directories
##
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR} xbmc guilib)

# Set platform specfic include directories
# Note: We check for WIN32 rather than UNIX because both UNIX and WIN32 are
# TRUE for CYGWIN
IF(WIN32)
  INCLUDE_DIRECTORIES(xbmc/win32)
  SET(_WIN32 1)
ELSE(WIN32)
  INCLUDE_DIRECTORIES(xbmc/linux)
  SET(_LINUX 1)
  IF(APPLE)
    INCLUDE_DIRECTORIES(xbmc/osx)
  ENDIF(APPLE)
ENDIF(WIN32)

# Find required libraries
FIND_PACKAGE(SDL REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM ${SDL_INCLUDE_DIR})

IF(APPLE)
  INCLUDE_DIRECTORIES(SYSTEM /opt/local/include/freetype2 /opt/local/include)
ENDIF(APPLE)


FOREACH(dir ${DIRS})
  ADD_SUBDIRECTORY(${dir})
ENDFOREACH(dir)

CONFIGURE_FILE(config.h.cmake ${CMAKE_BINARY_DIR}/config.h)

