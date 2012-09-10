project("Plex")
cmake_minimum_required(VERSION 2.8)

set(root ${PROJECT_SOURCE_DIR})
set(dependdir ${root}/vendor/osx-10.7_i386)
set(plexdir ${root}/plex)

include_directories(
	${plexdir}
	${plexdir}/Utility
	${plexdir}/Owned

	${root}/xbmc/osx
	${root}/xbmc/utils
	${root}/xbmc ${root}
	${root}/xbmc/linux
	${root}/xbmc/cores/dvdplayer
	${root}/xbmc/settings
	${root}/lib
	${root}/lib/ffmpeg

	${dependdir}/include
	${dependdir}/include/libcec
	${dependdir}/include/freetype2
	${dependdir}/include/python2.6
)
add_definitions(
#	-DAPI_DEBUG
	-DNDEBUG=1
	-DTARGET_POSIX
	-DTARGET_DARWIN
	-DTARGET_DARWIN_OSX
	-D_LINUX
	-D_FILE_DEFINED
	-D__STDC_CONSTANT_MACROS
	-DHAVE_CONFIG_H
	-D__PLEX__
)

enable_language(ASM_NASM)
set(CMAKE_ASM_NASM_COMPILER "yasm")
set(CMAKE_ASM_NASM_OBJECT_FORMAT "macho")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch i386")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch i386")

add_subdirectory(xbmc)
add_subdirectory(plex)
#add_subdirectory(lib)
