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

  $Id: load_s3m.c,v 1.32 1999/10/25 16:31:41 miod Exp $

  Screamtracker (S3M) module loader

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "unimod_priv.h"

/*========== Module structure */

/* header */
typedef struct S3MHEADER
  {
    CHAR songname[28];
    UBYTE t1a;
    UBYTE type;
    UBYTE unused1[2];
    UWORD ordnum;
    UWORD insnum;
    UWORD patnum;
    UWORD flags;
    UWORD tracker;
    UWORD fileformat;
    CHAR scrm[4];
    UBYTE mastervol;
    UBYTE initspeed;
    UBYTE inittempo;
    UBYTE mastermult;
    UBYTE ultraclick;
    UBYTE pantable;
    UBYTE unused2[8];
    UWORD special;
    UBYTE channels[32];
  }
S3MHEADER;

/* sample information */
typedef struct S3MSAMPLE
  {
    UBYTE type;
    CHAR filename[12];
    UBYTE memsegh;
    UWORD memsegl;
    ULONG length;
    ULONG loopbeg;
    ULONG loopend;
    UBYTE volume;
    UBYTE dsk;
    UBYTE pack;
    UBYTE flags;
    ULONG c2spd;
    UBYTE unused[12];
    CHAR sampname[28];
    CHAR scrs[4];
  }
S3MSAMPLE;

typedef struct S3MNOTE
  {
    UBYTE note, ins, vol, cmd, inf;
  }
S3MNOTE;

/*========== Loader variables */

static S3MNOTE *s3mbuf = NULL;	/* pointer to a complete S3M pattern */
static S3MHEADER *mh = NULL;
static UWORD *paraptr = NULL;	/* parapointer array (see S3M docs) */

/* tracker identifiers */
#define NUMTRACKERS 4
static CHAR *S3M_Version[] =
{
  "Screamtracker x.xx",
  "Imago Orpheus x.xx (S3M format)",
  "Impulse Tracker x.xx (S3M format)",
  "Unknown tracker x.xx (S3M format)",
  "Impulse Tracker 2.14p3 (S3M format)",
  "Impulse Tracker 2.14p4 (S3M format)"
};
/* version number position in above array */
static int numeric[NUMTRACKERS] =
{14, 14, 16, 16};

/*========== Loader code */

BOOL 
S3M_Test (void)
{
  UBYTE id[4];

  _mm_fseek (modreader, 0x2c, SEEK_SET);
  if (!_mm_read_UBYTES (id, 4, modreader))
    return 0;
  if (!memcmp (id, "SCRM", 4))
    return 1;
  return 0;
}

BOOL 
S3M_Init (void)
{
  if (!(s3mbuf = (S3MNOTE *) _mm_malloc (32 * 64 * sizeof (S3MNOTE))))
    return 0;
  if (!(mh = (S3MHEADER *) _mm_malloc (sizeof (S3MHEADER))))
    return 0;
  if (!(poslookup = (UBYTE *) _mm_malloc (sizeof (UBYTE) * 256)))
    return 0;
  memset (poslookup, -1, 256);

  return 1;
}

void 
S3M_Cleanup (void)
{
  _mm_free (s3mbuf);
  _mm_free (paraptr);
  _mm_free (poslookup);
  _mm_free (mh);
  _mm_free (origpositions);
}

/* Because so many s3m files have 16 channels as the set number used, but really
   only use far less (usually 8 to 12 still), I had to make this function, which
   determines the number of channels that are actually USED by a pattern.

   For every channel that's used, it sets the appropriate array entry of the
   global variable 'remap'

   NOTE: You must first seek to the file location of the pattern before calling
   this procedure.

   Returns 1 on fail.                                                         */
static BOOL 
S3M_GetNumChannels (void)
{
  int row = 0, flag, ch;

  while (row < 64)
    {
      flag = _mm_read_UBYTE (modreader);

      if (_mm_eof (modreader))
	{
	  _mm_errno = MMERR_LOADING_PATTERN;
	  return 1;
	}

      if (flag)
	{
	  ch = flag & 31;
	  if (mh->channels[ch] < 32)
	    remap[ch] = 0;
	  if (flag & 32)
	    {
	      _mm_read_UBYTE (modreader);
	      _mm_read_UBYTE (modreader);
	    }
	  if (flag & 64)
	    _mm_read_UBYTE (modreader);
	  if (flag & 128)
	    {
	      _mm_read_UBYTE (modreader);
	      _mm_read_UBYTE (modreader);
	    }
	}
      else
	row++;
    }
  return 0;
}

static BOOL 
S3M_ReadPattern (void)
{
  int row = 0, flag, ch;
  S3MNOTE *n, dummy;

  /* clear pattern data */
  memset (s3mbuf, 255, 32 * 64 * sizeof (S3MNOTE));

  while (row < 64)
    {
      flag = _mm_read_UBYTE (modreader);

      if (_mm_eof (modreader))
	{
	  _mm_errno = MMERR_LOADING_PATTERN;
	  return 0;
	}

      if (flag)
	{
	  ch = remap[flag & 31];

	  if (ch != -1)
	    n = &s3mbuf[(64U * ch) + row];
	  else
	    n = &dummy;

	  if (flag & 32)
	    {
	      n->note = _mm_read_UBYTE (modreader);
	      n->ins = _mm_read_UBYTE (modreader);
	    }
	  if (flag & 64)
	    n->vol = _mm_read_UBYTE (modreader);
	  if (flag & 128)
	    {
	      n->cmd = _mm_read_UBYTE (modreader);
	      n->inf = _mm_read_UBYTE (modreader);
	    }
	}
      else
	row++;
    }
  return 1;
}

static UBYTE *
S3M_ConvertTrack (S3MNOTE * tr)
{
  int t;

  UniReset ();
  for (t = 0; t < 64; t++)
    {
      UBYTE note, ins, vol;

      note = tr[t].note;
      ins = tr[t].ins;
      vol = tr[t].vol;

      if ((ins) && (ins != 255))
	UniInstrument (ins - 1);
      if (note != 255)
	{
	  if (note == 254)
	    {
	      UniPTEffect (0xc, 0);	/* note cut command */
	      vol = 255;
	    }
	  else
	    UniNote (((note >> 4) * OCTAVE) + (note & 0xf));	/* normal note */
	}
      if (vol < 255)
	UniPTEffect (0xc, vol);

      S3MIT_ProcessCmd (tr[t].cmd, tr[t].inf, 1);
      UniNewline ();
    }
  return UniDup ();
}

BOOL 
S3M_Load (BOOL curious)
{
  int t, u, track = 0;
  SAMPLE *q;
  UBYTE pan[32];

  /* try to read module header */
  _mm_read_string (mh->songname, 28, modreader);
  mh->t1a = _mm_read_UBYTE (modreader);
  mh->type = _mm_read_UBYTE (modreader);
  _mm_read_UBYTES (mh->unused1, 2, modreader);
  mh->ordnum = _mm_read_I_UWORD (modreader);
  mh->insnum = _mm_read_I_UWORD (modreader);
  mh->patnum = _mm_read_I_UWORD (modreader);
  mh->flags = _mm_read_I_UWORD (modreader);
  mh->tracker = _mm_read_I_UWORD (modreader);
  mh->fileformat = _mm_read_I_UWORD (modreader);
  _mm_read_string (mh->scrm, 4, modreader);
  mh->mastervol = _mm_read_UBYTE (modreader);
  mh->initspeed = _mm_read_UBYTE (modreader);
  mh->inittempo = _mm_read_UBYTE (modreader);
  mh->mastermult = _mm_read_UBYTE (modreader);
  mh->ultraclick = _mm_read_UBYTE (modreader);
  mh->pantable = _mm_read_UBYTE (modreader);
  _mm_read_UBYTES (mh->unused2, 8, modreader);
  mh->special = _mm_read_I_UWORD (modreader);
  _mm_read_UBYTES (mh->channels, 32, modreader);

  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  /* set module variables */
  of.songname = DupStr (mh->songname, 28, 0);
  of.numpat = mh->patnum;
  of.reppos = 0;
  of.numins = of.numsmp = mh->insnum;
  of.initspeed = mh->initspeed;
  of.inittempo = mh->inittempo;
  of.initvolume = mh->mastervol << 1;
  of.flags |= UF_ARPMEM;
  if ((mh->tracker == 0x1300) || (mh->flags & 64))
    of.flags |= UF_S3MSLIDES;

  /* read the order data */
  if (!AllocPositions (mh->ordnum))
    return 0;
  if (!(origpositions = _mm_calloc (mh->ordnum, sizeof (UWORD))))
    return 0;

  for (t = 0; t < mh->ordnum; t++)
    {
      origpositions[t] = _mm_read_UBYTE (modreader);
      if ((origpositions[t] >= mh->patnum) && (origpositions[t] < 254))
	origpositions[t] = 255 /*mh->patnum-1 */ ;
    }

  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  poslookupcnt = mh->ordnum;
  S3MIT_CreateOrders (curious);

  if (!(paraptr = (UWORD *) _mm_malloc ((of.numins + of.numpat) * sizeof (UWORD))))
    return 0;

  /* read the instrument+pattern parapointers */
  _mm_read_I_UWORDS (paraptr, of.numins + of.numpat, modreader);

  if (mh->pantable == 252)
    {
      /* read the panning table (ST 3.2 addition.  See below for further
         portions of channel panning [past reampper]). */
      _mm_read_UBYTES (pan, 32, modreader);
    }

  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  /* load samples */
  if (!AllocSamples ())
    return 0;
  q = of.samples;
  for (t = 0; t < of.numins; t++)
    {
      S3MSAMPLE s;

      /* seek to instrument position */
      _mm_fseek (modreader, ((long) paraptr[t]) << 4, SEEK_SET);
      /* and load sample info */
      s.type = _mm_read_UBYTE (modreader);
      _mm_read_string (s.filename, 12, modreader);
      s.memsegh = _mm_read_UBYTE (modreader);
      s.memsegl = _mm_read_I_UWORD (modreader);
      s.length = _mm_read_I_ULONG (modreader);
      s.loopbeg = _mm_read_I_ULONG (modreader);
      s.loopend = _mm_read_I_ULONG (modreader);
      s.volume = _mm_read_UBYTE (modreader);
      s.dsk = _mm_read_UBYTE (modreader);
      s.pack = _mm_read_UBYTE (modreader);
      s.flags = _mm_read_UBYTE (modreader);
      s.c2spd = _mm_read_I_ULONG (modreader);
      _mm_read_UBYTES (s.unused, 12, modreader);
      _mm_read_string (s.sampname, 28, modreader);
      _mm_read_string (s.scrs, 4, modreader);

      if (_mm_eof (modreader))
	{
	  _mm_errno = MMERR_LOADING_SAMPLEINFO;
	  return 0;
	}

      q->samplename = DupStr (s.sampname, 28, 0);
      q->speed = s.c2spd;
      q->length = s.length;
      q->loopstart = s.loopbeg > s.length ? s.length : s.loopbeg;
      q->loopend = s.loopend > s.length ? s.length : s.loopend;
      q->volume = s.volume;
      q->seekpos = (((long) s.memsegh) << 16 | s.memsegl) << 4;

      if (s.flags & 1)
	q->flags |= SF_LOOP;
      if (s.flags & 4)
	q->flags |= SF_16BITS;
      if (mh->fileformat == 1)
	q->flags |= SF_SIGNED;

      /* don't load sample if it doesn't have the SCRS tag */
      if (memcmp (s.scrs, "SCRS", 4))
	q->length = 0;

      q++;
    }

  /* determine the number of channels actually used. */
  of.numchn = 0;
  memset (remap, -1, 32 * sizeof (UBYTE));
  for (t = 0; t < of.numpat; t++)
    {
      /* seek to pattern position (+2 skip pattern length) */
      _mm_fseek (modreader, (long) ((paraptr[of.numins + t]) << 4) + 2, SEEK_SET);
      if (S3M_GetNumChannels ())
	return 0;
    }
  /* then we can decide the module type */
  t = mh->tracker >> 12;
  if ((!t) || (t > 3))
    t = NUMTRACKERS - 1;	/* unknown tracker */
  else
    {
      if (mh->tracker >= 0x3217)
	t = NUMTRACKERS + 1;	/* IT 2.14p4 */
      else if (mh->tracker >= 0x3216)
	t = NUMTRACKERS;	/* IT 2.14p3 */
      else
	t--;
    }
  of.modtype = strdup (S3M_Version[t]);
  if (t < NUMTRACKERS)
    {
      of.modtype[numeric[t]] = ((mh->tracker >> 8) & 0xf) + '0';
      of.modtype[numeric[t] + 2] = ((mh->tracker >> 4) & 0xf) + '0';
      of.modtype[numeric[t] + 3] = ((mh->tracker) & 0xf) + '0';
    }

  /* build the remap array  */
  for (t = 0; t < 32; t++)
    if (!remap[t])
      remap[t] = of.numchn++;

  /* set panning positions after building remap chart! */
  for (t = 0; t < 32; t++)
    if ((mh->channels[t] < 32) && (remap[t] != -1))
      {
	if (mh->channels[t] < 8)
	  of.panning[remap[t]] = 0x20;	/* 0x30 = std s3m val */
	else
	  of.panning[remap[t]] = 0xd0;	/* 0xc0 = std s3m val */
      }
  if (mh->pantable == 252)
    /* set panning positions according to panning table (new for st3.2) */
    for (t = 0; t < 32; t++)
      if ((pan[t] & 0x20) && (mh->channels[t] < 32) && (remap[t] != -1))
	of.panning[remap[t]] = (pan[t] & 0xf) << 4;

  /* load pattern info */
  of.numtrk = of.numpat * of.numchn;
  if (!AllocTracks ())
    return 0;
  if (!AllocPatterns ())
    return 0;

  for (t = 0; t < of.numpat; t++)
    {
      /* seek to pattern position (+2 skip pattern length) */
      _mm_fseek (modreader, (((long) paraptr[of.numins + t]) << 4) + 2, SEEK_SET);
      if (!S3M_ReadPattern ())
	return 0;
      for (u = 0; u < of.numchn; u++)
	if (!(of.tracks[track++] = S3M_ConvertTrack (&s3mbuf[u * 64])))
	  return 0;
    }

  return 1;
}

CHAR *
S3M_LoadTitle (void)
{
  CHAR s[28];

  _mm_fseek (modreader, 0, SEEK_SET);
  if (!_mm_read_UBYTES (s, 28, modreader))
    return NULL;

  return (DupStr (s, 28, 0));
}

/*========== Loader information */

MLOADER load_s3m =
{
  NULL,
  "S3M",
  "S3M (Scream Tracker 3)",
  S3M_Init,
  S3M_Test,
  S3M_Load,
  S3M_Cleanup,
  S3M_LoadTitle
};

/* ex:set ts=4: */
