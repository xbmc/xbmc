set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Plex for Home Theater")
set(CPACK_PACKAGE_VENDOR "Plex inc")
set(CPACK_PACKAGE_VERSION_MAJOR ${PLEX_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PLEX_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PLEX_VERSION_PATCH})
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
set(CPACK_COMPONENT_VCREDIST_DISPLAY_NAME "Visual Studio redistribution installer")
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
  "IfFileExists \\\"$INSTDIR\\\\Dependencies\\\\vcredist_2012_x86.exe\\\" 0 +2
   ExecWait \\\"$INSTDIR\\\\Dependencies\\\\vcredist_2012_x86.exe /q /norestart\\\"
   IfFileExists \\\"$INSTDIR\\\\Dependencies\\\\vcredist_2010_x86.exe\\\" 0 +2
   ExecWait \\\"$INSTDIR\\\\Dependencies\\\\vcredist_2010_x86.exe /q /norestart\\\"
   IfFileExists \\\"$INSTDIR\\\\Dependencies\\\\dxsetup\\\\dxsetup.exe\\\" 0 +2
   ExecWait \\\"$INSTDIR\\\\Dependencies\\\\dxsetup\\\\dxsetup.exe /silent\\\"
   RMDir /r \\\"$INSTDIR\\\\Dependencies\\\"
   !include \\\"WinVer.nsh\\\"
   !include \\\"LogicLib.nsh\\\"
   ${IfNot} ${AtLeastWinVista}
      MessageBox MB_OK|MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND \\\"Windows Vista or above required.$\nThis program can not be run on Windows XP\\\"
      Quit
   ${EndIf}")

if(TARGET_OSX)
  set(CPACK_GENERATOR "ZIP")
  set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
elseif(TARGET_COMMON_LINUX)
  set(CPACK_GENERATOR "TBZ2")
elseif(TARGET_WIN32)
  set(CPACK_GENERATOR "NSIS;ZIP")
  set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
endif()

set(CPACK_SOURCE_GENERATOR TBZ2)
set(CPACK_SOURCE_IGNORE_FILES
  "^${PROJECT_SOURCE_DIR}/.git"
  "^${PROJECT_SOURCE_DIR}/plex/build"
  "^${PROJECT_SOURCE_DIR}/plex/Dependencies/laika-depends"
  "^${PROJECT_SOURCE_DIR}/plex/Dependencies/.*-darwin-i686"
  "^${PROJECT_SOURCE_DIR}/upload"
)

set(ZIPFILE PlexHomeTheater-${PLEX_VERSION_STRING}-${CPACK_SYSTEM_NAME}.zip)

if(TARGET_WIN32)
  set(ZIPFILE ${CPACK_PACKAGE_DIRECTORY}/PlexHomeTheater-${PLEX_VERSION_STRING}-${CPACK_SYSTEM_NAME}.zip)
  set(MAIN_BINARY "-m \"Plex Home Theater.exe\"")
endif(TARGET_WIN32)

add_custom_command(
  OUTPUT PlexHomeTheater-${PLEX_VERSION_STRING}-${CPACK_SYSTEM_NAME}-manifest.xml
  COMMAND ${plexdir}/scripts/create_update.py -p ${CPACK_SYSTEM_NAME} ${MAIN_BINARY} -v ${PLEX_VERSION_STRING} -i ${ZIPFILE} -o ${CMAKE_BINARY_DIR}
  DEPENDS package
)

add_custom_target(update_manifest DEPENDS PlexHomeTheater-${PLEX_VERSION_STRING}-${CPACK_SYSTEM_NAME}-manifest.xml)

set(PKG update_manifest)
if(TARGET_WIN32)
  add_custom_target(signed_package ${plexdir}/scripts/WindowsSign.cmd ${CPACK_PACKAGE_DIRECTORY}/${CPACK_PACKAGE_FILE_NAME}.exe DEPENDS package update_manifest)
  set(PKG signed_package)
endif(TARGET_WIN32)
add_custom_target(release_package DEPENDS symbols ${PKG})

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/plex/CMakeModules ${CMAKE_MODULE_PATH})
include(CPack)
