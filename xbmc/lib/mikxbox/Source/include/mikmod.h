/*	MikMod sound library
	(c) 1998, 1999, 2000 Miodrag Vallat and others - see file AUTHORS
	for complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.
 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.
 
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/
  
/*==============================================================================

  $Id$

  MikMod sound library include file

  Modified by ozzy@Orkysquad on 23/08/2001 added MDRIVER drv_ds_raw driver.
	Disabled @ characters (0x40)
  Added by ozzy@orkysquad on 29/05/2002  compilation defines for all kind of 
    supported tracker formats.

==============================================================================*/

#ifndef _MIKMOD_H_
#define _MIKMOD_H_

#include <stdio.h>
#include <stdlib.h>


//Compiled formats.. comment undesired formats 
#define USE_669_FORMAT 
#define USE_AMF_FORMAT
#define USE_DMF_FORMAT
#define USE_DSM_FORMAT
#define USE_FAR_FORMAT
#define USE_GDM_FORMAT
#define USE_IMF_FORMAT
#define USE_IT_FORMAT
#define USE_M15_FORMAT
#define USE_MED_FORMAT
#define USE_MOD_FORMAT
#define USE_MTM_FORMAT
#define USE_OKT_FORMAT
#define USE_S3M_FORMAT
#define USE_STM_FORMAT
#define USE_SFX_FORMAT
#define USE_ULT_FORMAT
#define USE_UNI_FORMAT
#define USE_XM_FORMAT

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ========== Compiler magic for shared libraries
 */

#if defined WIN32 && defined _DLL
#ifdef DLL_EXPORTS
#define MIKMODAPI __declspec(dllexport)
#else
#define MIKMODAPI __declspec(dllimport)
#endif
#else
#define MIKMODAPI
#endif

/*
 *	========== Library version
 */

#define LIBMIKMOD_VERSION_MAJOR 3L
#define LIBMIKMOD_VERSION_MINOR 1L
#define LIBMIKMOD_REVISION      10L

#define LIBMIKMOD_VERSION		0x03110

MIKMODAPI extern long MikMod_GetVersion(void);

/*
 *	========== Platform independent-type definitions
 */

#include <xtl.h>
/*
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#include <mmsystem.h>
#endif
*/

//#include <xbox.h>

#if defined(__OS2__)||defined(__EMX__)
#define INCL_DOSSEMAPHORES
#include <os2.h>
#else
typedef char CHAR;
#endif

/*DOES_NOT_HAVE_SIGNED*/

#if defined(__arch64__) || defined(__alpha)
/* 64 bit architectures */

typedef signed char     SBYTE;      /* 1 byte, signed */
typedef unsigned char   UBYTE;      /* 1 byte, unsigned */
typedef signed short    SWORD;      /* 2 bytes, signed */
typedef unsigned short  UWORD;      /* 2 bytes, unsigned */
typedef signed int      SLONG;      /* 4 bytes, signed */
typedef unsigned int    ULONG;      /* 4 bytes, unsigned */
typedef int             BOOL;       /* 0=false, <>0 true */

#else
/* 32 bit architectures */

typedef signed char     SBYTE;      /* 1 byte, signed */
typedef unsigned char   UBYTE;      /* 1 byte, unsigned */
typedef signed short    SWORD;      /* 2 bytes, signed */
typedef unsigned short  UWORD;      /* 2 bytes, unsigned */
typedef signed long     SLONG;      /* 4 bytes, signed */
#if !defined(__OS2__)&&!defined(__EMX__)&&!defined(WIN32)
typedef unsigned long   ULONG;      /* 4 bytes, unsigned */
typedef int             BOOL;       /* 0=false, <>0 true */
#endif
#endif

/*
 *	========== Error codes
 */

enum {
	MMERR_OPENING_FILE = 1,
	MMERR_OUT_OF_MEMORY,
	MMERR_DYNAMIC_LINKING,

	MMERR_SAMPLE_TOO_BIG,
	MMERR_OUT_OF_HANDLES,
	MMERR_UNKNOWN_WAVE_TYPE,

	MMERR_LOADING_PATTERN,
	MMERR_LOADING_TRACK,
	MMERR_LOADING_HEADER,
	MMERR_LOADING_SAMPLEINFO,
	MMERR_NOT_A_MODULE,
	MMERR_NOT_A_STREAM,
	MMERR_MED_SYNTHSAMPLES,
	MMERR_ITPACK_INVALID_DATA,

	MMERR_DETECTING_DEVICE,
	MMERR_INVALID_DEVICE,
	MMERR_INITIALIZING_MIXER,
	MMERR_OPENING_AUDIO,
	MMERR_8BIT_ONLY,
	MMERR_16BIT_ONLY,
	MMERR_STEREO_ONLY,
	MMERR_ULAW,
	MMERR_NON_BLOCK,

	MMERR_AF_AUDIO_PORT,

	MMERR_AIX_CONFIG_INIT,
	MMERR_AIX_CONFIG_CONTROL,
	MMERR_AIX_CONFIG_START,

	MMERR_GUS_SETTINGS,
	MMERR_GUS_RESET,
	MMERR_GUS_TIMER,

	MMERR_HP_SETSAMPLESIZE,
	MMERR_HP_SETSPEED,
	MMERR_HP_CHANNELS,
	MMERR_HP_AUDIO_OUTPUT,
	MMERR_HP_AUDIO_DESC,
	MMERR_HP_BUFFERSIZE,

	MMERR_OSS_SETFRAGMENT,
	MMERR_OSS_SETSAMPLESIZE,
	MMERR_OSS_SETSTEREO,
	MMERR_OSS_SETSPEED,

	MMERR_SGI_SPEED,
	MMERR_SGI_16BIT,
	MMERR_SGI_8BIT,
	MMERR_SGI_STEREO,
	MMERR_SGI_MONO,

	MMERR_SUN_INIT,

	MMERR_OS2_MIXSETUP,
	MMERR_OS2_SEMAPHORE,
	MMERR_OS2_TIMER,
	MMERR_OS2_THREAD,

	MMERR_DS_PRIORITY,
	MMERR_DS_BUFFER,
	MMERR_DS_FORMAT,
	MMERR_DS_NOTIFY,
	MMERR_DS_EVENT,
	MMERR_DS_THREAD,
	MMERR_DS_UPDATE,

	MMERR_WINMM_HANDLE,
	MMERR_WINMM_ALLOCATED,
	MMERR_WINMM_DEVICEID,
	MMERR_WINMM_FORMAT,
	MMERR_WINMM_UNKNOWN,

	MMERR_MAC_SPEED,
	MMERR_MAC_START,

	MMERR_MAX
};

/*
 *	========== Error handling
 */

typedef void (MikMod_handler)(void);
typedef MikMod_handler *MikMod_handler_t;

MIKMODAPI extern int  MikMod_errno;
MIKMODAPI extern BOOL MikMod_critical;
MIKMODAPI extern char *MikMod_strerror(int);

MIKMODAPI extern MikMod_handler_t MikMod_RegisterErrorHandler(MikMod_handler_t);

/*
 *	========== Library initialization and core functions
 */

struct MDRIVER;

MIKMODAPI extern void   MikMod_RegisterAllDrivers(void);

MIKMODAPI extern CHAR*  MikMod_InfoDriver(void);
MIKMODAPI extern void   MikMod_RegisterDriver(struct MDRIVER*);
MIKMODAPI extern int    MikMod_DriverFromAlias(CHAR*);

MIKMODAPI extern BOOL   MikMod_Init(CHAR*);
MIKMODAPI extern void   MikMod_Exit(void);
MIKMODAPI extern BOOL   MikMod_Reset(CHAR*);
MIKMODAPI extern BOOL   MikMod_SetNumVoices(int,int);
MIKMODAPI extern BOOL   MikMod_Active(void);
MIKMODAPI extern BOOL   MikMod_EnableOutput(void);
MIKMODAPI extern void   MikMod_DisableOutput(void);
MIKMODAPI extern void   MikMod_Update(void);

MIKMODAPI extern BOOL   MikMod_InitThreads(void);
MIKMODAPI extern void   MikMod_Lock(void);
MIKMODAPI extern void   MikMod_Unlock(void);

/*
 *	========== Reader, Writer
 */

typedef struct MREADER {
	BOOL (*Seek)(struct MREADER*,long,int);
	long (*Tell)(struct MREADER*);
	BOOL (*Read)(struct MREADER*,void*,size_t);
	int  (*Get)(struct MREADER*);
	BOOL (*Eof)(struct MREADER*);
} MREADER;

typedef struct MWRITER {
	BOOL (*Seek)(struct MWRITER*,long,int);
	long (*Tell)(struct MWRITER*);
	BOOL (*Write)(struct MWRITER*,void*,size_t);
	BOOL (*Put)(struct MWRITER*,int);
} MWRITER;

/*
 *	========== Samples
 */

/* Sample playback should not be interrupted */
#define SFX_CRITICAL 1

/* Sample format [loading and in-memory] flags: */
#define SF_16BITS       0x0001
#define SF_STEREO       0x0002
#define SF_SIGNED       0x0004
#define SF_BIG_ENDIAN   0x0008
#define SF_DELTA        0x0010
#define SF_ITPACKED		0x0020

#define	SF_FORMATMASK	0x003F

/* General Playback flags */

#define SF_LOOP         0x0100
#define SF_BIDI         0x0200
#define SF_REVERSE      0x0400
#define SF_SUSTAIN      0x0800

#define SF_PLAYBACKMASK	0x0C00

/* Module-only Playback Flags */

#define SF_OWNPAN		0x1000
#define SF_UST_LOOP     0x2000

#define SF_EXTRAPLAYBACKMASK	0x3000

/* Panning constants */
#define PAN_LEFT		0
#define PAN_HALFLEFT 	64
#define PAN_CENTER		128
#define PAN_HALFRIGHT	192
#define PAN_RIGHT		255
#define PAN_SURROUND	512 /* panning value for Dolby Surround */

typedef struct SAMPLE {
	SWORD  panning;     /* panning (0-255 or PAN_SURROUND) */
	ULONG  speed;       /* Base playing speed/frequency of note */
	UBYTE  volume;      /* volume 0-64 */
	UWORD  inflags;		/* sample format on disk */
	UWORD  flags;       /* sample format in memory */
	ULONG  length;      /* length of sample (in samples!) */
	ULONG  loopstart;   /* repeat position (relative to start, in samples) */
	ULONG  loopend;     /* repeat end */
	ULONG  susbegin;    /* sustain loop begin (in samples) \  Not Supported */
	ULONG  susend;      /* sustain loop end                /      Yet! */
	ULONG  c5frq;

	/* Variables used by the module player only! (ignored for sound effects) */
	UBYTE  globvol;     /* global volume */
	UBYTE  vibflags;    /* autovibrato flag stuffs */
	UBYTE  vibtype;     /* Vibratos moved from INSTRUMENT to SAMPLE */
	UBYTE  vibsweep;
	UBYTE  vibdepth;
	UBYTE  vibrate;
	CHAR*  samplename;  /* name of the sample */

	/* Values used internally only */
	UWORD  avibpos;     /* autovibrato pos [player use] */
	UBYTE  divfactor;   /* for sample scaling, maintains proper period slides */
	ULONG  seekpos;     /* seek position in file */
	SWORD  handle;      /* sample handle used by individual drivers */
} SAMPLE;

/* Sample functions */

MIKMODAPI extern SAMPLE *Sample_Load(CHAR*);
MIKMODAPI extern SAMPLE *Sample_LoadFP(FILE*);
MIKMODAPI extern SAMPLE *Sample_LoadGeneric(MREADER*);
MIKMODAPI extern void   Sample_Free(SAMPLE*);
MIKMODAPI extern SBYTE  Sample_Play(SAMPLE*,ULONG,UBYTE);

MIKMODAPI extern void   Voice_SetVolume(SBYTE,UWORD);
MIKMODAPI extern UWORD  Voice_GetVolume(SBYTE);
MIKMODAPI extern void   Voice_SetFrequency(SBYTE,ULONG);
MIKMODAPI extern ULONG  Voice_GetFrequency(SBYTE);
MIKMODAPI extern void   Voice_SetPanning(SBYTE,ULONG);
MIKMODAPI extern ULONG  Voice_GetPanning(SBYTE);
MIKMODAPI extern void   Voice_Play(SBYTE,SAMPLE*,ULONG);
MIKMODAPI extern void   Voice_Stop(SBYTE);
MIKMODAPI extern BOOL   Voice_Stopped(SBYTE);
MIKMODAPI extern SLONG  Voice_GetPosition(SBYTE);
MIKMODAPI extern ULONG  Voice_RealVolume(SBYTE);

/*
 *	========== Internal module representation (UniMod)
 */

/*
	Instrument definition - for information only, the only field which may be
	of use in user programs is the name field
*/

/* Instrument note count */
#define INSTNOTES 120

/* Envelope point */
typedef struct ENVPT {
	SWORD pos;
	SWORD val;
} ENVPT;

/* Envelope point count */
#define ENVPOINTS 32

/* Instrument structure */
typedef struct INSTRUMENT {
	CHAR* insname;

	UBYTE flags;
	UWORD samplenumber[INSTNOTES];
	UBYTE samplenote[INSTNOTES];

	UBYTE nnatype;
	UBYTE dca;              /* duplicate check action */
	UBYTE dct;              /* duplicate check type */
	UBYTE globvol;
	UWORD volfade;
	SWORD panning;          /* instrument-based panning var */

	UBYTE pitpansep;        /* pitch pan separation (0 to 255) */
	UBYTE pitpancenter;     /* pitch pan center (0 to 119) */
	UBYTE rvolvar;          /* random volume varations (0 - 100%) */
	UBYTE rpanvar;          /* random panning varations (0 - 100%) */

	/* volume envelope */
	UBYTE volflg;           /* bit 0: on 1: sustain 2: loop */
	UBYTE volpts;
	UBYTE volsusbeg;
	UBYTE volsusend;
	UBYTE volbeg;
	UBYTE volend;
	ENVPT volenv[ENVPOINTS];
	/* panning envelope */
	UBYTE panflg;           /* bit 0: on 1: sustain 2: loop */
	UBYTE panpts;
	UBYTE pansusbeg;
	UBYTE pansusend;
	UBYTE panbeg;
	UBYTE panend;
	ENVPT panenv[ENVPOINTS];
	/* pitch envelope */
	UBYTE pitflg;           /* bit 0: on 1: sustain 2: loop */
	UBYTE pitpts;
	UBYTE pitsusbeg;
	UBYTE pitsusend;
	UBYTE pitbeg;
	UBYTE pitend;
	ENVPT pitenv[ENVPOINTS];
} INSTRUMENT;

struct MP_CONTROL;
struct MP_VOICE;

/*
	Module definition
*/

/* maximum master channels supported */
#define UF_MAXCHAN	64

/* Module flags */
#define UF_XMPERIODS	0x0001 /* XM periods / finetuning */
#define UF_LINEAR		0x0002 /* LINEAR periods (UF_XMPERIODS must be set) */
#define UF_INST			0x0004 /* Instruments are used */
#define UF_NNA			0x0008 /* IT: NNA used, set numvoices rather
								  than numchn */
#define UF_S3MSLIDES	0x0010 /* uses old S3M volume slides */
#define UF_BGSLIDES		0x0020 /* continue volume slides in the background */
#define UF_HIGHBPM		0x0040 /* MED: can use >255 bpm */
#define UF_NOWRAP		0x0080 /* XM-type (i.e. illogical) pattern break
								  semantics */
#define UF_ARPMEM		0x0100 /* IT: need arpeggio memory */
#define UF_FT2QUIRKS	0x0200 /* emulate some FT2 replay quirks */
#define UF_PANNING		0x0400 /* module uses panning effects or have
								  non-tracker default initial panning */

typedef struct MODULE {
	/* general module information */
		CHAR*       songname;    /* name of the song */
		CHAR*       modtype;     /* string type of module loaded */
		CHAR*       comment;     /* module comments */

		UWORD       flags;       /* See module flags above */
		UBYTE       numchn;      /* number of module channels */
		UBYTE       numvoices;   /* max # voices used for full NNA playback */
		UWORD       numpos;      /* number of positions in this song */
		UWORD       numpat;      /* number of patterns in this song */
		UWORD       numins;      /* number of instruments */
		UWORD       numsmp;      /* number of samples */
struct  INSTRUMENT* instruments; /* all instruments */
struct  SAMPLE*     samples;     /* all samples */
		UBYTE       realchn;     /* real number of channels used */
		UBYTE       totalchn;    /* total number of channels used (incl NNAs) */

	/* playback settings */
		UWORD       reppos;      /* restart position */
		UBYTE       initspeed;   /* initial song speed */
		UWORD       inittempo;   /* initial song tempo */
		UBYTE       initvolume;  /* initial global volume (0 - 128) */
		UWORD       panning[UF_MAXCHAN]; /* panning positions */
		UBYTE       chanvol[UF_MAXCHAN]; /* channel positions */
		UWORD       bpm;         /* current beats-per-minute speed */
		UWORD       sngspd;      /* current song speed */
		SWORD       volume;      /* song volume (0-128) (or user volume) */

		BOOL        extspd;      /* extended speed flag (default enabled) */
		BOOL        panflag;     /* panning flag (default enabled) */
		BOOL        wrap;        /* wrap module ? (default disabled) */
		BOOL        loop;		 /* allow module to loop ? (default enabled) */
		BOOL        fadeout;	 /* volume fade out during last pattern */

		UWORD       patpos;      /* current row number */
		SWORD       sngpos;      /* current song position */
		ULONG       sngtime;     /* current song time in 2^-10 seconds */

		SWORD       relspd;      /* relative speed factor */

	/* internal module representation */
		UWORD       numtrk;      /* number of tracks */
		UBYTE**     tracks;      /* array of numtrk pointers to tracks */
		UWORD*      patterns;    /* array of Patterns */
		UWORD*      pattrows;    /* array of number of rows for each pattern */
		UWORD*      positions;   /* all positions */

		BOOL        forbid;      /* if true, no player update! */
		UWORD       numrow;      /* number of rows on current pattern */
		UWORD       vbtick;      /* tick counter (counts from 0 to sngspd) */
		UWORD       sngremainder;/* used for song time computation */

struct MP_CONTROL*  control;     /* Effects Channel info (size pf->numchn) */
struct MP_VOICE*    voice;       /* Audio Voice information (size md_numchn) */

		UBYTE       globalslide; /* global volume slide rate */
		UBYTE       pat_repcrazy;/* module has just looped to position -1 */
		UWORD       patbrk;      /* position where to start a new pattern */
		UBYTE       patdly;      /* patterndelay counter (command memory) */
		UBYTE       patdly2;     /* patterndelay counter (real one) */
		SWORD       posjmp;      /* flag to indicate a jump is needed... */
		UWORD		bpmlimit;	 /* threshold to detect bpm or speed values */
} MODULE;

/*
 *	========== Module loaders
 */

struct MLOADER;

MIKMODAPI extern CHAR*   MikMod_InfoLoader(void);
MIKMODAPI extern void    MikMod_RegisterAllLoaders(void);
MIKMODAPI extern void    MikMod_RegisterLoader(struct MLOADER*);

MIKMODAPI extern struct MLOADER load_669; /* 669 and Extended-669 (by Tran/Renaissance) */
MIKMODAPI extern struct MLOADER load_amf; /* DMP Advanced Module Format (by Otto Chrons) */
MIKMODAPI extern struct MLOADER load_dsm; /* DSIK internal module format */
MIKMODAPI extern struct MLOADER load_far; /* Farandole Composer (by Daniel Potter) */
MIKMODAPI extern struct MLOADER load_gdm; /* General DigiMusic (by Edward Schlunder) */
MIKMODAPI extern struct MLOADER load_it;  /* Impulse Tracker (by Jeffrey Lim) */
MIKMODAPI extern struct MLOADER load_imf; /* Imago Orpheus (by Lutz Roeder) */
MIKMODAPI extern struct MLOADER load_med; /* Amiga MED modules (by Teijo Kinnunen) */
MIKMODAPI extern struct MLOADER load_m15; /* Soundtracker 15-instrument */
MIKMODAPI extern struct MLOADER load_mod; /* Standard 31-instrument Module loader */
MIKMODAPI extern struct MLOADER load_mtm; /* Multi-Tracker Module (by Renaissance) */
MIKMODAPI extern struct MLOADER load_okt; /* Amiga Oktalyzer */
MIKMODAPI extern struct MLOADER load_stm; /* ScreamTracker 2 (by Future Crew) */
MIKMODAPI extern struct MLOADER load_stx; /* STMIK 0.2 (by Future Crew) */
MIKMODAPI extern struct MLOADER load_s3m; /* ScreamTracker 3 (by Future Crew) */
MIKMODAPI extern struct MLOADER load_ult; /* UltraTracker (by MAS) */
MIKMODAPI extern struct MLOADER load_uni; /* MikMod and APlayer internal module format */
MIKMODAPI extern struct MLOADER load_xm;  /* FastTracker 2 (by Triton) */

/*
 *	========== Module player
 */

MIKMODAPI extern MODULE* Mod_Player_Load(CHAR*,int,BOOL);
MIKMODAPI extern MODULE* Mod_Player_LoadFP(FILE*,int,BOOL);
MIKMODAPI extern MODULE* Mod_Player_LoadGeneric(MREADER*,int,BOOL);
MIKMODAPI extern CHAR*   Mod_Player_LoadTitle(CHAR*);
MIKMODAPI extern CHAR*   Mod_Player_LoadTitleFP(FILE*);
MIKMODAPI extern void    Mod_Player_Free(MODULE*);
MIKMODAPI extern void    Mod_Player_Start(MODULE*);
MIKMODAPI extern BOOL    Mod_Player_Active(void);
MIKMODAPI extern void    Mod_Player_Stop(void);
MIKMODAPI extern void    Mod_Player_TogglePause(void);
MIKMODAPI extern BOOL    Mod_Player_Paused(void);
MIKMODAPI extern void    Mod_Player_NextPosition(void);
MIKMODAPI extern void    Mod_Player_PrevPosition(void);
MIKMODAPI extern void    Mod_Player_SetPosition(UWORD);
MIKMODAPI extern BOOL    Mod_Player_Muted(UBYTE);
MIKMODAPI extern void    Mod_Player_SetVolume(SWORD);
MIKMODAPI extern MODULE* Mod_Player_GetModule(void);
MIKMODAPI extern void    Mod_Player_SetSpeed(UWORD);
MIKMODAPI extern void    Mod_Player_SetTempo(UWORD);
MIKMODAPI extern void    Mod_Player_Unmute(SLONG,...);
MIKMODAPI extern void    Mod_Player_Mute(SLONG,...);
MIKMODAPI extern void    Mod_Player_ToggleMute(SLONG,...);
MIKMODAPI extern int     Mod_Player_GetChannelVoice(UBYTE);
MIKMODAPI extern UWORD   Mod_Player_GetChannelPeriod(UBYTE);

typedef void (MikMod_player)(void);
typedef MikMod_player *MikMod_player_t;

MIKMODAPI extern MikMod_player_t MikMod_RegisterPlayer(MikMod_player_t);

#define MUTE_EXCLUSIVE  32000
#define MUTE_INCLUSIVE  32001

/*
 *	========== Drivers
 */

enum {
	MD_MUSIC = 0,
	MD_SNDFX
};

enum {
	MD_HARDWARE = 0,
	MD_SOFTWARE
};

/* Mixing flags */

/* These ones take effect only after MikMod_Init or MikMod_Reset */
#define DMODE_16BITS     0x0001 /* enable 16 bit output */
#define DMODE_STEREO     0x0002 /* enable stereo output */
#define DMODE_SOFT_SNDFX 0x0004 /* Process sound effects via software mixer */
#define DMODE_SOFT_MUSIC 0x0008 /* Process music via software mixer */
#define DMODE_HQMIXER    0x0010 /* Use high-quality (slower) software mixer */
/* These take effect immediately. */
#define DMODE_SURROUND   0x0100 /* enable surround sound */
#define DMODE_INTERP     0x0200 /* enable interpolation */
#define DMODE_REVERSE    0x0400 /* reverse stereo */

struct SAMPLOAD;
typedef struct MDRIVER {
struct MDRIVER* next;
	CHAR*       Name;
	CHAR*       Version;

	UBYTE       HardVoiceLimit; /* Limit of hardware mixer voices */
	UBYTE       SoftVoiceLimit; /* Limit of software mixer voices */

	CHAR*       Alias;

	void        (*CommandLine)      (CHAR*);
	BOOL        (*IsPresent)        (void);
	SWORD       (*SampleLoad)       (struct SAMPLOAD*,int);
	void        (*SampleUnload)     (SWORD);
	ULONG       (*FreeSampleSpace)  (int);
	ULONG       (*RealSampleLength) (int,struct SAMPLE*);
	BOOL        (*Init)             (void);
	void        (*Exit)             (void);
	BOOL        (*Reset)            (void);
	BOOL        (*SetNumVoices)     (void);
	BOOL        (*PlayStart)        (void);
	void        (*PlayStop)         (void);
	void        (*Update)           (void);
	void        (*Pause)            (void);
	void        (*VoiceSetVolume)   (UBYTE,UWORD);
	UWORD       (*VoiceGetVolume)   (UBYTE);
	void        (*VoiceSetFrequency)(UBYTE,ULONG);
	ULONG       (*VoiceGetFrequency)(UBYTE);
	void        (*VoiceSetPanning)  (UBYTE,ULONG);
	ULONG       (*VoiceGetPanning)  (UBYTE);
	void        (*VoicePlay)        (UBYTE,SWORD,ULONG,ULONG,ULONG,ULONG,UWORD);
	void        (*VoiceStop)        (UBYTE);
	BOOL        (*VoiceStopped)     (UBYTE);
	SLONG       (*VoiceGetPosition) (UBYTE);
	ULONG       (*VoiceRealVolume)  (UBYTE);
} MDRIVER;

/* These variables can be changed at ANY time and results will be immediate */
MIKMODAPI extern UBYTE md_volume;      /* global sound volume (0-128) */
MIKMODAPI extern UBYTE md_musicvolume; /* volume of song */
MIKMODAPI extern UBYTE md_sndfxvolume; /* volume of sound effects */
MIKMODAPI extern UBYTE md_reverb;      /* 0 = none;  15 = chaos */
MIKMODAPI extern UBYTE md_pansep;      /* 0 = mono;  128 == 100% (full left/right) */

/* The variables below can be changed at any time, but changes will not be
   implemented until MikMod_Reset is called. A call to MikMod_Reset may result
   in a skip or pop in audio (depending on the soundcard driver and the settings
   changed). */
MIKMODAPI extern UWORD md_device;      /* device */
MIKMODAPI extern UWORD md_mixfreq;     /* mixing frequency */
MIKMODAPI extern UWORD md_mode;        /* mode. See DMODE_? flags above */

/* The following variable should not be changed! */
MIKMODAPI extern MDRIVER* md_driver;   /* Current driver in use. */

/* Known drivers list */


MIKMODAPI extern struct MDRIVER drv_nos;    /* no sound */
//MIKMODAPI extern struct MDRIVER drv_pipe;   /* piped output */
//MIKMODAPI extern struct MDRIVER drv_raw;    /* raw file disk writer [music.raw] */
//MIKMODAPI extern struct MDRIVER drv_stdout; /* output to stdout */
MIKMODAPI extern struct MDRIVER drv_wav;    /* RIFF WAVE file disk writer [music.wav] */

//MIKMODAPI extern struct MDRIVER drv_ultra;  /* Linux Ultrasound driver */
//MIKMODAPI extern struct MDRIVER drv_sam9407;	/* Linux sam9407 driver */

//MIKMODAPI extern struct MDRIVER drv_AF;     /* Dec Alpha AudioFile */
//MIKMODAPI extern struct MDRIVER drv_aix;    /* AIX audio device */
//MIKMODAPI extern struct MDRIVER drv_alsa;   /* Advanced Linux Sound Architecture (ALSA) */
//MIKMODAPI extern struct MDRIVER drv_esd;    /* Enlightened sound daemon (EsounD) */
//MIKMODAPI extern struct MDRIVER drv_hp;     /* HP-UX audio device */
//MIKMODAPI extern struct MDRIVER drv_oss;    /* OpenSound System (Linux,FreeBSD...) */
//MIKMODAPI extern struct MDRIVER drv_sgi;    /* SGI audio library */
//MIKMODAPI extern struct MDRIVER drv_sun;    /* Sun/NetBSD/OpenBSD audio device */

//MIKMODAPI extern struct MDRIVER drv_dart;   /* OS/2 Direct Audio RealTime */
//MIKMODAPI extern struct MDRIVER drv_os2;    /* OS/2 MMPM/2 */

MIKMODAPI extern struct MDRIVER drv_ds_raw; /* Windows DirectX3 software mixing - jm & ozzy */
MIKMODAPI extern struct MDRIVER drv_ds;     /* Win32 DirectSound driver */
MIKMODAPI extern struct MDRIVER drv_win;    /* Win32 multimedia API driver */

//MIKMODAPI extern struct MDRIVER drv_mac;    /* Macintosh Sound Manager driver */

#ifdef _XBOX
MIKMODAPI extern struct MDRIVER drv_xbox;   /* Xbox hardware accelerated driver */
#endif

/*========== Virtual channel mixer interface (for user-supplied drivers only) */

MIKMODAPI extern BOOL  VC_Init(void);
MIKMODAPI extern void  VC_Exit(void);
MIKMODAPI extern BOOL  VC_SetNumVoices(void);
MIKMODAPI extern ULONG VC_SampleSpace(int);
MIKMODAPI extern ULONG VC_SampleLength(int,SAMPLE*);

MIKMODAPI extern BOOL  VC_PlayStart(void);
MIKMODAPI extern void  VC_PlayStop(void);

MIKMODAPI extern SWORD VC_SampleLoad(struct SAMPLOAD*,int);
MIKMODAPI extern void  VC_SampleUnload(SWORD);

MIKMODAPI extern ULONG VC_WriteBytes(SBYTE*,ULONG);
MIKMODAPI extern ULONG VC_SilenceBytes(SBYTE*,ULONG);

MIKMODAPI extern void  VC_VoiceSetVolume(UBYTE,UWORD);
MIKMODAPI extern UWORD VC_VoiceGetVolume(UBYTE);
MIKMODAPI extern void  VC_VoiceSetFrequency(UBYTE,ULONG);
MIKMODAPI extern ULONG VC_VoiceGetFrequency(UBYTE);
MIKMODAPI extern void  VC_VoiceSetPanning(UBYTE,ULONG);
MIKMODAPI extern ULONG VC_VoiceGetPanning(UBYTE);
MIKMODAPI extern void  VC_VoicePlay(UBYTE,SWORD,ULONG,ULONG,ULONG,ULONG,UWORD);

MIKMODAPI extern void  VC_VoiceStop(UBYTE);
MIKMODAPI extern BOOL  VC_VoiceStopped(UBYTE);
MIKMODAPI extern SLONG VC_VoiceGetPosition(UBYTE);
MIKMODAPI extern ULONG VC_VoiceRealVolume(UBYTE);

#ifdef __cplusplus
}
#endif

#endif

/* ex:set ts=4: */
