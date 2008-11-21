CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(XBMC)

##
# DEBUG
##
SET(CMAKE_VERBOSE_MAKEFILE ON)

##
# Things that change "often" (sources and dirs)
##
SET(DIRS guilib xbmc)

##
# Set up include directories
##
INCLUDE_DIRECTORIES(xbmc guilib)

# Set platform specfic include directories
# Note: We check for WIN32 rather than UNIX because both UNIX and WIN32 are
# TRUE for CYGWIN
IF(WIN32)
  INCLUDE_DIRECTORIES(xbmc/win32)
ELSE(WIN32)
  INCLUDE_DIRECTORIES(xbmc/linux)
  IF(APPLE)
    ADD_DEFINITIONS(-D_LINUX)
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

ADD_EXECUTABLE(xbmc ${BIN_SRCS})

