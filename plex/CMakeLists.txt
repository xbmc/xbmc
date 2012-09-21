
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
#add_subdirectory(Windows)

aux_source_directory(. plex_SRCS)
set(Headers PlexTypes.h)
if(UNIX)
  list(REMOVE_ITEM plex_SRCS ./CocoaUtils.cpp)
  list(REMOVE_ITEM plex_SRCS ./MediaCenterLaunchHost.cpp)
  list(REMOVE_ITEM plex_SRCS ./PlexApplicationWin.cpp)
endif()

if(APPLE)
  add_executable(Plex MACOSX_BUNDLE ${plex_SRCS} ${Headers})
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.plexapp.plexmediacenter")
  set(MACOSX_BUNDLE_ICON_FILE Plex.icns)
  set(MACOSX_BUNDLE_COPYRIGHT "Plex inc")
  set(resourcesdir ${CMAKE_INSTALL_PREFIX}/Plex.app/Contents/Resources)
else()
  add_executable(Plex ${plex_SRCS} ${Headers})
endif()

add_dependencies(Plex git_revision.h ffmpeg)

install(TARGETS Plex BUNDLE DESTINATION .)

install(FILES ${plexdir}/Plex.icns DESTINATION ${resourcesdir})

#set(ffmpeglibs avcodec avformat avutil avcore avfilter avdevice postproc swscale)
#foreach(l ${ffmpeglibs})
#  install(FILES ${CMAKE_BINARY_DIR}/ffmpeg/lib/lib${l}.dylib DESTINATION ${CMAKE_INSTALL_PREFIX}/Plex.app/Contents/MacOS)
#endforeach()

install(CODE "
	include(BundleUtilities)
	set(BU_CHMOD_BUNDLE_ITEMS ON)
	fixup_bundle(${CMAKE_INSTALL_PREFIX}/Plex.app \"\" ${dependdir}/lib)"
COMPONENT RUNTIME)
install(CODE "
	execute_process(COMMAND ${plexdir}/scripts/copy-resources.sh Plex.app \"${root}\" \"${CMAKE_INSTALL_PREFIX}\")
")


target_link_libraries(Plex
	utils
	dllloader
	exports
	addons
	cdrip
	dbwrappers
	dialogs
	filesystem
	MusicDatabaseDirectory
	VideoDatabaseDirectory
	guilib
	input
	interfaces
	httpapi
	jsonrpc
	info
	music
	music_infoscanner
	music_tags
	music_dialogs
	music_karaoke
	music_windows
	network
	network_scrobbler
	network_osx
	network_linux
	osx
	peripherals
	peripherals_bus
	peripherals_devices
	peripherals_dialogs
	pictures
	powermanagement
	playlists
	programs
	rendering
	settings
	storage
	threads
	threads_platform
	video
	video_dialogs
	video_windows
	windowing
	windows
	cores
	audiorenderers
	dvdplayer
	dvdplayer_dvdcodecs
	dvdplayer_dvdcodecs_audio
	dvdplayer_dvdcodecs_video
	dvdplayer_dvdcodecs_overlay
	dvdplayer_dvddemuxers
	dvdplayer_dvdinputstreams
	dvdplayer_dvdsubtitles
	externalplayer
	paplayer
	playercorefactory
	videorenderers
	videorenderers_shaders
	xbmc_linux
	xbmc
	
	plex_filesystem
	plex_gui
	plex_network
	plex_players
	plex_utility
  plex_helper
  plex_owned
	
	lib_squish
	lib_apetag
	lib_hts
)

foreach(lib ${external_libs})
	get_property(l VARIABLE PROPERTY ${lib}_LIBRARY)
	target_link_libraries(Plex ${l})
endforeach()

foreach(lib ${system_libs})
	get_property(l VARIABLE PROPERTY ${lib}_LIBRARY)
	target_link_libraries(Plex ${l})
endforeach()

foreach(framework ${osx_frameworks})
	find_library(${framework}_LIBRARY ${framework})
	get_property(f VARIABLE PROPERTY ${framework}_LIBRARY)
	mark_as_advanced(${f})
	target_link_libraries(Plex ${f})
endforeach()
