
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
set(Headers PlexTypes.h)
if(APPLE)
  list(REMOVE_ITEM plex_SRCS ./CocoaUtils.cpp)
endif(APPLE)

if(NOT APPLE)
  list(REMOVE_ITEM plex_SRCS ./CocoaUtils.m)
  list(REMOVE_ITEM plex_SRCS ./CocoaUtilsPlus.mm)
endif(NOT APPLE)

if(NOT WINDOWS)
  list(REMOVE_ITEM plex_SRCS ./MediaCenterLaunchHost.cpp)
  list(REMOVE_ITEM plex_SRCS ./PlexApplicationWin.cpp)
endif(NOT WINDOWS)

get_property(PLEX_MODULE_SRCS GLOBAL PROPERTY SRCS_LIST)
set(PLEX_ALL_SRCS ${PLEX_MODULE_SRCS} ${plex_SRCS} ${Headers})

if(APPLE)
  add_executable(Plex MACOSX_BUNDLE ${PLEX_ALL_SRCS})
  set_target_properties(Plex PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${plexdir}/Resources/Info.plist.in")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PLEX_VERSION_STRING}")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PLEX_VERSION_STRING_SHORT}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${PLEX_VERSION_STRING}")
else()
  add_executable(Plex ${PLEX_ALL_SRCS})
endif()

add_dependencies(Plex git_revision.h ffmpeg)
target_link_libraries(Plex xbmc ${CONFIG_INTERNAL_LIBS} ${CONFIG_PLEX_LINK_LIBRARIES})


# find all image files in the skin
file(GLOB MEDIA_IMAGES ${root}/addons/skin.mediastream/Media/*.png ${root}/addons/skin.mediastream/Media/*.gif)

# Build the packed textures
add_custom_command(
  OUTPUT Textures.xbt
  COMMAND ${CMAKE_BINARY_DIR}/tools/TexturePacker/TexturePacker -input ${root}/addons/skin.mediastream/Media
  MAIN_DEPENDENCY ${MEDIA_IMAGES}
  DEPENDS TexturePacker
)
add_custom_target(CompressTextures ALL DEPENDS Textures.xbt)


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

install(CODE "
  execute_process(COMMAND ${plexdir}/scripts/copy-resources.sh \"${RESOURCEPATH}\" \"${ROOTDIR}\" \"\${CMAKE_INSTALL_PREFIX}\")
")

# Replace some XBMC files in the output
install(CODE "file(REMOVE \${CMAKE_INSTALL_PREFIX}/${RESOURCEPATH}/XBMC/media/Splash.png)")
install(CODE "file(REMOVE \${CMAKE_INSTALL_PREFIX}/${RESOURCEPATH}/Credits.html)")
install(FILES ${plexdir}/Resources/Credits.html DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME)
install(FILES ${plexdir}/Resources/Splash.png DESTINATION ${RESOURCEPATH}/XBMC/media COMPONENT RUNTIME)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Textures.xbt DESTINATION ${RESOURCEPATH}/XBMC/addons/skin.mediastream/Media)

if(NOT PLEX_SKIP_BUNDLING STREQUAL "1")
  install(SCRIPT ${CMAKE_BINARY_DIR}/plex/CMakeCompleteBundle.cmake COMPONENT RUNTIME)
else()
  message("-- Skipping bundling ${PLEX_SKIP_BUNDLING}")
endif()
