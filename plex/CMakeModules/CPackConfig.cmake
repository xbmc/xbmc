set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Plex for Home Theater")
set(CPACK_PACKAGE_VENDOR "Plex inc")
set(CPACK_PACKAGE_VERSION_MAJOR ${PLEX_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PLEX_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PLEX_VERSION_PATCH}0${PLEX_VERSION_SMALL})
if(TARGET_OSX)
  set(CPACK_SYSTEM_NAME "macosx-${OSX_ARCH}")
elseif(TARGET_WIN32)
  set(CPACK_SYSTEM_NAME "windows-x86")
  # use a shorter path to hopefully avoid stupid windows 260 chars path.
  set(CPACK_PACKAGE_DIRECTORY "C:/tmp")
else()
  set(CPACK_SYSTEM_NAME linux-${CMAKE_HOST_SYSTEM_PROCESSOR})
endif()
set(CPACK_PACKAGE_FILE_NAME "${PLEX_TARGET_NAME}-${PLEX_VERSION_STRING}-${CPACK_SYSTEM_NAME}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PLEX_TARGET_NAME}-${PLEX_VERSION_STRING}-src")

set(CPACK_PACKAGE_INSTALL_DIRECTORY "Plex Home Theater")
set(CPACK_COMPONENT_QDXSETUP_DISPLAY_NAME "DirectX Installer")
set(CPACK_COMPONENT_VCREDIST_DISPLAY_NAME "Visual Studio 2010 redistribution installer")
set(CPACK_COMPONENT_MCE_DISPLAY_NAME "Microsoft Media Center Integration")
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

if(TARGET_OSX)
  set(CPACK_GENERATOR "ZIP")
  set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
elseif(TARGET_COMMON_LINUX)
  set(CPACK_GENERATOR "DEB;TBZ2")
elseif(TARGET_WIN32)
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

if(TARGET_WIN32)
  add_custom_target(signed_package ${plexdir}/scripts/WindowsSign.cmd ${CPACK_PACKAGE_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.exe DEPENDS package)
endif()

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/plex/CMakeModules ${CMAKE_MODULE_PATH})
include(CPack)
