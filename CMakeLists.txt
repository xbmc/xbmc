project("Plex")
cmake_minimum_required(VERSION 2.8.8)
include(ExternalProject)

set(root ${PROJECT_SOURCE_DIR})
set(dependdir ${root}/plex/Dependencies/laika-depends)
set(plexdir ${root}/plex)

# Set some CFlags that is needed before we run our config tests.
if(NOT DEFINED OSX_SDK)
     set(OSX_SDK /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk)
endif()

if(APPLE)
	set(EXTRA_CFLAGS "-arch i386 -mmacosx-version-min=10.6 -isysroot ${OSX_SDK}")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS}")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

include(CMakeConfig.txt)

# MUST BE ADDED FIRST :)
# This will download our dependency tree
add_subdirectory(plex/Dependencies)

# Now build auxilary xbmc libraries.
# Note that we want to do this before we add all
# our defines and includes, otherwise they will
# carry over
add_subdirectory(lib)

include_directories(
  ${CMAKE_BINARY_DIR}/xbmc
	${plexdir}
	${plexdir}/Utility
	${plexdir}/Owned

	${root}/xbmc
	${root}/xbmc/osx
	${root}/xbmc/utils
	${root}
	${root}/xbmc/linux
	${root}/xbmc/cores/dvdplayer
	${root}/xbmc/settings
	${root}/lib
	${CMAKE_BINARY_DIR}/lib/cpluff/cpluff-prefix/src/cpluff/libcpluff
  ${CMAKE_BINARY_DIR}/lib/ffmpeg/ffmpeg-prefix/src/ffmpeg

	${dependdir}/include
	${dependdir}/include/libcec
	${dependdir}/include/freetype2
	${dependdir}/include/python2.6
	${root}/tools/darwin/depends/libcrystalhd
)

add_definitions(
#	-DAPI_DEBUG
	-DNDEBUG=1
	-DTARGET_POSIX
	-DTARGET_DARWIN
	-DTARGET_DARWIN_OSX
	-D_LINUX
	-D_REENTRANT
	-D_FILE_DEFINED
	-D__STDC_CONSTANT_MACROS
	-DHAVE_CONFIG_H
	-D_FILE_OFFSET_BITS=64
	-D_LARGEFILE64_SOURCE
	-D__PLEX__
  -DPLEX_TARGET_NAME="PlexAlpha"
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

# function to collect all the sources from sub-directories
# into a single list
function(add_sources)
  get_property(is_defined GLOBAL PROPERTY SRCS_LIST DEFINED)
  if(NOT is_defined)
    define_property(GLOBAL PROPERTY SRCS_LIST
      BRIEF_DOCS "List of source files"
      FULL_DOCS "List of source files to be compiled in one library")
  endif()
  # make absolute paths
  set(SRCS)
  foreach(s IN LISTS ARGN)
    if(NOT IS_ABSOLUTE "${s}")
      get_filename_component(s "${s}" ABSOLUTE)
    endif()
    list(APPEND SRCS "${s}")
  endforeach()
  # append to global list
  set_property(GLOBAL APPEND PROPERTY SRCS_LIST "${SRCS}")
endfunction(add_sources)

add_subdirectory(xbmc)
add_subdirectory(plex)
