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

  $Id: load_it.c,v 1.39 1999/10/25 16:31:41 miod Exp $

  Impulse tracker (IT) module loader

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <string.h>

#include "unimod_priv.h"

/*========== Module structure */

/* header */
typedef struct ITHEADER
  {
    CHAR songname[26];
    UBYTE blank01[2];
    UWORD ordnum;
    UWORD insnum;
    UWORD smpnum;
    UWORD patnum;
    UWORD cwt;			/* Created with tracker (y.xx = 0x0yxx) */
    UWORD cmwt;			/* Compatible with tracker ver > than val. */
    UWORD flags;
    UWORD special;		/* bit 0 set = song message attached */
    UBYTE globvol;
    UBYTE mixvol;		/* mixing volume [ignored] */
    UBYTE initspeed;
    UBYTE inittempo;
    UBYTE pansep;		/* panning separation between channels */
    UBYTE zerobyte;
    UWORD msglength;
    ULONG msgoffset;
    UBYTE blank02[4];
    UBYTE pantable[64];
    UBYTE voltable[64];
  }
ITHEADER;

/* sample information */
typedef struct ITSAMPLE
  {
    CHAR filename[12];
    UBYTE zerobyte;
    UBYTE globvol;
    UBYTE flag;
    UBYTE volume;
    UBYTE panning;
    CHAR sampname[28];
    UWORD convert;		/* sample conversion flag */
    ULONG length;
    ULONG loopbeg;
    ULONG loopend;
    ULONG c5spd;
    ULONG susbegin;
    ULONG susend;
    ULONG sampoffset;
    UBYTE vibspeed;
    UBYTE vibdepth;
    UBYTE vibrate;
    UBYTE vibwave;		/* 0=sine, 1=rampdown, 2=square, 3=random (speed ignored) */
  }
ITSAMPLE;

/* instrument information */

#define ITENVCNT 25
#define ITNOTECNT 120
typedef struct ITINSTHEADER
  {
    ULONG size;			/* (dword) Instrument size */
    CHAR filename[12];		/* (char) Instrument filename */
    UBYTE zerobyte;		/* (byte) Instrument type (always 0) */
    UBYTE volflg;
    UBYTE volpts;
    UBYTE volbeg;		/* (byte) Volume loop start (node) */
    UBYTE volend;		/* (byte) Volume loop end (node) */
    UBYTE volsusbeg;		/* (byte) Volume sustain begin (node) */
    UBYTE volsusend;		/* (byte) Volume Sustain end (node) */
    UBYTE panflg;
    UBYTE panpts;
    UBYTE panbeg;		/* (byte) channel loop start (node) */
    UBYTE panend;		/* (byte) channel loop end (node) */
    UBYTE pansusbeg;		/* (byte) channel sustain begin (node) */
    UBYTE pansusend;		/* (byte) channel Sustain end (node) */
    UBYTE pitflg;
    UBYTE pitpts;
    UBYTE pitbeg;		/* (byte) pitch loop start (node) */
    UBYTE pitend;		/* (byte) pitch loop end (node) */
    UBYTE pitsusbeg;		/* (byte) pitch sustain begin (node) */
    UBYTE pitsusend;		/* (byte) pitch Sustain end (node) */
    UWORD blank;
    UBYTE globvol;
    UBYTE chanpan;
    UWORD fadeout;		/* Envelope end / NNA volume fadeout */
    UBYTE dnc;			/* Duplicate note check */
    UBYTE dca;			/* Duplicate check action */
    UBYTE dct;			/* Duplicate check type */
    UBYTE nna;			/* New Note Action [0,1,2,3] */
    UWORD trkvers;		/* tracker version used to save [files only] */
    UBYTE ppsep;		/* Pitch-pan Separation */
    UBYTE ppcenter;		/* Pitch-pan Center */
    UBYTE rvolvar;		/* random volume varations */
    UBYTE rpanvar;		/* random panning varations */
    UWORD numsmp;		/* Number of samples in instrument [files only] */
    CHAR name[26];		/* Instrument name */
    UBYTE blank01[6];
    UWORD samptable[ITNOTECNT];	/* sample for each note [note / samp pairs] */
    UBYTE volenv[200];		/* volume envelope (IT 1.x stuff) */
    UBYTE oldvoltick[ITENVCNT];	/* volume tick position (IT 1.x stuff) */
    UBYTE volnode[ITENVCNT];	/* amplitude of volume nodes */
    UWORD voltick[ITENVCNT];	/* tick value of volume nodes */
    SBYTE pannode[ITENVCNT];	/* panenv - node points */
    UWORD pantick[ITENVCNT];	/* tick value of panning nodes */
    SBYTE pitnode[ITENVCNT];	/* pitchenv - node points */
    UWORD pittick[ITENVCNT];	/* tick value of pitch nodes */
  }
ITINSTHEADER;

/* unpacked note */

typedef struct ITNOTE
  {
    UBYTE note, ins, volpan, cmd, inf;
  }
ITNOTE;

/*========== Loader data */

static ULONG *paraptr = NULL;	/* parapointer array (see IT docs) */
static ITHEADER *mh = NULL;
static ITNOTE *itpat = NULL;	/* allocate to space for one full pattern */
static UBYTE *mask = NULL;	/* arrays allocated to 64 elements and used for */
static ITNOTE *last = NULL;	/* uncompressing IT's pattern information */
static int numtrk = 0;
static int old_effect;		/* if set, use S3M old-effects stuffs */

static CHAR *IT_Version[] =
{
  "ImpulseTracker  .  ",
  "Compressed ImpulseTracker  .  ",
  "ImpulseTracker 2.14p3",
  "Compressed ImpulseTracker 2.14p3",
  "ImpulseTracker 2.14p4",
  "Compressed ImpulseTracker 2.14p4",
};

/* table for porta-to-note command within volume/panning column */
static UBYTE portatable[10] =
{0, 1, 4, 8, 16, 32, 64, 96, 128, 255};

/*========== Loader code */

BOOL 
IT_Test (void)
{
  UBYTE id[4];

  if (!_mm_read_UBYTES (id, 4, modreader))
    return 0;
  if (!memcmp (id, "IMPM", 4))
    return 1;
  return 0;
}

BOOL 
IT_Init (void)
{
  if (!(mh = (ITHEADER *) _mm_malloc (sizeof (ITHEADER))))
    return 0;
  if (!(poslookup = (UBYTE *) _mm_malloc (256 * sizeof (UBYTE))))
    return 0;
  if (!(itpat = (ITNOTE *) _mm_malloc (200 * 64 * sizeof (ITNOTE))))
    return 0;
  if (!(mask = (UBYTE *) _mm_malloc (64 * sizeof (UBYTE))))
    return 0;
  if (!(last = (ITNOTE *) _mm_malloc (64 * sizeof (ITNOTE))))
    return 0;

  return 1;
}

void 
IT_Cleanup (void)
{
  FreeLinear ();

  _mm_free (mh);
  _mm_free (poslookup);
  _mm_free (itpat);
  _mm_free (mask);
  _mm_free (last);
  _mm_free (paraptr);
  _mm_free (origpositions);
}

/* Because so many IT files have 64 channels as the set number used, but really
   only use far less (usually from 8 to 24 still), I had to make this function,
   which determines the number of channels that are actually USED by a pattern.

   NOTE: You must first seek to the file location of the pattern before calling
   this procedure.

   Returns 1 on error
 */
static BOOL 
IT_GetNumChannels (UWORD patrows)
{
  int row = 0, flag, ch;

  do
    {
      if ((flag = _mm_read_UBYTE (modreader)) == EOF)
	{
	  _mm_errno = MMERR_LOADING_PATTERN;
	  return 1;
	}
      if (!flag)
	row++;
      else
	{
	  ch = (flag - 1) & 63;
	  remap[ch] = 0;
	  if (flag & 128)
	    mask[ch] = _mm_read_UBYTE (modreader);
	  if (mask[ch] & 1)
	    _mm_read_UBYTE (modreader);
	  if (mask[ch] & 2)
	    _mm_read_UBYTE (modreader);
	  if (mask[ch] & 4)
	    _mm_read_UBYTE (modreader);
	  if (mask[ch] & 8)
	    {
	      _mm_read_UBYTE (modreader);
	      _mm_read_UBYTE (modreader);
	    }
	}
    }
  while (row < patrows);

  return 0;
}

static UBYTE *
IT_ConvertTrack (ITNOTE * tr, UWORD numrows)
{
  int t;
  UBYTE note, ins, volpan;

  UniReset ();

  for (t = 0; t < numrows; t++)
    {
      note = tr[t * of.numchn].note;
      ins = tr[t * of.numchn].ins;
      volpan = tr[t * of.numchn].volpan;

      if (note != 255)
	{
	  if (note == 253)
	    UniWriteByte (UNI_KEYOFF);
	  else if (note == 254)
	    {
	      UniPTEffect (0xc, -1);	/* note cut command */
	      volpan = 255;
	    }
	  else
	    UniNote (note);
	}

      if ((ins) && (ins < 100))
	UniInstrument (ins - 1);
      else if (ins == 253)
	UniWriteByte (UNI_KEYOFF);
      else if (ins != 255)
	{			/* crap */
	  _mm_errno = MMERR_LOADING_PATTERN;
	  return NULL;
	}

      /* process volume / panning column
         volume / panning effects do NOT all share the same memory address
         yet. */
      if (volpan <= 64)
	UniVolEffect (VOL_VOLUME, volpan);
      else if (volpan <= 74)	/* fine volume slide up (65-74) */
	UniVolEffect (VOL_VOLSLIDE, 0x0f + ((volpan - 65) << 4));
      else if (volpan <= 84)	/* fine volume slide down (75-84) */
	UniVolEffect (VOL_VOLSLIDE, 0xf0 + (volpan - 75));
      else if (volpan <= 94)	/* volume slide up (85-94) */
	UniVolEffect (VOL_VOLSLIDE, ((volpan - 85) << 4));
      else if (volpan <= 104)	/* volume slide down (95-104) */
	UniVolEffect (VOL_VOLSLIDE, (volpan - 95));
      else if (volpan <= 114)	/* pitch slide down (105-114) */
	UniVolEffect (VOL_PITCHSLIDEDN, (volpan - 105));
      else if (volpan <= 124)	/* pitch slide up (115-124) */
	UniVolEffect (VOL_PITCHSLIDEUP, (volpan - 115));
      else if (volpan <= 127)
	{			/* crap */
	  _mm_errno = MMERR_LOADING_PATTERN;
	  return NULL;
	}
      else if (volpan <= 192)
	UniVolEffect (VOL_PANNING, ((volpan - 128) == 64) ? 255 : ((volpan - 128) << 2));
      else if (volpan <= 202)	/* portamento to note */
	UniVolEffect (VOL_PORTAMENTO, portatable[volpan - 193]);
      else if (volpan <= 212)	/* vibrato */
	UniVolEffect (VOL_VIBRATO, (volpan - 203));
      else if ((volpan != 239) && (volpan != 255))
	{			/* crap */
	  _mm_errno = MMERR_LOADING_PATTERN;
	  return NULL;
	}

      S3MIT_ProcessCmd (tr[t * of.numchn].cmd, tr[t * of.numchn].inf, old_effect | 2);

      UniNewline ();
    }
  return UniDup ();
}

static BOOL 
IT_ReadPattern (UWORD patrows)
{
  int row = 0, flag, ch, blah;
  ITNOTE *itt = itpat, dummy, *n, *l;

  memset (itt, 255, 200 * 64 * sizeof (ITNOTE));

  do
    {
      if ((flag = _mm_read_UBYTE (modreader)) == EOF)
	{
	  _mm_errno = MMERR_LOADING_PATTERN;
	  return 0;
	}
      if (!flag)
	{
	  itt = &itt[of.numchn];
	  row++;
	}
      else
	{
	  ch = remap[(flag - 1) & 63];
	  if (ch != -1)
	    {
	      n = &itt[ch];
	      l = &last[ch];
	    }
	  else
	    n = l = &dummy;

	  if (flag & 128)
	    mask[ch] = _mm_read_UBYTE (modreader);
	  if (mask[ch] & 1)
	    /* convert IT note off to internal note off */
	    if ((l->note = n->note = _mm_read_UBYTE (modreader)) == 255)
	      l->note = n->note = 253;
	  if (mask[ch] & 2)
	    l->ins = n->ins = _mm_read_UBYTE (modreader);
	  if (mask[ch] & 4)
	    l->volpan = n->volpan = _mm_read_UBYTE (modreader);
	  if (mask[ch] & 8)
	    {
	      l->cmd = n->cmd = _mm_read_UBYTE (modreader);
	      l->inf = n->inf = _mm_read_UBYTE (modreader);
	    }
	  if (mask[ch] & 16)
	    n->note = l->note;
	  if (mask[ch] & 32)
	    n->ins = l->ins;
	  if (mask[ch] & 64)
	    n->volpan = l->volpan;
	  if (mask[ch] & 128)
	    {
	      n->cmd = l->cmd;
	      n->inf = l->inf;
	    }
	}
    }
  while (row < patrows);

  for (blah = 0; blah < of.numchn; blah++)
    {
      if (!(of.tracks[numtrk++] = IT_ConvertTrack (&itpat[blah], patrows)))
	return 0;
    }

  return 1;
}

static void 
LoadMidiString (URL modreader, CHAR * dest)
{
  CHAR *cur, *last;

  _mm_read_UBYTES (dest, 32, modreader);
  cur = last = dest;
  /* remove blanks and uppercase all */
  while (*last)
    {
      if (isalnum ((int) *last))
	*(cur++) = toupper ((int) *last);
      last++;
    }
  *cur = 0;
}

/* Load embedded midi information for resonant filters */
static void 
IT_LoadMidiConfiguration (URL modreader)
{
  int i;

  memset (filtermacros, 0, sizeof (filtermacros));
  memset (filtersettings, 0, sizeof (filtersettings));

  if (modreader)
    {				/* information is embedded in file */
      UWORD dat;
      CHAR midiline[33];

      dat = _mm_read_I_UWORD (modreader);
      _mm_fseek (modreader, 8 * dat + 0x120, SEEK_CUR);

      /* read midi macros */
      for (i = 0; i < 16; i++)
	{
	  LoadMidiString (modreader, midiline);
	  if ((!strncmp (midiline, "F0F00", 5)) &&
	      ((midiline[5] == '0') || (midiline[5] == '1')))
	    filtermacros[i] = (midiline[5] - '0') | 0x80;
	}

      /* read standalone filters */
      for (i = 0x80; i < 0x100; i++)
	{
	  LoadMidiString (modreader, midiline);
	  if ((!strncmp (midiline, "F0F00", 5)) &&
	      ((midiline[5] == '0') || (midiline[5] == '1')))
	    {
	      filtersettings[i].filter = (midiline[5] - '0') | 0x80;
	      dat = (midiline[6]) ? (midiline[6] - '0') : 0;
	      if (midiline[7])
		dat = (dat << 4) | (midiline[7] - '0');
	      filtersettings[i].inf = dat;
	    }
	}
    }
  else
    {				/* use default information */
      filtermacros[0] = FILT_CUT;
      for (i = 0x80; i < 0x90; i++)
	{
	  filtersettings[i].filter = FILT_RESONANT;
	  filtersettings[i].inf = (i & 0x7f) << 3;
	}
    }
  activemacro = 0;
  for (i = 0; i < 0x80; i++)
    {
      filtersettings[i].filter = filtermacros[0];
      filtersettings[i].inf = i;
    }
}

BOOL 
IT_Load (BOOL curious)
{
  int t, u, lp;
  INSTRUMENT *d;
  SAMPLE *q;
  BOOL compressed = 0;

  numtrk = 0;
  filters = 0;

  /* try to read module header */
  _mm_read_I_ULONG (modreader);	/* kill the 4 byte header */
  _mm_read_string (mh->songname, 26, modreader);
  _mm_read_UBYTES (mh->blank01, 2, modreader);
  mh->ordnum = _mm_read_I_UWORD (modreader);
  mh->insnum = _mm_read_I_UWORD (modreader);
  mh->smpnum = _mm_read_I_UWORD (modreader);
  mh->patnum = _mm_read_I_UWORD (modreader);
  mh->cwt = _mm_read_I_UWORD (modreader);
  mh->cmwt = _mm_read_I_UWORD (modreader);
  mh->flags = _mm_read_I_UWORD (modreader);
  mh->special = _mm_read_I_UWORD (modreader);
  mh->globvol = _mm_read_UBYTE (modreader);
  mh->mixvol = _mm_read_UBYTE (modreader);
  mh->initspeed = _mm_read_UBYTE (modreader);
  mh->inittempo = _mm_read_UBYTE (modreader);
  mh->pansep = _mm_read_UBYTE (modreader);
  mh->zerobyte = _mm_read_UBYTE (modreader);
  mh->msglength = _mm_read_I_UWORD (modreader);
  mh->msgoffset = _mm_read_I_ULONG (modreader);
  _mm_read_UBYTES (mh->blank02, 4, modreader);
  _mm_read_UBYTES (mh->pantable, 64, modreader);
  _mm_read_UBYTES (mh->voltable, 64, modreader);

  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  /* set module variables */
  of.songname = DupStr (mh->songname, 26, 0);	/* make a cstr of songname  */
  of.reppos = 0;
  of.numpat = mh->patnum;
  of.numins = mh->insnum;
  of.numsmp = mh->smpnum;
  of.initspeed = mh->initspeed;
  of.inittempo = mh->inittempo;
  of.initvolume = mh->globvol;
  of.flags |= UF_BGSLIDES | UF_ARPMEM;

  if (mh->songname[25])
    {
      of.numvoices = 1 + mh->songname[25];
#ifdef MIKMOD_DEBUG
      fprintf (stderr, "Embedded IT limitation to %d voices\n", of.numvoices);
#endif
    }

  /* set the module type */
  /* 2.17 : IT 2.14p4 */
  /* 2.16 : IT 2.14p3 with resonant filters */
  /* 2.15 : IT 2.14p3 (improved compression) */
  if ((mh->cwt <= 0x219) && (mh->cwt >= 0x217))
    of.modtype = strdup (IT_Version[mh->cmwt < 0x214 ? 4 : 5]);
  else if (mh->cwt >= 0x215)
    of.modtype = strdup (IT_Version[mh->cmwt < 0x214 ? 2 : 3]);
  else
    {
      of.modtype = strdup (IT_Version[mh->cmwt < 0x214 ? 0 : 1]);
      of.modtype[mh->cmwt < 0x214 ? 15 : 26] = (mh->cwt >> 8) + '0';
      of.modtype[mh->cmwt < 0x214 ? 17 : 28] = ((mh->cwt >> 4) & 0xf) + '0';
      of.modtype[mh->cmwt < 0x214 ? 18 : 29] = ((mh->cwt) & 0xf) + '0';
    }

  if (mh->flags & 8)
    of.flags |= (UF_XMPERIODS | UF_LINEAR);

  if ((mh->cwt >= 0x106) && (mh->flags & 16))
    old_effect = 1;
  else
    old_effect = 0;

  /* set panning positions */
  for (t = 0; t < 64; t++)
    {
      mh->pantable[t] &= 0x7f;
      if (mh->pantable[t] < 64)
	of.panning[t] = mh->pantable[t] << 2;
      else if (mh->pantable[t] == 64)
	of.panning[t] = 255;
      else if (mh->pantable[t] == 100)
	of.panning[t] = PAN_SURROUND;
      else if (mh->pantable[t] == 127)
	of.panning[t] = PAN_CENTER;
      else
	{
	  _mm_errno = MMERR_LOADING_HEADER;
	  return 0;
	}
    }

  /* set channel volumes */
  memcpy (of.chanvol, mh->voltable, 64);

  /* read the order data */
  if (!AllocPositions (mh->ordnum))
    return 0;
  if (!(origpositions = _mm_calloc (mh->ordnum, sizeof (UWORD))))
    return 0;

  for (t = 0; t < mh->ordnum; t++)
    {
      origpositions[t] = _mm_read_UBYTE (modreader);
      if ((origpositions[t] > mh->patnum) && (origpositions[t] < 254))
	origpositions[t] = 255;
    }

  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  poslookupcnt = mh->ordnum;
  S3MIT_CreateOrders (curious);

  if (!(paraptr = (ULONG *) _mm_malloc ((mh->insnum + mh->smpnum + of.numpat) *
					sizeof (ULONG))))
    return 0;

  /* read the instrument, sample, and pattern parapointers */
  _mm_read_I_ULONGS (paraptr, mh->insnum + mh->smpnum + of.numpat, modreader);

  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  /* Check for and load midi information for resonant filters */
  if (mh->cmwt >= 0x216)
    {
      if (mh->special & 8)
	{
	  IT_LoadMidiConfiguration (modreader);
	  if (_mm_eof (modreader))
	    {
	      _mm_errno = MMERR_LOADING_HEADER;
	      return 0;
	    }
	}
      else
	IT_LoadMidiConfiguration (NULL);
      filters = 1;
    }

  /* Check for and load song comment */
  if ((mh->special & 1) && (mh->cwt >= 0x104) && (mh->msglength))
    {
      _mm_fseek (modreader, (long) (mh->msgoffset), SEEK_SET);
      if (!ReadComment (mh->msglength))
	return 0;
    }

  if (!(mh->flags & 4))
    of.numins = of.numsmp;
  if (!AllocSamples ())
    return 0;

  if (!AllocLinear ())
    return 0;

  /* Load all samples */
  q = of.samples;
  for (t = 0; t < mh->smpnum; t++)
    {
      ITSAMPLE s;

      /* seek to sample position */
      _mm_fseek (modreader, (long) (paraptr[mh->insnum + t] + 4), SEEK_SET);

      /* load sample info */
      _mm_read_string (s.filename, 12, modreader);
      s.zerobyte = _mm_read_UBYTE (modreader);
      s.globvol = _mm_read_UBYTE (modreader);
      s.flag = _mm_read_UBYTE (modreader);
      s.volume = _mm_read_UBYTE (modreader);
      _mm_read_string (s.sampname, 26, modreader);
      s.convert = _mm_read_UBYTE (modreader);
      s.panning = _mm_read_UBYTE (modreader);
      s.length = _mm_read_I_ULONG (modreader);
      s.loopbeg = _mm_read_I_ULONG (modreader);
      s.loopend = _mm_read_I_ULONG (modreader);
      s.c5spd = _mm_read_I_ULONG (modreader);
      s.susbegin = _mm_read_I_ULONG (modreader);
      s.susend = _mm_read_I_ULONG (modreader);
      s.sampoffset = _mm_read_I_ULONG (modreader);
      s.vibspeed = _mm_read_UBYTE (modreader);
      s.vibdepth = _mm_read_UBYTE (modreader);
      s.vibrate = _mm_read_UBYTE (modreader);
      s.vibwave = _mm_read_UBYTE (modreader);

      /* Some IT files have bogues loopbeg/loopend if looping is not used by
       * a sample. */
      if (!(s.flag & 80))
	s.loopbeg = s.loopend = 0;
      
      /* Generate an error if c5spd is > 512k, or samplelength > 256 megs
         (nothing would EVER be that high) */

      if (_mm_eof (modreader) || (s.c5spd > 0x7ffffL) || (s.length > 0xfffffffUL) ||
	  (s.loopbeg > 0xfffffffUL) || (s.loopend > 0xfffffffUL))
	{
	  _mm_errno = MMERR_LOADING_SAMPLEINFO;
	  return 0;
	}

      q->samplename = DupStr (s.sampname, 26, 0);
      q->speed = s.c5spd / 2;
      q->panning = ((s.panning & 127) == 64) ? 255 : (s.panning & 127) << 2;
      q->length = s.length;
      q->loopstart = s.loopbeg;
      q->loopend = s.loopend;
      q->volume = s.volume;
      q->globvol = s.globvol;
      q->seekpos = s.sampoffset;

      /* Convert speed to XM linear finetune */
      if (of.flags & UF_LINEAR)
	q->speed = speed_to_finetune (s.c5spd, t);

      if (s.panning & 128)
	q->flags |= SF_OWNPAN;

      if (s.vibrate)
	{
	  q->vibflags |= AV_IT;
	  q->vibtype = s.vibwave;
	  q->vibsweep = s.vibrate * 2;
	  q->vibdepth = s.vibdepth;
	  q->vibrate = s.vibspeed;
	}

      if (s.flag & 2)
	q->flags |= SF_16BITS;
      if ((s.flag & 8) && (mh->cwt >= 0x214))
	{
	  q->flags |= SF_ITPACKED;
	  compressed = 1;
	}
      if (s.flag & 16)
	q->flags |= SF_LOOP;
      if (s.flag & 64)
	q->flags |= SF_BIDI;

      if (mh->cwt >= 0x200)
	{
	  if (s.convert & 1)
	    q->flags |= SF_SIGNED;
	  if (s.convert & 4)
	    q->flags |= SF_DELTA;
	}

      q++;
    }

  /* Load instruments if instrument mode flag enabled */
  if (mh->flags & 4)
    {
      if (!AllocInstruments ())
	return 0;
      d = of.instruments;
      of.flags |= UF_NNA | UF_INST;

      for (t = 0; t < mh->insnum; t++)
	{
	  ITINSTHEADER ih;

	  /* seek to instrument position */
	  _mm_fseek (modreader, paraptr[t] + 4, SEEK_SET);

	  /* load instrument info */
	  _mm_read_string (ih.filename, 12, modreader);
	  ih.zerobyte = _mm_read_UBYTE (modreader);
	  if (mh->cwt < 0x200)
	    {
	      /* load IT 1.xx inst header */
	      ih.volflg = _mm_read_UBYTE (modreader);
	      ih.volbeg = _mm_read_UBYTE (modreader);
	      ih.volend = _mm_read_UBYTE (modreader);
	      ih.volsusbeg = _mm_read_UBYTE (modreader);
	      ih.volsusend = _mm_read_UBYTE (modreader);
	      _mm_read_I_UWORD (modreader);
	      ih.fadeout = _mm_read_I_UWORD (modreader);
	      ih.nna = _mm_read_UBYTE (modreader);
	      ih.dnc = _mm_read_UBYTE (modreader);
	    }
	  else
	    {
	      /* Read IT200+ header */
	      ih.nna = _mm_read_UBYTE (modreader);
	      ih.dct = _mm_read_UBYTE (modreader);
	      ih.dca = _mm_read_UBYTE (modreader);
	      ih.fadeout = _mm_read_I_UWORD (modreader);
	      ih.ppsep = _mm_read_UBYTE (modreader);
	      ih.ppcenter = _mm_read_UBYTE (modreader);
	      ih.globvol = _mm_read_UBYTE (modreader);
	      ih.chanpan = _mm_read_UBYTE (modreader);
	      ih.rvolvar = _mm_read_UBYTE (modreader);
	      ih.rpanvar = _mm_read_UBYTE (modreader);
	    }

	  ih.trkvers = _mm_read_I_UWORD (modreader);
	  ih.numsmp = _mm_read_UBYTE (modreader);
	  _mm_read_UBYTE (modreader);
	  _mm_read_string (ih.name, 26, modreader);
	  _mm_read_UBYTES (ih.blank01, 6, modreader);
	  _mm_read_I_UWORDS (ih.samptable, ITNOTECNT, modreader);
	  if (mh->cwt < 0x200)
	    {
	      /* load IT 1xx volume envelope */
	      _mm_read_UBYTES (ih.volenv, 200, modreader);
	      for (lp = 0; lp < ITENVCNT; lp++)
		{
		  ih.oldvoltick[lp] = _mm_read_UBYTE (modreader);
		  ih.volnode[lp] = _mm_read_UBYTE (modreader);
		}
	    }
	  else
	    {
	      /* load IT 2xx volume, pan and pitch envelopes */
#define IT_LoadEnvelope(name,type) 											\
				ih.name##flg   =_mm_read_UBYTE(modreader);				\
				ih.name##pts   =_mm_read_UBYTE(modreader);				\
				ih.name##beg   =_mm_read_UBYTE(modreader);				\
				ih.name##end   =_mm_read_UBYTE(modreader);				\
				ih.name##susbeg=_mm_read_UBYTE(modreader);				\
				ih.name##susend=_mm_read_UBYTE(modreader);				\
				for(lp=0;lp<ITENVCNT;lp++) {								\
					ih.name##node[lp]=_mm_read_##type(modreader);		\
					ih.name##tick[lp]=_mm_read_I_UWORD(modreader);		\
				}															\
				_mm_read_UBYTE(modreader);

	      IT_LoadEnvelope (vol, UBYTE);
	      IT_LoadEnvelope (pan, SBYTE);
	      IT_LoadEnvelope (pit, SBYTE);
#undef IT_LoadEnvelope
	    }

	  if (_mm_eof (modreader))
	    {
	      _mm_errno = MMERR_LOADING_SAMPLEINFO;
	      return 0;
	    }

	  d->volflg |= EF_VOLENV;
	  d->insname = DupStr (ih.name, 26, 0);
	  d->nnatype = ih.nna;

	  if (mh->cwt < 0x200)
	    {
	      d->volfade = ih.fadeout << 6;
	      if (ih.dnc)
		{
		  d->dct = DCT_NOTE;
		  d->dca = DCA_CUT;
		}

	      if (ih.volflg & 1)
		d->volflg |= EF_ON;
	      if (ih.volflg & 2)
		d->volflg |= EF_LOOP;
	      if (ih.volflg & 4)
		d->volflg |= EF_SUSTAIN;

	      /* XM conversion of IT envelope Array */
	      d->volbeg = ih.volbeg;
	      d->volend = ih.volend;
	      d->volsusbeg = ih.volsusbeg;
	      d->volsusend = ih.volsusend;

	      if (ih.volflg & 1)
		{
		  for (u = 0; u < ITENVCNT; u++)
		    if (ih.oldvoltick[d->volpts] != 0xff)
		      {
			d->volenv[d->volpts].val = (ih.volnode[d->volpts] << 2);
			d->volenv[d->volpts].pos = ih.oldvoltick[d->volpts];
			d->volpts++;
		      }
		    else
		      break;
		}
	    }
	  else
	    {
	      d->panning = ((ih.chanpan & 127) == 64) ? 255 : (ih.chanpan & 127) << 2;
	      if (!(ih.chanpan & 128))
		d->flags |= IF_OWNPAN;

	      if (!(ih.ppsep & 128))
		{
		  d->pitpansep = ih.ppsep << 2;
		  d->pitpancenter = ih.ppcenter;
		  d->flags |= IF_PITCHPAN;
		}
	      d->globvol = ih.globvol >> 1;
	      d->volfade = ih.fadeout << 5;
	      d->dct = ih.dct;
	      d->dca = ih.dca;

	      if (mh->cwt >= 0x204)
		{
		  d->rvolvar = ih.rvolvar;
		  d->rpanvar = ih.rpanvar;
		}

#define IT_ProcessEnvelope(name) 											\
				if(ih.name##flg&1) d->name##flg|=EF_ON;					\
				if(ih.name##flg&2) d->name##flg|=EF_LOOP;				\
				if(ih.name##flg&4) d->name##flg|=EF_SUSTAIN;			\
				d->name##pts=ih.name##pts;								\
				d->name##beg=ih.name##beg;								\
				d->name##end=ih.name##end;								\
				d->name##susbeg=ih.name##susbeg;						\
				d->name##susend=ih.name##susend;						\
																			\
				for(u=0;u<ih.name##pts;u++)								\
					d->name##env[u].pos=ih.name##tick[u];				\
																			\
				if((d->name##flg&EF_ON)&&(d->name##pts<2))				\
					d->name##flg&=~EF_ON;

	      IT_ProcessEnvelope (vol);
	      for (u = 0; u < ih.volpts; u++)
		d->volenv[u].val = (ih.volnode[u] << 2);

	      IT_ProcessEnvelope (pan);
	      for (u = 0; u < ih.panpts; u++)
		d->panenv[u].val =
		  ih.pannode[u] == 32 ? 255 : (ih.pannode[u] + 32) << 2;

	      IT_ProcessEnvelope (pit);
	      for (u = 0; u < ih.pitpts; u++)
		d->pitenv[u].val = ih.pitnode[u] + 32;
#undef IT_ProcessEnvelope

	      if (ih.pitflg & 0x80)
		{
		  /* filter envelopes not supported yet */
		  d->pitflg &= ~EF_ON;
		  ih.pitpts = ih.pitbeg = ih.pitend = 0;
#ifdef MIKMOD_DEBUG
		  {
		    static int warn = 0;

		    if (!warn)
		      fputs ("\rFilter envelopes not supported yet\n", stderr);
		    warn = 1;
		  }
#endif
		}

	      d->volpts = ih.volpts;
	      d->volbeg = ih.volbeg;
	      d->volend = ih.volend;
	      d->volsusbeg = ih.volsusbeg;
	      d->volsusend = ih.volsusend;

	      for (u = 0; u < ih.volpts; u++)
		{
		  d->volenv[u].val = (ih.volnode[u] << 2);
		  d->volenv[u].pos = ih.voltick[u];
		}

	      d->panpts = ih.panpts;
	      d->panbeg = ih.panbeg;
	      d->panend = ih.panend;
	      d->pansusbeg = ih.pansusbeg;
	      d->pansusend = ih.pansusend;

	      for (u = 0; u < ih.panpts; u++)
		{
		  d->panenv[u].val = ih.pannode[u] == 32 ? 255 : (ih.pannode[u] + 32) << 2;
		  d->panenv[u].pos = ih.pantick[u];
		}

	      d->pitpts = ih.pitpts;
	      d->pitbeg = ih.pitbeg;
	      d->pitend = ih.pitend;
	      d->pitsusbeg = ih.pitsusbeg;
	      d->pitsusend = ih.pitsusend;

	      for (u = 0; u < ih.pitpts; u++)
		{
		  d->pitenv[u].val = ih.pitnode[u] + 32;
		  d->pitenv[u].pos = ih.pittick[u];
		}
	    }

	  for (u = 0; u < ITNOTECNT; u++)
	    {
	      d->samplenote[u] = (ih.samptable[u] & 255);
	      d->samplenumber[u] =
		(ih.samptable[u] >> 8) ? ((ih.samptable[u] >> 8) - 1) : 0xffff;
	      if (d->samplenumber[u] >= of.numsmp)
		d->samplenote[u] = 255;
	      else if (of.flags & UF_LINEAR)
		{
		  int note = (int) d->samplenote[u] + noteindex[d->samplenumber[u]];
		  d->samplenote[u] = (note < 0) ? 0 : (note > 255 ? 255 : note);
		}
	    }

	  d++;
	}
    }
  else if (of.flags & UF_LINEAR)
    {
      if (!AllocInstruments ())
	return 0;
      d = of.instruments;
      of.flags |= UF_INST;

      for (t = 0; t < mh->smpnum; t++, d++)
	for (u = 0; u < ITNOTECNT; u++)
	  {
	    if (d->samplenumber[u] >= of.numsmp)
	      d->samplenote[u] = 255;
	    else
	      {
		int note = (int) d->samplenote[u] + noteindex[d->samplenumber[u]];
		d->samplenote[u] = (note < 0) ? 0 : (note > 255 ? 255 : note);
	      }
	  }
    }

  /* Figure out how many channels this song actually uses */
  of.numchn = 0;
  memset (remap, -1, 64 * sizeof (UBYTE));
  for (t = 0; t < of.numpat; t++)
    {
      UWORD packlen;

      /* seek to pattern position */
      if (paraptr[mh->insnum + mh->smpnum + t])
	{			/* 0 -> empty 64 row pattern */
	  _mm_fseek (modreader, ((long) paraptr[mh->insnum + mh->smpnum + t]), SEEK_SET);
	  _mm_read_I_UWORD (modreader);
	  /* read pattern length (# of rows)
	     Impulse Tracker never creates patterns with less than 32 rows,
	     but some other trackers do, so we only check for more than 256
	     rows */
	  packlen = _mm_read_I_UWORD (modreader);
	  if (packlen > 256)
	    {
	      _mm_errno = MMERR_LOADING_PATTERN;
	      return 0;
	    }
	  _mm_read_I_ULONG (modreader);
	  if (IT_GetNumChannels (packlen))
	    return 0;
	}
    }

  /* give each of them a different number */
  for (t = 0; t < 64; t++)
    if (!remap[t])
      remap[t] = of.numchn++;

  of.numtrk = of.numpat * of.numchn;
  if (of.numvoices)
    if (of.numvoices < of.numchn)
      of.numvoices = of.numchn;

  if (!AllocPatterns ())
    return 0;
  if (!AllocTracks ())
    return 0;

  for (t = 0; t < of.numpat; t++)
    {
      UWORD packlen;

      /* seek to pattern position */
      if (!paraptr[mh->insnum + mh->smpnum + t])
	{			/* 0 -> empty 64 row pattern */
	  of.pattrows[t] = 64;
	  for (u = 0; u < of.numchn; u++)
	    {
	      int k;

	      UniReset ();
	      for (k = 0; k < 64; k++)
		UniNewline ();
	      of.tracks[numtrk++] = UniDup ();
	    }
	}
      else
	{
	  _mm_fseek (modreader, ((long) paraptr[mh->insnum + mh->smpnum + t]), SEEK_SET);
	  packlen = _mm_read_I_UWORD (modreader);
	  of.pattrows[t] = _mm_read_I_UWORD (modreader);
	  _mm_read_I_ULONG (modreader);
	  if (!IT_ReadPattern (of.pattrows[t]))
	    return 0;
	}
    }

  return 1;
}

CHAR *
IT_LoadTitle (void)
{
  CHAR s[26];

  _mm_fseek (modreader, 4, SEEK_SET);
  if (!_mm_read_UBYTES (s, 26, modreader))
    return NULL;

  return (DupStr (s, 26, 0));
}

/*========== Loader information */

MLOADER load_it =
{
  NULL,
  "IT",
  "IT (Impulse Tracker)",
  IT_Init,
  IT_Test,
  IT_Load,
  IT_Cleanup,
  IT_LoadTitle
};

/* ex:set ts=4: */
