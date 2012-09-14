
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
add_subdirectory(Windows)

set(osx_frameworks
	AudioToolbox
	AudioUnit
	Cocoa
	CoreAudio
	CoreServices
	Foundation
	OpenGL
	AppKit
	ApplicationServices
	IOKit
	QuickTime
	Carbon
	DiskArbitration
	QuartzCore
	SystemConfiguration
)

set(external_libs
	z
#	bz2
	xml2
	iconv
#	ncurses
	freetype
	SDL
	SDL_mixer
	SDL_image
	microhttpd
	tinyXML
#	boost_thread
#	boost_date_time
	GLEW
	sqlite3
	yajl
	ssh
	ssh2
	cdio
	pcre
	pcrecpp
	jpeg
	lzo2
	samplerate
)

aux_source_directory(. plex_SRCS)
if(UNIX)
	list(REMOVE_ITEM plex_SRCS ./MediaCenterLaunchHost.cpp)
	list(REMOVE_ITEM plex_SRCS ./PlexApplicationWin.cpp)
endif(UNIX)

add_executable(Plex ${plex_SRCS})
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
	plex_owned
	plex_players
	plex_utility
)

foreach(lib ${external_libs})
	find_library(${lib}_LIBRARY ${lib} ${dependdir}/lib NO_DEFAULT_PATH)
	get_property(l VARIABLE PROPERTY ${lib}_LIBRARY)
	target_link_libraries(Plex ${l})
endforeach()

foreach(framework ${osx_frameworks})
	find_library(${framework}_LIBRARY ${framework})
	get_property(f VARIABLE PROPERTY ${framework}_LIBRARY)
	mark_as_advanced(${f})
	target_link_libraries(Plex ${f})
endforeach()
