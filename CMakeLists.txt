project("Plex")
cmake_minimum_required(VERSION 2.8.8)
include(ExternalProject)

set(root ${PROJECT_SOURCE_DIR})
set(dependdir ${root}/plex/Dependencies/laika-depends)
set(plexdir ${root}/plex)

include_directories(
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

#### Frameworks for MacOSX
if(APPLE)
	set(osx_frameworks
		AudioToolbox
		AudioUnit
		Cocoa
		CoreAudio
		CoreServices
		Foundation
		OpenGL
		AppKit
		ApplicationServices
		IOKit
		QuickTime
		Carbon
		DiskArbitration
		QuartzCore
		SystemConfiguration
	)
endif()

#### libraries we want to link to
set(external_libs 
	ssh
	lzo2
	pcre
	pcrecpp
	fribidi
	cdio
	freetype
	fontconfig
	sqlite3
	samplerate
	microhttpd
	yajl
  jpeg
	crypto
	GLEW
	SDL
	SDL_mixer
	tinyxml
	boost_thread
	boost_system
	z
)

set(find_libs
	rtmp
	plist
	shairport
	cec
	curl
	flac
	modplug
	vorbis
	vorbisfile
	vorbisenc
	ogg
	ass
	mad
	mpeg2
	bluray
  png
  tiff
)

set(system_libs
	stdc++
  iconv
#	png
)

foreach(lib ${external_libs} ${find_libs})
	find_library(${lib}_LIBRARY ${lib} ${dependdir}/lib NO_DEFAULT_PATH)
	get_property(l VARIABLE PROPERTY ${lib}_LIBRARY)
	get_filename_component(${lib}_FNAME ${l} NAME)
endforeach()

foreach(lib ${system_libs})
	find_library(${lib}_LIBRARY ${lib})
endforeach()


#### Generate git_revision.h
add_custom_target(git_revision.h ALL DEPENDS ${root}/git_revision.h)
add_custom_command(
  OUTPUT ${root}/git_revision.h
  COMMAND ${plexdir}/scripts/generate_git.sh
  DEPENDS ${plexdir}/scripts/generate_git.sh
  WORKING_DIRECTORY ${root}
)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/git_revision.h PROPERTIES GENERATED TRUE HEADER_FILE_ONLY TRUE)

#### Deal with some generated files
if(APPLE)
	set(ARCH "x86-osx")
endif()

set(RTMP_SONAME ${rtmp_FNAME})
set(PLIST_SONAME ${plist_FNAME})
set(SHAIRPORT_SONAME ${shairport_FNAME})
set(LIBCEC_SONAME ${cec_FNAME})
set(CURL_SONAME ${curl_FNAME})
set(FLAC_SONAME ${flac_FNAME})
set(MODPLUG_SONAME ${modplug_FNAME})
set(VORBISFILE_SONAME ${vorbisfile_FNAME})
set(ASS_SONAME ${ass_FNAME})
set(MAD_SONAME ${mad_FNAME})
set(MPEG2_SONAME ${mpeg2_FNAME})
set(BLURAY_SONAME ${bluray_FNAME})
set(VORBIS_SONAME ${vorbis_FNAME})
set(VORBISENC_SONAME ${vorbisenc_FNAME})
set(OGG_SONAME ${ogg_FNAME})

configure_file(${root}/xbmc/DllPaths_generated.h.in ${root}/xbmc/DllPaths_generated.h)

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

set(internal_libs
	lib_squish
	lib_apetag
	lib_hts
)

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

# MUST BE ADDED FIRST :)
add_subdirectory(plex/Dependencies)

add_subdirectory(lib)
add_subdirectory(xbmc)

add_custom_target(internal_lib DEPENDS ${internal_libs})
add_custom_target(xbmc_libs DEPENDS ${xbmc_libs} internal_libs wraplibs)

add_subdirectory(plex)
