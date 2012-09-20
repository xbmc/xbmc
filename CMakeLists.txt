project("Plex")
cmake_minimum_required(VERSION 2.8)

set(root ${PROJECT_SOURCE_DIR})
set(dependdir ${root}/plex/Dependencies/7deb2e8-darwin-i686)
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
	${root}/lib/ffmpeg

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
	z
#	bz2
	xml2
	iconv
#	ncurses
	freetype
	SDL
	SDL_mixer
	SDL_image
	microhttpd
	tinyXML
  boost_thread
  boost_date_time
  boost_system
	GLEW
	sqlite3
	yajl
	ssh
	ssh2
	cdio
	pcre
	pcrecpp
	jpeg
	lzo2
	samplerate
	fribidi
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
)
set(system_libs
	stdc++
)

foreach(lib ${external_libs})
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

set(EXTRA_CFLAGS "-arch i386 -mmacosx-version-min=10.6 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS}")

add_subdirectory(lib)
add_subdirectory(xbmc)
add_subdirectory(plex)
