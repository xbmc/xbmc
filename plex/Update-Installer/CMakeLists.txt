project(updater)

cmake_minimum_required(VERSION 2.6)
enable_testing()
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
include(VersionConfiguration)

# If the SIGN_UPDATER option is enabled, the updater.exe
# binary is digitally signed on Windows after it has been built.
#
# The script used to sign the binary is specified by BINARY_SIGNING_TOOL.
# This tool must take a single argument which is the filename of the
# binary to sign.
option(SIGN_UPDATER "Enable signing of the updater binary" OFF)
set(BINARY_SIGNING_TOOL sign-updater.bat CACHE PATH "Path to the tool used to sign the updater")

include_directories(external)
include_directories(external/TinyThread/source)
include_directories(external/sha1 external/bsdiff)

## PLEX
if(NOT APPNAME)
	set(APPNAME "Plex Home Theater")
endif(NOT APPNAME)
add_definitions(-DAPPNAME="${APPNAME}")

option(DISABLE_GUI "Disable the installer GUI" OFF)
if(DISABLE_GUI)
	add_definitions(-DDISABLE_GUI)
endif(DISABLE_GUI)
## End PLEX

## PLEX
if(NOT APPNAME)
	set(APPNAME "Plex Home Theater")
endif(NOT APPNAME)
add_definitions(-DAPPNAME="${APPNAME}")

option(DISABLE_GUI "Disable the installer GUI" OFF)
if(DISABLE_GUI)
	add_definitions(-DDISABLE_GUI)
endif(DISABLE_GUI)
## End PLEX

if (WIN32)
    include_directories(external/zlib/)
    include_directories(external/bzip2)
    include_directories(external/win32cpp/include)

    # - Link the updater binary statically with the Visual C++ runtime
    #   so that the executable can function standalone.
    # - Enable PDB generation for release builds
    set(CMAKE_CXX_FLAGS_DEBUG "/MT")
    set(CMAKE_C_FLAGS_DEBUG "/MT")

    set(CMAKE_CXX_FLAGS_RELEASE "/MT /Zi /O2 /Ob2 /D NDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "/MT /Zi /O2 /Ob2 /D NDEBUG")

    remove_definitions(-DUNICODE -D_UNICODE)
else()
	# optimize for reduced code size
	set(CMAKE_CXX_FLAGS_RELEASE "-Os")
	set(CMAKE_C_FLAGS_RELEASE "-Os")
endif()

if (APPLE)
	# Build the updater as a dual 32/64bit binary.  If only one architecture
	# is required, removing the other architecture will reduce the size
	# of the updater binary
	set(CMAKE_OSX_ARCHITECTURES x86_64;i386)

	# Build the updater so that it works on OS X 10.5 and above.
	set(MIN_OSX_VERSION 10.5)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmacosx-version-min=${MIN_OSX_VERSION}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=${MIN_OSX_VERSION}")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "gnu++98")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libstdc++")
endif()

add_subdirectory(src)
add_subdirectory(external/AnyOption)
add_subdirectory(external/minizip)
add_subdirectory(external/tinyxml2)
add_subdirectory(external/TinyThread)
add_subdirectory(external/bsdiff)
add_subdirectory(external/sha1)
