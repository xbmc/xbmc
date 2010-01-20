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

  $Id: load_stx.c,v 1.12 1999/10/25 16:31:41 miod Exp $

  STMIK 0.2 (STX) module loader

==============================================================================*/

/*

   Written by Claudio Matsuoka <claudio@helllabs.org>

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "unimod_priv.h"

/*========== Module structure */

/* header */
typedef struct STXHEADER
  {
    CHAR songname[20];
    CHAR trackername[8];
    UWORD patsize;
    UWORD unknown1;
    UWORD patptr;
    UWORD insptr;
    UWORD chnptr;		/* not sure */
    UWORD unknown2;
    UWORD unknown3;
    UBYTE mastermult;
    UBYTE initspeed;
    UWORD unknown4;
    UWORD unknown5;
    UWORD patnum;
    UWORD insnum;
    UWORD ordnum;
    UWORD unknown6;
    UWORD unknown7;
    UWORD unknown8;
    CHAR scrm[4];
  }
STXHEADER;

/* sample information */
typedef struct STXSAMPLE
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
STXSAMPLE;

typedef struct STXNOTE
  {
    UBYTE note, ins, vol, cmd, inf;
  }
STXNOTE;

/*========== Loader variables */

static STXNOTE *stxbuf = NULL;	/* pointer to a complete STX pattern */
static STXHEADER *mh = NULL;
static UWORD *paraptr = NULL;	/* parapointer array (see STX docs) */

/*========== Loader code */

static BOOL 
STX_Test (void)
{
  UBYTE id[8];
  int t;

  _mm_fseek(modreader,0x14,SEEK_SET);
  if(!_mm_read_UBYTES(id, 8, modreader))
    return 0;

  for(t=0;t<STM_NTRACKERS;t++)
    if(!memcmp(id,STM_Signatures[t],8)) return 1;

  _mm_fseek (modreader, 0x3C, SEEK_SET);
  if (!_mm_read_UBYTES (id, 4, modreader))
    return 0;
  if (memcmp (id, "SCRM", 4))
    return 0;

  return 1;
}

static BOOL 
STX_Init (void)
{
  if (!(stxbuf = (STXNOTE *) _mm_malloc (4 * 64 * sizeof (STXNOTE))))
    return 0;
  if (!(mh = (STXHEADER *) _mm_malloc (sizeof (STXHEADER))))
    return 0;
  if (!(poslookup = (UBYTE *) _mm_malloc (sizeof (UBYTE) * 256)))
    return 0;
  memset (poslookup, -1, 256);

  return 1;
}

static void 
STX_Cleanup (void)
{
  _mm_free (stxbuf);
  _mm_free (paraptr);
  _mm_free (poslookup);
  _mm_free (mh);
}

static BOOL 
STX_ReadPattern (void)
{
  int row = 0, flag, ch;
  STXNOTE *n, dummy;

  /* clear pattern data */
  memset (stxbuf, 255, 4 * 64 * sizeof (STXNOTE));

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
	  ch = flag & 31;

	  if ((ch >= 0) && (ch < 4))
	    n = &stxbuf[(64U * ch) + row];
	  else
	    n = &dummy;

	  if (flag & 32)
	    {
	      n->note = _mm_read_UBYTE (modreader);
	      n->ins = _mm_read_UBYTE (modreader);
	    }
	  if (flag & 64)
	    {
	      n->vol = _mm_read_UBYTE (modreader);
	      if (n->vol > 64)
	        n->vol = 64;
	    }
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
STX_ConvertTrack (STXNOTE * tr)
{
  int t;

  UniReset ();
  for (t = 0; t < 64; t++)
    {
      UBYTE note, ins, vol, cmd, inf;

      note = tr[t].note;
      ins = tr[t].ins;
      vol = tr[t].vol;
      cmd = tr[t].cmd;
      inf = tr[t].inf;

      if ((ins) && (ins != 255))
	UniInstrument (ins - 1);
      if ((note) && (note != 255))
	{
	  if (note == 254)
	    {
	      UniPTEffect (0xc, 0);	/* note cut command */
	      vol = 255;
	    }
	  else
	    UniNote (24 + ((note >> 4) * OCTAVE) + (note & 0xf));	/* normal note */
	}

      if (vol < 255)
	UniPTEffect (0xc, vol);

      if (cmd < 255)
	switch (cmd)
	  {
	  case 1:		/* Axx set speed to xx */
	    UniPTEffect (0xf, inf >> 4);
	    break;
	  case 2:		/* Bxx position jump */
	    UniPTEffect (0xb, inf);
	    break;
	  case 3:		/* Cxx patternbreak to row xx */
	    UniPTEffect (0xd, (((inf & 0xf0) >> 4) * 10) + (inf & 0xf));
	    break;
	  case 4:		/* Dxy volumeslide */
	    UniEffect (UNI_S3MEFFECTD, inf);
	    break;
	  case 5:		/* Exy toneslide down */
	    UniEffect (UNI_S3MEFFECTE, inf);
	    break;
	  case 6:		/* Fxy toneslide up */
	    UniEffect (UNI_S3MEFFECTF, inf);
	    break;
	  case 7:		/* Gxx Tone portamento,speed xx */
	    UniPTEffect (0x3, inf);
	    break;
	  case 8:		/* Hxy vibrato */
	    UniPTEffect (0x4, inf);
	    break;
	  case 9:		/* Ixy tremor, ontime x, offtime y */
	    UniEffect (UNI_S3MEFFECTI, inf);
	    break;
	  case 0:		/* protracker arpeggio */
	    if (!inf)
	      break;
	    /* fall through */
	  case 0xa:		/* Jxy arpeggio */
	    UniPTEffect (0x0, inf);
	    break;
	  case 0xb:		/* Kxy Dual command H00 & Dxy */
	    UniPTEffect (0x4, 0);
	    UniEffect (UNI_S3MEFFECTD, inf);
	    break;
	  case 0xc:		/* Lxy Dual command G00 & Dxy */
	    UniPTEffect (0x3, 0);
	    UniEffect (UNI_S3MEFFECTD, inf);
	    break;
	    /* Support all these above, since ST2 can LOAD these values but can
	       actually only play up to J - and J is only half-way implemented
	       in ST2 */
	  case 0x18:		/* Xxx amiga panning command 8xx */
	    UniPTEffect (0x8, inf);
	    break;
	  }
      UniNewline ();
    }
  return UniDup ();
}

static BOOL 
STX_Load (BOOL curious)
{
  int t, u, track = 0;
  int version = 0;
  SAMPLE *q;
  char *tracker;

  /* try to read module header */
  _mm_read_string (mh->songname, 20, modreader);
  _mm_read_string (mh->trackername, 8, modreader);
  mh->patsize = _mm_read_I_UWORD (modreader);
  mh->unknown1 = _mm_read_I_UWORD (modreader);
  mh->patptr = _mm_read_I_UWORD (modreader);
  mh->insptr = _mm_read_I_UWORD (modreader);
  mh->chnptr = _mm_read_I_UWORD (modreader);
  mh->unknown2 = _mm_read_I_UWORD (modreader);
  mh->unknown3 = _mm_read_I_UWORD (modreader);
  mh->mastermult = _mm_read_UBYTE (modreader);
  mh->initspeed = _mm_read_UBYTE (modreader) >> 4;
  mh->unknown4 = _mm_read_I_UWORD (modreader);
  mh->unknown5 = _mm_read_I_UWORD (modreader);
  mh->patnum = _mm_read_I_UWORD (modreader);
  mh->insnum = _mm_read_I_UWORD (modreader);
  mh->ordnum = _mm_read_I_UWORD (modreader);
  mh->unknown6 = _mm_read_I_UWORD (modreader);
  mh->unknown7 = _mm_read_I_UWORD (modreader);
  mh->unknown8 = _mm_read_I_UWORD (modreader);
  _mm_read_string (mh->scrm, 4, modreader);

  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  tracker = "unknown tracker";
  for (t = 0; t < STM_NTRACKERS; t++)
    if (!memcmp (mh->trackername, STM_Signatures[t], 8))
      {
	tracker = STM_Version[t];
	break;
      }

  of.modtype = _mm_malloc(
    strlen(tracker) +
    strlen("STM2STX 1.x ()") + 1);

  sprintf(of.modtype, "STM2STX 1.x (%s)", tracker);

  /* set module variables */
  of.songname = DupStr (mh->songname, 20, 1);
  of.numpat = mh->patnum;
  of.reppos = 0;
  of.numins = of.numsmp = mh->insnum;
  of.initspeed = mh->initspeed;
  of.inittempo = 125;
  of.numchn = 4;
  of.flags |= UF_S3MSLIDES;

  if (!(paraptr = (UWORD *) _mm_malloc ((of.numins + of.numpat) * sizeof (UWORD))))
    return 0;

  /* read the instrument+pattern parapointers */
  _mm_fseek (modreader, mh->insptr << 4, SEEK_SET);
  _mm_read_I_UWORDS (paraptr, of.numins, modreader);
  _mm_fseek (modreader, mh->patptr << 4, SEEK_SET);
  _mm_read_I_UWORDS (paraptr + of.numins, of.numpat, modreader);

  /* check module version */
  _mm_fseek (modreader, paraptr[of.numins] << 4, SEEK_SET);
  version = _mm_read_I_UWORD (modreader);
  if (version == mh->patsize)
    {
      version = 0x10;
      of.modtype[10] = '0';
    }
  else
    {
      version = 0x11;
      of.modtype[10] = '1';
    }

  /* read the order data */
  _mm_fseek (modreader, (mh->chnptr << 4) + 32, SEEK_SET);
  if (!AllocPositions (mh->ordnum))
    return 0;
  for (t = 0; t < mh->ordnum; t++)
    {
      of.positions[t] = _mm_read_UBYTE (modreader);
      _mm_fseek (modreader, 4, SEEK_CUR);
    }

  of.numpos = 0;
  poslookupcnt = mh->ordnum;
  for (t = 0; t < mh->ordnum; t++)
    {
      of.positions[of.numpos] = of.positions[t];
      poslookup[t] = of.numpos;	/* bug fix for freaky S3Ms */
      if (of.positions[t] < 254)
	of.numpos++;
      else
	/* special end of song pattern */
      if ((of.positions[t] == 255) && (!curious))
	break;
    }

  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  /* load samples */
  if (!AllocSamples ())
    return 0;
  for (q = of.samples, t = 0; t < of.numins; t++, q++)
    {
      STXSAMPLE s;

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

      q->samplename = DupStr (s.sampname, 28, 1);
      q->speed = (s.c2spd * 8363) / 8448;
      q->length = s.length;
      q->loopstart = s.loopbeg;
      q->loopend = s.loopend;
      q->volume = s.volume;
      q->seekpos = (((long) s.memsegh) << 16 | s.memsegl) << 4;
      q->flags |= SF_SIGNED;

      /* fix for bad converted STMs */
      if (q->loopstart >= q->length)
	q->loopstart = q->loopend = 0;

      /* some modules come with loopstart == loopend == 0, yet have the
       * looping flag set */
      if ((s.flags & 1) && (q->loopstart != q->loopend))
	{
	  q->flags |= SF_LOOP;
	  if (q->loopend > q->length)
	    q->loopend = q->length;
	}
      if (s.flags & 4)
	q->flags |= SF_16BITS;
    }

  /* load pattern info */
  of.numtrk = of.numpat * of.numchn;
  if (!AllocTracks ())
    return 0;
  if (!AllocPatterns ())
    return 0;

  for (t = 0; t < of.numpat; t++)
    {
      /* seek to pattern position (+2 skip pattern length) */
      _mm_fseek (modreader, (((long) paraptr[of.numins + t]) << 4) +
		 (version == 0x10 ? 2 : 0), SEEK_SET);
      if (!STX_ReadPattern ())
	return 0;
      for (u = 0; u < of.numchn; u++)
	if (!(of.tracks[track++] = STX_ConvertTrack (&stxbuf[u * 64])))
	  return 0;
    }

  return 1;
}

static CHAR *
STX_LoadTitle (void)
{
  CHAR s[28];

  _mm_fseek (modreader, 0, SEEK_SET);
  if (!_mm_read_UBYTES (s, 20, modreader))
    return NULL;

  return (DupStr (s, 28, 1));
}

/*========== Loader information */

MLOADER load_stx =
{
  NULL,
  "STX",
  "STX (Scream Tracker Music Interface Kit)",
  STX_Init,
  STX_Test,
  STX_Load,
  STX_Cleanup,
  STX_LoadTitle
};

/* ex:set ts=4: */
