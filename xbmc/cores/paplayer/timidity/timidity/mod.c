/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    Most of this file is taken from the MikMod sound library, which is
    (c) 1998, 1999 Miodrag Vallat and others - see file libunimod/AUTHORS
    for complete list.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* Interface to libunimod + module player */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef SUNOS
extern long int random (void);
#endif

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "tables.h"
#include "mod.h"
#include "output.h"
#include "controls.h"
#include "unimod.h"
#include "unimod_priv.h"
#include "mod2midi.h"

static BOOL mod_do_play (MODULE *);

int 
load_module_file (struct timidity_file *tf, int mod_type)
{
  MODULE *mf;

#ifdef LOOKUP_HACK
  ML_8bitsamples = 1;
#else
  ML_8bitsamples = 0;
#endif
  ML_monosamples = 1;

  ML_RegisterAllLoaders ();
  mf = ML_Load (tf->url, MOD_NUM_VOICES, 0);
  if (ML_errno)
    return 1;

  current_file_info->file_type = mod_type;
  load_module_samples (mf->samples, mf->numsmp, mod_type == IS_MOD_FILE);
  mod_do_play (mf);
  ML_Free (mf);
  return 0;
}


int 
get_module_type (char *fn)
{
  if (check_file_extension (fn, ".mod", 1))	/* Most common first */
    return IS_MOD_FILE;

  if (check_file_extension (fn, ".xm", 1)
      || check_file_extension (fn, ".s3m", 1)
      || check_file_extension (fn, ".it", 1)
      || check_file_extension (fn, ".669", 1)	/* Then the others in alphabetic order */
      || check_file_extension (fn, ".amf", 1)
      || check_file_extension (fn, ".dsm", 1)
      || check_file_extension (fn, ".far", 1)
      || check_file_extension (fn, ".gdm", 1)
      || check_file_extension (fn, ".imf", 1)
      || check_file_extension (fn, ".med", 1)
      || check_file_extension (fn, ".mtm", 1)
      || check_file_extension (fn, ".stm", 1)
      || check_file_extension (fn, ".stx", 1)
      || check_file_extension (fn, ".ult", 1)
      || check_file_extension (fn, ".uni", 1))

    return IS_S3M_FILE;

  return IS_OTHER_FILE;
}

char *
get_module_title (struct timidity_file *tf, int mod_type)
{
  return ML_LoadTitle (tf->url);
}

/*========== Playing */

#define POS_NONE        (-2)	/* no loop position defined */

typedef struct ENVPR
{
  UBYTE flg;			/* envelope flag */
  UBYTE pts;			/* number of envelope points */
  UBYTE susbeg;			/* envelope sustain index begin */
  UBYTE susend;			/* envelope sustain index end */
  UBYTE beg;			/* envelope loop begin */
  UBYTE end;			/* envelope loop end */
  SWORD p;			/* current envelope counter */
  UWORD a;			/* envelope index a */
  UWORD b;			/* envelope index b */
  ENVPT *env;			/* envelope points */
}
ENVPR;

typedef struct MP_CONTROL
  {
    INSTRUMENT *i;
    SAMPLE *s;
    UBYTE sample;		/* which sample number */
    UBYTE note;			/* the audible note as heard, direct rep of period */
    SWORD outvolume;		/* output volume (vol + sampcol + instvol) */
    SBYTE chanvol;		/* channel's "global" volume */
    UWORD fadevol;		/* fading volume rate */
    SWORD panning;		/* panning position */
    UBYTE kick;			/* if true = sample has to be restarted */
    UWORD period;		/* period to play the sample at */
    UBYTE nna;			/* New note action type + master/slave flags */

    UBYTE volflg;		/* volume envelope settings */
    UBYTE panflg;		/* panning envelope settings */
    UBYTE pitflg;		/* pitch envelope settings */

    UBYTE keyoff;		/* if true = fade out and stuff */
    SWORD *data;		/* which sample-data to play */
    UBYTE notedelay;		/* (used for note delay) */
    SLONG start;		/* The starting byte index in the sample */

    UWORD ultoffset;		/* fine sample offset memory */

    struct MP_VOICE *slave;	/* Audio Slave of current effects control channel */
    UBYTE slavechn;		/* Audio Slave of current effects control channel */
    UBYTE anote;		/* the note that indexes the audible */
    UBYTE oldnote;
    SWORD ownper;
    SWORD ownvol;
    UBYTE dca;			/* duplicate check action */
    UBYTE dct;			/* duplicate check type */
    UBYTE *row;			/* row currently playing on this channel */
    SBYTE retrig;		/* retrig value (0 means don't retrig) */
    ULONG speed;		/* what finetune to use */
    SWORD volume;		/* amiga volume (0 t/m 64) to play the sample at */

    SWORD tmpvolume;		/* tmp volume */
    UWORD tmpperiod;		/* tmp period */
    UWORD wantedperiod;		/* period to slide to (with effect 3 or 5) */

    UBYTE arpmem;		/* arpeggio command memory */
    UBYTE pansspd;		/* panslide speed */
    UWORD slidespeed;		/* */
    UWORD portspeed;		/* noteslide speed (toneportamento) */

    UBYTE s3mtremor;		/* s3m tremor (effect I) counter */
    UBYTE s3mtronof;		/* s3m tremor ontime/offtime */
    UBYTE s3mvolslide;		/* last used volslide */
    SBYTE sliding;
    UBYTE s3mrtgspeed;		/* last used retrig speed */
    UBYTE s3mrtgslide;		/* last used retrig slide */

    UBYTE glissando;		/* glissando (0 means off) */
    UBYTE wavecontrol;

    SBYTE vibpos;		/* current vibrato position */
    UBYTE vibspd;		/* "" speed */
    UBYTE vibdepth;		/* "" depth */

    SBYTE trmpos;		/* current tremolo position */
    UBYTE trmspd;		/* "" speed */
    UBYTE trmdepth;		/* "" depth */

    UBYTE fslideupspd;
    UBYTE fslidednspd;
    UBYTE fportupspd;		/* fx E1 (extra fine portamento up) data */
    UBYTE fportdnspd;		/* fx E2 (extra fine portamento dn) data */
    UBYTE ffportupspd;		/* fx X1 (extra fine portamento up) data */
    UBYTE ffportdnspd;		/* fx X2 (extra fine portamento dn) data */

    ULONG hioffset;		/* last used high order of sample offset */
    UWORD soffset;		/* last used low order of sample-offset (effect 9) */

    UBYTE sseffect;		/* last used Sxx effect */
    UBYTE ssdata;		/* last used Sxx data info */
    UBYTE chanvolslide;		/* last used channel volume slide */

    UBYTE panbwave;		/* current panbrello waveform */
    UBYTE panbpos;		/* current panbrello position */
    SBYTE panbspd;		/* "" speed */
    UBYTE panbdepth;		/* "" depth */

    UWORD newsamp;		/* set to 1 upon a sample / inst change */
    UBYTE voleffect;		/* Volume Column Effect Memory as used by IT */
    UBYTE voldata;		/* Volume Column Data Memory */

    SWORD pat_reppos;		/* patternloop position */
    UWORD pat_repcnt;		/* times to loop */
  }
MP_CONTROL;

/* Used by NNA only player (audio control.  MP_CONTROL is used for full effects
   control). */
typedef struct MP_VOICE
  {
    INSTRUMENT *i;
    SAMPLE *s;
    UBYTE sample;		/* which instrument number */

    SWORD volume;		/* output volume (vol + sampcol + instvol) */
    SWORD panning;		/* panning position */
    SBYTE chanvol;		/* channel's "global" volume */
    UWORD fadevol;		/* fading volume rate */
    UWORD period;		/* period to play the sample at */

    UBYTE volflg;		/* volume envelope settings */
    UBYTE panflg;		/* panning envelope settings */
    UBYTE pitflg;		/* pitch envelope settings */

    UBYTE keyoff;		/* if true = fade out and stuff */
    UBYTE kick;			/* if true = sample has to be restarted */
    UBYTE note;			/* the audible note (as heard, direct rep of period) */
    UBYTE nna;			/* New note action type + master/slave flags */
    SWORD *data;		/* which sample-data to play */
    SLONG start;		/* The start byte index in the sample */

/* Below here is info NOT in MP_CONTROL!! */
    ENVPR venv;
    ENVPR penv;
    ENVPR cenv;

    UWORD avibpos;		/* autovibrato pos */
    UWORD aswppos;		/* autovibrato sweep pos */

    ULONG totalvol;		/* total volume of channel (before global mixings) */

    BOOL mflag;
    SWORD masterchn;
    UWORD masterperiod;

    MP_CONTROL *master;		/* index of "master" effects channel */
  }
MP_VOICE;

typedef struct MP_STATUS
  {
    SWORD channel;		/* channel we're working on */
    UWORD oldsngspd;		/* old song speed */
    UWORD sngspd;		/* current song speed */
    SWORD volume;		/* song volume (0-128) (or user volume) */
    UWORD bpm;			/* current beats-per-minute speed */
    UWORD newbpm;		/* next beats-per-minute speed */

    UBYTE realchn;		/* real number of channels used */
    UBYTE totalchn;		/* total number of channels used (incl NNAs) */

    UWORD patpos;		/* current row number */
    SWORD sngpos;		/* current song position */

    UWORD numrow;		/* number of rows on current pattern */
    UWORD vbtick;		/* tick counter (counts from 0 to sngspd) */

    struct MP_CONTROL *control;	/* Effects Channel info (size pf->numchn) */
    struct MP_VOICE voice[MOD_NUM_VOICES];	/* Audio Voice information */

    UBYTE globalslide;		/* global volume slide rate */
    UBYTE pat_repcrazy;		/* module has just looped to position -1 */
    UWORD patbrk;		/* position where to start a new pattern */
    UBYTE patdly;		/* patterndelay counter (command memory) */
    UBYTE patdly2;		/* patterndelay counter (real one) */
    SWORD posjmp;		/* flag to indicate a jump is needed... */
    int explicitslides;
  }
MP_STATUS;

MODULE *pf = NULL;		/* modfile being played */
static MP_CONTROL *a;		/* current AUDTMP we're working on */
static MP_STATUS mp;		/* player status */

static UBYTE VibratoTable[32] =
{
  0, 24, 49, 74, 97, 120, 141, 161, 180, 197, 212, 224, 235, 244, 250, 253,
  255, 253, 250, 244, 235, 224, 212, 197, 180, 161, 141, 120, 97, 74, 49, 24
};

static UBYTE avibtab[128] =
{
  0, 1, 3, 4, 6, 7, 9, 10, 12, 14, 15, 17, 18, 20, 21, 23,
  24, 25, 27, 28, 30, 31, 32, 34, 35, 36, 38, 39, 40, 41, 42, 44,
  45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 54, 55, 56, 57, 57, 58,
  59, 59, 60, 60, 61, 61, 62, 62, 62, 63, 63, 63, 63, 63, 63, 63,
  64, 63, 63, 63, 63, 63, 63, 63, 62, 62, 62, 61, 61, 60, 60, 59,
  59, 58, 57, 57, 56, 55, 54, 54, 53, 52, 51, 50, 49, 48, 47, 46,
  45, 44, 42, 41, 40, 39, 38, 36, 35, 34, 32, 31, 30, 28, 27, 25,
  24, 23, 21, 20, 18, 17, 15, 14, 12, 10, 9, 7, 6, 4, 3, 1
};

static SBYTE PanbrelloTable[256] =
{
  0, 2, 3, 5, 6, 8, 9, 11, 12, 14, 16, 17, 19, 20, 22, 23,
  24, 26, 27, 29, 30, 32, 33, 34, 36, 37, 38, 39, 41, 42, 43, 44,
  45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 57, 58, 59,
  59, 60, 60, 61, 61, 62, 62, 62, 63, 63, 63, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 63, 63, 63, 62, 62, 62, 61, 61, 60, 60,
  59, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46,
  45, 44, 43, 42, 41, 39, 38, 37, 36, 34, 33, 32, 30, 29, 27, 26,
  24, 23, 22, 20, 19, 17, 16, 14, 12, 11, 9, 8, 6, 5, 3, 2,
  0, -2, -3, -5, -6, -8, -9, -11, -12, -14, -16, -17, -19, -20, -22, -23,
  -24, -26, -27, -29, -30, -32, -33, -34, -36, -37, -38, -39, -41, -42, -43, -44,
  -45, -46, -47, -48, -49, -50, -51, -52, -53, -54, -55, -56, -56, -57, -58, -59,
  -59, -60, -60, -61, -61, -62, -62, -62, -63, -63, -63, -64, -64, -64, -64, -64,
  -64, -64, -64, -64, -64, -64, -63, -63, -63, -62, -62, -62, -61, -61, -60, -60,
  -59, -59, -58, -57, -56, -56, -55, -54, -53, -52, -51, -50, -49, -48, -47, -46,
  -45, -44, -43, -42, -41, -39, -38, -37, -36, -34, -33, -32, -30, -29, -27, -26,
  -24, -23, -22, -20, -19, -17, -16, -14, -12, -11, -9, -8, -6, -5, -3, -2
};

/* returns a random value between 0 and ceil-1, ceil must be a power of two */
static int 
getrandom (int ceil)
{
#ifdef HAVE_SRANDOM
  return random () & (ceil - 1);
#else
  return (rand () * ceil) / (RAND_MAX + 1.0);
#endif
}

/*      New Note Action Scoring System :
   --------------------------------
   1)   total-volume (fadevol, chanvol, volume) is the main scorer.
   2)   a looping sample is a bonus x2
   3)   a foreground channel is a bonus x4
   4)   an active envelope with keyoff is a handicap -x2                          */
static int 
MP_FindEmptyChannel (void)
{
  MP_VOICE *a;
  ULONG t, k, tvol, pp;

  a = mp.voice;
  for (t = 0; t < MOD_NUM_VOICES; t++)
    {
      /* allow us to take over a nonexisting sample */
      if (!a->s)
        return t;

      if (((mp.voice[t].kick == KICK_ABSENT) || (mp.voice[t].kick == KICK_ENV)) &&
	  Voice_Stopped (t))
        return t;
    }

  tvol = 0xffffffUL;
  t = 0;
  for (k = 0; k < MOD_NUM_VOICES; k++, a++)
    if ((a->kick == KICK_ABSENT) || (a->kick == KICK_ENV))
      {
	pp = a->totalvol << ((a->s->flags & SF_LOOP) ? 1 : 0);
	if ((a->master) && (a == a->master->slave))
	  pp <<= 2;

	if (pp < tvol)
	  {
	    tvol = pp;
	    t = k;
	  }
      }

  if (tvol > 8000 * 7)
    return -1;

  return t;
}

static UWORD GetPeriod (UWORD note, ULONG speed)
{
  if (pf->flags & UF_XMPERIODS)
    return (pf->flags & UF_LINEAR) ? getlinearperiod (note, speed) : getlogperiod (note, speed);

  return getoldperiod (note, speed);
}


static SWORD 
Interpolate (SWORD p, SWORD p1, SWORD p2, SWORD v1, SWORD v2)
{
  if ((p1 == p2) || (p == p1))
    return v1;
  return v1 + ((SLONG) ((p - p1) * (v2 - v1)) / (p2 - p1));
}


static SWORD 
InterpolateEnv (SWORD p, ENVPT * a, ENVPT * b)
{
  return (Interpolate (p, a->pos, b->pos, a->val, b->val));
}

static SWORD 
DoPan (SWORD envpan, SWORD pan)
{
  int newpan;

  newpan = pan + (((envpan - PAN_CENTER) * (128 - abs (pan - PAN_CENTER))) / 128);

  return (newpan < PAN_LEFT) ? PAN_LEFT : (newpan > PAN_RIGHT ? PAN_RIGHT : newpan);
}

static void 
StartEnvelope (ENVPR * t, UBYTE flg, UBYTE pts, UBYTE susbeg, UBYTE susend, UBYTE beg, UBYTE end, ENVPT * p, UBYTE keyoff)
{
  t->flg = flg;
  t->pts = pts;
  t->susbeg = susbeg;
  t->susend = susend;
  t->beg = beg;
  t->end = end;
  t->env = p;
  t->p = 0;
  t->a = 0;
  t->b = ((t->flg & EF_SUSTAIN) && (!(keyoff & KEY_OFF))) ? 0 : 1;

  /* Imago Orpheus sometimes stores an extra initial point in the envelope */
  if ((t->pts >= 2) && (t->env[0].pos == t->env[1].pos))
    {
      t->a++;
      t->b++;
    }

  if (t->b >= t->pts)
    t->b = t->pts - 1;
}

/* This procedure processes all envelope types, include volume, pitch, and
   panning.  Envelopes are defined by a set of points, each with a magnitude
   [relating either to volume, panning position, or pitch modifier] and a tick
   position.

   Envelopes work in the following manner:

   (a) Each tick the envelope is moved a point further in its progression. For
   an accurate progression, magnitudes between two envelope points are
   interpolated.

   (b) When progression reaches a defined point on the envelope, values are
   shifted to interpolate between this point and the next, and checks for
   loops or envelope end are done.

   Misc:
   Sustain loops are loops that are only active as long as the keyoff flag is
   clear.  When a volume envelope terminates, so does the current fadeout.  */
static SWORD 
ProcessEnvelope (ENVPR * t, SWORD v, UBYTE keyoff)
{
  if (t->flg & EF_ON)
    {
      UBYTE a, b;		/* actual points in the envelope */
      UWORD p;			/* the 'tick counter' - real point being played */

      a = t->a;
      b = t->b;
      p = t->p;

      /* if sustain loop on one point (XM type), don't move and don't
         interpolate when the point is reached */
      if ((t->flg & EF_SUSTAIN) && (t->susbeg == t->susend) &&
	  (!(keyoff & KEY_OFF)) && (p == t->env[t->susbeg].pos))
	v = t->env[t->susbeg].val;
      else
	{
	  /* compute the current envelope value between points a and b */
	  if (a == b)
	    v = t->env[a].val;
	  else
	    v = InterpolateEnv (p, &t->env[a], &t->env[b]);

	  p++;
	  /* pointer reached point b? */
	  if (p >= t->env[b].pos)
	    {
	      a = b++;		/* shift points a and b */

	      /* Check for loops, sustain loops, or end of envelope. */
	      if ((t->flg & EF_SUSTAIN) && (!(keyoff & KEY_OFF)) && (b > t->susend))
		{
		  a = t->susbeg;
		  b = (t->susbeg == t->susend) ? a : a + 1;
		  p = t->env[a].pos;
		}
	      else if ((t->flg & EF_LOOP) && (b > t->end))
		{
		  a = t->beg;
		  b = (t->beg == t->end) ? a : a + 1;
		  p = t->env[a].pos;
		}
	      else
		{
		  if (b >= t->pts)
		    {
		      if ((t->flg & EF_VOLENV) && (mp.channel != -1))
			{
			  mp.voice[mp.channel].keyoff |= KEY_FADE;
			  if (!v)
			    mp.voice[mp.channel].fadevol = 0;
			}
		      b--;
		      p--;
		    }
		}
	    }
	  t->a = a;
	  t->b = b;
	  t->p = p;
	}
    }
  return v;
}

/*========== Protracker effects */

static void 
DoEEffects (UBYTE dat)
{
  UBYTE nib = dat & 0xf;

  switch (dat >> 4)
    {
    case 0x0:			/* hardware filter toggle, not supported */
      break;
    case 0x1:			/* fineslide up */
      if (a->period)
	if (!mp.vbtick)
	  a->tmpperiod -= (nib << 2);
      break;
    case 0x2:			/* fineslide dn */
      if (a->period)
	if (!mp.vbtick)
	  a->tmpperiod += (nib << 2);
      break;
    case 0x3:			/* glissando ctrl */
      a->glissando = nib;
      break;
    case 0x4:			/* set vibrato waveform */
      a->wavecontrol &= 0xf0;
      a->wavecontrol |= nib;
      break;
    case 0x5:			/* set finetune */
      if (a->period)
	{
	  if (pf->flags & UF_XMPERIODS)
	    a->speed = nib + 128;
	  else
	    a->speed = finetune[nib];
	  a->tmpperiod = GetPeriod ((UWORD) a->note << 1, a->speed);
	}
      break;
    case 0x6:			/* set patternloop */
      if (mp.vbtick)
	break;
      if (nib)
	{			/* set reppos or repcnt ? */
	  /* set repcnt, so check if repcnt already is set, which means we
	     are already looping */
	  if (a->pat_repcnt)
	    a->pat_repcnt--;	/* already looping, decrease counter */
	  else
	    {
#if 0
	      /* this would make walker.xm, shipped with Xsoundtracker,
	         play correctly, but it's better to remain compatible
	         with FT2 */
	      if ((!(pf->flags & UF_NOWRAP)) || (a->pat_reppos != POS_NONE))
#endif
		a->pat_repcnt = nib;	/* not yet looping, so set repcnt */
	    }

	  if (a->pat_repcnt)
	    {			/* jump to reppos if repcnt>0 */
	      if (a->pat_reppos == POS_NONE)
		a->pat_reppos = mp.patpos - 1;
	      if (a->pat_reppos == -1)
		{
		  mp.pat_repcrazy = 1;
		  mp.patpos = 0;
		}
	      else
		mp.patpos = a->pat_reppos;
	    }
	  else
	    a->pat_reppos = POS_NONE;
	}
      else
	a->pat_reppos = mp.patpos - 1;	/* set reppos - can be (-1) */
      break;
    case 0x7:			/* set tremolo waveform */
      a->wavecontrol &= 0x0f;
      a->wavecontrol |= nib << 4;
      break;
    case 0x8:			/* set panning */
      if (nib <= 8)
	nib <<= 4;
      else
	nib *= 17;
      a->panning = pf->panning[mp.channel] = nib;
      break;
    case 0x9:			/* retrig note */
      /* only retrigger if data nibble > 0 */
      if (nib)
	{
	  if (!a->retrig)
	    {
	      /* when retrig counter reaches 0, reset counter and restart
	         the sample */
	      if (a->period)
		a->kick = KICK_NOTE;
	      a->retrig = nib;
	    }
	  a->retrig--;		/* countdown */
	}
      break;
    case 0xa:			/* fine volume slide up */
      if (mp.vbtick)
	break;
      a->tmpvolume += nib;
      if (a->tmpvolume > 64)
	a->tmpvolume = 64;
      break;
    case 0xb:			/* fine volume slide dn  */
      if (mp.vbtick)
	break;
      a->tmpvolume -= nib;
      if (a->tmpvolume < 0)
	a->tmpvolume = 0;
      break;
    case 0xc:			/* cut note */
      /* When mp.vbtick reaches the cut-note value, turn the volume to
         zero ( Just like on the amiga) */
      if (mp.vbtick >= nib)
	a->tmpvolume = 0;	/* just turn the volume down */
      break;
    case 0xd:			/* note delay */
      /* delay the start of the sample until mp.vbtick==nib */
      if (!mp.vbtick)
	a->notedelay = nib;
      else if (a->notedelay)
	a->notedelay--;
      break;
    case 0xe:			/* pattern delay */
      if (mp.vbtick)
	break;
      if (!mp.patdly2)
	mp.patdly = nib + 1;	/* only once, when vbtick=0 */
      break;
    case 0xf:			/* invert loop, not supported  */
      break;
    }
}

static void 
DoVibrato (void)
{
  UBYTE q;
  UWORD temp = 0;

  q = (a->vibpos >> 2) & 0x1f;

  switch (a->wavecontrol & 3)
    {
    case 0:			/* sine */
      temp = VibratoTable[q];
      break;
    case 1:			/* ramp down */
      q <<= 3;
      if (a->vibpos < 0)
	q = 255 - q;
      temp = q;
      break;
    case 2:			/* square wave */
      temp = 255;
      break;
    case 3:			/* random wave */
      temp = getrandom (256);
      break;
    }

  temp *= a->vibdepth;
  temp >>= 7;
  temp <<= 2;

  if (a->vibpos >= 0)
    a->period = a->tmpperiod + temp;
  else
    a->period = a->tmpperiod - temp;

  if (mp.vbtick)
    a->vibpos += a->vibspd;
}

static void 
DoTremolo (void)
{
  UBYTE q;
  UWORD temp = 0;

  q = (a->trmpos >> 2) & 0x1f;

  switch ((a->wavecontrol >> 4) & 3)
    {
    case 0:			/* sine */
      temp = VibratoTable[q];
      break;
    case 1:			/* ramp down */
      q <<= 3;
      if (a->trmpos < 0)
	q = 255 - q;
      temp = q;
      break;
    case 2:			/* square wave */
      temp = 255;
      break;
    case 3:			/* random wave */
      temp = getrandom (256);
      break;
    }
  temp *= a->trmdepth;
  temp >>= 6;

  if (a->trmpos >= 0)
    {
      a->volume = a->tmpvolume + temp;
      if (a->volume > 64)
	a->volume = 64;
    }
  else
    {
      a->volume = a->tmpvolume - temp;
      if (a->volume < 0)
	a->volume = 0;
    }

  if (mp.vbtick)
    a->trmpos += a->trmspd;
}

static void 
DoVolSlide (UBYTE dat)
{
  if (!mp.vbtick)
    return;

  if (dat & 0xf)
    {
      a->tmpvolume -= (dat & 0x0f);
      if (a->tmpvolume < 0)
	a->tmpvolume = 0;
    }
  else
    {
      a->tmpvolume += (dat >> 4);
      if (a->tmpvolume > 64)
	a->tmpvolume = 64;
    }
}

static void 
DoToneSlide (void)
{
  if (mp.vbtick)
    {
      int dist;

      /* We have to slide a->period towards a->wantedperiod, so compute the
         difference between those two values */
      dist = a->period - a->wantedperiod;

      /* if they are equal or if portamentospeed is too big ... */
      if ((!dist) || a->portspeed > abs (dist))
	/* ...make tmpperiod equal tperiod */
	a->tmpperiod = a->period = a->wantedperiod;
      else if (dist > 0)
	{
	  a->tmpperiod -= a->portspeed;
	  a->period -= a->portspeed;	/* dist>0, slide up */
	}
      else
	{
	  a->tmpperiod += a->portspeed;
	  a->period += a->portspeed;	/* dist<0, slide down */
	}
    }
  else
    a->tmpperiod = a->period;
}

static void 
DoArpeggio (UBYTE dat)
{
  UBYTE note = a->note;

  if (dat)
    {
      switch (mp.vbtick % 3)
	{
	case 1:
	  note += (dat >> 4);
	  break;
	case 2:
	  note += (dat & 0xf);
	  break;
	}
      a->period = GetPeriod ((UWORD) note << 1, a->speed);
      a->ownper = 1;
    }
}

/*========== Scream Tracker effects */

static void 
DoS3MVolSlide (UBYTE inf)
{
  UBYTE lo, hi;

  if (inf)
    a->s3mvolslide = inf;
  else
    inf = a->s3mvolslide;

  lo = inf & 0xf;
  hi = inf >> 4;

  if (!lo)
    {
      if ((mp.vbtick) || (pf->flags & UF_S3MSLIDES))
	a->tmpvolume += hi;
    }
  else if (!hi)
    {
      if ((mp.vbtick) || (pf->flags & UF_S3MSLIDES))
	a->tmpvolume -= lo;
    }
  else if (lo == 0xf)
    {
      if (!mp.vbtick)
	a->tmpvolume += (hi ? hi : 0xf);
    }
  else if (hi == 0xf)
    {
      if (!mp.vbtick)
	a->tmpvolume -= (lo ? lo : 0xf);
    }
  else
    return;

  if (a->tmpvolume < 0)
    a->tmpvolume = 0;
  else if (a->tmpvolume > 64)
    a->tmpvolume = 64;
}

static void 
DoS3MSlideDn (UBYTE inf)
{
  UBYTE hi, lo;

  if (inf)
    a->slidespeed = inf;
  else
    inf = a->slidespeed;

  hi = inf >> 4;
  lo = inf & 0xf;

  if (hi == 0xf)
    {
      if (!mp.vbtick)
	a->tmpperiod += (UWORD) lo << 2;
    }
  else if (hi == 0xe)
    {
      if (!mp.vbtick)
	a->tmpperiod += lo;
    }
  else
    {
      if (mp.vbtick)
	a->tmpperiod += (UWORD) inf << 2;
    }
}

static void 
DoS3MSlideUp (UBYTE inf)
{
  UBYTE hi, lo;

  if (inf)
    a->slidespeed = inf;
  else
    inf = a->slidespeed;

  hi = inf >> 4;
  lo = inf & 0xf;

  if (hi == 0xf)
    {
      if (!mp.vbtick)
	a->tmpperiod -= (UWORD) lo << 2;
    }
  else if (hi == 0xe)
    {
      if (!mp.vbtick)
	a->tmpperiod -= lo;
    }
  else
    {
      if (mp.vbtick)
	a->tmpperiod -= (UWORD) inf << 2;
    }
}

static void 
DoS3MTremor (UBYTE inf)
{
  UBYTE on, off;

  if (inf)
    a->s3mtronof = inf;
  else
    {
      inf = a->s3mtronof;
      if (!inf)
	return;
    }

  if (!mp.vbtick)
    return;

  on = (inf >> 4) + 1;
  off = (inf & 0xf) + 1;
  a->s3mtremor %= (on + off);
  a->volume = (a->s3mtremor < on) ? a->tmpvolume : 0;
  a->s3mtremor++;
}

static void 
DoS3MRetrig (UBYTE inf)
{
  if (inf)
    {
      a->s3mrtgslide = inf >> 4;
      a->s3mrtgspeed = inf & 0xf;
    }

  /* only retrigger if low nibble > 0 */
  if (a->s3mrtgspeed > 0)
    {
      if (!a->retrig)
	{
	  /* when retrig counter reaches 0, reset counter and restart the
	     sample */
	  if (a->kick != KICK_NOTE)
	    a->kick = KICK_KEYOFF;
	  a->retrig = a->s3mrtgspeed;

	  if ((mp.vbtick) || (pf->flags & UF_S3MSLIDES))
	    {
	      switch (a->s3mrtgslide)
		{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		  a->tmpvolume -= (1 << (a->s3mrtgslide - 1));
		  break;
		case 6:
		  a->tmpvolume = (2 * a->tmpvolume) / 3;
		  break;
		case 7:
		  a->tmpvolume >>= 1;
		  break;
		case 9:
		case 0xa:
		case 0xb:
		case 0xc:
		case 0xd:
		  a->tmpvolume += (1 << (a->s3mrtgslide - 9));
		  break;
		case 0xe:
		  a->tmpvolume = (3 * a->tmpvolume) >> 1;
		  break;
		case 0xf:
		  a->tmpvolume = a->tmpvolume << 1;
		  break;
		}
	      if (a->tmpvolume < 0)
		a->tmpvolume = 0;
	      else if (a->tmpvolume > 64)
		a->tmpvolume = 64;
	    }
	}
      a->retrig--;		/* countdown  */
    }
}

static void 
DoS3MSpeed (UBYTE speed)
{
  if (mp.vbtick || mp.patdly2)
    return;

  if (speed > 128)
    speed -= 128;
  if (speed)
    {
      mp.sngspd = speed;
      mp.vbtick = 0;
    }
}

static void 
DoS3MTempo (UBYTE tempo)
{
  if (mp.vbtick || mp.patdly2)
    return;

  mp.newbpm = (tempo < 32) ? 32 : tempo;
}

static void 
DoS3MFineVibrato (void)
{
  UBYTE q;
  UWORD temp = 0;

  q = (a->vibpos >> 2) & 0x1f;

  switch (a->wavecontrol & 3)
    {
    case 0:			/* sine */
      temp = VibratoTable[q];
      break;
    case 1:			/* ramp down */
      q <<= 3;
      if (a->vibpos < 0)
	q = 255 - q;
      temp = q;
      break;
    case 2:			/* square wave */
      temp = 255;
      break;
    case 3:			/* random */
      temp = getrandom (256);
      break;
    }

  temp *= a->vibdepth;
  temp >>= 8;

  if (a->vibpos >= 0)
    a->period = a->tmpperiod + temp;
  else
    a->period = a->tmpperiod - temp;

  a->vibpos += a->vibspd;
}

static void 
DoS3MTremolo (void)
{
  UBYTE q;
  UWORD temp = 0;

  q = (a->trmpos >> 2) & 0x1f;

  switch ((a->wavecontrol >> 4) & 3)
    {
    case 0:			/* sine */
      temp = VibratoTable[q];
      break;
    case 1:			/* ramp down */
      q <<= 3;
      if (a->trmpos < 0)
	q = 255 - q;
      temp = q;
      break;
    case 2:			/* square wave */
      temp = 255;
      break;
    case 3:			/* random */
      temp = getrandom (256);
      break;
    }

  temp *= a->trmdepth;
  temp >>= 7;

  if (a->trmpos >= 0)
    {
      a->volume = a->tmpvolume + temp;
      if (a->volume > 64)
	a->volume = 64;
    }
  else
    {
      a->volume = a->tmpvolume - temp;
      if (a->volume < 0)
	a->volume = 0;
    }

  if (mp.vbtick)
    a->trmpos += a->trmspd;
}

/*========== Fast Tracker effects */

static void 
DoXMVolSlide (UBYTE inf)
{
  UBYTE lo, hi;

  mp.explicitslides = 2;

  if (inf)
    a->s3mvolslide = inf;
  else
    inf = a->s3mvolslide;

  if (!mp.vbtick)
    return;

  lo = inf & 0xf;
  hi = inf >> 4;

  if (!hi)
    {
      a->tmpvolume -= lo;
      if (a->tmpvolume < 0)
	a->tmpvolume = 0;
    }
  else
    {
      a->tmpvolume += hi;
      if (a->tmpvolume > 64)
	a->tmpvolume = 64;
    }
}

static void 
DoXMGlobalSlide (UBYTE inf)
{
  if (mp.vbtick)
    {
      if (inf)
	mp.globalslide = inf;
      else
	inf = mp.globalslide;
      if (inf & 0xf0)
	inf &= 0xf0;
      mp.volume = mp.volume + ((inf >> 4) - (inf & 0xf)) * 2;

      if (mp.volume < 0)
	mp.volume = 0;
      else if (mp.volume > 128)
	mp.volume = 128;
    }
}

static void 
DoXMPanSlide (UBYTE inf)
{
  UBYTE lo, hi;
  SWORD pan;

  if (inf)
    a->pansspd = inf;
  else
    inf = a->pansspd;

  if (!mp.vbtick)
    return;

  lo = inf & 0xf;
  hi = inf >> 4;

  /* slide right has absolute priority */
  if (hi)
    lo = 0;

  pan = ((a->panning == PAN_SURROUND) ? PAN_CENTER : a->panning) + hi - lo;

  a->panning = (pan < PAN_LEFT) ? PAN_LEFT : (pan > PAN_RIGHT ? PAN_RIGHT : pan);
}

static void 
DoXMExtraFineSlideUp (UBYTE inf)
{
  if (!mp.vbtick)
    {
      a->period -= inf;
      a->tmpperiod -= inf;
    }
}

static void 
DoXMExtraFineSlideDown (UBYTE inf)
{
  if (!mp.vbtick)
    {
      a->period += inf;
      a->tmpperiod += inf;
    }
}

/*========== Impulse Tracker effects */

static void 
DoITChanVolSlide (UBYTE inf)
{
  UBYTE lo, hi;

  if (inf)
    a->chanvolslide = inf;
  inf = a->chanvolslide;

  lo = inf & 0xf;
  hi = inf >> 4;

  if (!hi)
    a->chanvol -= lo;
  else if (!lo)
    {
      a->chanvol += hi;
    }
  else if (hi == 0xf)
    {
      if (!mp.vbtick)
	a->chanvol -= lo;
    }
  else if (lo == 0xf)
    {
      if (!mp.vbtick)
	a->chanvol += hi;
    }

  if (a->chanvol < 0)
    a->chanvol = 0;
  if (a->chanvol > 64)
    a->chanvol = 64;
}

static void 
DoITGlobalSlide (UBYTE inf)
{
  UBYTE lo, hi;

  if (inf)
    mp.globalslide = inf;
  inf = mp.globalslide;

  lo = inf & 0xf;
  hi = inf >> 4;

  if (!lo)
    {
      if (mp.vbtick)
	mp.volume += hi;
    }
  else if (!hi)
    {
      if (mp.vbtick)
	mp.volume -= lo;
    }
  else if (lo == 0xf)
    {
      if (!mp.vbtick)
	mp.volume += hi;
    }
  else if (hi == 0xf)
    {
      if (!mp.vbtick)
	mp.volume -= lo;
    }

  if (mp.volume < 0)
    mp.volume = 0;
  if (mp.volume > 128)
    mp.volume = 128;
}

static void 
DoITPanSlide (UBYTE inf)
{
  UBYTE lo, hi;
  SWORD pan;

  if (inf)
    a->pansspd = inf;
  else
    inf = a->pansspd;

  lo = inf & 0xf;
  hi = inf >> 4;

  pan = (a->panning == PAN_SURROUND) ? PAN_CENTER : a->panning;

  if (!hi)
    pan += lo << 2;
  else if (!lo)
    {
      pan -= hi << 2;
    }
  else if (hi == 0xf)
    {
      if (!mp.vbtick)
	pan += lo << 2;
    }
  else if (lo == 0xf)
    {
      if (!mp.vbtick)
	pan -= hi << 2;
    }
  a->panning =			/*pf->panning[mp.channel]= */
    (pan < PAN_LEFT) ? PAN_LEFT : (pan > PAN_RIGHT ? PAN_RIGHT : pan);
}

static void 
DoITTempo (UBYTE tempo)
{
  SWORD temp = mp.newbpm;

  if (mp.patdly2)
    return;

  if (tempo & 0x10)
    temp += (tempo & 0x0f);
  else
    temp -= tempo;

  mp.newbpm = (temp > 255) ? 255 : (temp < 1 ? 1 : temp);
}

static void 
DoITVibrato (void)
{
  UBYTE q;
  UWORD temp = 0;

  q = (a->vibpos >> 2) & 0x1f;

  switch (a->wavecontrol & 3)
    {
    case 0:			/* sine */
      temp = VibratoTable[q];
      break;
    case 1:			/* square wave */
      temp = 255;
      break;
    case 2:			/* ramp down */
      q <<= 3;
      if (a->vibpos < 0)
	q = 255 - q;
      temp = q;
      break;
    case 3:			/* random */
      temp = getrandom (256);
      break;
    }

  temp *= a->vibdepth;
  temp >>= 8;
  temp <<= 2;

  if (a->vibpos >= 0)
    a->period = a->tmpperiod + temp;
  else
    a->period = a->tmpperiod - temp;

  a->vibpos += a->vibspd;
}

static void 
DoITFineVibrato (void)
{
  UBYTE q;
  UWORD temp = 0;

  q = (a->vibpos >> 2) & 0x1f;

  switch (a->wavecontrol & 3)
    {
    case 0:			/* sine */
      temp = VibratoTable[q];
      break;
    case 1:			/* square wave */
      temp = 255;
      break;
    case 2:			/* ramp down */
      q <<= 3;
      if (a->vibpos < 0)
	q = 255 - q;
      temp = q;
      break;
    case 3:			/* random */
      temp = getrandom (256);
      break;
    }

  temp *= a->vibdepth;
  temp >>= 8;

  if (a->vibpos >= 0)
    a->period = a->tmpperiod + temp;
  else
    a->period = a->tmpperiod - temp;

  a->vibpos += a->vibspd;
}

static void 
DoITTremor (UBYTE inf)
{
  UBYTE on, off;

  if (inf)
    a->s3mtronof = inf;
  else
    {
      inf = a->s3mtronof;
      if (!inf)
	return;
    }

  if (!mp.vbtick)
    return;

  on = (inf >> 4);
  off = (inf & 0xf);

  a->s3mtremor %= (on + off);
  a->volume = (a->s3mtremor < on) ? a->tmpvolume : 0;
  a->s3mtremor++;
}

static void 
DoITPanbrello (void)
{
  UBYTE q;
  SLONG temp = 0;

  q = a->panbpos;

  switch (a->panbwave)
    {
    case 0:			/* sine */
      temp = PanbrelloTable[q];
      break;
    case 1:			/* square wave */
      temp = (q < 0x80) ? 64 : 0;
      break;
    case 2:			/* ramp down */
      q <<= 3;
      temp = q;
      break;
    case 3:			/* random */
      if (a->panbpos >= a->panbspd)
	{
	  a->panbpos = 0;
	  temp = getrandom (256);
	}
    }

  temp *= a->panbdepth;
  temp = (temp / 8) + pf->panning[mp.channel];

  a->panning = (temp < PAN_LEFT) ? PAN_LEFT : (temp > PAN_RIGHT ? PAN_RIGHT : temp);
  a->panbpos += a->panbspd;
}

static void 
DoITToneSlide (void)
{
  /* if we don't come from another note, ignore the slide and play the note
     as is */
  if (!a->oldnote)
    return;

  if (mp.vbtick)
    {
      int dist;

      /* We have to slide a->period towards a->wantedperiod, compute the
         difference between those two values */
      dist = a->period - a->wantedperiod;

      /* if they are equal or if portamentospeed is too big... */
      if ((!dist) || ((a->portspeed << 2) > abs (dist)))
	/* ... make tmpperiod equal tperiod */
	a->tmpperiod = a->period = a->wantedperiod;
      else if (dist > 0)
	{
	  a->tmpperiod -= a->portspeed << 2;
	  a->period -= a->portspeed << 2;	/* dist>0 slide up */
	}
      else
	{
	  a->tmpperiod += a->portspeed << 2;
	  a->period += a->portspeed << 2;	/* dist<0 slide down */
	}
    }
  else
    a->tmpperiod = a->period;
}

static void DoNNAEffects (UBYTE dat);
/* Impulse/Scream Tracker Sxx effects.
   All Sxx effects share the same memory space. */
static void 
DoSSEffects (UBYTE dat)
{
  UBYTE inf, c;

  inf = dat & 0xf;
  c = dat >> 4;

  if (!dat)
    {
      c = a->sseffect;
      inf = a->ssdata;
    }
  else
    {
      a->sseffect = c;
      a->ssdata = inf;
    }

  switch (c)
    {
    case SS_GLISSANDO:		/* S1x set glissando voice */
      DoEEffects (0x30 | inf);
      break;
    case SS_FINETUNE:		/* S2x set finetune */
      DoEEffects (0x50 | inf);
      break;
    case SS_VIBWAVE:		/* S3x set vibrato waveform */
      DoEEffects (0x40 | inf);
      break;
    case SS_TREMWAVE:		/* S4x set tremolo waveform */
      DoEEffects (0x70 | inf);
      break;
    case SS_PANWAVE:		/* S5x panbrello */
      a->panbwave = inf;
      break;
    case SS_FRAMEDELAY:	/* S6x delay x number of frames (patdly) */
      DoEEffects (0xe0 | inf);
      break;
    case SS_S7EFFECTS:		/* S7x instrument / NNA commands */
      DoNNAEffects (inf);
      break;
    case SS_PANNING:		/* S8x set panning position */
      DoEEffects (0x80 | inf);
      break;
    case SS_SURROUND:		/* S9x set surround sound */
      a->panning = pf->panning[mp.channel] = PAN_SURROUND;
      break;
    case SS_HIOFFSET:		/* SAy set high order sample offset yxx00h */
      if (!mp.vbtick)
	{
	  a->hioffset = inf << 16;
	  a->start = a->hioffset | a->soffset;

	  if ((a->s) && (a->start > a->s->length))
	    a->start = a->s->flags & (SF_LOOP | SF_BIDI) ? a->s->loopstart : a->s->length;
	}
      break;
    case SS_PATLOOP:		/* SBx pattern loop */
      DoEEffects (0x60 | inf);
      break;
    case SS_NOTECUT:		/* SCx notecut */
      DoEEffects (0xC0 | inf);
      break;
    case SS_NOTEDELAY:		/* SDx notedelay */
      DoEEffects (0xD0 | inf);
      break;
    case SS_PATDELAY:		/* SEx patterndelay */
      DoEEffects (0xE0 | inf);
      break;
    }
}

/* Impulse Tracker Volume/Pan Column effects.
   All volume/pan column effects share the same memory space. */
static void 
DoVolEffects (UBYTE c)
{
  UBYTE inf = UniGetByte ();

  if ((!c) && (!inf))
    {
      c = a->voleffect;
      inf = a->voldata;
    }
  else
    {
      a->voleffect = c;
      a->voldata = inf;
    }

  if (c)
    switch (c)
      {
      case VOL_VOLUME:
	if (mp.vbtick)
	  break;
	if (inf > 64)
	  inf = 64;
	a->tmpvolume = inf;
	break;
      case VOL_PANNING:
	a->panning = /*pf->panning[mp.channel]= */ inf;
	break;
      case VOL_VOLSLIDE:
	DoS3MVolSlide (inf);
	break;
      case VOL_PITCHSLIDEDN:
	if (a->period)
	  DoS3MSlideDn (inf);
	break;
      case VOL_PITCHSLIDEUP:
	if (a->period)
	  DoS3MSlideUp (inf);
	break;
      case VOL_PORTAMENTO:
	if (inf)
	  a->slidespeed = inf;
	if (a->period)
	  {
	    if ((!mp.vbtick) || (a->newsamp))
	      {
		a->kick = KICK_NOTE;
		a->start = -1;
	      }
	    else
	      a->kick = (a->kick == KICK_NOTE) ? KICK_ENV : KICK_ABSENT;
	    DoITToneSlide ();
	    a->ownper = 1;
	  }
	break;
      case VOL_VIBRATO:
	if (!mp.vbtick)
	  {
	    if (inf & 0x0f)
	      a->vibdepth = inf & 0xf;
	    if (inf & 0xf0)
	      a->vibspd = (inf & 0xf0) >> 2;
	  }
	if (a->period)
	  {
	    DoITVibrato ();
	    a->ownper = 1;
	  }
	break;
      }
}

/*========== UltraTracker effects */

static void 
DoULTSampleOffset (void)
{
  UWORD offset = UniGetWord ();

  if (offset)
    a->ultoffset = offset;

  a->start = a->ultoffset << 2;
  if ((a->s) && (a->start > a->s->length))
    a->start = a->s->flags & (SF_LOOP | SF_BIDI) ? a->s->loopstart : a->s->length;
}

/*========== OctaMED effects */

static void 
DoMEDSpeed (void)
{
  UWORD speed = UniGetWord ();

  mp.newbpm = speed;
}

/*========== General player functions */

static void 
pt_playeffects (void)
{
  UBYTE dat, c, oldc = 0;

  while ((c = UniGetByte ()))
    {
      int oldsliding = a->sliding;

      a->sliding = 0;

      /* libunimod doesn't *quite* do Ultimate Soundtracker portas correctly */
      if (strcmp(of.modtype, "Ultimate Soundtracker") == 0)
        {
      if (c == 5 && oldc == 4)
        {
          oldc = 5;
          a->sliding = oldsliding;
          UniSkipOpcode (c);
          continue;
        }
        oldc = c;
        if (c == 3 || c == 4)
            c++;
      }

      switch (c)
	{
	case UNI_PTEFFECT0:
	  dat = UniGetByte ();
	  if (!mp.vbtick)
	    {
	      if ((!dat) && (pf->flags & UF_ARPMEM))
		dat = a->arpmem;
	      else
	        a->arpmem = dat;
	    }
	  if (a->period)
	    DoArpeggio (a->arpmem);
	  break;
	case UNI_PTEFFECT1:
	  dat = UniGetByte ();
	  if ((!mp.vbtick) && (dat))
	    a->slidespeed = (UWORD) dat << 2;
	  if (a->period)
	    if (mp.vbtick)
	      a->tmpperiod -= a->slidespeed;
	  break;
	case UNI_PTEFFECT2:
	  dat = UniGetByte ();
	  if ((!mp.vbtick) && (dat))
	    a->slidespeed = (UWORD) dat << 2;
	  if (a->period)
	    if (mp.vbtick)
	      a->tmpperiod += a->slidespeed;
	  break;
	case UNI_PTEFFECT3:
	  dat = UniGetByte ();
	  if ((!mp.vbtick) && (dat))
	    a->portspeed = (UWORD) dat << 2;
	  if (a->period)
	    {
	      if (!a->fadevol)
		a->kick = (a->kick == KICK_NOTE) ? KICK_NOTE : KICK_KEYOFF;
	      else
		a->kick = (a->kick == KICK_NOTE) ? KICK_ENV : KICK_ABSENT;
	      DoToneSlide ();
	      a->ownper = 1;
	    }
	  break;
	case UNI_PTEFFECT4:
	case UNI_XMEFFECT4:
	  dat = UniGetByte ();
	  if (!mp.vbtick)
	    {
	      if (dat & 0x0f)
		a->vibdepth = dat & 0xf;
	      if (dat & 0xf0)
		a->vibspd = (dat & 0xf0) >> 2;
	    }
	  else if (a->period)
	    {
	      DoVibrato ();
	      a->ownper = 1;
	    }
	  break;
	case UNI_PTEFFECT5:
	  dat = UniGetByte ();
	  if (a->period)
	    {
	      if (!a->fadevol)
		a->kick = (a->kick == KICK_NOTE) ? KICK_NOTE : KICK_KEYOFF;
	      else
		a->kick = (a->kick == KICK_NOTE) ? KICK_ENV : KICK_ABSENT;
	      DoToneSlide ();
	      a->ownper = 1;
	    }
	  DoVolSlide (dat);
	  break;
	case UNI_PTEFFECT6:
	  dat = UniGetByte ();
	  if ((a->period) && (mp.vbtick))
	    {
	      DoVibrato ();
	      a->ownper = 1;
	    }
	  DoVolSlide (dat);
	  break;
	case UNI_PTEFFECT7:
	  dat = UniGetByte ();
	  if (!mp.vbtick)
	    {
	      if (dat & 0x0f)
		a->trmdepth = dat & 0xf;
	      if (dat & 0xf0)
		a->trmspd = (dat & 0xf0) >> 2;
	    }
	  if (a->period)
	    {
	      DoTremolo ();
	      a->ownvol = 1;
	    }
	  break;
	case UNI_PTEFFECT8:
	  dat = UniGetByte ();
	  a->panning = pf->panning[mp.channel] = dat;
	  break;
	case UNI_PTEFFECT9:
	  dat = UniGetByte ();
	  if (!mp.vbtick)
	    {
	      if (dat)
		a->soffset = (UWORD) dat << 8;
	      a->start = a->hioffset | a->soffset;

	      if ((a->s) && (a->start > a->s->length))
		a->start = a->s->flags & (SF_LOOP | SF_BIDI) ? a->s->loopstart : a->s->length;
	    }
	  break;
	case UNI_PTEFFECTA:
	  DoVolSlide (UniGetByte ());
	  break;
	case UNI_PTEFFECTB:
	  dat = UniGetByte ();
	  if ((mp.vbtick) || (mp.patdly2))
	    break;
	  /* Vincent Voois uses a nasty trick in "Universal Bolero" */
	  if (dat == mp.sngpos && mp.patbrk == mp.patpos)
	    break;
	  if ((!mp.patbrk) && ((dat < mp.sngpos) ||
			  ((mp.sngpos == pf->numpos - 1) && (!mp.patbrk)) ||
			   ((dat == mp.sngpos) && (pf->flags & UF_NOWRAP))))
	    {
	      /* if we don't loop, better not to skip the end of the
	         pattern, after all... so:
	         mp.patbrk=0; */
	      mp.posjmp = 3;
	    }
	  else
	    {
	      /* if we were fading, adjust... */
	      if (mp.sngpos == (pf->numpos - 1))
		mp.volume = pf->initvolume > 128 ? 128 : pf->initvolume;
	      
	      mp.sngpos = dat;
	      mp.posjmp = 2;
	      mp.patpos = 0;
	    }
	  break;
	case UNI_PTEFFECTC:
	  dat = UniGetByte ();
	  if (mp.vbtick)
	    break;
	  if (dat == (UBYTE) - 1)
	    a->anote = dat = 0;	/* note cut */
	  else if (dat > 64)
	    dat = 64;
	  a->tmpvolume = dat;
	  break;
	case UNI_PTEFFECTD:
	  dat = UniGetByte ();
	  if ((mp.vbtick) || (mp.patdly2))
	    break;
	  if ((pf->positions[mp.sngpos] != 255) &&
	      (dat > pf->pattrows[pf->positions[mp.sngpos]]))
	    dat = pf->pattrows[pf->positions[mp.sngpos]];
	  mp.patbrk = dat;
	  if (!mp.posjmp)
	    {
	      /* don't ask me to explain this code - it makes
	       * backwards.s3m and children.xm (heretic's version) play
	       * correctly, among others. Take that for granted, or write
	       * the page of comments yourself... you might need some
	       * aspirin - Miod */
	      if ((mp.sngpos == pf->numpos - 1) && (dat) &&
		  ((pf->positions[mp.sngpos] == (pf->numpat - 1)
		    && (pf->flags & UF_NOWRAP))))
		{
		  /* printf("%d -- Pattern 0!\n", __LINE__); */
		  mp.sngpos = 0;
		  mp.posjmp = 2;
		}
	      else
		mp.posjmp = 3;
	    }
	  break;
	case UNI_PTEFFECTE:
	  DoEEffects (UniGetByte ());
	  break;
	case UNI_PTEFFECTF:
	  dat = UniGetByte ();
	  if (mp.vbtick || mp.patdly2)
	    break;
	  if (dat > 0x20)
	    mp.newbpm = dat;
	  else if (dat)
	    {
	      mp.sngspd = (dat > 32) ? 32 : dat;
	      mp.vbtick = 0;
	    }
	  break;
	case UNI_S3MEFFECTA:
	  DoS3MSpeed (UniGetByte ());
	  break;
	case UNI_S3MEFFECTD:
	  DoS3MVolSlide (UniGetByte ());
	  break;
	case UNI_S3MEFFECTE:
	  dat = UniGetByte ();
	  if (a->period)
	    DoS3MSlideDn (dat);
	  break;
	case UNI_S3MEFFECTF:
	  dat = UniGetByte ();
	  if (a->period)
	    DoS3MSlideUp (dat);
	  break;
	case UNI_S3MEFFECTI:
	  DoS3MTremor (UniGetByte ());
	  a->ownvol = 1;
	  break;
	case UNI_S3MEFFECTQ:
	  dat = UniGetByte ();
	  if (a->period)
	    DoS3MRetrig (dat);
	  break;
	case UNI_S3MEFFECTR:
	  dat = UniGetByte ();
	  if (!mp.vbtick)
	    {
	      if (dat & 0x0f)
		a->trmdepth = dat & 0xf;
	      if (dat & 0xf0)
		a->trmspd = (dat & 0xf0) >> 2;
	    }
	  DoS3MTremolo ();
	  a->ownvol = 1;
	  break;
	case UNI_S3MEFFECTT:
	  DoS3MTempo (UniGetByte ());
	  break;
	case UNI_S3MEFFECTU:
	  dat = UniGetByte ();
	  if (!mp.vbtick)
	    {
	      if (dat & 0x0f)
		a->vibdepth = dat & 0xf;
	      if (dat & 0xf0)
		a->vibspd = (dat & 0xf0) >> 2;
	    }
	  else if (a->period)
	    {
	      DoS3MFineVibrato ();
	      a->ownper = 1;
	    }
	  break;
	case UNI_KEYOFF:
	  a->keyoff |= KEY_OFF;
	  if ((!(a->volflg & EF_ON)) || (a->volflg & EF_LOOP))
	    a->keyoff = KEY_KILL;
	  break;
	case UNI_KEYFADE:
	  dat = UniGetByte ();
	  if ((mp.vbtick >= dat) || (mp.vbtick == mp.sngspd - 1))
	    {
	      a->keyoff = KEY_KILL;
	      if (!(a->volflg & EF_ON))
		a->fadevol = 0;
	    }
	  break;
	case UNI_VOLEFFECTS:
	  DoVolEffects (UniGetByte ());
	  break;
	case UNI_XMEFFECTA:
	  DoXMVolSlide (UniGetByte ());
	  break;
	case UNI_XMEFFECTE1:	/* XM fineslide up */
	  if (mp.vbtick)
	    break;
	  dat = UniGetByte ();
	  if (!mp.vbtick)
	    {
	      if (dat)
		a->fportupspd = dat;
	      if (a->period)
		a->tmpperiod -= (a->fportupspd << 2);
	    }
	  break;
	case UNI_XMEFFECTE2:	/* XM fineslide dn */
	  if (mp.vbtick)
	    break;
	  dat = UniGetByte ();
	  if (!mp.vbtick)
	    {
	      if (dat)
		a->fportdnspd = dat;
	      if (a->period)
		a->tmpperiod += (a->fportdnspd << 2);
	    }
	  break;
	case UNI_XMEFFECTEA:	/* fine volume slide up */
	  if (mp.vbtick)
	    break;
	  dat = UniGetByte ();
	  if (dat)
	    a->fslideupspd = dat;
	  a->tmpvolume += a->fslideupspd;
	  if (a->tmpvolume > 64)
	    a->tmpvolume = 64;
	  break;
	case UNI_XMEFFECTEB:	/* fine volume slide dn */
	  if (mp.vbtick)
	    break;
	  dat = UniGetByte ();
	  if (dat)
	    a->fslidednspd = dat;
	  a->tmpvolume -= a->fslidednspd;
	  if (a->tmpvolume < 0)
	    a->tmpvolume = 0;
	  break;
	case UNI_XMEFFECTG:
	  mp.volume = UniGetByte () << 1;
	  if (mp.volume > 128)
	    mp.volume = 128;
	  break;
	case UNI_XMEFFECTH:
	  DoXMGlobalSlide (UniGetByte ());
	  break;
	case UNI_XMEFFECTL:
	  dat = UniGetByte ();
	  if ((!mp.vbtick) && (a->i))
	    {
	      UWORD points;
	      INSTRUMENT *i = a->i;
	      MP_VOICE *aout;

	      if ((aout = a->slave))
		{
		  if (aout->venv.env) {
		    points = i->volenv[i->volpts - 1].pos;
		    aout->venv.p = aout->venv.env[(dat > points) ? points : dat].pos;
		  }
		  if (aout->penv.env) {
		    points = i->panenv[i->panpts - 1].pos;
		    aout->penv.p = aout->penv.env[(dat > points) ? points : dat].pos;
		  }
		}
	    }
	  break;
	case UNI_XMEFFECTP:
	  dat = UniGetByte ();
	  DoXMPanSlide (dat);
	  break;
	case UNI_XMEFFECTX1:
	  if (mp.vbtick)
	    break;
	  dat = UniGetByte ();
	  if (dat)
	    a->ffportupspd = dat;
	  else
	    dat = a->ffportupspd;
	  if (a->period)
	    {
	      DoXMExtraFineSlideUp (dat);
	      a->ownper = 1;
	    }
	  break;
	case UNI_XMEFFECTX2:
	  if (mp.vbtick)
	    break;
	  dat = UniGetByte ();
	  if (dat)
	    a->ffportdnspd = dat;
	  else
	    dat = a->ffportdnspd;
	  if (a->period)
	    {
	      DoXMExtraFineSlideDown (dat);
	      a->ownper = 1;
	    }
	  break;
	case UNI_ITEFFECTG:
	  dat = UniGetByte ();
	  if (dat)
	    {
	      a->portspeed = dat;
	    }
	  if (a->period)
	    {
	      if ((!mp.vbtick) && (a->newsamp))
		{
		  a->kick = KICK_NOTE;
		  a->start = -1;
		}
	      else
		a->kick = (a->kick == KICK_NOTE) ? KICK_ENV : KICK_ABSENT;
	      DoITToneSlide ();
	      a->ownper = 1;
	    }
	  break;
	case UNI_ITEFFECTH:	/* IT vibrato */
	  dat = UniGetByte ();
	  if (!mp.vbtick)
	    {
	      if (dat & 0x0f)
		a->vibdepth = dat & 0xf;
	      if (dat & 0xf0)
		a->vibspd = (dat & 0xf0) >> 2;
	    }
	  if (a->period)
	    {
	      DoITVibrato ();
	      a->ownper = 1;
	    }
	  break;
	case UNI_ITEFFECTI:	/* IT tremor */
	  DoITTremor (UniGetByte ());
	  a->ownvol = 1;
	  break;
	case UNI_ITEFFECTM:
	  a->chanvol = UniGetByte ();
	  if (a->chanvol > 64)
	    a->chanvol = 64;
	  else if (a->chanvol < 0)
	    a->chanvol = 0;
	  break;
	case UNI_ITEFFECTN:	/* slide / fineslide channel volume */
	  DoITChanVolSlide (UniGetByte ());
	  break;
	case UNI_ITEFFECTP:	/* slide / fineslide channel panning */
	  dat = UniGetByte ();
	  DoITPanSlide (dat);
	  break;
	case UNI_ITEFFECTT:	/* slide / fineslide tempo */
	  DoITTempo (UniGetByte ());
	  break;
	case UNI_ITEFFECTU:	/* fine vibrato */
	  dat = UniGetByte ();
	  if (!mp.vbtick)
	    {
	      if (dat & 0x0f)
		a->vibdepth = dat & 0xf;
	      if (dat & 0xf0)
		a->vibspd = (dat & 0xf0) >> 2;
	    }
	  if (a->period)
	    {
	      DoITFineVibrato ();
	      a->ownper = 1;
	    }
	  break;
	case UNI_ITEFFECTW:	/* slide / fineslide global volume */
	  DoITGlobalSlide (UniGetByte ());
	  break;
	case UNI_ITEFFECTY:	/* panbrello */
	  dat = UniGetByte ();
	  if (!mp.vbtick)
	    {
	      if (dat & 0x0f)
		a->panbdepth = (dat & 0xf);
	      if (dat & 0xf0)
		a->panbspd = (dat & 0xf0) >> 4;
	    }
	  DoITPanbrello ();
	  break;
	case UNI_ITEFFECTS0:
	  DoSSEffects (UniGetByte ());
	  break;
	case UNI_ITEFFECTZ:
	  /* FIXME not yet implemented */
	  UniSkipOpcode (UNI_ITEFFECTZ);
	  break;
	case UNI_ULTEFFECT9:
	  DoULTSampleOffset ();
	  break;
	case UNI_MEDSPEED:
	  DoMEDSpeed ();
	  break;
	case UNI_MEDEFFECTF1:
	  DoEEffects (0x90 | (mp.sngspd / 2));
	  break;
	case UNI_MEDEFFECTF2:
	  DoEEffects (0xd0 | (mp.sngspd / 2));
	  break;
	case UNI_MEDEFFECTF3:
	  DoEEffects (0x90 | (mp.sngspd / 3));
	  break;
	/* case UNI_NOTE: */
	/* case UNI_INSTRUMENT: */
	default:
	  a->sliding = oldsliding;
	  UniSkipOpcode (c);
	  break;
	}
    }
}

static void 
DoNNAEffects (UBYTE dat)
{
  int t;
  MP_VOICE *aout;

  dat &= 0xf;
  aout = (a->slave) ? a->slave : NULL;

  switch (dat)
    {
    case 0x0:			/* past note cut */
      for (t = 0; t < MOD_NUM_VOICES; t++)
	if (mp.voice[t].master == a)
	  mp.voice[t].fadevol = 0;
      break;
    case 0x1:			/* past note off */
      for (t = 0; t < MOD_NUM_VOICES; t++)
	if (mp.voice[t].master == a)
	  {
	    mp.voice[t].keyoff |= KEY_OFF;
	    if ((!(mp.voice[t].venv.flg & EF_ON)) ||
		(mp.voice[t].venv.flg & EF_LOOP))
	      mp.voice[t].keyoff = KEY_KILL;
	  }
      break;
    case 0x2:			/* past note fade */
      for (t = 0; t < MOD_NUM_VOICES; t++)
	if (mp.voice[t].master == a)
	  mp.voice[t].keyoff |= KEY_FADE;
      break;
    case 0x3:			/* set NNA note cut */
      a->nna = (a->nna & ~NNA_MASK) | NNA_CUT;
      break;
    case 0x4:			/* set NNA note continue */
      a->nna = (a->nna & ~NNA_MASK) | NNA_CONTINUE;
      break;
    case 0x5:			/* set NNA note off */
      a->nna = (a->nna & ~NNA_MASK) | NNA_OFF;
      break;
    case 0x6:			/* set NNA note fade */
      a->nna = (a->nna & ~NNA_MASK) | NNA_FADE;
      break;
    case 0x7:			/* disable volume envelope */
      if (aout)
	aout->volflg &= ~EF_ON;
      break;
    case 0x8:			/* enable volume envelope  */
      if (aout)
	aout->volflg |= EF_ON;
      break;
    case 0x9:			/* disable panning envelope */
      if (aout)
	aout->panflg &= ~EF_ON;
      break;
    case 0xa:			/* enable panning envelope */
      if (aout)
	aout->panflg |= EF_ON;
      break;
    case 0xb:			/* disable pitch envelope */
      if (aout)
	aout->pitflg &= ~EF_ON;
      break;
    case 0xc:			/* enable pitch envelope */
      if (aout)
	aout->pitflg |= EF_ON;
      break;
    }
}

static void 
pt_UpdateVoices ()
{
  SWORD envpan, envvol, envpit;
  UWORD playperiod;
  SLONG vibval, vibdpt;
  ULONG tmpvol;
  BOOL  kick_voice;

  MP_VOICE *aout;
  INSTRUMENT *i;
  SAMPLE *s;

  mp.totalchn = mp.realchn = 0;
  for (mp.channel = 0; mp.channel < MOD_NUM_VOICES; mp.channel++)
    {
      aout = &mp.voice[mp.channel];
      i = aout->i;
      s = aout->s;

      if ((!s) || (!s->length))
	continue;

      if (aout->period < 14 || aout->period > 50000)
        {
          Voice_Stop(mp.channel);
          continue;
        }

      kick_voice = 0;
      if ((aout->kick == KICK_NOTE) || (aout->kick == KICK_KEYOFF))
	{
	  kick_voice = 1;
	  aout->fadevol = 32768;
	  aout->aswppos = 0;
	}

      /* check for a dead note (fadevol=0) */
      if (!aout->fadevol || kick_voice)
	Voice_Stop (mp.channel);

      if (i && ((aout->kick == KICK_NOTE) || (aout->kick == KICK_ENV)))
	{
	  StartEnvelope (&aout->venv, aout->volflg, i->volpts, i->volsusbeg,
	       i->volsusend, i->volbeg, i->volend, i->volenv, aout->keyoff);
	  StartEnvelope (&aout->penv, aout->panflg, i->panpts, i->pansusbeg,
	       i->pansusend, i->panbeg, i->panend, i->panenv, aout->keyoff);
	  StartEnvelope (&aout->cenv, aout->pitflg, i->pitpts, i->pitsusbeg,
	       i->pitsusend, i->pitbeg, i->pitend, i->pitenv, aout->keyoff);
	  if (aout->cenv.flg & EF_ON)
	    aout->masterperiod = GetPeriod ((UWORD) aout->note << 1, aout->master->speed);
	}
      aout->kick = KICK_ABSENT;

      envvol = (!(aout->volflg & EF_ON)) ? 256 :
	ProcessEnvelope (&aout->venv, 256, aout->keyoff);
      envpan = (!(aout->panflg & EF_ON)) ? PAN_CENTER :
	ProcessEnvelope (&aout->penv, PAN_CENTER, aout->keyoff);
      envpit = (!(aout->pitflg & EF_ON)) ? 32 :
	ProcessEnvelope (&aout->cenv, 32, aout->keyoff);

      tmpvol = aout->fadevol;	/* max 32768 */
      tmpvol *= aout->chanvol;	/* * max 64 */
      tmpvol *= aout->volume;	/* * max 256 */
      tmpvol /= 16384L;		/* tmpvol is max 32768 */
      aout->totalvol = tmpvol >> 2;	/* totalvolume used to determine samplevolume */
      tmpvol *= envvol;		/* * max 256 */
      tmpvol *= mp.volume;	/* * max 128 */
      tmpvol /= 4194304UL;

      Voice_SetVolume (mp.channel, tmpvol);
      if ((tmpvol) && (aout->master) && (aout->master->slave == aout))
	mp.realchn++;
      mp.totalchn++;

      if (aout->panning == PAN_SURROUND)
	Voice_SetPanning (mp.channel, PAN_SURROUND);
      else if (aout->penv.flg & EF_ON)
	Voice_SetPanning (mp.channel, DoPan (envpan, aout->panning));
      else
	Voice_SetPanning (mp.channel, aout->panning);

      if (aout->period && s->vibdepth)
	switch (s->vibtype)
	  {
	  case 0:
	    vibval = avibtab[aout->avibpos & 127];
	    if (aout->avibpos & 0x80)
	      vibval = -vibval;
	    break;
	  case 1:
	    vibval = 64;
	    if (aout->avibpos & 0x80)
	      vibval = -vibval;
	    break;
	  case 2:
	    vibval = 63 - (((aout->avibpos + 128) & 255) >> 1);
	    break;
	  default:
	    vibval = (((aout->avibpos + 128) & 255) >> 1) - 64;
	    break;
	  }
      else
	vibval = 0;

      if (s->vibflags & AV_IT)
	{
	  if ((aout->aswppos >> 8) < s->vibdepth)
	    {
	      aout->aswppos += s->vibsweep;
	      vibdpt = aout->aswppos;
	    }
	  else
	    vibdpt = s->vibdepth << 8;
	  vibval = (vibval * vibdpt) >> 16;
	  if (aout->mflag)
	    {
	      if (!(pf->flags & UF_LINEAR))
		vibval >>= 1;
	      aout->period -= vibval;
	    }
	}
      else
	{
	  /* do XM style auto-vibrato */
	  if (!(aout->keyoff & KEY_OFF))
	    {
	      if (aout->aswppos < s->vibsweep)
		{
		  vibdpt = (aout->aswppos * s->vibdepth) / s->vibsweep;
		  aout->aswppos++;
		}
	      else
		vibdpt = s->vibdepth;
	    }
	  else
	    {
	      /* keyoff -> depth becomes 0 if final depth wasn't reached or
	         stays at final level if depth WAS reached */
	      if (aout->aswppos >= s->vibsweep)
		vibdpt = s->vibdepth;
	      else
		vibdpt = 0;
	    }
	  vibval = (vibval * vibdpt) >> 8;
	  aout->period -= vibval;
	}

      /* update vibrato position */
      aout->avibpos = (aout->avibpos + s->vibrate) & 0xff;

      /* process pitch envelope */
      playperiod = aout->period;

      if ((aout->pitflg & EF_ON) && (envpit != 32))
	{
	  long p1;

	  envpit -= 32;
	  if ((aout->note << 1) + envpit <= 0)
	    envpit = -(aout->note << 1);

	  p1 = GetPeriod (((UWORD) aout->note << 1) + envpit, aout->master->speed) - aout->masterperiod;
	  if (p1 > 0)
	    {
	      if ((UWORD) (playperiod + p1) <= playperiod)
		{
		  p1 = 0;
		  aout->keyoff |= KEY_OFF;
		}
	    }
	  else if (p1 < 0)
	    {
	      if ((UWORD) (playperiod + p1) >= playperiod)
		{
		  p1 = 0;
		  aout->keyoff |= KEY_OFF;
		}
	    }
	  playperiod += p1;
	}

      if (!aout->fadevol)
	{			/* check for a dead note (fadevol=0) */
	  mp.totalchn--;
	  if ((tmpvol) && (aout->master) && (aout->master->slave == aout))
	    mp.realchn--;
	}
      else
	{
	  Voice_SetPeriod (mp.channel,
		      getAmigaPeriod (pf->flags, playperiod));

	  if (kick_voice)
	    Voice_Play (mp.channel, s, (aout->start == -1) ? ((s->flags & SF_UST_LOOP) ? s->loopstart : 0) : aout->start);

	  /* if keyfade, start substracting fadeoutspeed from fadevol: */
	  if ((i) && (aout->keyoff & KEY_FADE))
	    {
	      if (aout->fadevol >= i->volfade)
		aout->fadevol -= i->volfade;
	      else
		aout->fadevol = 0;
	    }
	}

      if (mp.bpm != mp.newbpm || mp.sngspd != mp.oldsngspd) {
	mp.bpm = mp.newbpm;
	mp.oldsngspd = mp.sngspd;
        Voice_NewTempo(mp.bpm, mp.sngspd);
      }
    }
}

/* Handles new notes or instruments */
static void 
pt_Notes (void)
{
  UBYTE c, inst;
  int tr, funky;		/* funky is set to indicate note or instrument change */

  for (mp.channel = 0; mp.channel < pf->numchn; mp.channel++)
    {
      a = &mp.control[mp.channel];

      if (mp.sngpos >= pf->numpos)
	{
	  tr = pf->numtrk;
	  mp.numrow = 0;
	}
      else
	{
	  tr = pf->patterns[(pf->positions[mp.sngpos] * pf->numchn) + mp.channel];
	  mp.numrow = pf->pattrows[pf->positions[mp.sngpos]];
	}

      a->row = (tr < pf->numtrk) ? UniFindRow (pf->tracks[tr], mp.patpos) : NULL;
      a->newsamp = 0;
      if (!mp.vbtick)
	a->notedelay = 0;

      if (!a->row)
	continue;
      UniSetRow (a->row);
      funky = 0;

      while ((c = UniGetByte ()))
	switch (c)
	  {
	  case UNI_NOTE:
	    funky |= 1;
	    a->oldnote = a->anote, a->anote = UniGetByte ();
	    a->kick = KICK_NOTE;
	    a->start = -1;
	    a->sliding = 0;

	    /* retrig tremolo and vibrato waves ? */
	    if (!(a->wavecontrol & 0x80))
	      a->trmpos = 0;
	    if (!(a->wavecontrol & 0x08))
	      a->vibpos = 0;
	    if (!a->panbwave)
	      a->panbpos = 0;
	    break;
	  case UNI_INSTRUMENT:
	    inst = UniGetByte ();
	    if (inst >= pf->numins)
	      break;		/* safety valve */
	    funky |= 2;
	    a->i = (pf->flags & UF_INST) ? &pf->instruments[inst] : NULL;
	    a->retrig = 0;
	    a->s3mtremor = 0;
	    a->ultoffset = 0;
	    a->sample = inst;
	    break;
	  default:
	    UniSkipOpcode (c);
	    break;
	  }

      if (funky)
	{
	  INSTRUMENT *i;
	  SAMPLE *s;

	  i = a->i;
	  if (i)
	    {
	      if (i->samplenumber[a->anote] >= pf->numsmp)
		continue;
	      s = &pf->samples[i->samplenumber[a->anote]];
	      a->note = i->samplenote[a->anote];
	    }
	  else
	    {
	      a->note = a->anote;
	      s = &pf->samples[a->sample];
	    }

	  if (a->s != s)
	    {
	      a->s = s;
	      a->newsamp = a->period;
	    }

	  /* channel or instrument determined panning ? */
	  a->panning = pf->panning[mp.channel];
	  if (s->flags & SF_OWNPAN)
	    a->panning = s->panning;
	  else if ((i) && (i->flags & IF_OWNPAN))
	    a->panning = i->panning;

	  a->data = s->data;
	  a->speed = s->speed;

	  if (i)
	    {
	      if ((i->flags & IF_PITCHPAN)
		  && (a->panning != PAN_SURROUND))
		{
		  a->panning += ((a->anote - i->pitpancenter) * i->pitpansep) / 8;
		  if (a->panning < PAN_LEFT)
		    a->panning = PAN_LEFT;
		  else if (a->panning > PAN_RIGHT)
		    a->panning = PAN_RIGHT;
		}
	      a->pitflg = i->pitflg;
	      a->volflg = i->volflg;
	      a->panflg = i->panflg;
	      a->nna = i->nnatype;
	      a->dca = i->dca;
	      a->dct = i->dct;
	    }
	  else
	    {
	      a->pitflg = 0;
	      a->volflg = 0;
	      a->panflg = 0;
	      a->nna = 0;
	      a->dca = 0;
	      a->dct = DCT_OFF;
	    }

	  if (funky & 2)	/* instrument change */
	    {
	      /* IT random volume variations: 0:8 bit fixed, and one bit for
	         sign. */
	      a->volume = a->tmpvolume = s->volume;
	      if ((s) && (i))
		{
		  if (i->rvolvar)
		    {
		      a->volume = a->tmpvolume = s->volume +
			((s->volume * ((SLONG) i->rvolvar * (SLONG) getrandom (512)
			  )) / 25600);
		      if (a->volume < 0)
			a->volume = a->tmpvolume = 0;
		      else if (a->volume > 64)
			a->volume = a->tmpvolume = 64;
		    }
		  if ((a->panning != PAN_SURROUND))
		    {
		      a->panning += ((a->panning * ((SLONG) i->rpanvar *
					 (SLONG) getrandom (512))) / 25600);
		      if (a->panning < PAN_LEFT)
			a->panning = PAN_LEFT;
		      else if (a->panning > PAN_RIGHT)
			a->panning = PAN_RIGHT;
		    }
		}
	    }

	  a->wantedperiod = a->tmpperiod = GetPeriod ((UWORD) a->note << 1, a->speed);
	  a->keyoff = KEY_KICK;
	}
    }
}

/* Handles effects */
static void 
pt_EffectsPass1 (void)
{
  MP_VOICE *aout;

  for (mp.channel = 0; mp.channel < pf->numchn; mp.channel++)
    {
      a = &mp.control[mp.channel];

      if ((aout = a->slave))
	{
	  a->fadevol = aout->fadevol;
	  a->period = aout->period;
	  if (a->kick == KICK_KEYOFF)
	    a->keyoff = aout->keyoff;
	}

      if (!a->row)
	continue;
      UniSetRow (a->row);

      a->ownper = a->ownvol = 0;
      mp.explicitslides = 0;
      pt_playeffects ();

      /* continue volume slide if necessary for XM and IT */
      if (pf->flags & UF_BGSLIDES)
	{
	  if (!mp.explicitslides && a->sliding)
	    DoS3MVolSlide(0);
	  else if (a->tmpvolume)
	    a->sliding = mp.explicitslides;
	}

      if (!a->ownper)
	a->period = a->tmpperiod;
      if (!a->ownvol)
	a->volume = a->tmpvolume;

      if (a->s)
	{
	  if (a->i)
	    a->outvolume = (a->volume * a->s->globvol * a->i->globvol) >> 10;
	  else
	    a->outvolume = (a->volume * a->s->globvol) >> 4;
	  if (a->outvolume > 256)
	    a->volume = 256;
	  else if (a->outvolume < 0)
	    a->outvolume = 0;
	}
    }
}

/* NNA management */
static void 
pt_NNA (void)
{
  for (mp.channel = 0; mp.channel < pf->numchn; mp.channel++)
    {
      a = &mp.control[mp.channel];

      if (a->kick == KICK_NOTE)
	{
	  BOOL k = 0;

	  if (a->slave)
	    {
	      MP_VOICE *aout;

	      aout = a->slave;
	      if (aout->nna & NNA_MASK)
		{
		  /* Make sure the old MP_VOICE channel knows it has no
		     master now ! */
		  a->slave = NULL;
		  /* assume the channel is taken by NNA */
		  aout->mflag = 0;

		  switch (aout->nna)
		    {
		    case NNA_CONTINUE:		/* continue note, do nothing */
		      break;
		    case NNA_OFF:	/* note off */
		      aout->keyoff |= KEY_OFF;
		      if ((!(aout->volflg & EF_ON)) || (aout->volflg & EF_LOOP))
			aout->keyoff = KEY_KILL;
		      break;
		    case NNA_FADE:
		      aout->keyoff |= KEY_FADE;
		      break;
		    }
		}
	    }

	  if (a->dct != DCT_OFF)
	    {
	      int t;

	      for (t = 0; t < MOD_NUM_VOICES; t++)
		if ((!Voice_Stopped (t)) &&
		    (mp.voice[t].masterchn == mp.channel) &&
		    (a->sample == mp.voice[t].sample))
		  {
		    k = 0;
		    switch (a->dct)
		      {
		      case DCT_NOTE:
			if (a->note == mp.voice[t].note)
			  k = 1;
			break;
		      case DCT_SAMPLE:
			if (a->data == mp.voice[t].data)
			  k = 1;
			break;
		      case DCT_INST:
			k = 1;
			break;
		      }
		    if (k)
		      switch (a->dca)
			{
			case DCA_CUT:
			  mp.voice[t].fadevol = 0;
			  break;
			case DCA_OFF:
			  mp.voice[t].keyoff |= KEY_OFF;
			  if ((!(mp.voice[t].volflg & EF_ON)) ||
			      (mp.voice[t].volflg & EF_LOOP))
			    mp.voice[t].keyoff = KEY_KILL;
			  break;
			case DCA_FADE:
			  mp.voice[t].keyoff |= KEY_FADE;
			  break;
			}
		  }
	    }
	}			/* if (a->kick==KICK_NOTE) */
    }
}

/* Setup module and NNA voices */
static void 
pt_SetupVoices (void)
{
  MP_VOICE *aout;

  for (mp.channel = 0; mp.channel < pf->numchn; mp.channel++)
    {
      a = &mp.control[mp.channel];

      if (a->notedelay)
	continue;
      if (a->kick == KICK_NOTE)
	{
	  /* if no channel was cut above, find an empty or quiet channel
	     here */
	  if (pf->flags & UF_NNA)
	    {
	      if (!a->slave)
		{
		  int newchn;

		  if ((newchn = MP_FindEmptyChannel ()) != -1)
		    a->slave = &mp.voice[a->slavechn = newchn];
		}
	    }
	  else
	    a->slave = &mp.voice[a->slavechn = mp.channel];

	  /* assign parts of MP_VOICE only done for a KICK_NOTE */
	  if ((aout = a->slave))
	    {
	      if (aout->mflag && aout->master)
		aout->master->slave = NULL;
	      aout->master = a;
	      a->slave = aout;
	      aout->masterchn = mp.channel;
	      aout->mflag = 1;
	    }
	}
      else
	aout = a->slave;

      if (aout)
	{
	  aout->i = a->i;
	  aout->s = a->s;
	  aout->sample = a->sample;
	  aout->data = a->data;
	  aout->period = a->period;
	  aout->panning = a->panning;
	  aout->chanvol = a->chanvol;
	  aout->fadevol = a->fadevol;
	  aout->kick = a->kick;
	  aout->start = a->start;
	  aout->volflg = a->volflg;
	  aout->panflg = a->panflg;
	  aout->pitflg = a->pitflg;
	  aout->volume = a->outvolume;
	  aout->keyoff = a->keyoff;
	  aout->note = a->note;
	  aout->nna = a->nna;
	}
      a->kick = KICK_ABSENT;
    }
}

/* second effect pass */
static void 
pt_EffectsPass2 (void)
{
  UBYTE c;

  for (mp.channel = 0; mp.channel < pf->numchn; mp.channel++)
    {
      a = &mp.control[mp.channel];

      if (!a->row)
	continue;
      UniSetRow (a->row);

      while ((c = UniGetByte ()))
	if (c == UNI_ITEFFECTS0)
	  {
	    c = UniGetByte ();
	    if ((c >> 4) == SS_S7EFFECTS)
	      DoNNAEffects (c & 0xf);
	  }
	else
	  UniSkipOpcode (c);
    }
}

static BOOL 
HandleTick (void)
{
  if ((!pf) || (mp.sngpos >= pf->numpos))
    return 0;

  if (++mp.vbtick >= mp.sngspd)
    {
      if (mp.pat_repcrazy)
	mp.pat_repcrazy = 0;	/* play 2 times row 0 */
      else
	mp.patpos++;
      mp.vbtick = 0;

      /* process pattern-delay. mp.patdly2 is the counter and mp.patdly is
         the command memory. */
      if (mp.patdly)
	mp.patdly2 = mp.patdly, mp.patdly = 0;
      if (mp.patdly2)
	{
	  /* patterndelay active */
	  if (--mp.patdly2)
	    /* so turn back mp.patpos by 1 */
	    if (mp.patpos)
	      mp.patpos--;
	}

      /* do we have to get a new patternpointer ? (when mp.patpos reaches the
         pattern size, or when a patternbreak is active) */
      if (((mp.patpos >= mp.numrow) && (mp.numrow > 0)) && (!mp.posjmp))
	mp.posjmp = 3;

      if (mp.posjmp)
	{
	  mp.patpos = mp.numrow ? (mp.patbrk % mp.numrow) : 0;
	  mp.pat_repcrazy = 0;
	  mp.sngpos += (mp.posjmp - 2);

	  for (mp.channel = 0; mp.channel < pf->numchn; mp.channel++)
	    mp.control[mp.channel].pat_reppos = -1;

	  mp.patbrk = mp.posjmp = 0;
	  /* handle the "---" (end of song) pattern since it can occur
	     *inside* the module in .IT and .S3M */
	  if ((mp.sngpos >= pf->numpos) || (pf->positions[mp.sngpos] == 255))
	    return 0;

	  if (mp.sngpos < 0)
	    mp.sngpos = pf->numpos - 1;

	}

      if (!mp.patdly2)
	pt_Notes ();
    }

  pt_EffectsPass1 ();
  if (pf->flags & UF_NNA)
    pt_NNA ();
  pt_SetupVoices ();
  pt_EffectsPass2 ();

  /* now set up the actual hardware channel playback information */
  pt_UpdateVoices ();
  return 1;
}

BOOL
mod_do_play (MODULE * mf)
{
  int t;

  /* make sure the player doesn't start with garbage */
  memset(&mp, 0, sizeof(mp));
  mp.control = (MP_CONTROL *) safe_malloc (mf->numchn * sizeof (MP_CONTROL));
  if (!mp.control)
    return 1;

  memset (mp.control, 0, mf->numchn * sizeof (MP_CONTROL));
  for (t = 0; t < mf->numchn; t++)
    {
      mp.control[t].chanvol = mf->chanvol[t];
      mp.control[t].panning = mf->panning[t];
    }

  mp.pat_repcrazy = 0;
  mp.sngpos = 0;
  mp.sngspd = mf->initspeed ? (mf->initspeed <= 32 ? mf->initspeed : 32) : 6;
  mp.volume = mf->initvolume > 128 ? 128 : mf->initvolume;

  mp.oldsngspd = mp.sngspd;
  mp.vbtick = mp.sngspd;
  mp.patdly = 0;
  mp.patdly2 = 0;
  mp.bpm = mf->inittempo <= 32 ? 32 : mf->inittempo;
  mp.newbpm = mp.bpm;

  mp.patpos = 0;
  mp.posjmp = 2;		/* make sure the player fetches the first note */
  mp.numrow = -1;
  mp.patbrk = 0;
  pf = mf;

  Voice_StartPlaying ();
  Voice_NewTempo (mp.bpm, mp.sngspd);
  do
    Voice_TickDone ();
  while (HandleTick ());
  Voice_TickDone ();
  Voice_EndPlaying ();

  /* reset all sample pans to center */
  /* mod routines have already adjusted the pan events, so we don't want
     the regular mixing routines to apply them yet again */
  for (t = 0; t < 256; t++)
  {
    if (special_patch[t])
      special_patch[t]->sample->panning = 64;
  }

  /* Done! */
  free (mp.control);
  return 0;
}
