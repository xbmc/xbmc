project("Plex Home Theater")
cmake_minimum_required(VERSION 2.8.7 FATAL_ERROR)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

set(root ${PROJECT_SOURCE_DIR})
set(plexdir ${root}/plex)
set(CMAKE_MODULE_PATH ${plexdir}/CMakeModules ${CMAKE_MODULE_PATH})

include(GetGitRevisionDescription)
include(GetDate)
today(PLEX_CURRENT_DATE)
get_git_head_revision(REFSPEC PLEX_FULL_GIT_REVISION)
if(PLEX_FULL_GIT_REVISION STREQUAL "GITDIR-NOTFOUND")
  if(EXISTS ${root}/GitRevision.txt)
    file(STRINGS "${root}/GitRevision.txt" PLEX_GIT_REVISION)
  else()
    set(PLEX_GIT_REVISION "git")
  endif()
else()
  string(SUBSTRING ${PLEX_FULL_GIT_REVISION} 0 8 PLEX_GIT_REVISION)
endif()
configure_file(${plexdir}/git_revision.h.in git_revision.h)

set(PLEX_VERSION_MAJOR 0)
set(PLEX_VERSION_MINOR 9)
set(PLEX_VERSION_PATCH 9)
set(PLEX_VERSION_SMALL 6)
if(DEFINED ENV{BUILD_NUMBER})
  set(PLEX_VERSION_BUILD ".$ENV{BUILD_NUMBER}")
else()
  set(PLEX_VERSION_BUILD "")
endif()
set(PLEX_VERSION_STRING ${PLEX_VERSION_MAJOR}.${PLEX_VERSION_MINOR}.${PLEX_VERSION_PATCH}.${PLEX_VERSION_SMALL}${PLEX_VERSION_BUILD}-${PLEX_GIT_REVISION})
set(PLEX_VERSION_STRING_SHORT ${PLEX_VERSION_MAJOR}.${PLEX_VERSION_MINOR}.${PLEX_VERSION_PATCH}.${PLEX_VERSION_SMALL})
set(PLEX_VERSION_STRING_SHORT_BUILD ${PLEX_VERSION_STRING_SHORT}${PLEX_VERSION_BUILD})
set(XBMC_VERSION "Frodo")

message(STATUS "Building Plex Home Theater, version: ${PLEX_VERSION_STRING}")

include(CMakeFunctions)

# Now check for available functions
include(CMakeConfig)

# Now build auxilary xbmc libraries.
# Note that we want to do this before we add all
# our defines and includes, otherwise they will
# carry over
add_subdirectory(lib)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/xbmc

  ${dependdir}/include
  ${dependdir}/include/libcec
  ${dependdir}/include/freetype2
  ${dependdir}/include/python2.6
  ${dependdir}/include/python2.7
  ${ffmpegdir}/include
  ${root}/xbmc
)

# Platfrom includes
if(TARGET_POSIX)
  include_directories(${root}/xbmc/linux)
endif(TARGET_POSIX)

if(TARGET_COMMON_DARWIN)
  include_directories(${root}/xbmc/osx)
endif(TARGET_COMMON_DARWIN)

if(TARGET_WIN32)
  include_directories(
    ${root}/xbmc/win32
    ${root}/lib/ffmpeg
    ${root}/lib/ffmpeg/include-xbmc-win32
    ${root}/lib/liblame/include
    ${root}/lib/win32/pcre
    ${root}/lib/win32
    "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include"
  )
endif(TARGET_WIN32)

# Build the texturepacker
add_subdirectory(tools/TexturePacker)

# add our visualizations
add_subdirectory(xbmc/visualizations)

# global includes
include_directories(
  ${plexdir}
  ${plexdir}/Utility
  ${plexdir}/Owned

  ${root}/xbmc/utils
  ${root}
  ${root}/xbmc/cores/dvdplayer
  ${root}/xbmc/settings
  ${root}/lib
  ${CMAKE_BINARY_DIR}/lib/cpluff/cpluff-prefix/src/cpluff/libcpluff
  
  ${root}/tools/darwin/depends/libcrystalhd
  ${root}/xbmc/cores/AudioEngine
)

# UPnP includes
include_directories(
  ${root}/lib/libUPnP/Platinum/Source/Platinum
  ${root}/lib/libUPnP/Platinum/Source/Core
  ${root}/lib/libUPnP/Platinum/Source/Extras
  ${root}/lib/libUPnP/Platinum/Source/Devices/MediaConnect
  ${root}/lib/libUPnP/Platinum/Source/Devices/MediaRenderer
  ${root}/lib/libUPnP/Platinum/Source/Devices/MediaServer
  ${root}/lib/libUPnP/Neptune/Source/Core
  ${root}/lib/libUPnP/Neptune/Source/System/Posix
)

add_subdirectory(xbmc)
if(TARGET_OSX)
  add_subdirectory(tools/EventClients/Clients/OSXRemote)
endif()
add_subdirectory(plex)

if(XRANDR_FOUND)
  add_executable(xbmc-xrandr xbmc-xrandr.c)
  target_link_libraries(xbmc-xrandr ${CONFIG_LIBRARY_XRANDR} ${CONFIG_LIBRARY_X11} -lm)
  install(TARGETS xbmc-xrandr DESTINATION ${BINPATH})
endif()
  
include(CPackConfig)
