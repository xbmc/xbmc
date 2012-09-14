project("Plex")
cmake_minimum_required(VERSION 2.8)

set(root ${PROJECT_SOURCE_DIR})
set(dependdir ${root}/plex/Dependencies/7deb2e8-darwin-i686)
set(plexdir ${root}/plex)

include_directories(
#	${plexdir}
#	${plexdir}/Utility
#	${plexdir}/Owned

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

set(EXTRA_CFLAGS "-arch i386 -mmacosx-version-min=10.6 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS}")

add_subdirectory(lib)
add_subdirectory(xbmc)
#add_subdirectory(plex)
#add_subdirectory(lib)
