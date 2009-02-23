//
// File:       iTunesAPI.h
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
#ifndef ITUNESAPI_H_
#define ITUNESAPI_H_

#if PRAGMA_ONCE
#pragma once
#endif


#if defined(_MSC_VER)
	#define TARGET_OS_MAC		0
	#define TARGET_OS_WIN32		1
#else
	#define TARGET_OS_MAC		1
	#define TARGET_OS_WIN32		0
#endif

#if TARGET_OS_MAC
	#include <Carbon/Carbon.h>
	#include <CoreAudio/CoreAudioTypes.h>
#endif
#if TARGET_OS_WIN32
	#include <windows.h>
#endif
	
#if !defined(__CONDITIONALMACROS__)
typedef unsigned long	UInt32;
typedef signed long		SInt32;
typedef unsigned short	UInt16;
typedef signed short	SInt16;
typedef	unsigned char	UInt8;
typedef signed char		SInt8;
typedef UInt32			OptionBits;

typedef UInt8			Str255[256];
typedef UInt8			Str63[64];
typedef UInt8 *			StringPtr;
typedef const UInt8 *	ConstStringPtr;
typedef UInt32			OSType;

typedef SInt32			OSStatus;

typedef UInt16			EventKind;

typedef UInt16          EventModifiers;

typedef UInt16			UniChar;

typedef double			Float64;


#if TARGET_OS_WIN32
typedef unsigned __int64	UInt64;
#else
typedef unsigned long long	UInt64;
#endif

typedef void **			Handle;

struct NumVersion {
	UInt8			majorRev;
	UInt8			minorAndBugRev;
	UInt8			stage;
	UInt8			nonRelRev;
};
typedef struct NumVersion NumVersion;

struct Point {
    short			v;
    short			h;
};
typedef struct Point Point;

struct Rect {
	short			top;
	short			left;
	short			bottom;
	short			right;
};
typedef struct Rect Rect;

struct EventRecord {
	EventKind		what;
	UInt32			message;
	UInt32			when;
	Point			where;
	EventModifiers	modifiers;
};
typedef struct EventRecord EventRecord;

typedef UInt8			Boolean;

typedef UInt32			UnsignedFixed;

typedef void *			LogicalAddress;

#define	false	0
#define	true	1
#define	nil		NULL

enum {
	noErr			= 0,
	unimpErr		= -4,
	readErr			= -19,
	writErr			= -20,
	openErr			= -23,
	closErr			= -24,
	dirFulErr		= -33,
	dskFulErr		= -34,
	nsvErr			= -35,
	ioErr			= -36,
	bdNamErr		= -37,
	fnOpnErr		= -38,
	eofErr			= -39,
	posErr			= -40,
	tmfoErr			= -42,
	fnfErr			= -43,
	wPrErr			= -44,
	fLckdErr		= -45,
	vLckdErr		= -46,
	fBsyErr			= -47,
	dupFNErr		= -48,
	opWrErr			= -49,
	paramErr		= -50,
	permErr			= -54,
	nsDrvErr		= -56,
	wrPermErr		= -61,
	memFullErr		= -108,
	dirNFErr		= -120,
	badMovErr		= -122
};

enum {
	developStage	= 0x20,
	alphaStage		= 0x40,
	betaStage		= 0x60,
	finalStage		= 0x80
};

struct SoundComponentData {
	long			flags;
	OSType			format;
	short			numChannels;
	short			sampleSize;
	UnsignedFixed	sampleRate;
	long			sampleCount;
	UInt8 *			buffer;
	long			reserved;
};
typedef struct SoundComponentData SoundComponentData;

struct AudioStreamBasicDescription
{
    Float64 mSampleRate;
    UInt32  mFormatID;
    UInt32  mFormatFlags;
    UInt32  mBytesPerPacket;
    UInt32  mFramesPerPacket;
    UInt32  mBytesPerFrame;
    UInt32  mChannelsPerFrame;
    UInt32  mBitsPerChannel;
    UInt32  mReserved;
};
typedef struct AudioStreamBasicDescription AudioStreamBasicDescription;

#endif

#if TARGET_OS_WIN32
#define GRAPHICS_DEVICE				HWND
#define	GRAPHICS_DEVICE_NAME		window
#else
#define GRAPHICS_DEVICE				CGrafPtr
#define	GRAPHICS_DEVICE_NAME		port
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


enum {
	kITPluginMajorMessageVersion = 10,
	kITPluginMinorMessageVersion = 9
};

enum {
	kTrackSupportsID3Tags		= (1L << 0),
	kTrackHasVariableBitRate	= (1L << 1),
	kTrackIsLocked				= (1L << 3),		/* Device tracks only. Track cannot be deleted or renamed */
	kTrackCanBeDownloaded		= (1L << 4),		/* Device tracks only. Track can be copied from device to desktop. */
	kTrackIsHidden				= (1L << 5),		/* Device tracks only. Track should not be displayed in the device window. */
	kTrackHasVideo				= (1L << 6)			/* Track has video track which can be played in iTunes */
};
typedef OptionBits	ITTrackAttributes;

enum {
	/*
		These mask values are specified in ITTrackInfo.validFields
		to indicate which fields contain valid data
	*/
	
	kITTIFieldInvalid				= 0,
	kITTINameFieldMask				= (1L << 0),
	kITTIFileNameFieldMask			= (1L << 1),
	kITTIArtistFieldMask			= (1L << 2),
	kITTIAlbumFieldMask				= (1L << 3),
	kITTIGenreFieldMask				= (1L << 4),
	kITTIKindFieldMask				= (1L << 5),
	kITTITrackNumberFieldsMask		= (1L << 6),
	kITTIYearFieldMask				= (1L << 7),
	kITTISoundVolumeFieldMask		= (1L << 8),
	kITTIEQPresetFieldMask			= (1L << 9),
	kITTICommentsFieldMask			= (1L << 10),
	kITTITotalTimeFieldMask			= (1L << 11),			
	kITTIStartTimeFieldMask			= (1L << 12),
	kITTIStopTimeFieldMask			= (1L << 13),
	kITTISizeFieldMask				= (1L << 14),
	kITTIBitRateFieldMask			= (1L << 15),
	kITTISampleRateFieldMask		= (1L << 16),
	kITTIAttributesFieldMask		= (1L << 17),
	kITTIFileTypeFieldMask			= (1L << 18),
	kITTIDateFieldMask				= (1L << 19),
	kITTIFileCreatorFieldMask		= (1L << 20),
	kITTIComposerFieldMask			= (1L << 21),	/* Added in iTunes 3.0 */
	kITTICompilationFieldMask		= (1L << 22),	/* Added in iTunes 3.0 */
	kITTIDiscNumberFieldsMask		= (1L << 23),	/* Added in iTunes 3.0 */
	kITTITrackRatingFieldMask		= (1L << 24),	/* Added in iTunes 3.0 - used to be called kITTIUserRatingFieldMask */
	kITTIPlayCountFieldMask 		= (1L << 25),	/* Added in iTunes 3.0 */
	kITTILastPlayDateFieldMask		= (1L << 26),	/* Added in iTunes 3.0 */
	kITTIBeatsPerMinuteFieldMask	= (1L << 27),	/* Added in iTunes 4.0 */
	kITTIGroupingFieldMask			= (1L << 28),	/* Added in iTunes 4.2 */
	kITTIGaplessAlbumFieldMask		= (1L << 29),	/* Added in iTunes 7.0 */
	kITTIAlbumArtistFieldMask		= (1L << 30)	/* Added in iTunes 7.0 */
};
typedef OptionBits	ITTIFieldMask;

#define kLastKnownITTIField				kITTIAlbumArtistFieldMask
#define	kAllKnownITTIFieldsMask			((((UInt32) kLastKnownITTIField) << 1) - 1)

enum
{
	kITTIUserModifiableFieldsMask 	= kITTINameFieldMask | kITTIArtistFieldMask | kITTIAlbumFieldMask | kITTIGroupingFieldMask | kITTIGenreFieldMask |
										kITTITrackNumberFieldsMask | kITTIYearFieldMask | kITTISoundVolumeFieldMask | kITTIEQPresetFieldMask |
										kITTICommentsFieldMask | kITTIStartTimeFieldMask | kITTIStopTimeFieldMask | kITTIComposerFieldMask |
										kITTICompilationFieldMask | kITTIDiscNumberFieldsMask | kITTITrackRatingFieldMask | kITTIBeatsPerMinuteFieldMask |
										kITTIGaplessAlbumFieldMask | kITTIAlbumArtistFieldMask
};

typedef UniChar 		ITUniStr255[256];				/* Similar to Str255. First element is length of string in characters. */
typedef UniChar *		ITUniStringPtr;
typedef const UniChar *	ConstITUniStringPtr;

#if TARGET_OS_MAC
	typedef FSRef	ITFileSpec;
#endif

#if TARGET_OS_WIN32
	#define	kITFileSpecMaxPathLength	(MAX_PATH - 1)

	typedef struct ITFileSpec
	{
		UInt16	length;		// Length in characters
		UniChar	fullPath[kITFileSpecMaxPathLength];
	} ITFileSpec;
#endif

struct ITTrackInfo {
	ITTIFieldMask		validFields;
	UInt32				recordLength;					/* Size of this structure in bytes */
	
	ITUniStr255			name;	
	ITUniStr255			fileName;
	ITUniStr255			artist;
	ITUniStr255			album;
	ITUniStr255			genre;
	ITUniStr255			kind;
	ITUniStr255			eqPresetName;
	ITUniStr255			comments;
	
	UInt32				trackNumber;
	UInt32				numTracks;
		
	UInt16				year;
	SInt16				soundVolumeAdjustment;			/* Valid range is -255 to +255 */
		
	UInt32				totalTimeInMS;
	UInt32				startTimeInMS;
	UInt32				stopTimeInMS;

	UInt32				date;
	UInt32				oldSizeInBytes;					/* Deprecated in iTunes 7.1 */

	UInt32				bitRate;
	UInt32				oldSampleRateFixed;				/* Deprecated in iTunes 5.0 */

	OSType				fileType;
	OSType				fileCreator;
	
	ITTrackAttributes	attributes;
	ITTrackAttributes	validAttributes;				/* Mask indicating which attributes are applicable */

	ITUniStr255			composer;						/* Added in iTunes 3.0 */
	
	Boolean				isCompilationTrack;				/* Added in iTunes 3.0 */
	Boolean				partOfGaplessAlbum;				/* Added in iTunes 7.0 (was reserved) */
	
	UInt16				trackRating;						/* Added in iTunes 3.0. 0 = unrated, valid values are 20, 40, 60, 80 and 100. Used to be called userRating */

	UInt16				discNumber;						/* Added in iTunes 3.0 */
	UInt16				numDiscs;						/* Added in iTunes 3.0 */
	
	UInt32				playCount;						/* Added in iTunes 3.0 */
	UInt32				lastPlayDate;					/* Added in iTunes 3.0 */
	
	UInt16				beatsPerMinute;					/* Added in iTunes 4.0 */
	UInt16				reserved;						/* Reserved. Must be zero. */		

	ITUniStr255			grouping;						/* Added in iTunes 4.0 */
	
	float				sampleRateFloat;				/* Added in iTunes 5.0 */

	ITUniStr255			albumArtist;					/* Added in iTunes 7.0 */
	
	UInt64				sizeInBytes;					/* Added in iTunes 7.1 */
};
typedef struct ITTrackInfo ITTrackInfo;

struct ITStreamInfo {
	SInt32				version;
	ITUniStr255			streamTitle;
	ITUniStr255			streamURL;
	ITUniStr255			streamMessage;
};
typedef struct ITStreamInfo ITStreamInfo;

enum {
	/* messages sent to plugin main */
	
	kPluginInitMessage					= 'init',
	kPluginCleanupMessage				= 'clr ',
	kPluginPrepareToQuitMessage			= 'prqt',
	
	kPluginIdleMessage					= 'idle'
};


enum {
	/* PluginInitMessage.options */
	
	kPluginWantsIdleMessages		= (1L << 1),	/* Send idle messages to plugin main */
	kPluginWantsToBeLeftOpen		= (1L << 2),	/* Don't close this plugin just because it didn't register anyone */
	kPluginWantsVolumeMessages		= (1L << 3),	/* The plugin wants to be notified when volumes are mounted/unmounted/renamed */
	kPluginWantsDisplayNotification = (1L << 5)		/* The plugin wants to know when the display depth/size changes */
};


enum {
	/* iTunes API messages */

	kPlayerRegisterVisualPluginMessage	= 'rvis',	/* Register a visual plugin */
	
	/* Available for all plugins */

	kPlayerUnregisterPluginMessage		= 'unrg',	/* Unregister the plugin this comes from */
	kPlayerIdleMessage					= 'idle',	/* Give iTunes some time */
	
	kPlayerShowAboutMessage				= 'abou',	/* Show the about box. */
	kPlayerOpenURLMessage				= 'url ',	/* Open a URL */

	kPlayerSetPluginDataMessage			= 'sprf',	/* Set plugin preferences */
	kPlayerGetPluginDataMessage			= 'gprf',	/* Get plugin preferences */
	
	kPlayerSetPluginNamedDataMessage	= 'snpr',	/* Set plugin named preferenes */
	kPlayerGetPluginNamedDataMessage	= 'gnpr',	/* Get plugin named preferenes */
	
	kPlayerGetFileTrackInfoMessage		= 'gfti',	/* Query iTunes for information about a file */ 
	kPlayerSetFileTrackInfoMessage		= 'sfti',	/* Ask iTunes to set information about a file */ 
	
	kPlayerGetITTrackInfoSizeMessage	= 'itsz',	/* Query iTunes for the sizeof(ITTrackInfo). This allows newer plugins to correctly workd with older versions of iTunes. */

	kPlayerHandleMacOSEventMessage		= 'evnt',	/* Tell player to handle unhandled event */

#if TARGET_OS_MAC
	kPlayerGetPluginFileSpecMessage		= 'pspc',	/* Get the location of the plugin executable (compatibility only) */
#endif
	
	kPlayerGetPluginITFileSpecMessage	= 'itfs',	/* Get the location of the plugin executable (iTunes 4.1 or later) */

	kPluginDisplayChangedMessage		= 'disp'	/* Something about some display has changed */
};

struct PlayerMessageInfo;

typedef OSStatus (*ITAppProcPtr)(void *appCookie, OSType message, struct PlayerMessageInfo *messageInfo);


/*
	Plugin main Messages
*/

struct PluginInitMessage {
	UInt32							majorVersion;			/* Input */
	UInt32							minorVersion;			/* Input */

	void *							appCookie;				/* Input */
	ITAppProcPtr					appProc;				/* Input */
	
	OptionBits						options;				/* Output, see above for values */
	void *							refCon;					/* Output */
};
typedef struct PluginInitMessage PluginInitMessage;


struct PluginMessageInfo {
	union {
		PluginInitMessage				initMessage;
	} u;
};
typedef struct PluginMessageInfo PluginMessageInfo;

/* Plugin main entry point message handler */
typedef OSStatus (*PluginProcPtr)(OSType message, PluginMessageInfo *messageInfo, void *refCon);

/* Visual plugin message handler */
struct VisualPluginMessageInfo;
typedef OSStatus (*VisualPluginProcPtr)(OSType message, struct VisualPluginMessageInfo *messageInfo, void *refCon);


/*
	Callbacks to iTunes
*/
struct PlayerOpenURLMessage {
	SInt8 *							url;
	UInt32							length;
};
typedef struct PlayerOpenURLMessage PlayerOpenURLMessage;

struct PlayerSetPluginDataMessage {
	void *							dataPtr;		/* Input */
	UInt32							dataSize;		/* Input */
};
typedef struct PlayerSetPluginDataMessage PlayerSetPluginDataMessage;

struct PlayerGetPluginDataMessage {
	void *							dataPtr;		/* Input */
	UInt32							dataBufferSize;	/* Input */
	
	UInt32							dataSize;		/* Output */
};
typedef struct PlayerGetPluginDataMessage PlayerGetPluginDataMessage;

struct PlayerSetPluginNamedDataMessage {
	ConstStringPtr					dataName;		/* Input */

	void *							dataPtr;		/* Input */
	UInt32							dataSize;		/* Input */
};
typedef struct PlayerSetPluginNamedDataMessage PlayerSetPluginNamedDataMessage;

struct PlayerGetPluginNamedDataMessage {
	ConstStringPtr					dataName;		/* Input */

	void *							dataPtr;		/* Input */
	UInt32							dataBufferSize;	/* Input */
	
	UInt32							dataSize;		/* Output */
};
typedef struct PlayerGetPluginNamedDataMessage PlayerGetPluginNamedDataMessage;

struct PlayerHandleMacOSEventMessage {
	const EventRecord *				theEvent;		/* Input */
	
	Boolean							handled;		/* Output */
};
typedef struct PlayerHandleMacOSEventMessage PlayerHandleMacOSEventMessage;

#if TARGET_OS_MAC
struct PlayerGetPluginFileSpecMessage {
	FSSpec *						fileSpec;		/* Output */
};
typedef struct PlayerGetPluginFileSpecMessage PlayerGetPluginFileSpecMessage;
#endif

struct PlayerGetPluginITFileSpecMessage {
	ITFileSpec *					fileSpec;		/* Output */
};
typedef struct PlayerGetPluginITFileSpecMessage PlayerGetPluginITFileSpecMessage;

struct PlayerGetFileTrackInfoMessage {
	const ITFileSpec *				fileSpec;		/* Input */
	ITTrackInfo *					trackInfo;		/* Output */
};
typedef struct PlayerGetFileTrackInfoMessage PlayerGetFileTrackInfoMessage;

struct PlayerSetFileTrackInfoMessage {
	const ITFileSpec *				fileSpec;		/* Input */
	const ITTrackInfo *				trackInfo;		/* Input */
};
typedef struct PlayerSetFileTrackInfoMessage PlayerSetFileTrackInfoMessage;

struct PlayerGetITTrackInfoSizeMessage {
	UInt32							itTrackInfoSize;	/* Output */
};
typedef struct PlayerGetITTrackInfoSizeMessage PlayerGetITTrackInfoSizeMessage;

/*
	iTunes API callback visual structures
*/
enum {
	/* PlayerRegisterVisualPluginMessage.options */
	
	kVisualWantsIdleMessages			= (1L << 3),
	kVisualWantsConfigure				= (1L << 5),
	kVisualProvidesUnicodeName			= (1L << 6)				/* Added in iTunes 7.3 */
};

struct PlayerRegisterVisualPluginMessage {
	/* Input from plugin */
	
	Str63							name;						/* Displayed in the Visual menu -- may be empty if options include kVisualProvidesUnicodeName */
	OptionBits						options;					/* See above */
	
	OSType							creator;					/* Identifies the plugin */
	
	NumVersion						pluginVersion;				/* Version number of the plugin */

	VisualPluginProcPtr				handler;					/* Handler for the plugin's messages */
	void *							registerRefCon;				/* RefCon for the plugin's handler */

	UInt32							timeBetweenDataInMS;		/* How often to call the plugin (0xFFFFFFFF = as often as possible) */
	UInt32							numWaveformChannels;		/* 0-2 waveforms requested */
	UInt32							numSpectrumChannels;		/* 0-2 spectrums requested */
	
	SInt16							minWidth;					/* Minimum resizeable width */
	SInt16							minHeight;					/* Minimum resizeable height */
	
	SInt16							maxWidth;					/* Maximum resizeable width */
	SInt16							maxHeight;					/* Maximum resizeable height */
		
	UInt16							minFullScreenBitDepth;		/* 0 = Any */
	UInt16							maxFullScreenBitDepth;		/* 0 = Any */
	
	UInt16							windowAlignmentInBytes;		/* Reserved (should be zero) */
	
	ITUniStr255						unicodeName;				/* options must include kVisualProvidesUnicodeName for this to be used -- Added in iTunes 7.3*/
};
typedef struct PlayerRegisterVisualPluginMessage PlayerRegisterVisualPluginMessage;

struct PlayerSetFullScreenMessage {
	Boolean							fullScreen;
};
typedef struct PlayerSetFullScreenMessage PlayerSetFullScreenMessage;

struct PlayerSetFullScreenOptionsMessage {
	SInt16							minBitDepth;				/* 0 = Any */
	SInt16							maxBitDepth;				/* 0 = Any */
	SInt16							preferredBitDepth;			/* 0 = Current */
	
	SInt16							desiredWidth;				/* Must be within minWidth & maxWidth */
	SInt16							desiredHeight;				/* Must be within minHeight & maxHeight */
};
typedef struct PlayerSetFullScreenOptionsMessage PlayerSetFullScreenOptionsMessage;

struct PlayerGetCurrentTrackCoverArtMessage {
	Handle						coverArt;		/* output - client must dispose */
	OSType						coverArtFormat;	/* output - format of cover art */
};
typedef struct PlayerGetCurrentTrackCoverArtMessage PlayerGetCurrentTrackCoverArtMessage;


// iTunes API callback union structure
struct PlayerMessageInfo {
	UInt32									messageMajorVersion;		/* Should be kITPluginMajorMessageVersion */
	UInt32									messageMinorVersion;		/* Should be kITPluginMinorMessageVersion */
	UInt32									messageInfoSize;			/* Should be sizeof(PlayerMessageInfo) */
	
	union {
		PlayerOpenURLMessage				openURLMessage;
		
		PlayerSetPluginDataMessage			setPluginDataMessage;
		PlayerGetPluginDataMessage			getPluginDataMessage;
		
		PlayerSetPluginNamedDataMessage		setPluginNamedDataMessage;
		PlayerGetPluginNamedDataMessage		getPluginNamedDataMessage;
		
				
		PlayerGetFileTrackInfoMessage		getFileTrackInfoMessage;
		PlayerSetFileTrackInfoMessage		setFileTrackInfoMessage;
		PlayerGetITTrackInfoSizeMessage		getITTrackInfoSizeMessage;

		PlayerHandleMacOSEventMessage		handleMacOSEventMessage;

#if TARGET_OS_MAC
		PlayerGetPluginFileSpecMessage		getPluginFileSpecMessage;
#endif

		PlayerGetPluginITFileSpecMessage	getPluginITFileSpecMessage;

		// visual APIs
		PlayerRegisterVisualPluginMessage		registerVisualPluginMessage;
		PlayerSetFullScreenMessage				setFullScreenMessage;
		PlayerSetFullScreenOptionsMessage		setFullScreenOptionsMessage;
		PlayerGetCurrentTrackCoverArtMessage	getCurrentTrackCoverArtMessage;
	} u;
};
typedef struct PlayerMessageInfo PlayerMessageInfo;

extern OSStatus ITCallApplication (void *appCookie, ITAppProcPtr appProc, OSType message, PlayerMessageInfo *messageInfo);
extern void		SetNumVersion (NumVersion *numVersion, UInt8 majorRev, UInt8 minorAndBugRev, UInt8 stage, UInt8 nonRelRev);


/* For all plugins */

extern OSStatus	PlayerUnregisterPlugin (void *appCookie, ITAppProcPtr appProc, PlayerMessageInfo *messageInfo);
extern OSStatus PlayerIdle (void *appCookie, ITAppProcPtr appProc);

extern void		PlayerShowAbout (void *appCookie, ITAppProcPtr appProc);
extern void		PlayerOpenURL (void *appCookie, ITAppProcPtr appProc, SInt8 *string, UInt32 length);

extern OSStatus	PlayerGetPluginData (void *appCookie, ITAppProcPtr appProc, void *dataPtr, UInt32 dataBufferSize, UInt32 *dataSize);
extern OSStatus	PlayerSetPluginData (void *appCookie, ITAppProcPtr appProc, void *dataPtr, UInt32 dataSize);

extern OSStatus	PlayerGetPluginNamedData (void *appCookie, ITAppProcPtr appProc, ConstStringPtr dataName, void *dataPtr, UInt32 dataBufferSize, UInt32 *dataSize);
extern OSStatus	PlayerSetPluginNamedData (void *appCookie, ITAppProcPtr appProc, ConstStringPtr dataName, void *dataPtr, UInt32 dataSize);

extern OSStatus	PlayerGetFileTrackInfo (void *appCookie, ITAppProcPtr appProc, const ITFileSpec *fileSpec, ITTrackInfo *trackInfo);
extern OSStatus	PlayerSetFileTrackInfo (void *appCookie, ITAppProcPtr appProc, const ITFileSpec *fileSpec, const ITTrackInfo *trackInfo);

extern OSStatus PlayerGetITTrackInfoSize (void *appCookie, ITAppProcPtr appProc, UInt32 appPluginMajorVersion, UInt32 appPluginMinorVersion, UInt32 *itTrackInfoSize);

extern OSStatus PlayerHandleMacOSEvent (void *appCookie, ITAppProcPtr appProc, const EventRecord *theEvent, Boolean *eventHandled);


#if TARGET_OS_MAC
extern OSStatus	PlayerGetPluginFileSpec (void *appCookie, ITAppProcPtr appProc, FSSpec *pluginFileSpec);
#endif
extern OSStatus	PlayerGetPluginITFileSpec (void *appCookie, ITAppProcPtr appProc, ITFileSpec *pluginFileSpec);

/* iTunes APIs For visual plugins */
enum {
	kPlayerSetFullScreenMessage			= 'sful',	/* Set full screen mode */
	kPlayerSetFullScreenOptionsMessage	= 'sfop',	/* Set full screen options */
	kPlayerGetCurrentTrackCoverArtMessage = 'covr'	/* Get current player track cover artwork */
};

extern OSStatus PlayerRegisterVisualPlugin (void *appCookie, ITAppProcPtr appProc, PlayerMessageInfo *messageInfo);

extern OSStatus PlayerSetFullScreen (void *appCookie, ITAppProcPtr appProc, Boolean fullScreen);
extern OSStatus PlayerSetFullScreenOptions (void *appCookie, ITAppProcPtr appProc, SInt16 minBitDepth, SInt16 maxBitDepth, SInt16 preferredBitDepth, SInt16 desiredWidth, SInt16 desiredHeight);
extern OSStatus PlayerGetCurrentTrackCoverArt (void *appCookie, ITAppProcPtr appProc, Handle *coverArt, OSType *coverArtFormat);


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

#endif /* ITUNESAPI_H_ */
