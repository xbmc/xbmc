project(minizip)

cmake_minimum_required(VERSION 2.6)

if (UNIX)
	find_package(ZLIB REQUIRED)
endif()

set (SOURCES
 ioapi.c
 unzip.c
 zip.c
)

set (HEADERS
 ioapi.h
 unzip.h
 zip.h
)

if (APPLE)
	# Mac OS X does not have fopen64()
	# and several related functions as the standard fopen()
	# calls are 64bit
	add_definitions("-DUSE_FILE32API")
endif()

add_definitions("-DHAVE_BZIP2")

add_library(minizip
 ${SOURCES}
 ${HEADERS}
)

if (UNIX)
	target_link_libraries(minizip z bz2)
else()
	target_link_libraries(minizip
		"${CMAKE_CURRENT_SOURCE_DIR}/../zlib/prebuilt/zlib.lib"
		"${CMAKE_CURRENT_SOURCE_DIR}/../bzip2/libbz2.lib"
	)
endif()


