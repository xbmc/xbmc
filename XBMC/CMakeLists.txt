CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(XBMC)

##
# DEBUG
##
SET(CMAKE_VERBOSE_MAKEFILE ON)

##
# Set up include directories
##
INCLUDE_DIRECTORIES(guilib xbmc)

# Set platform specfic include directories
# Note: We check for WIN32 rather than UNIX because both UNIX and WIN32 are
# TRUE for CYGWIN
IF(WIN32)
  INCLUDE_DIRECTORIES(xbmc/win32)
ELSE(WIN32)
  INCLUDE_DIRECTORIES(xbmc/linux)
  ADD_DEFINITIONS(-D_FILE_DEFINED)
  IF(APPLE)
    ADD_DEFINITIONS(-D_LINUX)
    INCLUDE_DIRECTORIES(xbmc/osx)
  ENDIF(APPLE)
ENDIF(WIN32)

# Find required libraries
FIND_PACKAGE(SDL REQUIRED)
INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})

IF(APPLE)
  INCLUDE_DIRECTORIES(/opt/local/include/freetype2 /opt/local/include)
ENDIF(APPLE)

# Be sure to add guilib before xbmc
ADD_SUBDIRECTORY(guilib)
ADD_SUBDIRECTORY(xbmc)

