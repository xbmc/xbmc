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

  $Id: load_stm.c,v 1.30 1999/10/25 16:31:41 miod Exp $

  Screamtracker 2 (STM) module loader

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "unimod_priv.h"

/*========== Module structure */

/* sample information */
typedef struct STMSAMPLE
  {
    CHAR filename[12];
    UBYTE unused;		/* 0x00 */
    UBYTE instdisk;		/* Instrument disk */
    UWORD reserved;
    UWORD length;		/* Sample length */
    UWORD loopbeg;		/* Loop start point */
    UWORD loopend;		/* Loop end point */
    UBYTE volume;		/* Volume */
    UBYTE reserved2;
    UWORD c2spd;		/* Good old c2spd */
    ULONG reserved3;
    UWORD isa;
  }
STMSAMPLE;

/* header */
typedef struct STMHEADER
  {
    CHAR songname[20];
    CHAR trackername[8];	/* !Scream! for ST 2.xx  */
    UBYTE unused;		/* 0x1A  */
    UBYTE filetype;		/* 1=song, 2=module */
    UBYTE ver_major;
    UBYTE ver_minor;
    UBYTE inittempo;		/* initspeed= stm inittempo>>4  */
    UBYTE numpat;		/* number of patterns  */
    UBYTE globalvol;
    UBYTE reserved[13];
    STMSAMPLE sample[31];	/* STM sample data */
    UBYTE patorder[128];	/* Docs say 64 - actually 128 */
  }
STMHEADER;

typedef struct STMNOTE
  {
    UBYTE note, insvol, volcmd, cmdinf;
  }
STMNOTE;

/*========== Loader variables */

static STMNOTE *stmbuf = NULL;
static STMHEADER *mh = NULL;

/*========== Loader code */

BOOL 
STM_Test (void)
{
  UBYTE str[44];
  int t;

  _mm_fseek (modreader, 20, SEEK_SET);
  _mm_read_UBYTES (str, 44, modreader);
  if (str[9] != 2)
    return 0;			/* STM Module = filetype 2 */

  if(!memcmp (str + 40, "SCRM", 4))
    return 0;

  for (t=0;t<STM_NTRACKERS;t++)
    if(!memcmp (str, STM_Signatures[t], 8))
      return 1;

  return 0;
}

BOOL 
STM_Init (void)
{
  if (!(mh = (STMHEADER *) _mm_malloc (sizeof (STMHEADER))))
    return 0;
  if (!(stmbuf = (STMNOTE *) _mm_calloc (64U * 4, sizeof (STMNOTE))))
    return 0;

  return 1;
}

static void 
STM_Cleanup (void)
{
  _mm_free (mh);
  _mm_free (stmbuf);
}

static void 
STM_ConvertNote (STMNOTE * n)
{
  UBYTE note, ins, vol, cmd, inf;

  /* extract the various information from the 4 bytes that make up a note */
  note = n->note;
  ins = n->insvol >> 3;
  vol = (n->insvol & 7) + ((n->volcmd & 0x70) >> 1);
  cmd = n->volcmd & 15;
  inf = n->cmdinf;

  if ((ins) && (ins < 32))
    UniInstrument (ins - 1);

  /* special values of [SBYTE0] are handled here 
     we have no idea if these strange values will ever be encountered.
     but it appears as those stms sound correct. */
  if ((note == 254) || (note == 252))
    {
      UniPTEffect (0xc, 0);	/* note cut */
      n->volcmd |= 0x80;
    }
  else
    /* if note < 251, then all three bytes are stored in the file */
  if (note < 251)
    UniNote ((((note >> 4) + 2) * OCTAVE) + (note & 0xf));

  if ((!(n->volcmd & 0x80)) && (vol < 65))
    UniPTEffect (0xc, vol);
  if (cmd != 255)
    switch (cmd)
      {
      case 1:			/* Axx set speed to xx */
	UniPTEffect (0xf, inf >> 4);
	break;
      case 2:			/* Bxx position jump */
	UniPTEffect (0xb, inf);
	break;
      case 3:			/* Cxx patternbreak to row xx */
	UniPTEffect (0xd, (((inf & 0xf0) >> 4) * 10) + (inf & 0xf));
	break;
      case 4:			/* Dxy volumeslide */
	UniEffect (UNI_S3MEFFECTD, inf);
	break;
      case 5:			/* Exy toneslide down */
	UniEffect (UNI_S3MEFFECTE, inf);
	break;
      case 6:			/* Fxy toneslide up */
	UniEffect (UNI_S3MEFFECTF, inf);
	break;
      case 7:			/* Gxx Tone portamento,speed xx */
	UniPTEffect (0x3, inf);
	break;
      case 8:			/* Hxy vibrato */
	UniPTEffect (0x4, inf);
	break;
      case 9:			/* Ixy tremor, ontime x, offtime y */
	UniEffect (UNI_S3MEFFECTI, inf);
	break;
      case 0:			/* protracker arpeggio */
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
}

static UBYTE *
STM_ConvertTrack (STMNOTE * n)
{
  int t;

  UniReset ();
  for (t = 0; t < 64; t++)
    {
      STM_ConvertNote (n);
      UniNewline ();
      n += of.numchn;
    }
  return UniDup ();
}

static BOOL 
STM_LoadPatterns (void)
{
  int t, s, tracks = 0;

  if (!AllocPatterns ())
    return 0;
  if (!AllocTracks ())
    return 0;

  /* Allocate temporary buffer for loading and converting the patterns */
  for (t = 0; t < of.numpat; t++)
    {
      for (s = 0; s < (64U * of.numchn); s++)
	{
	  stmbuf[s].note = _mm_read_UBYTE (modreader);
	  stmbuf[s].insvol = _mm_read_UBYTE (modreader);
	  stmbuf[s].volcmd = _mm_read_UBYTE (modreader);
	  stmbuf[s].cmdinf = _mm_read_UBYTE (modreader);
	}

      if (_mm_eof (modreader))
	{
	  _mm_errno = MMERR_LOADING_PATTERN;
	  return 0;
	}

      for (s = 0; s < of.numchn; s++)
	if (!(of.tracks[tracks++] = STM_ConvertTrack (stmbuf + s)))
	  return 0;
    }
  return 1;
}

BOOL 
STM_Load (BOOL curious)
{
  int t;
  ULONG ourISA;			/* We must generate our own ISA, it's not stored in stm */
  SAMPLE *q;

  /* try to read stm header */
  _mm_read_string (mh->songname, 20, modreader);
  _mm_read_string (mh->trackername, 8, modreader);
  mh->unused = _mm_read_UBYTE (modreader);
  mh->filetype = _mm_read_UBYTE (modreader);
  mh->ver_major = _mm_read_UBYTE (modreader);
  mh->ver_minor = _mm_read_UBYTE (modreader);
  mh->inittempo = _mm_read_UBYTE (modreader);
  if (!mh->inittempo)
    {
      _mm_errno = MMERR_NOT_A_MODULE;
      return 0;
    }
  mh->numpat = _mm_read_UBYTE (modreader);
  mh->globalvol = _mm_read_UBYTE (modreader);
  _mm_read_UBYTES (mh->reserved, 13, modreader);

  for (t = 0; t < 31; t++)
    {
      STMSAMPLE *s = &mh->sample[t];	/* STM sample data */

      _mm_read_string (s->filename, 12, modreader);
      s->unused = _mm_read_UBYTE (modreader);
      s->instdisk = _mm_read_UBYTE (modreader);
      s->reserved = _mm_read_I_UWORD (modreader);
      s->length = _mm_read_I_UWORD (modreader);
      s->loopbeg = _mm_read_I_UWORD (modreader);
      s->loopend = _mm_read_I_UWORD (modreader);
      s->volume = _mm_read_UBYTE (modreader);
      s->reserved2 = _mm_read_UBYTE (modreader);
      s->c2spd = _mm_read_I_UWORD (modreader);
      s->reserved3 = _mm_read_I_ULONG (modreader);
      s->isa = _mm_read_I_UWORD (modreader);
    }
  _mm_read_UBYTES (mh->patorder, 128, modreader);

  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  /* set module variables */
  for (t = 0; t < STM_NTRACKERS; t++)
    if (!memcmp (mh->trackername, STM_Signatures[t], 8))
      break;
  of.modtype = strdup (STM_Version[t]);
  of.songname = DupStr (mh->songname, 20, 1);	/* make a cstr of songname */
  of.numpat = mh->numpat;
  of.inittempo = 125;		/* mh->inittempo+0x1c; */
  of.initspeed = mh->inittempo >> 4;
  of.numchn = 4;		/* get number of channels */
  of.reppos = 0;
  of.flags |= UF_S3MSLIDES;

  t = 0;
  if (!AllocPositions (0x80))
    return 0;
  /* 99 terminates the patorder list */
  while ((mh->patorder[t] != 99) && (mh->patorder[t] < mh->numpat))
    {
      of.positions[t] = mh->patorder[t];
      t++;
    }
  if (mh->patorder[t] != 99)
    t++;
  of.numpos = t;
  of.numtrk = of.numpat * of.numchn;
  of.numins = of.numsmp = 31;

  if (!AllocSamples ())
    return 0;
  if (!STM_LoadPatterns ())
    return 0;
  ourISA = _mm_ftell (modreader);
  ourISA = (ourISA + 15) & 0xfffffff0;	/* normalize */

  for (q = of.samples, t = 0; t < of.numsmp; t++, q++)
    {
      /* load sample info */
      q->samplename = DupStr (mh->sample[t].filename, 12, 1);
      q->speed = (mh->sample[t].c2spd * 8363L) / 8448;
      q->volume = mh->sample[t].volume;
      q->length = mh->sample[t].length;
      if ( /*(!mh->sample[t].volume)|| */ (q->length == 1))
	q->length = 0;
      q->loopstart = mh->sample[t].loopbeg;
      q->loopend = mh->sample[t].loopend;
      q->seekpos = ourISA;

      ourISA += q->length;
      ourISA = (ourISA + 15) & 0xfffffff0;	/* normalize */

      /* contrary to the STM specs, sample data is signed */
      q->flags = SF_SIGNED;

      /* fix for bad STMs */
      if (q->loopstart >= q->length)
	q->loopstart = q->loopend = 0;

      if ((q->loopend > 0) && (q->loopend != 0xffff) && (q->loopend!=q->loopstart))
	q->flags |= SF_LOOP;
      /* fix replen if repend>length */
      if (q->loopend > q->length)
	q->loopend = q->length;
    }
  return 1;
}

CHAR *
STM_LoadTitle (void)
{
  CHAR s[20];

  _mm_fseek (modreader, 0, SEEK_SET);
  if (!_mm_read_UBYTES (s, 20, modreader))
    return NULL;

  return (DupStr (s, 20, 1));
}

/*========== Loader information */

MLOADER load_stm =
{
  NULL,
  "STM",
  "STM (Scream Tracker)",
  STM_Init,
  STM_Test,
  STM_Load,
  STM_Cleanup,
  STM_LoadTitle
};


/* ex:set ts=4: */
