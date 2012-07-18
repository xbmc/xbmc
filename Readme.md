SPOTYXBMC2
==========
This is a rewrite of the s potyxbmc project, the code is cleaner and better encapsulated from XBMC. It is now being prepared to be lifted out to a binary addon.
The code is not heavily tested and has known issues, don´t install if you don´t know what you are doing.

The main discussion for spotyxbmc is [here](http://forum.xbmc.org/showthread.php?t=67012)
A discussion concerning a unified music addon front-end can be read [here](http://forum.xbmc.org/showthread.php?t=105147).

You must have a valid spotify premium account to be able to use spotyXBMC.

Features and usage
------------------
This implementation adds spotify content to the regular music categories under the music section, do not try to run the addon, its only there for settings!

A video showing most of the features can be seen [here](http://www.youtube.com/watch?v=xFSdxKyWXpU).

* Starred tracks, albums and artists will show up in the songs, albums and artists sections alongside the local music.
* Spotify playlists shows up in the playlist section.
* The normal music search will return local music and spotify search result.
* Top 100 spotify lists with artists, albums and tracks is available in the top 100 section.
* Modifying playlists and star/unstar items in the spotify client will result in that the lists is updated in XBMC.
* Multi-disc albums are split up into separate albums with suffix "disc #".
* Browsing artist albums will provide a "similar artists" folder with spotify similar artists.
* Navigate to the album artist from an album or to the artist or album from a track using the context menu.
* Star/unstar albums and tracks from within XBMC using the context menu.
* The settings is changed from within a built-in addon.
* Two spotify "radios" is provided int the music root, visit the settings to set name, years and genres.
* Top-lists will update once every 24 hour.

Missing features
----------------
* Interaction with spotify items like creating and modify playlists, adding spotify tracks to playlists and so on is not supported yet.
* The year and genre nodes will not lead to any spotify items yet.
* A lot more I guess.

Platforms
---------

* Linux - supported
* Windows - supported
* OSX - Not supported

Known issues
------------
Enable preloading of artists together with preloading of top 100 lists and/or a massive collection of starred tracks will result in a short freeze of XBMC during start (about 5-10 seconds depending on your Internet speed, computer...).

Memory leaks do exist, beware.

A lot of other bugs, the implementation is not heavily tested.
 
Want to help killing a bug?
---------------------------
Right now the there is a lot of trace prints, they are printed out straight to the console so be sure that you start XBMC from a console in order to fetch the traces and create a bug report.

Please submit a report to the github issues and provide all relevant data like logs, OS info, what track, playlist or album you have problems with. Or even better, fix it yourself and send me a pull request or an e-mail.

SpotyXBMC2 for OpenElec
-----------------------------------------------
Thanks to Tompen a version of OpenElec with spotyXBMC2 integration is available.

Read about it [here](http://openelec.tv/forum/13-general-discussion/7010-spotify).


Installation instructions for Microsoft Windows
-----------------------------------------------
1. Obtain spotyXBMC2 source
   Use your favorite git tool to clone the repo: `git://github.com/akezeke/spotyxbmc2.git`

2. Spotify API key
   Get your own spotify API key from http://developer.spotify.com/en/libspotify/application-key/
   Click on c-code and copy the content to a new file called appkey.h placed in the xbmc source root folder. (where this readme is located).
  
3. Build
   Follow a guide from XBMC.org and build it yourself, if you are not using the Win32BuildSetup utility make sure that you copy libspotify.dll to your XBMC install location.
  
4. Start xbmc
   From the start menu.

9. Start spotyXBMC
   start the preinstalled music addon spotyXBMC and set the settings

10. Restart XBMC

11. Enable the music library and enjoy spotify music inside xbmc
		The spotify music is located inside the normal music categories, do not try to start the spotyXBMC addon!


Installation instructions for Ubuntu Linux 32/64
------------------------------------------------
1. Download libSpotify

	For 32 bit:
	`$ wget http://developer.spotify.com/download/libspotify/libspotify-10.1.16-Linux-i686-release.tar.gz`

	64 bit OS:
   `$ wget http://developer.spotify.com/download/libspotify/libspotify-10.1.16-Linux-x86_64-release.tar.gz`

2. Untar:
   `$ tar xzf libspotify-*.tar.gz`

3. Install libspotify
   `$ cd libspotify-Linux...`
   `$ sudo make install prefix=/usr/local`

4. Obtain spotyXBMC2 source
   Make sure you have git installed, if not and in ubuntu install with `sudo apt-get install git-core`
   `$ cd ..`
   `$ git clone git://github.com/akezeke/spotyxbmc2.git`
   `$ cd xbmc`

5. Spotify API key
   Get your own spotify API key from http://developer.spotify.com/en/libspotify/application-key/
   Click on c-code and copy the content to a new file called appkey.h placed in the xbmc source root folder. (where this readme is located).

6. Install all XBMC dependencies listed in the corresponding readme file.
   For ubuntu 11.04 run:

`$ sudo apt-get install git-core make g++ gcc gawk pmount libtool nasm yasm automake cmake gperf zip unzip bison libltdl-dev libsdl-dev libsdl-image1.2-dev libsdl-gfx1.2-dev libsdl-mixer1.2-dev libfribidi-dev liblzo2-dev libfreetype6-dev libsqlite3-dev libogg-dev libasound2-dev python-sqlite libglew-dev libcurl3 libcurl4-gnutls-dev libxrandr-dev libxrender-dev libmad0-dev libogg-dev libvorbisenc2 libsmbclient-dev libmysqlclient-dev libpcre3-dev libdbus-1-dev libhal-dev libhal-storage-dev libjasper-dev libfontconfig-dev libbz2-dev libboost-dev libenca-dev libxt-dev libxmu-dev libpng-dev libjpeg-dev libpulse-dev mesa-utils libcdio-dev libsamplerate-dev libmpeg3-dev libflac-dev libiso9660-dev libass-dev libssl-dev fp-compiler gdc libmpeg2-4-dev libmicrohttpd-dev libmodplug-dev libssh-dev gettext cvs python-dev libyajl-dev libboost-thread-dev libplist-dev libusb-dev libudev-dev autopoint`


7. Configure, make and install xbmc
   `$ ./bootstrap`
   `$ ./configure`
   `$ make`
   `$ sudo make install`

8. Start xbmc
   `$ xbmc`

9. Start spotyXBMC
   start the preinstalled music addon spotyXBMC and set the settings

10. Restart XBMC

11. Enable the music library and enjoy spotify music inside xbmc
    The spotify music is located inside the normal music categories, do not try to start the spotyXBMC addon!

Done!

Source
------
The spotify related code lives all in xbmc/music/spotyXBMC/ and can (fairly) easely be extracted and used in other applications.

Added files:

* xbmc/music/spotyXBMC/Addon.music.spotify.cpp
* xbmc/music/spotyXBMC/Addon.music.spotify.h
* xbmc/music/spotyXBMC/Logger.cpp
* xbmc/music/spotyXBMC/Logger.h
* xbmc/music/spotyXBMC/SxSettings.cpp
* xbmc/music/spotyXBMC/SxSettings.h
* xbmc/music/spotyXBMC/Utils.cpp
* xbmc/music/spotyXBMC/Utils.h
* xbmc/music/spotyXBMC/radio/SxRadio.cpp
* xbmc/music/spotyXBMC/radio/SxRadio.h
* xbmc/music/spotyXBMC/radio/RadioHandler.cpp
* xbmc/music/spotyXBMC/radio/RadioHandler.h
* xbmc/music/spotyXBMC/album/SxAlbum.cpp
* xbmc/music/spotyXBMC/album/SxAlbum.h
* xbmc/music/spotyXBMC/album/AlbumStore.cpp
* xbmc/music/spotyXBMC/album/AlbumStore.h
* xbmc/music/spotyXBMC/album/AlbumContainer.cpp
* xbmc/music/spotyXBMC/album/AlbumContainer.h
* xbmc/music/spotyXBMC/artist/SxArtist.cpp
* xbmc/music/spotyXBMC/artist/SxArtist.h
* xbmc/music/spotyXBMC/artist/ArtistStore.cpp
* xbmc/music/spotyXBMC/artist/ArtistStore.h
* xbmc/music/spotyXBMC/artist/ArtistContainer.cpp
* xbmc/music/spotyXBMC/artist/ArtistContainer.h
* xbmc/music/spotyXBMC/search/Search.cpp
* xbmc/music/spotyXBMC/search/Search.h
* xbmc/music/spotyXBMC/search/SearchHandler.cpp
* xbmc/music/spotyXBMC/search/SearchHandler.h
* xbmc/music/spotyXBMC/search/SearchBackgroundLoader.cpp
* xbmc/music/spotyXBMC/search/SearchBackgroundLoader.h
* xbmc/music/spotyXBMC/player/Codec.cpp
* xbmc/music/spotyXBMC/player/Codec.h
* xbmc/music/spotyXBMC/player/PlayerHandler.cpp
* xbmc/music/spotyXBMC/player/PlayerHandler.h
* xbmc/music/spotyXBMC/playlist/TopLists.cpp
* xbmc/music/spotyXBMC/playlist/TopLists.h
* xbmc/music/spotyXBMC/playlist/SxPlaylist.cpp
* xbmc/music/spotyXBMC/playlist/SxPlaylist.h
* xbmc/music/spotyXBMC/playlist/StarredList.cpp
* xbmc/music/spotyXBMC/playlist/StarredList.h
* xbmc/music/spotyXBMC/playlist/StarredBackgroundLoader.cpp
* xbmc/music/spotyXBMC/playlist/StarredBackgroundLoader.h
* xbmc/music/spotyXBMC/playlist/PlaylistStore.cpp
* xbmc/music/spotyXBMC/playlist/PlaylistStore.h
* xbmc/music/spotyXBMC/session/Session.cpp
* xbmc/music/spotyXBMC/session/Session.h
* xbmc/music/spotyXBMC/session/SessionCallbacks.cpp
* xbmc/music/spotyXBMC/session/SessionCallbacks.h
* xbmc/music/spotyXBMC/session/BackgroundThread.cpp
* xbmc/music/spotyXBMC/session/BackgroundThread.h
* xbmc/music/spotyXBMC/thumb/SxThumb.cpp
* xbmc/music/spotyXBMC/thumb/SxThumb.h
* xbmc/music/spotyXBMC/thumb/ThumbStore.cpp
* xbmc/music/spotyXBMC/thumb/ThumbStore.h
* xbmc/music/spotyXBMC/track/SxTrack.cpp
* xbmc/music/spotyXBMC/track/SxTrack.h
* xbmc/music/spotyXBMC/track/TrackStore.cpp
* xbmc/music/spotyXBMC/track/TrackStore.h
* xbmc/music/spotyXBMC/track/TrackContainer.cpp
* xbmc/music/spotyXBMC/track/TrackContainer.h

* addons/plugin.music.spotyXBMC/icon.png
* addons/plugin.music.spotyXBMC/fanart.jpg
* addons/plugin.music.spotyXBMC/default.py
* addons/plugin.music.spotyXBMC/changelog.txt
* addons/plugin.music.spotyXBMC/addon.xml
* addons/plugin.music.spotyXBMC/resources/settings.xml
* addons/plugin.music.spotyXBMC/resources/language/English/strings.xml
* addons/skin.confluence/media/flagging/audio/spotify.png

Modified files:

* xbmc/cores/paplayer/CodecFactory.cpp
* xbmc/filesystem/MusicSearchDirectory.cpp
* xbmc/MusicDatabaseDirectory/DirectoryNodeAlbum.cpp
* xbmc/MusicDatabaseDirectory/DirectoryNodeArtist.cpp
* xbmc/MusicDatabaseDirectory/DirectoryNodeOverview.cpp
* xbmc/MusicDatabaseDirectory/DirectoryNodeSong.cpp
* xbmc/MusicDatabaseDirectory/DirectoryNodeTop100.cpp
* xbmc/music/windows/GUIWindowMusicBase.cpp
* xbmc/music/Windows/GuiWindowMusicNav.cpp
* xbmc/music/Makefile
* xbmc/settings/Settings.cpp
* xbmc/Application.cpp
* xbmc/GUIInfoManager.cpp

Buy me a beer?
-------------
<a href='http://www.pledgie.com/campaigns/15827'><img alt='Click here to lend your support to: spotyXBMC2 and make a donation at www.pledgie.com !' src='http://www.pledgie.com/campaigns/15827.png?skin_name=chrome' border='0' /></a>
Contact
-------

http://github.com/akezeke/spotyxbmc
david.erenger@gmail.com

/David


