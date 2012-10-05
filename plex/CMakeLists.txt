
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
)

add_subdirectory(FileSystem)
add_subdirectory(GUI)
add_subdirectory(Network)
add_subdirectory(Owned)
add_subdirectory(Players)
add_subdirectory(Utility)
add_subdirectory(Helper)

find_all_sources(. plex_SRCS)
set(Headers PlexTypes.h)
if(UNIX)
  list(REMOVE_ITEM plex_SRCS ./CocoaUtils.cpp)
  list(REMOVE_ITEM plex_SRCS ./MediaCenterLaunchHost.cpp)
  list(REMOVE_ITEM plex_SRCS ./PlexApplicationWin.cpp)
endif()

get_property(PLEX_MODULE_SRCS GLOBAL PROPERTY SRCS_LIST)
set(PLEX_ALL_SRCS ${PLEX_MODULE_SRCS} ${plex_SRCS} ${Headers})

if(APPLE)
  add_executable(Plex MACOSX_BUNDLE ${PLEX_ALL_SRCS})
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.plexapp.plexmediacenter")
  set(MACOSX_BUNDLE_ICON_FILE Plex.icns)
  set(MACOSX_BUNDLE_COPYRIGHT "Plex inc")
  set(resourcesdir ${CMAKE_INSTALL_PREFIX}/Plex.app/Contents/Resources)
else()
  add_executable(Plex ${PLEX_ALL_SRCS})
endif()

add_dependencies(Plex git_revision.h ffmpeg)

target_link_libraries(Plex xbmc ${CONFIG_INTERNAL_LIBS} ${CONFIG_PLEX_LINK_LIBRARIES})

install(TARGETS Plex BUNDLE DESTINATION .)
install(FILES ${plexdir}/Plex.icns DESTINATION ${resourcesdir})

set(plugins ${resourcesdir}/XBMC/system/ImageLib-x86-osx.so)

install(CODE "
	include(BundleUtilities)
	set(BU_CHMOD_BUNDLE_ITEMS ON)
	fixup_bundle(${CMAKE_INSTALL_PREFIX}/Plex.app ${plugins} ${dependdir}/lib)"
COMPONENT RUNTIME)
install(CODE "
	execute_process(COMMAND ${plexdir}/scripts/copy-resources.sh Plex.app \"${root}\" \"${CMAKE_INSTALL_PREFIX}\")
")
install(CODE "file(REMOVE ${resourcesdir}/XBMC/media/Splash.png)")
install(FILES ${plexdir}/Splash.png DESTINATION ${resourcesdir}/XBMC/media)
