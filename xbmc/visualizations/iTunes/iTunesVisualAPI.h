//
// File:       iTunesVisualAPI.h
//
// Abstract:   part of iTunes Visual SDK
//
// Version:    1.2
//
// Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple Inc. ( "Apple" )
//             in consideration of your agreement to the following terms, and your use,
//             installation, modification or redistribution of this Apple software
//             constitutes acceptance of these terms.  If you do not agree with these
//             terms, please do not use, install, modify or redistribute this Apple
//             software.
//
//             In consideration of your agreement to abide by the following terms, and
//             subject to these terms, Apple grants you a personal, non - exclusive
//             license, under Apple's copyrights in this original Apple software ( the
//             "Apple Software" ), to use, reproduce, modify and redistribute the Apple
//             Software, with or without modifications, in source and / or binary forms;
//             provided that if you redistribute the Apple Software in its entirety and
//             without modifications, you must retain this notice and the following text
//             and disclaimers in all such redistributions of the Apple Software. Neither
//             the name, trademarks, service marks or logos of Apple Inc. may be used to
//             endorse or promote products derived from the Apple Software without specific
//             prior written permission from Apple.  Except as expressly stated in this
//             notice, no other rights or licenses, express or implied, are granted by
//             Apple herein, including but not limited to any patent rights that may be
//             infringed by your derivative works or by other works in which the Apple
//             Software may be incorporated.
//
//             The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
//             WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
//             WARRANTIES OF NON - INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A
//             PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION
//             ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
//
//             IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
//             CONSEQUENTIAL DAMAGES ( INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//             SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//             INTERRUPTION ) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION
//             AND / OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER
//             UNDER THEORY OF CONTRACT, TORT ( INCLUDING NEGLIGENCE ), STRICT LIABILITY OR
//             OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright ( C ) 2000-2007 Apple Inc. All Rights Reserved.
//
#ifndef ITUNESVISUALAPI_H_
#define ITUNESVISUALAPI_H_

#include "iTunesAPI.h"

#if PRAGMA_ONCE
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_STRUCT_ALIGN
    #pragma options align=power
#elif PRAGMA_STRUCT_PACKPUSH
    #pragma pack(push, 4)
#elif PRAGMA_STRUCT_PACK
    #pragma pack(4)
#endif

struct ITTrackInfoV1 {
	ITTIFieldMask		validFields;
	UInt32				reserved;						/* Must be zero */
	
	Str255				name;	
	Str255				fileName;
	Str255				artist;
	Str255				album;
		
	Str255				genre;
	Str255				kind;
	
	UInt32				trackNumber;
	UInt32				numTracks;
		
	UInt16				year;
	SInt16				soundVolumeAdjustment;			/* Valid range is -255 to +255 */
	
	Str255				eqPresetName;
	Str255				comments;
	
	UInt32				totalTimeInMS;
	UInt32				startTimeInMS;
	UInt32				stopTimeInMS;

	UInt32				sizeInBytes;

	UInt32				bitRate;
	UInt32				sampleRateFixed;

	OSType				fileType;
	
	UInt32				date;
	UInt32				unusedReserved2;				/* Must be zero */
	
	ITTrackAttributes	attributes;
	ITTrackAttributes	validAttributes;				/* Mask indicating which attributes are applicable */

	OSType				fileCreator;
};
typedef struct ITTrackInfoV1 ITTrackInfoV1;

enum {
	kCurrentITStreamInfoVersion = 1
};

struct ITStreamInfoV1 {
	SInt32				version;
	Str255				streamTitle;
	Str255				streamURL;
	Str255				streamMessage;
};
typedef struct ITStreamInfoV1 ITStreamInfoV1;


enum {
	kITVisualPluginMajorMessageVersion = 10,
	kITVisualPluginMinorMessageVersion = 7
};

enum {
	/* VisualPlugin messages */
	
	kVisualPluginIdleMessage			= 'null',

	kVisualPluginInitMessage			= 'init',
	kVisualPluginCleanupMessage			= 'clr ',
	
	kVisualPluginConfigureMessage		= 'cnfg',	/* Configure the plugin (may not be enabled) */
	
	kVisualPluginEnableMessage			= 'von ',	/* Turn on the module (automatic)*/
	kVisualPluginDisableMessage			= 'voff',	/* Turn off the module */
	
	kVisualPluginShowWindowMessage		= 'show',	/* Show the plugin window (allocate large memory here!) */
	kVisualPluginHideWindowMessage		= 'hide',	/* Hide the plugin window (deallocate large memory here!) */
		
	kVisualPluginSetWindowMessage		= 'swin',	/* Change the window parameters */

	kVisualPluginRenderMessage			= 'vrnd',	/* Render to window */
	
	kVisualPluginUpdateMessage			= 'vupd',	/* Update the window */
	
	kVisualPluginPlayMessage			= 'vply',	/* Playing a track */
	kVisualPluginChangeTrackMessage		= 'ctrk',	/* Change track (for CD continuous play) or info about currently playing track has changed */
	kVisualPluginStopMessage			= 'vstp',	/* Stopping a track */
	kVisualPluginSetPositionMessage		= 'setp',	/* Setting the position of a track */
	
	kVisualPluginPauseMessage			= 'vpau',	/* Pausing a track (unused - Pause is stop) */
	kVisualPluginUnpauseMessage			= 'vunp',	/* Unpausing a track (unused - Pause is stop) */
	
	kVisualPluginEventMessage			= 'vevt',	/* Mac-event. */
	
	kVisualPluginDisplayChangedMessage	= 'dchn'	/* Something about display state changed */
};

/*
	VisualPlugin messages
*/

enum {
	kVisualMaxDataChannels		= 2,

	kVisualNumWaveformEntries	= 512,
	kVisualNumSpectrumEntries	= 512
};

enum {
	/* Set/ShowWindow options */
	
	kWindowIsFullScreen = (1L << 0),
	kWindowIsStretched	= (1L << 1)
};

enum {
	/* Initialize options */
	
	kVisualDoesNotNeedResolutionSwitch		= (1L << 0),		/* Added in 7.0 */
	kVisualDoesNotNeedErase				 	= (1L << 1)			/* Added in 7.0 */
};

struct RenderVisualData {
	UInt8							numWaveformChannels;
	UInt8							waveformData[kVisualMaxDataChannels][kVisualNumWaveformEntries];
	
	UInt8							numSpectrumChannels;
	UInt8							spectrumData[kVisualMaxDataChannels][kVisualNumSpectrumEntries];
};
typedef struct RenderVisualData RenderVisualData;

struct VisualPluginInitMessage {
	UInt32							messageMajorVersion;	/* Input */
	UInt32							messageMinorVersion;	/* Input */
	NumVersion						appVersion;				/* Input */

	void *							appCookie;				/* Input */
	ITAppProcPtr					appProc;				/* Input */

	OptionBits						options;				/* Output */
	void *							refCon;					/* Output */
};
typedef struct VisualPluginInitMessage VisualPluginInitMessage;

struct VisualPluginShowWindowMessage {
	GRAPHICS_DEVICE					GRAPHICS_DEVICE_NAME;	/* Input */
	Rect							drawRect;				/* Input */
	OptionBits						options;				/* Input */
	Rect							totalVisualizerRect;	/* Input -- Added in 7.0 */
};
typedef struct VisualPluginShowWindowMessage VisualPluginShowWindowMessage;

struct VisualPluginSetWindowMessage {
	GRAPHICS_DEVICE					GRAPHICS_DEVICE_NAME;	/* Input */
	Rect							drawRect;				/* Input */
	OptionBits						options;				/* Input */
	Rect							totalVisualizerRect;	/* Input -- Added in 7.0 */
};
typedef struct VisualPluginSetWindowMessage VisualPluginSetWindowMessage;

struct VisualPluginPlayMessage {
	ITTrackInfoV1 *					trackInfo;				/* Input */
	ITStreamInfoV1 *				streamInfo;				/* Input */
	SInt32							volume;					/* Input */
	
	UInt32							bitRate;				/* Input */
	
	SoundComponentData				oldSoundFormat;			/* Input -- deprecated in 7.1 */
	ITTrackInfo *					trackInfoUnicode;		/* Input */
	ITStreamInfo *					streamInfoUnicode;		/* Input */
	AudioStreamBasicDescription		audioFormat;			/* Input -- added in 7.1 */
};
typedef struct VisualPluginPlayMessage VisualPluginPlayMessage;

struct VisualPluginChangeTrackMessage {
	ITTrackInfoV1 *					trackInfo;				/* Input */
	ITStreamInfoV1 *				streamInfo;				/* Input */
	ITTrackInfo *					trackInfoUnicode;		/* Input */
	ITStreamInfo *					streamInfoUnicode;		/* Input */
};
typedef struct VisualPluginChangeTrackMessage VisualPluginChangeTrackMessage;

struct VisualPluginRenderMessage {
	RenderVisualData *				renderData;				/* Input */
	UInt32							timeStampID;			/* Input */
	UInt32							currentPositionInMS;	/* Input -- added in 4.7 */
};
typedef struct VisualPluginRenderMessage VisualPluginRenderMessage;

struct VisualPluginSetPositionMessage {
	UInt32							positionTimeInMS;		/* Input */
};
typedef struct VisualPluginSetPositionMessage VisualPluginSetPositionMessage;

#if TARGET_OS_MAC
struct VisualPluginEventMessage {
	EventRecord *					event;					/* Input */
};
#endif
typedef struct VisualPluginEventMessage VisualPluginEventMessage;

enum {
	kVisualDisplayDepthChanged 	= 1 << 0,					/* the display's depth has changed */
	kVisualDisplayRectChanged	= 1 << 1,					/* the display's location changed */
	kVisualWindowMovedMoved 	= 1 << 2,					/* the window has moved location */	
	kVisualDisplayConfigChanged	= 1 << 3,					/* something else about the display changed */
};

struct VisualPluginDisplayChangedMessage {
	UInt32							flags;		/* Input */
};
typedef struct VisualPluginDisplayChangedMessage VisualPluginDisplayChangedMessage;

struct VisualPluginIdleMessage {
	UInt32							timeBetweenDataInMS;	/* Output -- added in 4.8 */
};
typedef struct VisualPluginIdleMessage VisualPluginIdleMessage;

struct VisualPluginMessageInfo {
	union {
		VisualPluginInitMessage				initMessage;
		VisualPluginShowWindowMessage		showWindowMessage;
		VisualPluginSetWindowMessage		setWindowMessage;
		VisualPluginPlayMessage				playMessage;
		VisualPluginChangeTrackMessage		changeTrackMessage;
		VisualPluginRenderMessage			renderMessage;
		VisualPluginSetPositionMessage		setPositionMessage;
#if TARGET_OS_MAC
		VisualPluginEventMessage			eventMessage;
#endif
		VisualPluginDisplayChangedMessage	displayChangedMessage;
		VisualPluginIdleMessage				idleMessage;
	} u;
};
typedef struct VisualPluginMessageInfo VisualPluginMessageInfo;

#if PRAGMA_STRUCT_ALIGN
    #pragma options align=reset
#elif PRAGMA_STRUCT_PACKPUSH
    #pragma pack(pop)
#elif PRAGMA_STRUCT_PACK
    #pragma pack()
#endif

#ifdef __cplusplus
}
#endif

#endif /* ITUNESVISUALAPI_H_ */
