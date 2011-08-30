project(updater)

cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

include_directories(external)
include_directories(external/TinyThread/source)

if (WIN32)
include_directories(external/zlib/)
include_directories(external/win32cpp/include)
endif()

if (APPLE)
	set(CMAKE_OSX_ARCHITECTURES i386;x86_64)

	# Build the updater so that it works on OS X 10.5 and above.
	# If you are building with XCode 4 or newer, you will need to up
	# this target - depending on the versions of the Mac OS X SDK that
	# are available to build with in /Developer/SDKs/

	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.5)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
	set(CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX${CMAKE_OSX_DEPLOYMENT_TARGET}.sdk")
endif()

add_subdirectory(src)
add_subdirectory(external/AnyOption)
add_subdirectory(external/minizip)
add_subdirectory(external/tinyxml)
add_subdirectory(external/TinyThread)

