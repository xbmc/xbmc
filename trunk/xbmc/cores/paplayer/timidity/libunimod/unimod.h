/*      MikMod sound library
   (c) 1998, 1999 Miodrag Vallat and others - see file AUTHORS for
   complete list.

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

  $Id: unimod.h,v 1.35 1999/10/25 16:31:41 miod Exp $

  MikMod sound library include file

==============================================================================*/

#ifndef _UNIMOD_H_
#define _UNIMOD_H_

#include <stdio.h>
#include <stdlib.h>
#include "url.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
 *    ========== Platform independent-type definitions
 */

#ifdef __W32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#elif defined(__OS2__)||defined(__EMX__)
#define INCL_DOSSEMAPHORES
#include <os2.h>
#else
typedef char CHAR;
#endif

#if defined(__alpha)
/* 64 bit architectures */

typedef signed char SBYTE;	/* 1 byte, signed */
typedef unsigned char UBYTE;	/* 1 byte, unsigned */
typedef signed short SWORD;	/* 2 bytes, signed */
typedef unsigned short UWORD;	/* 2 bytes, unsigned */
typedef signed int SLONG;	/* 4 bytes, signed */
typedef unsigned int ULONG;	/* 4 bytes, unsigned */
typedef int BOOL;		/* 0=false, <>0 true */

#else
/* 32 bit architectures */

typedef signed char SBYTE;	/* 1 byte, signed */
typedef unsigned char UBYTE;	/* 1 byte, unsigned */
typedef signed short SWORD;	/* 2 bytes, signed */
typedef unsigned short UWORD;	/* 2 bytes, unsigned */
typedef signed long SLONG;	/* 4 bytes, signed */
#if !defined(__OS2__)&&!defined(__EMX__)&&!defined(__W32__)
typedef unsigned long ULONG;	/* 4 bytes, unsigned */
typedef int BOOL;		/* 0=false, <>0 true */
#endif
#endif

/*
 *    ========== Error handling and error codes
 */

extern int ML_errno;
extern char *ML_strerror (int);

enum
{
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

  MMERR_MAX
};

/*========== Internal module representation (UniMod) interface */

/* number of notes in an octave */
#define OCTAVE 12

extern void UniSetRow (UBYTE *);
extern UBYTE UniGetByte (void);
extern UWORD UniGetWord (void);
extern UBYTE *UniFindRow (UBYTE *, UWORD);
extern void UniSkipOpcode (UBYTE);
extern void UniReset (void);
extern void UniWriteByte (UBYTE);
extern void UniWriteWord (UWORD);
extern void UniNewline (void);
extern UBYTE *UniDup (void);
extern BOOL UniInit (void);
extern void UniCleanup (void);
extern void UniEffect (UWORD, UWORD);
#define UniInstrument(x) UniEffect(UNI_INSTRUMENT,x)
#define UniNote(x) UniEffect(UNI_NOTE,x)
extern void UniPTEffect (UBYTE, UBYTE);
extern void UniVolEffect (UWORD, UBYTE);

/*========== Module Commands */

enum
{
  /* Simple note */
  UNI_NOTE = 1,
  /* Instrument change */
  UNI_INSTRUMENT,
  /* Protracker effects */
  UNI_PTEFFECT0,		/* arpeggio */
  UNI_PTEFFECT1,		/* porta up */
  UNI_PTEFFECT2,		/* porta down */
  UNI_PTEFFECT3,		/* porta to note */
  UNI_PTEFFECT4,		/* vibrato */
  UNI_PTEFFECT5,		/* dual effect 3+A */
  UNI_PTEFFECT6,		/* dual effect 4+A */
  UNI_PTEFFECT7,		/* tremolo */
  UNI_PTEFFECT8,		/* pan */
  UNI_PTEFFECT9,		/* sample offset */
  UNI_PTEFFECTA,		/* volume slide */
  UNI_PTEFFECTB,		/* pattern jump */
  UNI_PTEFFECTC,		/* set volume */
  UNI_PTEFFECTD,		/* pattern break */
  UNI_PTEFFECTE,		/* extended effects */
  UNI_PTEFFECTF,		/* set speed */
  /* Scream Tracker effects */
  UNI_S3MEFFECTA,		/* set speed */
  UNI_S3MEFFECTD,		/* volume slide */
  UNI_S3MEFFECTE,		/* porta down */
  UNI_S3MEFFECTF,		/* porta up */
  UNI_S3MEFFECTI,		/* tremor */
  UNI_S3MEFFECTQ,		/* retrig */
  UNI_S3MEFFECTR,		/* tremolo */
  UNI_S3MEFFECTT,		/* set tempo */
  UNI_S3MEFFECTU,		/* fine vibrato */
  UNI_KEYOFF,		/* note off */
  /* Fast Tracker effects */
  UNI_KEYFADE,		/* note fade */
  UNI_VOLEFFECTS,		/* volume column effects */
  UNI_XMEFFECT4,		/* vibrato */
  UNI_XMEFFECTA,		/* volume slide */
  UNI_XMEFFECTE1,		/* fine porta up */
  UNI_XMEFFECTE2,		/* fine porta down */
  UNI_XMEFFECTEA,		/* fine volume slide up */
  UNI_XMEFFECTEB,		/* fine volume slide down */
  UNI_XMEFFECTG,		/* set global volume */
  UNI_XMEFFECTH,		/* global volume slide */
  UNI_XMEFFECTL,		/* set envelope position */
  UNI_XMEFFECTP,		/* pan slide */
  UNI_XMEFFECTX1,		/* extra fine porta up */
  UNI_XMEFFECTX2,		/* extra fine porta down */
  /* Impulse Tracker effects */
  UNI_ITEFFECTG,		/* porta to note */
  UNI_ITEFFECTH,		/* vibrato */
  UNI_ITEFFECTI,		/* tremor (xy not incremented) */
  UNI_ITEFFECTM,		/* set channel volume */
  UNI_ITEFFECTN,		/* slide / fineslide channel volume */
  UNI_ITEFFECTP,		/* slide / fineslide channel panning */
  UNI_ITEFFECTT,		/* slide tempo */
  UNI_ITEFFECTU,		/* fine vibrato */
  UNI_ITEFFECTW,		/* slide / fineslide global volume */
  UNI_ITEFFECTY,		/* panbrello */
  UNI_ITEFFECTZ,		/* resonant filters */
  UNI_ITEFFECTS0,
  /* UltraTracker effects */
  UNI_ULTEFFECT9,		/* Sample fine offset */
  /* OctaMED effects */
  UNI_MEDSPEED,
  UNI_MEDEFFECTF1,		/* play note twice */
  UNI_MEDEFFECTF2,		/* delay note */
  UNI_MEDEFFECTF3,		/* play note three times */

  UNI_LAST
};

extern UWORD unioperands[UNI_LAST];

/* IT / S3M Extended SS effects: */
enum
{
  SS_GLISSANDO = 1,
  SS_FINETUNE,
  SS_VIBWAVE,
  SS_TREMWAVE,
  SS_PANWAVE,
  SS_FRAMEDELAY,
  SS_S7EFFECTS,
  SS_PANNING,
  SS_SURROUND,
  SS_HIOFFSET,
  SS_PATLOOP,
  SS_NOTECUT,
  SS_NOTEDELAY,
  SS_PATDELAY
};

/* IT Volume column effects */
enum
{
  VOL_VOLUME = 1,
  VOL_PANNING,
  VOL_VOLSLIDE,
  VOL_PITCHSLIDEDN,
  VOL_PITCHSLIDEUP,
  VOL_PORTAMENTO,
  VOL_VIBRATO
};

/* IT resonant filter information */

#define FILT_CUT      0x80
#define FILT_RESONANT 0x81

typedef struct FILTER
{
    UBYTE filter, inf;
}
FILTER;

/*========== Instruments */

/* Instrument format flags */
#define IF_OWNPAN       1
#define IF_PITCHPAN     2

/* Envelope flags: */
#define EF_ON           1
#define EF_SUSTAIN      2
#define EF_LOOP         4
#define EF_VOLENV       8

/* New Note Action Flags */
#define NNA_CUT         0
#define NNA_CONTINUE    1
#define NNA_OFF         2
#define NNA_FADE        3

#define NNA_MASK        3

#define DCT_OFF         0
#define DCT_NOTE        1
#define DCT_SAMPLE      2
#define DCT_INST        3

#define DCA_CUT         0
#define DCA_OFF         1
#define DCA_FADE        2

#define KEY_KICK        0
#define KEY_OFF         1
#define KEY_FADE        2
#define KEY_KILL        (KEY_OFF|KEY_FADE)

#define KICK_ABSENT     0
#define KICK_NOTE       1
#define KICK_KEYOFF     2
#define KICK_ENV        4

#define AV_IT           1	/* IT vs. XM vibrato info */


/*
 *    ========== Samples
 */

/* Sample format [loading and in-memory] flags: */
#define SF_16BITS       0x0001
#define SF_STEREO       0x0002
#define SF_SIGNED       0x0004
#define SF_BIG_ENDIAN   0x0008
#define SF_DELTA        0x0010
#define SF_ITPACKED	0x0020

#define	SF_FORMATMASK	0x003F

/* General Playback flags */

#define SF_LOOP         0x0100
#define SF_BIDI         0x0200
#define SF_REVERSE      0x0400
#define SF_SUSTAIN      0x0800

#define SF_PLAYBACKMASK	0x0C00

/* Module-only Playback Flags */

#define SF_OWNPAN	0x1000
#define SF_UST_LOOP     0x2000

#define SF_EXTRAPLAYBACKMASK	0x3000

/* Panning constants */
#define PAN_LEFT       0
#define PAN_CENTER   128
#define PAN_RIGHT    255
#define PAN_SURROUND 512	/* panning value for Dolby Surround */

/* This stuff is all filled -- it's up to you whether to implement it. */
typedef struct SAMPLE
{
  SWORD panning;		/* panning (0-255 or PAN_SURROUND) */
  ULONG speed;		/* Base playing speed/frequency of note */
  UBYTE volume;		/* volume 0-64 */
  UWORD inflags;		/* sample format on disk */
  UWORD flags;		/* sample format in memory */
  ULONG length;		/* length of sample (in samples!) */
  ULONG loopstart;		/* repeat position (relative to start, in samples) */
  ULONG loopend;		/* repeat end */
  ULONG susbegin;		/* sustain loop begin (in samples) */
  ULONG susend;		/* sustain loop end                */

  UBYTE globvol;		/* global volume */
  UBYTE vibflags;		/* autovibrato flag stuffs */
  UBYTE vibtype;		/* Vibratos moved from INSTRUMENT to SAMPLE */
  UBYTE vibsweep;
  UBYTE vibdepth;
  UBYTE vibrate;
  CHAR *samplename;		/* name of the sample */

  UWORD id;			/* available for user */
  UBYTE divfactor;		/* for sample scaling */
  ULONG seekpos;		/* seek position in file -- internal */
  SWORD *data;			/* ptr to actual sample data */
}
SAMPLE;

/*
 *    ========== Internal module representation (UniMod)
 */

/*
   Instrument definition - for information only, the only field which may be
   of use in user programs is the name field
 */

/* Instrument note count */
#define INSTNOTES 120

/* Envelope point */
typedef struct ENVPT
{
  SWORD pos;
  SWORD val;
}
ENVPT;

/* Envelope point count */
#define ENVPOINTS 32

/* Instrument structure */
typedef struct INSTRUMENT
{
  CHAR *insname;

  UBYTE flags;
  UWORD samplenumber[INSTNOTES];
  UBYTE samplenote[INSTNOTES];

  UBYTE nnatype;
  UBYTE dca;		/* duplicate check action */
  UBYTE dct;		/* duplicate check type */
  UBYTE globvol;
  UWORD volfade;
  SWORD panning;		/* instrument-based panning var */

  UBYTE pitpansep;		/* pitch pan separation (0 to 255) */
  UBYTE pitpancenter;	/* pitch pan center (0 to 119) */
  UBYTE rvolvar;		/* random volume varations (0 - 100%) */
  UBYTE rpanvar;		/* random panning varations (0 - 100%) */

  /* volume envelope */
  UBYTE volflg;		/* bit 0: on 1: sustain 2: loop */
  UBYTE volpts;
  UBYTE volsusbeg;
  UBYTE volsusend;
  UBYTE volbeg;
  UBYTE volend;
  ENVPT volenv[ENVPOINTS];
  /* panning envelope */
  UBYTE panflg;		/* bit 0: on 1: sustain 2: loop */
  UBYTE panpts;
  UBYTE pansusbeg;
  UBYTE pansusend;
  UBYTE panbeg;
  UBYTE panend;
  ENVPT panenv[ENVPOINTS];
  /* pitch envelope */
  UBYTE pitflg;		/* bit 0: on 1: sustain 2: loop */
  UBYTE pitpts;
  UBYTE pitsusbeg;
  UBYTE pitsusend;
  UBYTE pitbeg;
  UBYTE pitend;
  ENVPT pitenv[ENVPOINTS];
}
INSTRUMENT;

/* Module flags */
#define UF_XMPERIODS 0x0001	/* XM periods / finetuning */
#define UF_LINEAR    0x0002	/* LINEAR periods (UF_XMPERIODS must be set) */
#define UF_INST      0x0004	/* Instruments are used */
#define UF_NNA       0x0008	/* IT: NNA used, set numvoices rather than numchn */
#define UF_S3MSLIDES 0x0010	/* uses old S3M volume slides */
#define UF_BGSLIDES  0x0020	/* continue volume slides in the background */
#define UF_HIGHBPM   0x0040	/* MED: can use >255 bpm */
#define UF_NOWRAP    0x0080	/* XM-type (i.e. illogical) pattern brk semantics */
#define UF_ARPMEM    0x0100	/* IT: need arpeggio memory */
#define UF_FT2QUIRKS 0x0200	/* emulate some FT2 replay quirks */

typedef struct MODULE
{
  /* general module information */
  CHAR *songname;		/* name of the song */
  CHAR *modtype;		/* string type of module loaded */
  CHAR *comment;		/* module comments */

  UWORD flags;		/* See module flags above */
  UBYTE numchn;		/* number of module channels */
  UBYTE numvoices;		/* max # voices used for full NNA playback */
  UWORD numpos;		/* number of positions in this song */
  UWORD numpat;		/* number of patterns in this song */
  UWORD numins;		/* number of instruments */
  UWORD numsmp;		/* number of samples */
  INSTRUMENT *instruments;	/* all instruments */
  SAMPLE *samples;		/* all samples */

  /* playback settings */
  UWORD reppos;		/* restart position */
  UBYTE initspeed;		/* initial song speed */
  UWORD inittempo;		/* initial song tempo */
  UBYTE initvolume;		/* initial global volume (0 - 128) */
  UWORD panning[64];	/* 64 panning positions */
  UBYTE chanvol[64];	/* 64 channel positions */
  UWORD bpm;		/* current beats-per-minute speed */

  /* internal module representation */
  UWORD numtrk;		/* number of tracks */
  UBYTE **tracks;		/* array of numtrk pointers to tracks */
  UWORD *patterns;		/* array of Patterns */
  UWORD *pattrows;		/* array of number of rows for each pattern */
  UWORD *positions;		/* all positions */
}
  MODULE;

/* used to convert c4spd to linear XM periods (IT and IMF loaders). */
extern UWORD finetune[];
extern UWORD getlinearperiod (UWORD, ULONG);
extern UWORD getlogperiod (UWORD note, ULONG fine);
extern UWORD getoldperiod (UWORD, ULONG);
extern ULONG getfrequency (UBYTE, ULONG);
extern ULONG getAmigaPeriod (UBYTE, ULONG);

/*
 *    ========== External interface
 */

extern UWORD finetune[];
extern BOOL ML_8bitsamples;
extern BOOL ML_monosamples;
extern CHAR *ML_InfoLoader (void);
extern void ML_RegisterAllLoaders (void);
extern BOOL ML_Test (URL);
extern MODULE *ML_Load (URL, int, BOOL);
extern CHAR *ML_LoadTitle (URL);
extern void ML_Free (MODULE *);

#ifdef __cplusplus
}
#endif

#endif

/* ex:set ts=4: */
