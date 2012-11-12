project("Plex")
cmake_minimum_required(VERSION 2.8.8)
include(ExternalProject)

execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_VARIABLE PLEX_VERSION_REVISION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(PLEX_VERSION_MAJOR 0)
set(PLEX_VERSION_MINOR 9)
set(PLEX_VERSION_PATCH 9)
if(DEFINED ENV{BUILD_NUMBER})
  set(PLEX_VERSION_BUILD $ENV{BUILD_NUMBER})
else()
  set(PLEX_VERSION_BUILD 0)
endif()
set(PLEX_VERSION_STRING ${PLEX_VERSION_MAJOR}.${PLEX_VERSION_MINOR}.${PLEX_VERSION_PATCH}.${PLEX_VERSION_BUILD}-${PLEX_VERSION_REVISION})
set(PLEX_VERSION_STRING_SHORT ${PLEX_VERSION_MAJOR}.${PLEX_VERSION_MINOR}.${PLEX_VERSION_PATCH})
set(XBMC_VERSION "Frodo")

set(root ${PROJECT_SOURCE_DIR})
if(NOT WIN32)
  set(dependdir ${root}/plex/Dependencies/laika-depends)
else()
  set(dependdir ${root}/project/BuildDependencies)
endif()
set(plexdir ${root}/plex)
message("-- Using dependency dir ${dependdir}")

include(CMakeFunctions.txt)

if(DEFINED XCODE_VERSION)
  message("Building with XCode Generator")
  set(USING_XCODE 1)
endif()

# Set some CFlags that is needed before we run our config tests.
if(NOT DEFINED OSX_SDK)
     set(OSX_SDK /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk)
endif()

if(APPLE)
	set(EXTRA_CFLAGS "-arch i386 -mmacosx-version-min=10.6 -isysroot ${OSX_SDK}")
elseif(UNIX)
    set(EXTRA_CFLAGS "-fPIC -DPIC")
endif()

if(${CMAKE_C_COMPILER} MATCHES "clang")
  if(NOT DEFINED ${PLEX_DISABLE_COLORS})
    set(EXTRA_CFLAGS "${EXTRA_CFLAGS} -fcolor-diagnostics")
  endif()
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS}")
if(UNIX)
  set(CMAKE_C_FLAGS_DEBUG "-g -O0")
  set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
endif()

# global definitions
add_definitions(-DNDEBUG=1 -D__PLEX__ -DPLEX_TARGET_NAME="PlexAlpha")

# Linux and OSX definitios
if(UNIX)
  add_definitions(
    #	-DAPI_DEBUG
    -DNDEBUG=1
    -DTARGET_POSIX
    -D_REENTRANT
    -D_FILE_DEFINED
    -D__STDC_CONSTANT_MACROS
    -DHAVE_CONFIG_H
    -D_FILE_OFFSET_BITS=64
    -D_LARGEFILE64_SOURCE
    -DUSE_EXTERNAL_FFMPEG
    -D_LINUX
  )
  if(APPLE)
    add_definitions(-DTARGET_DARWIN -DTARGET_DARWIN_OSX)
  else(APPLE)
    add_definitions(-DTARGET_LINUX -D_LINUX)
  endif(APPLE)
endif(UNIX)

# windows specific
if(WIN32)
  add_definitions(
    -DNDEBUG=1
    -DTARGET_WINDOWS
    -D_WINDOWS
    -D_MSVC
    -DWIN32
    -D_WIN32_WINNT=0x0501
    -DNTDDI_VERSION=0x05010300
    -DNOMINMAX
    -D_USE_32BIT_TIME_T
    -DHAS_DX
    -DD3D_DEBUG_INFO
    -D__STDC_CONSTANT_MACROS
    -D_SECURE_SCL=0
    -D_HAS_ITERATOR_DEBUGGING=0
    -D_CRT_SECURE_NO_WARNINGS
  )
endif()

# We need to add these before we compile the dependencies,
# since CxImage depends on libjpeg and friends
# But we don't add our own include directories to avoid
# header confusion when building the dependencies.
include_directories(
  ${dependdir}/include
  ${dependdir}/include/libcec
  ${dependdir}/include/freetype2
  ${dependdir}/include/python2.6
)

# MUST BE ADDED FIRST :)
# This will download our dependency tree
if(NOT WIN32)
  add_subdirectory(plex/Dependencies)
endif()

# Now check for available functions
include(CMakeConfig.txt)

# Now build auxilary xbmc libraries.
# Note that we want to do this before we add all
# our defines and includes, otherwise they will
# carry over
if(NOT WIN32)
  add_subdirectory(lib)
endif()

# Platfrom includes
if(UNIX)
  include_directories(${root}/xbmc/linux)
endif(UNIX)

if(APPLE)
  include_directories(${root}/xbmc/osx)
endif(APPLE)

if(WIN32)
  include_directories(${root}/xbmc/win32)
  include_directories(
    ${root}/lib/ffmpeg
    ${root}/lib/ffmpeg/include-xbmc-win32
    ${root}/lib/liblame/include
    ${root}/lib/win32/pcre
    ${root}/lib/win32
  )
endif()

# Build the texturepacker
add_subdirectory(tools/TexturePacker)

# global includes
include_directories(
  ${CMAKE_BINARY_DIR}/xbmc
	${plexdir}
	${plexdir}/Utility
	${plexdir}/Owned

	${root}/xbmc
	${root}/xbmc/utils
	${root}
	${root}/xbmc/cores/dvdplayer
	${root}/xbmc/settings
	${root}/lib
	${CMAKE_BINARY_DIR}/lib/cpluff/cpluff-prefix/src/cpluff/libcpluff

	${root}/tools/darwin/depends/libcrystalhd
	${root}/xbmc/cores/AudioEngine
)


#### Generate git_revision.h
add_custom_target(git_revision.h ALL DEPENDS ${root}/git_revision.h)
add_custom_command(
  OUTPUT ${root}/git_revision.h
  COMMAND ${plexdir}/scripts/generate_git.sh
  DEPENDS ${plexdir}/scripts/generate_git.sh
  WORKING_DIRECTORY ${root}
)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/git_revision.h PROPERTIES GENERATED TRUE HEADER_FILE_ONLY TRUE)


add_subdirectory(xbmc)
if(APPLE)
  add_subdirectory(tools/EventClients/Clients/OSXRemote)
endif()
add_subdirectory(plex)


set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Plex Media Center")
set(CPACK_PACKAGE_VENDOR "Plex inc")
set(CPACK_PACKAGE_VERSION_MAJOR ${PLEX_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PLEX_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PLEX_VERSION_PATCH})
set(CPACK_MONOLITHIC_INSTALL "1")
set(CPACK_PACKAGE_FILE_NAME "Plex-${XBMC_VERSION}-${PLEX_VERSION_STRING}")
if(APPLE)
  set(CPACK_GENERATOR "DragNDrop")
else()
  set(CPACK_GENERATOR "TGZ")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CPACK_STRIP_FILES 0)
endif()

include(CPack)
