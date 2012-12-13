
include_directories(
	${root}/xbmc/cores
	${root}/xbmc/guilib
	${root}/xbmc/network
	${root}/xbmc/music
	${root}/xbmc/filesystem
	${root}/xbmc/threads
	${root}/xbmc/video
	${root}/xbmc/music/tags
	${root}/xbmc/dialogs
	${root}/xbmc/pictures
	${root}/xbmc/playlists
	${root}/plex/FileSystem
	${root}/plex/Network
  ${CONFIG_INCLUDE_PATH_SPARKLE}
)

add_subdirectory(FileSystem)
add_subdirectory(GUI)
add_subdirectory(Network)
add_subdirectory(Owned)
add_subdirectory(Players)
add_subdirectory(Utility)
if(APPLE)
  add_subdirectory(Helper)
endif(APPLE)
add_subdirectory(AutoUpdate)

find_all_sources(. plex_SRCS)

list(REMOVE_ITEM plex_SRCS ./pch_plex.cpp)
list(APPEND plex_SRCS Resources/Plex.rc)
list(APPEND plex_SRCS Resources/Plex.rc2)

set(Headers PlexTypes.h)
if(APPLE)
  list(REMOVE_ITEM plex_SRCS ./CocoaUtils.cpp)
endif(APPLE)

if(NOT APPLE)
  list(REMOVE_ITEM plex_SRCS ./CocoaUtils.m)
  list(REMOVE_ITEM plex_SRCS ./CocoaUtilsPlus.mm)
endif(NOT APPLE)

if(NOT WIN32)
  list(REMOVE_ITEM plex_SRCS ./MediaCenterLaunchHost.cpp)
  list(REMOVE_ITEM plex_SRCS ./PlexApplicationWin.cpp)
endif(NOT WIN32)

get_property(PLEX_MODULE_SRCS GLOBAL PROPERTY SRCS_LIST)
set(PLEX_ALL_SRCS ${PLEX_MODULE_SRCS} ${plex_SRCS} ${Headers})

ADD_MSVC_PRECOMPILED_HEADER("pch_plex.h" "pch_plex.cpp" PLEX_ALL_SRCS)

if(APPLE)
  add_executable(Plex MACOSX_BUNDLE ${PLEX_ALL_SRCS})
  set_target_properties(Plex PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${plexdir}/Resources/Info.plist.in")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PLEX_VERSION_STRING}")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PLEX_VERSION_STRING_SHORT}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${PLEX_VERSION_STRING}")
else()
  add_executable(Plex WIN32 ${PLEX_ALL_SRCS})
endif()

add_dependencies(Plex git_revision.h ffmpeg)
target_link_libraries(Plex xbmc pcre ${CONFIG_INTERNAL_LIBS} ${CONFIG_PLEX_LINK_LIBRARIES})


# find all image files in the skin
file(GLOB MEDIA_IMAGES_MEDIASTREAM ${root}/addons/skin.mediastream/Media/*.png ${root}/addons/skin.mediastream/Media/*.gif)

if(NOT WIN32)
  # Build the packed textures
  if(DEFINED USING_XCODE)
    set(TEXTURE_PACKER_BIN ${CMAKE_BINARY_DIR}/tools/TexturePacker/Debug/TexturePacker)
  else()
    set(TEXTURE_PACKER_BIN ${CMAKE_BINARY_DIR}/tools/TexturePacker/TexturePacker)
  endif()
  add_custom_command(
    OUTPUT Textures_MediaStream.xbt
    COMMAND ${TEXTURE_PACKER_BIN} -input ${root}/addons/skin.mediastream/Media -output Textures_MediaStream.xbt
    MAIN_DEPENDENCY ${MEDIA_IMAGES_MEDIASTREAM}
    DEPENDS TexturePacker
  )

  add_custom_target(CompressTextures ALL DEPENDS Textures_MediaStream.xbt)
endif()

# Set some variables we need to configure the CMakeCompleteBundle file
set(PLEXDIR ${plexdir})
set(ROOTDIR ${root})
set(DEPENDDIR ${dependdir})
configure_file(CMakeCompleteBundle.cmake.in CMakeCompleteBundle.cmake @ONLY)

install(FILES ${CONFIG_PLEX_INSTALL_LIBRARIES} DESTINATION ${LIBPATH} COMPONENT RUNTIME)
if(APPLE)
  install(TARGETS Plex BUNDLE DESTINATION . COMPONENT RUNTIME)
  install(FILES ${plexdir}/Resources/Plex.icns DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME)
else()
  install(TARGETS Plex DESTINATION ${BINPATH} COMPONENT RUNTIME)
endif()

# Replace some XBMC files in the output
if(NOT WIN32)
  install(CODE "
    execute_process(COMMAND ${plexdir}/scripts/copy-resources.sh \"${RESOURCEPATH}\" \"${ROOTDIR}\" \"\${CMAKE_INSTALL_PREFIX}\")
  ")
  install(CODE "file(REMOVE \${CMAKE_INSTALL_PREFIX}/${RESOURCEPATH}/XBMC/media/Splash.png)")
  install(CODE "file(REMOVE \${CMAKE_INSTALL_PREFIX}/${RESOURCEPATH}/Credits.html)")
  install(FILES ${plexdir}/Resources/com.plexapp.helper.plist DESTINATION ${RESOURCEPATH}/XBMC/tools/darwin/runtime COMPONENT RUNTIME)
  install(FILES ${plexdir}/Resources/Credits.html DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Textures_MediaStream.xbt DESTINATION ${RESOURCEPATH}/XBMC/addons/skin.mediastream/Media RENAME Textures.xbt COMPONENT RUNTIME)
  #install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Textures_Plex.xbt DESTINATION ${RESOURCEPATH}/XBMC/addons/skin.plex/Media RENAME Textures.xbt)
  install(FILES ${plexdir}/Resources/Splash.png DESTINATION ${RESOURCEPATH}/XBMC/media COMPONENT RUNTIME)
else()
  install(DIRECTORY ${root}/media ${root}/sounds ${root}/language DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME)
  install(DIRECTORY ${root}/system DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME PATTERN python EXCLUDE)
  install(DIRECTORY ${root}/addons DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME PATTERN skin.confluence EXCLUDE PATTERN skin.touched EXCLUDE)
  install(CODE "file(REMOVE \${CMAKE_INSTALL_PREFIX}/${RESOURCEPATH}/media/Splash.png)")
  install(FILES ${root}/system/zlib1.dll DESTINATION ${BINPATH} COMPONENT RUNTIME)
  install(FILES ${plexdir}/Resources/Splash.png DESTINATION ${RESOURCEPATH}/media COMPONENT RUNTIME)
  install(FILES ${root}/project/Win32BuildSetup/dependencies/glew32.dll DESTINATION ${BINPATH} COMPONENT RUNTIME)
  install(FILES ${root}/project/Win32BuildSetup/dependencies/libiconv-2.dll DESTINATION ${BINPATH} COMPONENT RUNTIME)
  install(FILES ${plexdir}/Resources/Plex.ico ${plexdir}/Resources/PlexBanner.bmp DESTINATION ${RESOURCEPATH}/media COMPONENT RUNTIME)

  install(FILES ${plexdir}/build/dependencies/vcredist/2010/vcredist_x86.exe DESTINATION ${BINPATH}/Dependencies COMPONENT VCREDIST)
  install(DIRECTORY ${plexdir}/build/dependencies/dxsetup DESTINATION ${BINPATH}/Dependencies COMPONENT QDXSETUP)
endif()

if(NOT PLEX_SKIP_BUNDLING STREQUAL "1")
  if(NOT WIN32)
    install(SCRIPT ${CMAKE_BINARY_DIR}/plex/CMakeCompleteBundle.cmake COMPONENT RUNTIME)
  endif()
else()
  message("-- Skipping bundling ${PLEX_SKIP_BUNDLING}")
endif()
