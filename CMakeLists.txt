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
string(SUBSTRING ${PLEX_FULL_GIT_REVISION} 0 8 PLEX_GIT_REVISION)
configure_file(${plexdir}/git_revision.h.in git_revision.h)

set(PLEX_VERSION_MAJOR 0)
set(PLEX_VERSION_MINOR 9)
set(PLEX_VERSION_PATCH 9)
set(PLEX_VERSION_SMALL 2)
if(DEFINED ENV{BUILD_NUMBER})
  set(PLEX_VERSION_BUILD ".$ENV{BUILD_NUMBER}")
else()
  set(PLEX_VERSION_BUILD "")
endif()
set(PLEX_VERSION_STRING ${PLEX_VERSION_MAJOR}.${PLEX_VERSION_MINOR}.${PLEX_VERSION_PATCH}.${PLEX_VERSION_SMALL}${PLEX_VERSION_BUILD}-${PLEX_GIT_REVISION})
set(PLEX_VERSION_STRING_SHORT ${PLEX_VERSION_MAJOR}.${PLEX_VERSION_MINOR}.${PLEX_VERSION_PATCH}.${PLEX_VERSION_SMALL})
set(PLEX_VERSION_STRING_SHORT_BUILD ${PLEX_VERSION_STRING_SHORT}${PLEX_VERSION_BUILD})
set(XBMC_VERSION "Frodo")

if(APPLE OR WIN32)
  if(NOT WIN32)
    set(dependdir ${root}/plex/Dependencies/laika-depends)
  else()
    set(dependdir ${root}/project/BuildDependencies)
  endif()
  message("-- Using dependency dir ${dependdir}")
else()
  message("-- On Linux we don't use a dependency dir")
endif()


include(CMakeFunctions)

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

if(WIN32)
  # override runtime
  set(CMAKE_C_FLAGS_DEBUG "/Zi /MP /Od /Oy- /D_DEBUG /Gm- /MTd /GS /arch:SSE /fp:precise /Zc:wchar_t /Zc:forScope /wd\"4996\"")
  set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})

  set(CMAKE_C_FLAGS_RELEASE "/DNDEBUG /WX- /MP /Ox /Ot /Oy /GF- /Gm- /EHa /MT /GS /Gy- /arch:SSE /fp:precise /Zc:wchar_t /Zc:forScope /wd\"4996\"")
  set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

  set(CMAKE_CXX_FLAGS "/DWIN32 /D_WINDOWS /W3 /Zm1000 /GR /EHa")
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
  set(CMAKE_C_FLAGS_RELEASE "-g")
  set(CMAKE_CXX_FLAGS_RELEASE "-g")
endif()

if(WIN32)
  set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x86" ${dependdir}/lib)
  set(ENV{LIBS} "$ENV{LIBS};${dependdir}/lib")
  if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:\"libc\" /NODEFAULTLIB:\"msvcrt\" /NODEFAULTLIB:\"libcmt\" /NODEFAULTLIB:\"msvcrtd\" /NODEFAULTLIB:\"msvcprtd\" /DELAYLOAD:\"dnssd.dll\" /DELAYLOAD:\"dwmapi.dll\" /DELAYLOAD:\"libmicrohttpd-5.dll\" /DELAYLOAD:\"ssh.dll\" /DELAYLOAD:\"sqlite3.dll\" /DELAYLOAD:\"libsamplerate-0.dll\"  /SUBSYSTEM:WINDOWS /LARGEADDRESSAWARE /TLBID:1 /DYNAMICBASE /NXCOMPAT /MACHINE:X86 /ERRORREPORT:QUEUE")
  else()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:\"libc\" /NODEFAULTLIB:\"msvcrt\" /NODEFAULTLIB:\"libci\" /NODEFAULTLIB:\"msvcprt\" /DELAYLOAD:\"dnssd.dll\" /DELAYLOAD:\"dwmapi.dll\" /DELAYLOAD:\"libmicrohttpd-5.dll\" /DELAYLOAD:\"ssh.dll\" /DELAYLOAD:\"sqlite3.dll\" /DELAYLOAD:\"libsamplerate-0.dll\"  /SUBSYSTEM:WINDOWS /LARGEADDRESSAWARE /TLBID:1 /DYNAMICBASE /NXCOMPAT /MACHINE:X86 /ERRORREPORT:QUEUE")
  endif()
endif()

# MUST BE ADDED FIRST :)
# This will download our dependency tree
if(APPLE)
  add_subdirectory(plex/Dependencies)
endif()

# Now check for available functions
include(CMakeConfig)

# global definitions
add_definitions(-D__PLEX__ -D__PLEX__XBMC__ -DPLEX_TARGET_NAME="${EXECUTABLE_NAME}" -DPLEX_VERSION="${PLEX_VERSION_STRING_SHORT_BUILD}")

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  add_definitions(-DDEBUG)
else()
  add_definitions(-DNDEBUG)
endif()

# Linux and OSX definitios
if(UNIX)
  add_definitions(
    -DTARGET_POSIX
    -D_REENTRANT
    -D_FILE_DEFINED
    -D__STDC_CONSTANT_MACROS
    -DHAVE_CONFIG_H
    -D_FILE_OFFSET_BITS=64
    -D_LARGEFILE64_SOURCE
    -DUSE_EXTERNAL_FFMPEG
    -D_LINUX
    -D__STDC_LIMIT_MACROS
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
    -DTAGLIB_STATIC
    -DBOOST_ALL_NO_LIB
    -DBOOST_THREAD_USE_LIB
  )
endif()

# Now build auxilary xbmc libraries.
# Note that we want to do this before we add all
# our defines and includes, otherwise they will
# carry over
add_subdirectory(lib)

include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/xbmc

  ${dependdir}/include
  ${dependdir}/include/libcec
  ${dependdir}/include/freetype2
  ${dependdir}/include/python2.6
  ${root}/xbmc
)

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
    "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include"
  )
endif()

# Build the texturepacker
add_subdirectory(tools/TexturePacker)

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
if(APPLE)
  add_subdirectory(tools/EventClients/Clients/OSXRemote)
endif()
add_subdirectory(plex)

if(XRANDR_FOUND)
  add_executable(xbmc-xrandr xbmc-xrandr.c)
  target_link_libraries(xbmc-xrandr ${CONFIG_LIBRARY_XRANDR} ${CONFIG_LIBRARY_X11} -lm)
  install(TARGETS xbmc-xrandr DESTINATION ${BINPATH})
endif()
  
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Plex for Home Theater")
set(CPACK_PACKAGE_VENDOR "Plex inc")
set(CPACK_PACKAGE_VERSION_MAJOR ${PLEX_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PLEX_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PLEX_VERSION_PATCH}0${PLEX_VERSION_SMALL})
if(APPLE)
  set(CPACK_SYSTEM_NAME "macosx-x86")
elseif(WIN32)
  set(CPACK_SYSTEM_NAME "windows-x86")
  # use a shorter path to hopefully avoid stupid windows 260 chars path.
  set(CPACK_PACKAGE_DIRECTORY "C:/tmp")
else()
  set(CPACK_SYSTEM_NAME linux-${CMAKE_HOST_SYSTEM_PROCESSOR})
endif()
set(CPACK_PACKAGE_FILE_NAME "${PLEX_TARGET_NAME}-${PLEX_VERSION_STRING}-${CPACK_SYSTEM_NAME}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PLEX_TARGET_NAME}-${PLEX_VERSION_STRING}-src")

set(CPACK_COMPONENT_QDXSETUP_DISPLAY_NAME "DirectX Installer")
set(CPACK_COMPONENT_VCREDIST_DISPLAY_NAME "Visual Studio 2010 redistribution installer")
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Plex for Home Theater")
set(CPACK_COMPONENT_RUNTIME_REQUIRED 1)

# Windows installer stuff
set(CPACK_NSIS_MUI_UNIICON ${plexdir}\\\\Resources\\\\Plex.ico)
set(CPACK_NSIS_MUI_ICON ${plexdir}\\\\Resources\\\\Plex.ico)
#set(CPACK_PACKAGE_ICON ${plexdir}\\\\Resources\\\\PlexBanner.bmp)
set(CPACK_NSIS_HELP_LINK "http://plexapp.com")
set(CPACK_NSIS_URL_INFO_ABOUT ${CPACK_NSIS_HELP_LINK})
set(CPACK_PACKAGE_EXECUTABLES ${EXECUTABLE_NAME} "Plex for Home Theater" ${CPACK_PACKAGE_EXECUTABLES})
set(CPACK_RESOURCE_FILE_LICENSE ${root}/LICENSE.GPL)
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
  "IfFileExists \\\"$INSTDIR\\\\Dependencies\\\\vcredist_x86.exe\\\" 0 +2
   ExecWait \\\"$INSTDIR\\\\Dependencies\\\\vcredist_x86.exe /q /norestart\\\"
   IfFileExists \\\"$INSTDIR\\\\Dependencies\\\\dxsetup\\\\dxsetup.exe\\\" 0 +2
   ExecWait \\\"$INSTDIR\\\\Dependencies\\\\dxsetup\\\\dxsetup.exe /silent\\\"
   RMDir /r \\\"$INSTDIR\\\\Dependencies\\\"")

if(APPLE)
  set(CPACK_GENERATOR "ZIP")
  set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
elseif(UNIX)
  set(CPACK_GENERATOR "DEB;TBZ2")
elseif(WIN32)
  set(CPACK_GENERATOR "NSIS")
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(CPACK_STRIP_FILES 0)
endif()

#debian stuff
set(CPACK_DEBIAN_PACKAGE_DEPENDS
  "libasound2 (>= 1.0.23), libavahi-client3 (>= 0.6.16), libavahi-common3 (>= 0.6.16), libavcodec53 (>= 6:0.10.5~) | libavcodec-extra-53 (>= 6:0.10.6), libavfilter2 (>= 6:0.10.5~), libavformat53 (>= 6:0.10.5~), libavutil51 (>= 6:0.10.5~), libboost-system1.46.1 (>= 1.46.1-1), libboost-thread1.46.1 (>= 1.46.1-1), libc6 (>= 2.3.6-6~), libc6 (>= 2.8), libfreetype6 (>= 2.2.1), libfribidi0 (>= 0.19.2), libgcc1 (>= 1:4.1.1), libgl1-mesa-glx | libgl1, libglew1.6 (>= 1.6.0), libglu1-mesa | libglu1, libjpeg8 (>= 8c), liblzo2-2, libmicrohttpd5, libpcre3 (>= 8.10), libpostproc52 (>= 6:0.10.5~), libpulse0 (>= 1:1.0), libsamplerate0 (>= 0.1.7), libsdl1.2debian (>= 1.2.10-1), libsqlite3-0 (>= 3.6.11), libstdc++6 (>= 4.6), libswresample0 (>= 6:0.10.5~), libswscale2 (>= 6:0.10.5~), libtinyxml2.6.2, libx11-6, libxext6, libxrandr2 (>= 4.3), libyajl1 (>= 1.0.5), zlib1g (>= 1:1.1.4), libpulse0 (>= 1:1.1), libasound2 (>= 1.0.25), libcec2 (>= 2.0.5), libass4 (>= 0.10.0), libshairport1, libmad0 (>= 0.15.1b), libcurl3-gnutls (>= 7.16.2-1), libplist1 (>= 0.13), librtmp0 (>= 2.3), libvdpau1 (>= 0.2)")
set(CPACK_PACKAGE_CONTACT "http://plexapp.com/")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Plex inc")
set(CPACK_DEBIAN_PACKAGE_SECTION "universe/video")
set(CPACK_DEBIAN_PACKAGE_NAME "plexhometheater")
if(CPACK_GENERATOR MATCHES "DEB")
  set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/plexhometheater")
endif()

set(CPACK_SOURCE_GENERATOR TBZ2)
set(CPACK_SOURCE_IGNORE_FILES
  "^${PROJECT_SOURCE_DIR}/.git"
  "^${PROJECT_SOURCE_DIR}/plex/build"
  "^${PROJECT_SOURCE_DIR}/plex/Dependencies/laika-depends"
  "^${PROJECT_SOURCE_DIR}/plex/Dependencies/.*-darwin-i686"
  "^${PROJECT_SOURCE_DIR}/upload"
)

# We want to make sure that CPack uses our own NSIS.template.in
list(APPEND CMAKE_MODULE_PATH ${plexdir}/Resources)

if(WIN32)
  add_custom_target(signed_package ${plexdir}/scripts/WindowsSign.cmd ${CPACK_PACKAGE_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.exe DEPENDS package)
endif()


include(CPack)

