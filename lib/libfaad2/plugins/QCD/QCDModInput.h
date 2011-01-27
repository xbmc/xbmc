//-----------------------------------------------------------------------------
//
// File:	QCDModInput.h
//
// About:	Input plugin module interface.  This file is published with the 
//			Input plugin SDK.
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

#ifndef QCDMODINPUT_H
#define QCDMODINPUT_H

#include "QCDModDefs.h"

// name of the DLL export for input plugins
#define INPUTDLL_ENTRY_POINT		QInputModule2	// (updated plugin api version 240+)

// media insert flags
#define MEDIAINSERT_PLAY			0x1
#define MEDIAINSERT_ADDTRACKS		0x2
#define MEDIAINSERT_ADDSEGMENTS		0x4
#define MEDIAINSERT_CLEARPLAYLIST	0x8

// Stop will receive one of these flags (pass to output plugin's stop())
#define STOPFLAG_FORCESTOP			0	// stop occuring due to user action or other event
#define STOPFLAG_PLAYDONE			1	// stop occuring due to playlist completion

// play flags
#define PLAYFLAG_PLAYBACK			0x0
#define PLAYFLAG_ENCODING			0x1
#define PLAYFLAG_SEEKING			0x2

// Wave Marker flags
#define WAVE_VIS_DATA_ONLY			-1	// set to WaveDataStruct.markerstart in OutputWrite() call have data only go to vis 
										// and not to output plugin
// pause flags
#define PAUSE_DISABLED				0	// Pause() call is to unpause playback
#define PAUSE_ENABLED				1	// Pause() call is to pause playback

//-----------------------------------------------------------------------------
// Input Module
//-----------------------------------------------------------------------------
typedef struct 
{
	unsigned int		size;			// size of init structure
	unsigned int		version;		// plugin structure version (set to PLUGIN_API_VERSION)
	PluginServiceFunc	Service;		// player supplied services callback

	struct
	{
		void (*PositionUpdate)(unsigned int position);
		void (*PlayStopped)(const char* medianame);					// notify player of play stop
		void (*PlayStarted)(const char* medianame);					// notify player of play start
		void (*PlayPaused)(const char* medianame, int flags);		// notify player of play pause
		void (*PlayDone)(const char* medianame);					// notify player when play done
		void (*PlayTrackChanged)(const char* medianame);			// notify player when playing track changes (cd audio relevant only)
		void (*MediaEjected)(const char* medianame);				// notify player of media eject (cd audio relevant)
		void (*MediaInserted)(const char* medianame, int flags);	// notify player of media insert (cd audio relevant)

																	// output plugin calls
		int  (*OutputOpen)(const char* medianame, WAVEFORMATEX*);	// open output for wave data
		int  (*OutputWrite)(WriteDataStruct*);						// send PCM audio data to output 
																		// (blocks until write completes, thus if output is paused can 
																		// block until unpaused)
		int  (*OutputDrain)(int flags);								// wait for all output to complete (blocking)
		int  (*OutputDrainCancel)(int flags);						// break a drain in progress
		int  (*OutputFlush)(unsigned int marker);					// flush output upto marker
		int  (*OutputStop)(int flags);								// stop output
		int  (*OutputPause)(int flags);								// pause output

		int  (*OutputSetVol)(int levelleft, int levelright, int flags);
		int  (*OutputGetCurrentPosition)(unsigned int *position, int flags);

		void *Reserved[10];
	} toPlayer;

	struct 
	{
		int  (*Initialize)(QCDModInfo *modInfo, int flags);			// initialize plugin
		void (*ShutDown)(int flags);								// shutdown plugin

		int  (*Play)(const char* medianame, int playfrom, int playto, int flags);	// start playing playfrom->playto
		int  (*Stop)(const char* medianame, int flags);				// stop playing
		int  (*Pause)(const char* medianame, int flags);			// pause playback
		int  (*Eject)(const char* medianame, int flags);			// eject media
		void (*SetEQ)(EQInfo*);										// update EQ settings

		int  (*GetMediaSupported)(const char* medianame, MediaInfo *mediaInfo);			// does plugin support medianame (and provides info for media)
		int  (*GetTrackExtents)(const char* medianame, TrackExtents *ext, int flags);	// get media start, end & units
		int  (*GetCurrentPosition)(const char* medianame, long *track, long *offset);	// get playing media's position

		void (*Configure)(int flags);									// launch configuration
		void (*About)(int flags);										// launch about info

		void (*SetVolume)(int levelleft, int levelright, int flags);	// level 0 - 100

		void *Reserved[10];
	} toModule;

} QCDModInitIn;

#endif //QCDMODINPUT_H
