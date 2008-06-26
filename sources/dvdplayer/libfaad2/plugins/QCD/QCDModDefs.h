//-----------------------------------------------------------------------------
//
// File:	QCDModDefs.h
//
// About:	Module definitions file.  Miscellanious definitions used by different
//			module types.  This file is published with the plugin SDKs.
//
// Authors:	Written by Paul Quinn and Richard Carlson.
//
// Copyright:
//
//	QCD multimedia player application Software Development Kit Release 1.0.
//
//	Copyright (C) 1997-2002 Quinnware
//
//	This code is free.  If you redistribute it in any form, leave this notice 
//	here.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//-----------------------------------------------------------------------------

#ifndef QCDMODDEFS_H
#define QCDMODDEFS_H

#include <windows.h>

#ifdef __cplusplus
#define PLUGIN_API extern "C" __declspec(dllexport)
#else
#define PLUGIN_API __declspec(dllexport)
#endif

// Current plugin version
#define PLUGIN_API_VERSION 250

//-----------------------------------------------------------------------------

typedef struct {
	char				*moduleString;
	char				*moduleExtensions;

} QCDModInfo;

//-----------------------------------------------------------------------------
// Services (ops) provided by the Player
//-----------------------------------------------------------------------------
typedef enum 
{									//*** below returns numeric info (*buffer not used)

	opGetPlayerVersion = 0,			// high-order word = major version (eg 3.01 is 3), low-order word = minor (eg 3.01 = 1)
	opGetParentWnd = 1,				// handle to player window
	opGetPlayerInstance = 2,		// HINSTANCE to player executable

	opGetPlayerState = 9,			// get current state of the player (returns: 1 = stopped, 2 = playing, 3 = paused, 0 = failed)
	opGetNumTracks = 10,			// number of tracks in playlist
	opGetCurrentIndex = 11,			// index of current track in playlist (0 based)
	opGetNextIndex = 12,			// get index of next track to play (0 based), param1 = index start index. -1 for after current
	opGetTrackNum = 13,				// get track number of index, param1 = index of track in playlist, -1 for current
									//		- 'track number' is the number of the track in it's respective album, as opposed to playlist number
									//		- the 'track number' for digital files will be 1, unless they are tagged with the CDDB identifier

	opGetTrackLength = 14,			// get track length, param1 = index of track in playlist, -1 for current
									//                   param2 = 0 for seconds, 1 for milliseconds
	opGetTime = 15,					// get time on player, param1 = 0 for time displayed, 1 for track time, 2 for playlist time
									//					   param2 = 0 for elapsed, 1 for remaining														   
	opGetTrackState = 16,			// get whether track is marked, param1 = index of track, -1 for current
	opGetPlaylistNum = 17,			// get playlist number of index, param1 = index of track in playlist, -1 for current
	opGetMediaType = 18,			// get media type of track, param1 = index if track in playlist, -1 for current
									//		- see MediaTypes below for return values

	opGetAudioInfo = 19,			// get format info about currently playing track
									//		- param1 = 0 for samplerate, 1 for bitrate, 2 for num channels

	opGetOffline = 20,				// true if client is in Offline Mode
	opGetVisTarget = 21,			// where is vis being drawn > 0 - internal to skin, 1 - external window, 2 - full screen
	opGetAlwaysOnTop = 22,			// true if player is set to 'Always on Top'
	opGetRepeatState = 23,			// returns: 0 - repeat off, 1 - repeat track, 2 - repeat all
	opGetShuffleState = 27,			// returns: 0 - shuffle off, 1 - shuffle enabled

	opGetTimerState = 24,			// low-order word: 0 - track ascend, 1 - playlist ascend, 2 - track descend, 3 - playlist descend
									// hi-order word: 1 if 'show hours' is set, else 0

	opGetVolume = 25,				// get master volume level (0 - 100), param1: 0 = combined, 1 = left, 2 = right
	opSetVolume = 26,				// set master volume level, param1: vol level 0 - 100, param2: balance (-100 left, 0 center, 100 right)

	opGetIndexFromPLNum = 28,		// get index from playlist number, param1 = playlist number

	opGetChildWnd = 30,				// handle to the draggable window extension (only available on some skins) 
	opGetExtVisWnd = 31,			// handle to the external visual window
	opGetMusicBrowserWnd = 32,		// handle to the music browser window 
	opGetSkinPreviewWnd = 33,		// handle to the skin preview window 
	opGetPropertiesWnd = 34,		// handle to the player properties window 
	opGetExtInfoWnd = 35,			// handle to the extended information window 
	opGetAboutWnd = 36,				// handle to the about window 
	opGetSegmentsWnd = 37,			// handle to the segments window 
	opGetEQPresetsWnd = 38,			// handle to the EQ presets window 

	opGetVisDimensions = 50,		// gets the width and height of visual window (param1 = -1 current vis window, 0 internal vis, 1 external vis, 2 full screen)
									//		returns: HEIGHT in high word, WIDTH in low word 

	opGetQueriesComplete = 60,		// get status on whether all tracks in playlist have been queryied for their info

									// playlist manipulation
	opDeleteIndex = 90,				// delete index from playlist (param1 = index)
	opSelectIndex = 91,				// mark index as selected (param1 = index, param2 = 1 - set, 0 - unset)
	opBlockIndex = 92,				// mark index as blocked (param1 = index, param2 = 1 - set, 0 - unset)

	opGetMediaInfo = 99,			// get the CddbDisc object for the index specified, param1 = index of track, -1 for current
									//		param2 = pointer to integer that receives track value
									//		returns: pointer to CddbDisc object. Do not release or deallocate this pointer


   									//*** below returns string info in buffer, param1 = size of buffer
   									//*** returns 1 on success, 0 on failure

	opGetTrackName = 100,			// get track name, param2 = index of track in playlist, -1 for current
	opGetArtistName = 101,			// get artist name, param2 = index of track in playlist, -1 for current
	opGetDiscName = 102,			// get disc name, param2 = index of track in playlist, -1 for current

	opGetTrackFile = 103,			// file name of track in playlist, param2 = index of track in playlist, -1 for current
	opGetSkinName = 104,			// get current skin name

	opGetPluginFolder = 105,		// get current plugin folder
	opGetPluginSettingsFile = 106,	// get settings file (plugins.ini) that plugin should save settings to
	opGetPluginCacheFile = 107,		// get file that describes plugin validity, functions and names
	opGetPlayerSettingsFile = 108,	// get settings file (qcd.ini) that player saves it settings to (should use for read-only)

	opGetMusicFolder = 110,			// get current music folder
	opGetPlaylistFolder = 111,		// get current playlist folder
	opGetSkinFolder = 112,			// get current skin folder
	opGetCDDBCacheFolder = 113,		// get current folder for CDDB cached info

	opGetCurrentPlaylist = 114,		// get full pathname of playlist currently loaded 

	opGetMediaID = 115,				// get media identifier, param2 = index of track in playlist, -1 for current
									//		- for CD's it's the TOC - for anything else, right now it's 0      

	opGetSupportedExtensions = 116,	// get file extensions supported by the player, param2 = 0 - get all extensions, 1 - get registered extensions
									//		- returned extensions will be colon delimited


   									//*** below buffer points to struct or other object
   									//*** returns 1 on success, 0 on failure

	opShowMainMenu = 120,			// Display Main QCD Menu (buffer = POINT* - location to display menu)
	opGetMainMenu = 121,			// Returns copy of HMENU handle to QCD Menu (must use DestroyMenu on handle when complete)

	opShowQuickTrack = 125,			// Display QuickTrack Menu (buffer = POINT* - location to display menu)

	opGetEQVals = 200,				// get current EQ levels/on/off (buffer = EQInfo*)
	opSetEQVals = 201,				// set EQ levels/on/off (buffer = EQInfo*)

	opGetProxyInfo = 202,			// get proxy info (buffer = ProxyInfo*), returns 0 if proxy not in use


									//*** below returns numeric info, buffer used

	opGetIndexFromFilename = 210,	// get the index of a file that exists in current playlist (buffer = full path of file),
									//		param1 = startindex (index to start searching on)
                                    //		returns -1 if file not in playlist


									//*** below send information to player
									//*** returns 1 on success, 0 on failure

	opSetStatusMessage = 1000,		// display message in status area (buffer = msg buffer (null term), param1 = text flags (see below))

	opSetBrowserUrl = 1001,			// set music browser URL (buffer = url (null term))
	                                //		null url buffer - closes browser
	                                //		param1 = 0 - normal, 1 - force open

	opSetAudioInfo = 1002,			// set the current music bitrate/khz (buffer = AudioInfo*, param1 = size of AudioInfo)

	opSetTrackAlbum = 1003,			// update track ablum name (buffer = album (null term), param1 = (string ptr)file name), param2 = MediaTypes
	opSetTrackTitle = 1004,			// update track title (buffer = title (null term), param1 = (string ptr)file name), param2 = MediaTypes
	opSetTrackArtist = 1005,		// update track artist name (buffer = artist (null term), param1 = (string ptr)file name), param2 = MediaTypes

	opSetPlaylist = 1006,			// add files to or reset playlist with new files (buffer = file list (null term), param1 = (string ptr)originating path (can be NULL), param2 = 1 - clear playlist flag, 2 - enqueue to top

	opSetTrackExtents = 1007,		// update track TrackExtents info (buffer = &TrackExtents), param1 = (string ptr)file name)
	opSetTrackSeekable = 1008,		// update track seekable flag (buffer = (string ptr)file name), param1 = TRUE/FALSE
	opSetPlayNext = 1009,			// set the next index to be played (buffer = NULL, param1 = index, index = -1 unsets playnext)
	opSetIndexFilename = 1010,		// updates the filename (or stream) that an index in the current playlist refers to, buffer = new filename, param1 = index

	opSetSeekPosition = 1100,		// seek to position during playback (buffer = NULL, param1 = position, param2 = 0 - position is in seconds, 1 - position is in milliseconds, 2 - position is in percent (use (float)param1))


									//*** below configures custom plugin menu items for the 'plugin menu'
									//*** Player will call plugin's configure routine with menu value when menu item selected
									//*** returns 1 on success, 0 on failure

	opSetPluginMenuItem = 2000,		// buffer = HINSTANCE of plugin, param1 = item id, param2 = (string ptr)string to display
									//		- set param2 = 0 to remove item id from menu
									//		- set param1 = 0 and param2 = 0 to remove whole menu
	opSetPluginMenuState = 2001,	// buffer = HINSTANCE of plugin, param1 = item id, param2 = menu flags (same as windows menu flags - eg: MF_CHECKED)


									//*** other services

	opSafeWait = 10000				// plugin's can use this to wait on an object without worrying about deadlocking the player.
									// this should only be called by the thread that enters the plugin, not by any plugin-created threads

} PluginServiceOp;

//-----------------------------------------------------------------------------
// Info services api provided by the Player, called by Plugin.
//-----------------------------------------------------------------------------
typedef long (*PluginServiceFunc)(PluginServiceOp op, void *buffer, long param1, long param2);


//-----------------------------------------------------------------------------
typedef struct				// for Output Plugin Write callback
{
	void	*data;			// pointer to valid data
	int		bytelen;		// length of data pointed to by 'data' in bytes
	UINT	numsamples;		// number of samples represented by 'data'
	UINT	bps;			// bits per sample
	UINT	nch;			// number of channels
	UINT	srate;			// sample rate

	UINT	markerstart;	// Marker position at start of data (marker is time value of data) 
							// (set to WAVE_VIS_DATA_ONLY to not have data sent to output plugins)
	UINT	markerend;		// Marker position at end of data (not currently used, set to 0)
} WriteDataStruct;

//-----------------------------------------------------------------------------
typedef struct			// for GetTrackExtents Input Plugin callback
{
	UINT track;			// for CD's, set the track number. Otherwise set to 1.
	UINT start;			// for CD's or media that doesn't start at the beginning 
						// of the file, set to start position. Otherwise set to 0.
	UINT end;			// set to end position of media.
	UINT unitpersec;	// whatever units are being used for this media, how many
						// of them per second. 
						// (Note: ((end - start) / unitpersecond) = file length
	UINT bytesize;		// size of file in bytes (if applicable, otherwise 0).
} TrackExtents;

//-----------------------------------------------------------------------------
typedef struct			// for opSetAudioInfo service
{		
    long struct_size;	// sizeof(AudioInfo)
    long level;			// MPEG level (1 for MPEG1, 2 for MPEG2, 3 for MPEG2.5, 7 for MPEGpro)
    long layer;			// and layer (1, 2 or 3)
    long bitrate;		// audio bitrate in bits per second
    long frequency;		// audio freq in Hz
    long mode;			// 0 for stereo, 1 for joint-stereo, 2 for dual-channel, 3 for mono, 4 for multi-channel
} AudioInfo;

//-----------------------------------------------------------------------------
// Equalizer Info
//-----------------------------------------------------------------------------
typedef struct			// for coming QCD version
{
	long struct_size;	// sizeof(EQInfo)
	char enabled;		
	char preamp;		// -128 to 127, 0 is even
	char bands[10];		// -128 to 127, 0 is even
} EQInfo;

//-----------------------------------------------------------------------------
typedef struct
{
	long struct_size;	// sizeof(ProxyInfo)
	char hostname[200];
	long port;
	char username[100];
	char password[100];
} ProxyInfo;

//-----------------------------------------------------------------------------
typedef enum			// for MediaInfo.mediaType
{ 
	UNKNOWN_MEDIA = 0,
	CD_AUDIO_MEDIA = 1,
	DIGITAL_FILE_MEDIA = 2,
	DIGITAL_STREAM_MEDIA = 3
} MediaTypes;

//-----------------------------------------------------------------------------
#define MAX_TOC_LEN				2048
typedef struct
{
	// media descriptors
	CHAR		mediaFile[MAX_PATH];
	MediaTypes	mediaType;

	// cd audio media info
	CHAR		cd_mediaTOC[MAX_TOC_LEN];
	int			cd_numTracks;
	int			cd_hasAudio;

	// operation info
	int			op_canSeek;

	// not used
	int			reserved[4];

} MediaInfo;


//-----------------------------------------------------------------------------
// When subclassing the parent window, a plugin can watch for these messages
// to react to events going on between plugins and player
// DO NOT SEND THESE MESSAGES - can only watch for them

// Plugin to Player Notifiers
#define WM_PN_POSITIONUPDATE	(WM_USER + 100)	// playback progress updated
#define WM_PN_PLAYSTARTED		(WM_USER + 101)	// playback has started
#define WM_PN_PLAYSTOPPED		(WM_USER + 102) // playback has stopped by user
#define WM_PN_PLAYPAUSED		(WM_USER + 103) // playback has been paused
#define WM_PN_PLAYDONE			(WM_USER + 104) // playback has finished (track completed)
#define WM_PN_MEDIAEJECTED		(WM_USER + 105) // a CD was ejected (lParam = (LPCSTR)medianame)
#define WM_PN_MEDIAINSERTED		(WM_USER + 106) // a CD was inserted (lParam = (LPCSTR)medianame)
#define WM_PN_INFOCHANGED		(WM_USER + 107) // track information was updated (lParam = (LPCSTR)medianame)
#define WM_PN_TRACKCHANGED		(WM_USER + 109)	// current track playing has changed (relevant from CD plugin) (lParam = (LPCSTR)medianame)

// Player to Plugin Notifiers
#define WM_PN_PLAYLISTCHANGED	(WM_USER + 200) // playlist has changed in some way (add, delete, sort, shuffle, drag-n-drop, etc...)

// For intercepting main menu display
// (so you can get handle, modify, and display your own)
#define WM_SHOWMAINMENU			(WM_USER + 20)

//-----------------------------------------------------------------------------
// To shutdown player, send this command
#define WM_SHUTDOWN				(WM_USER + 5)

//-----------------------------------------------------------------------------
// opSetStatusMessage textflags
#define TEXT_DEFAULT		0x0		// message scrolls by in status window
#define TEXT_TOOLTIP		0x1		// message acts as tooltip in status window
#define TEXT_URGENT			0x2		// forces message to appear even if no status window (using msg box)
#define TEXT_HOLD			0x4		// tooltip message stays up (no fade out)


#endif //QCDMODDEFS_H