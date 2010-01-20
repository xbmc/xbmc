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

  $Id: load_med.c,v 1.27 1999/10/25 16:31:41 miod Exp $

  Amiga MED module loader

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "unimod_priv.h"

/*========== Module information */

typedef struct MEDHEADER
  {
    ULONG id;
    ULONG modlen;
    ULONG pMEDSONG;		/* struct MEDSONG *song; */
    UWORD psecnum;		/* for the player routine, MMD2 only */
    UWORD pseq;			/*  "   "   "   " */
    ULONG pMEDBLOCKP;		/* struct MMD0Block **blockarr; */
    ULONG reserved1;
    ULONG ppMedInstrHdr;		/* struct InstrHdr **smplarr; */
    ULONG reserved2;
    ULONG pMEDEXP;		/* struct MEDEXP *expdata; */
    ULONG reserved3;
    UWORD pstate;		/* some data for the player routine */
    UWORD pblock;
    UWORD pline;
    UWORD pseqnum;
    SWORD actplayline;
    UBYTE counter;
    UBYTE extra_songs;		/* number of songs - 1 */
  }
MEDHEADER;

typedef struct MEDSAMPLE
  {
    UWORD rep, replen;		/* offs: 0(s), 2(s) */
    UBYTE midich;		/* offs: 4(s) */
    UBYTE midipreset;		/* offs: 5(s) */
    UBYTE svol;			/* offs: 6(s) */
    SBYTE strans;		/* offs: 7(s) */
  }
MEDSAMPLE;

typedef struct MEDSONG
  {
    MEDSAMPLE sample[63];	/* 63 * 8 bytes = 504 bytes */
    UWORD numblocks;		/* offs: 504 */
    UWORD songlen;		/* offs: 506 */
    UBYTE playseq[256];		/* offs: 508 */
    UWORD deftempo;		/* offs: 764 */
    SBYTE playtransp;		/* offs: 766 */
    UBYTE flags;		/* offs: 767 */
    UBYTE flags2;		/* offs: 768 */
    UBYTE tempo2;		/* offs: 769 */
    UBYTE trkvol[16];		/* offs: 770 */
    UBYTE mastervol;		/* offs: 786 */
    UBYTE numsamples;		/* offs: 787 */
  }
MEDSONG;

typedef struct MEDEXP
  {
    ULONG nextmod;		/* pointer to next module */
    ULONG exp_smp;		/* pointer to InstrExt array */
    UWORD s_ext_entries;
    UWORD s_ext_entrsz;
    ULONG annotxt;		/* pointer to annotation text */
    ULONG annolen;
    ULONG iinfo;		/* pointer to InstrInfo array */
    UWORD i_ext_entries;
    UWORD i_ext_entrsz;
    ULONG jumpmask;
    ULONG rgbtable;
    ULONG channelsplit;
    ULONG n_info;
    ULONG songname;		/* pointer to songname */
    ULONG songnamelen;
    ULONG dumps;
    ULONG reserved2[7];
  }
MEDEXP;

typedef struct MMD0NOTE
  {
    UBYTE a, b, c;
  }
MMD0NOTE;

typedef struct MMD1NOTE
  {
    UBYTE a, b, c, d;
  }
MMD1NOTE;

typedef struct MEDINSTHEADER
  {
    ULONG length;
    SWORD type;
    /* Followed by actual data */
  }
MEDINSTHEADER;

typedef struct MEDINSTEXT
  {
    UBYTE hold;
    UBYTE decay;
    UBYTE suppress_midi_off;
    SBYTE finetune;
  }
MEDINSTEXT;

typedef struct MEDINSTINFO
  {
    UBYTE name[40];
  }
MEDINSTINFO;

/*========== Loader variables */

#define MMD0_string 0x4D4D4430
#define MMD1_string 0x4D4D4431

static MEDHEADER *mh = NULL;
static MEDSONG *ms = NULL;
static MEDEXP *me = NULL;
static ULONG *ba = NULL;
static MMD0NOTE *mmd0pat = NULL;
static MMD1NOTE *mmd1pat = NULL;

static BOOL decimalvolumes;
static BOOL bpmtempos;

#define d0note(row,col) mmd0pat[((row)*(UWORD)of.numchn)+(col)]
#define d1note(row,col) mmd1pat[((row)*(UWORD)of.numchn)+(col)]

static CHAR MED_Version[] = "OctaMED (MMDx)";

/*========== Loader code */

BOOL 
MED_Test (void)
{
  UBYTE id[4];

  if (!_mm_read_UBYTES (id, 4, modreader))
    return 0;
  if ((!memcmp (id, "MMD0", 4)) || (!memcmp (id, "MMD1", 4)))
    return 1;
  return 0;
}

BOOL 
MED_Init (void)
{
  if (!(me = (MEDEXP *) _mm_malloc (sizeof (MEDEXP))))
    return 0;
  if (!(mh = (MEDHEADER *) _mm_malloc (sizeof (MEDHEADER))))
    return 0;
  if (!(ms = (MEDSONG *) _mm_malloc (sizeof (MEDSONG))))
    return 0;
  return 1;
}

void 
MED_Cleanup (void)
{
  _mm_free (me);
  _mm_free (mh);
  _mm_free (ms);
  _mm_free (ba);
  _mm_free (mmd0pat);
  _mm_free (mmd1pat);
}

static void 
EffectCvt (UBYTE eff, UBYTE dat)
{
  switch (eff)
    {
      /* 0x0 0x1 0x2 0x3 0x4 PT effects */
    case 0x5:			/* PT vibrato with speed/depth nibbles swapped */
      UniPTEffect (0x4, (dat >> 4) | ((dat & 0xf) << 4));
      break;
      /* 0x6 0x7 not used */
    case 0x6:
    case 0x7:
      break;
    case 0x8:			/* midi hold/decay */
      break;
    case 0x9:
      if (bpmtempos)
	{
	  if (!dat)
	    dat = of.initspeed;
	  UniEffect (UNI_S3MEFFECTA, dat);
	}
      else
	{
	  if (dat <= 0x20)
	    {
	      if (!dat)
		dat = of.initspeed;
	      else
		dat /= 4;
	      UniPTEffect (0xf, dat);
	    }
	  else
	    UniEffect (UNI_MEDSPEED, ((UWORD) dat * 125) / (33 * 4));
	}
      break;
      /* 0xa 0xb PT effects */
    case 0xc:
      if (decimalvolumes)
	dat = (dat >> 4) * 10 + (dat & 0xf);
      UniPTEffect (0xc, dat);
      break;
    case 0xd:			/* same as PT volslide */
      UniPTEffect (0xa, dat);
      break;
    case 0xe:			/* synth jmp - midi */
      break;
    case 0xf:
      switch (dat)
	{
	case 0:		/* patternbreak */
	  UniPTEffect (0xd, 0);
	  break;
	case 0xf1:		/* play note twice */
	  UniWriteByte (UNI_MEDEFFECTF1);
	  break;
	case 0xf2:		/* delay note */
	  UniWriteByte (UNI_MEDEFFECTF2);
	  break;
	case 0xf3:		/* play note three times */
	  UniWriteByte (UNI_MEDEFFECTF3);
	  break;
	case 0xfe:		/* stop playing */
	  UniPTEffect (0xb, of.numpat);
	  break;
	case 0xff:		/* note cut */
	  UniPTEffect (0xc, 0);
	  break;
	default:
	  if (dat <= 10)
	    UniPTEffect (0xf, dat);
	  else if (dat <= 240)
	    {
	      if (bpmtempos)
		UniPTEffect (0xf, (dat < 32) ? 32 : dat);
	      else
		UniEffect (UNI_MEDSPEED, ((UWORD) dat * 125) / 33);
	    }
	}
      break;
    default:			/* all normal PT effects are handled here */
      UniPTEffect (eff, dat);
      break;
    }
}

static UBYTE *
MED_Convert1 (int count, int col)
{
  int t;
  UBYTE inst, note, eff, dat;
  MMD1NOTE *n;

  UniReset ();
  for (t = 0; t < count; t++)
    {
      n = &d1note (t, col);

      note = n->a & 0x7f;
      inst = n->b & 0x3f;
      eff = n->c & 0xf;
      dat = n->d;

      if (inst)
	UniInstrument (inst - 1);
      if (note)
	UniNote (note + 3 * OCTAVE - 1);
      EffectCvt (eff, dat);
      UniNewline ();
    }
  return UniDup ();
}

static UBYTE *
MED_Convert0 (int count, int col)
{
  int t;
  UBYTE a, b, inst, note, eff, dat;
  MMD0NOTE *n;

  UniReset ();
  for (t = 0; t < count; t++)
    {
      n = &d0note (t, col);
      a = n->a;
      b = n->b;

      note = a & 0x3f;
      a >>= 6;
      a = ((a & 1) << 1) | (a >> 1);
      inst = (b >> 4) | (a << 4);
      eff = b & 0xf;
      dat = n->c;

      if (inst)
	UniInstrument (inst - 1);
      if (note)
	UniNote (note + 3 * OCTAVE - 1);
      EffectCvt (eff, dat);
      UniNewline ();
    }
  return UniDup ();
}

static BOOL 
LoadMMD0Patterns (void)
{
  int t, row, col;
  UWORD numtracks, numlines, maxlines = 0, track = 0;
  MMD0NOTE *mmdp;

  /* first, scan patterns to see how many channels are used */
  for (t = 0; t < of.numpat; t++)
    {
      _mm_fseek (modreader, ba[t], SEEK_SET);
      numtracks = _mm_read_UBYTE (modreader);
      numlines = _mm_read_UBYTE (modreader);

      if (numtracks > of.numchn)
	of.numchn = numtracks;
      if (numlines > maxlines)
	maxlines = numlines;
    }

  of.numtrk = of.numpat * of.numchn;
  if (!AllocTracks ())
    return 0;
  if (!AllocPatterns ())
    return 0;

  if (!(mmd0pat = (MMD0NOTE *) _mm_calloc (of.numchn * (maxlines + 1), sizeof (MMD0NOTE))))
    return 0;

  /* second read: read and convert patterns */
  for (t = 0; t < of.numpat; t++)
    {
      _mm_fseek (modreader, ba[t], SEEK_SET);
      numtracks = _mm_read_UBYTE (modreader);
      numlines = _mm_read_UBYTE (modreader);

      of.pattrows[t] = ++numlines;
      memset (mmdp = mmd0pat, 0, of.numchn * maxlines * sizeof (MMD0NOTE));
      for (row = numlines; row; row--)
	{
	  for (col = numtracks; col; col--, mmdp++)
	    {
	      mmdp->a = _mm_read_UBYTE (modreader);
	      mmdp->b = _mm_read_UBYTE (modreader);
	      mmdp->c = _mm_read_UBYTE (modreader);
	    }
	}

      for (col = 0; col < of.numchn; col++)
	of.tracks[track++] = MED_Convert0 (numlines, col);
    }
  return 1;
}

static BOOL 
LoadMMD1Patterns (void)
{
  int t, row, col;
  UWORD numtracks, numlines, maxlines = 0, track = 0;
  MMD1NOTE *mmdp;

  /* first, scan patterns to see how many channels are used */
  for (t = 0; t < of.numpat; t++)
    {
      _mm_fseek (modreader, ba[t], SEEK_SET);
      numtracks = _mm_read_M_UWORD (modreader);
      numlines = _mm_read_M_UWORD (modreader);
      if (numtracks > of.numchn)
	of.numchn = numtracks;
      if (numlines > maxlines)
	maxlines = numlines;
    }

  of.numtrk = of.numpat * of.numchn;
  if (!AllocTracks ())
    return 0;
  if (!AllocPatterns ())
    return 0;

  if (!(mmd1pat = (MMD1NOTE *) _mm_calloc (of.numchn * (maxlines + 1), sizeof (MMD1NOTE))))
    return 0;

  /* second read: really read and convert patterns */
  for (t = 0; t < of.numpat; t++)
    {
      _mm_fseek (modreader, ba[t], SEEK_SET);
      numtracks = _mm_read_M_UWORD (modreader);
      numlines = _mm_read_M_UWORD (modreader);

      _mm_fseek (modreader, sizeof (ULONG), SEEK_CUR);
      of.pattrows[t] = ++numlines;
      memset (mmdp = mmd1pat, 0, of.numchn * maxlines * sizeof (MMD1NOTE));

      for (row = numlines; row; row--)
	{
	  for (col = numtracks; col; col--, mmdp++)
	    {
	      mmdp->a = _mm_read_UBYTE (modreader);
	      mmdp->b = _mm_read_UBYTE (modreader);
	      mmdp->c = _mm_read_UBYTE (modreader);
	      mmdp->d = _mm_read_UBYTE (modreader);
	    }
	}

      for (col = 0; col < of.numchn; col++)
	of.tracks[track++] = MED_Convert1 (numlines, col);
    }
  return 1;
}

BOOL 
MED_Load (BOOL curious)
{
  int t;
  ULONG sa[64];
  MEDINSTHEADER s;
  SAMPLE *q;
  MEDSAMPLE *mss;

  /* try to read module header */
  mh->id = _mm_read_M_ULONG (modreader);
  mh->modlen = _mm_read_M_ULONG (modreader);
  mh->pMEDSONG = _mm_read_M_ULONG (modreader);
  mh->psecnum = _mm_read_M_UWORD (modreader);
  mh->pseq = _mm_read_M_UWORD (modreader);
  mh->pMEDBLOCKP = _mm_read_M_ULONG (modreader);
  mh->reserved1 = _mm_read_M_ULONG (modreader);
  mh->ppMedInstrHdr = _mm_read_M_ULONG (modreader);
  mh->reserved2 = _mm_read_M_ULONG (modreader);
  mh->pMEDEXP = _mm_read_M_ULONG (modreader);
  mh->reserved3 = _mm_read_M_ULONG (modreader);
  mh->pstate = _mm_read_M_UWORD (modreader);
  mh->pblock = _mm_read_M_UWORD (modreader);
  mh->pline = _mm_read_M_UWORD (modreader);
  mh->pseqnum = _mm_read_M_UWORD (modreader);
  mh->actplayline = _mm_read_M_SWORD (modreader);
  mh->counter = _mm_read_UBYTE (modreader);
  mh->extra_songs = _mm_read_UBYTE (modreader);

  /* Seek to MEDSONG struct */
  _mm_fseek (modreader, mh->pMEDSONG, SEEK_SET);

  /* Load the MMD0 Song Header */
  mss = ms->sample;		/* load the sample data first */
  for (t = 63; t; t--, mss++)
    {
      mss->rep = _mm_read_M_UWORD (modreader);
      mss->replen = _mm_read_M_UWORD (modreader);
      mss->midich = _mm_read_UBYTE (modreader);
      mss->midipreset = _mm_read_UBYTE (modreader);
      mss->svol = _mm_read_UBYTE (modreader);
      mss->strans = _mm_read_SBYTE (modreader);
    }

  ms->numblocks = _mm_read_M_UWORD (modreader);
  ms->songlen = _mm_read_M_UWORD (modreader);
  _mm_read_UBYTES (ms->playseq, 256, modreader);
  ms->deftempo = _mm_read_M_UWORD (modreader);
  ms->playtransp = _mm_read_SBYTE (modreader);
  ms->flags = _mm_read_UBYTE (modreader);
  ms->flags2 = _mm_read_UBYTE (modreader);
  ms->tempo2 = _mm_read_UBYTE (modreader);
  _mm_read_UBYTES (ms->trkvol, 16, modreader);
  ms->mastervol = _mm_read_UBYTE (modreader);
  ms->numsamples = _mm_read_UBYTE (modreader);

  /* check for a bad header */
  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  /* load extension structure */
  if (mh->pMEDEXP)
    {
      _mm_fseek (modreader, mh->pMEDEXP, SEEK_SET);
      me->nextmod = _mm_read_M_ULONG (modreader);
      me->exp_smp = _mm_read_M_ULONG (modreader);
      me->s_ext_entries = _mm_read_M_UWORD (modreader);
      me->s_ext_entrsz = _mm_read_M_UWORD (modreader);
      me->annotxt = _mm_read_M_ULONG (modreader);
      me->annolen = _mm_read_M_ULONG (modreader);
      me->iinfo = _mm_read_M_ULONG (modreader);
      me->i_ext_entries = _mm_read_M_UWORD (modreader);
      me->i_ext_entrsz = _mm_read_M_UWORD (modreader);
      me->jumpmask = _mm_read_M_ULONG (modreader);
      me->rgbtable = _mm_read_M_ULONG (modreader);
      me->channelsplit = _mm_read_M_ULONG (modreader);
      me->n_info = _mm_read_M_ULONG (modreader);
      me->songname = _mm_read_M_ULONG (modreader);
      me->songnamelen = _mm_read_M_ULONG (modreader);
      me->dumps = _mm_read_M_ULONG (modreader);
    }

  /* seek to and read the samplepointer array */
  _mm_fseek (modreader, mh->ppMedInstrHdr, SEEK_SET);
  if (!_mm_read_M_ULONGS (sa, ms->numsamples, modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  /* alloc and read the blockpointer array */
  if (!(ba = (ULONG *) _mm_calloc (ms->numblocks, sizeof (ULONG))))
    return 0;
  _mm_fseek (modreader, mh->pMEDBLOCKP, SEEK_SET);
  if (!_mm_read_M_ULONGS (ba, ms->numblocks, modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  /* copy song positions */
  if (!AllocPositions (ms->songlen))
    return 0;
  for (t = 0; t < ms->songlen; t++)
    of.positions[t] = ms->playseq[t];

  decimalvolumes = (ms->flags & 0x10) ? 0 : 1;
  bpmtempos = (ms->flags2 & 0x20) ? 1 : 0;

  if (bpmtempos)
    {
      int bpmlen = (ms->flags2 & 0x1f) + 1;
      of.initspeed = ms->tempo2;
      of.inittempo = ms->deftempo * bpmlen / 4;

      if (bpmlen != 4)
	{
	  /* Let's do some math : compute GCD of BPM beat length and speed */
	  int a, b;

	  a = bpmlen;
	  b = ms->tempo2;

	  if (a > b)
	    {
	      t = b;
	      b = a;
	      a = t;
	    }
	  while ((a != b) && (a))
	    {
	      t = a;
	      a = b - a;
	      b = t;
	      if (a > b)
		{
		  t = b;
		  b = a;
		  a = t;
		}
	    }

	  of.initspeed /= b;
	  of.inittempo = ms->deftempo * bpmlen / (4 * b);
	}
    }
  else
    {
      of.initspeed = ms->tempo2;
      of.inittempo = ms->deftempo ? ((UWORD) ms->deftempo * 125) / 33 : 128;
      if ((ms->deftempo <= 10) && (ms->deftempo))
	of.inittempo = (of.inittempo * 33) / 6;
      of.flags |= UF_HIGHBPM;
    }
  MED_Version[12] = mh->id;
  of.modtype = strdup (MED_Version);
  of.numchn = 0;		/* will be counted later */
  of.numpat = ms->numblocks;
  of.numpos = ms->songlen;
  of.numins = ms->numsamples;
  of.numsmp = of.numins;
  of.reppos = 0;
  if ((mh->pMEDEXP) && (me->songname) && (me->songnamelen))
    {
      char *name;

      _mm_fseek (modreader, me->songname, SEEK_SET);
      name = _mm_malloc (me->songnamelen);
      _mm_read_UBYTES (name, me->songnamelen, modreader);
      of.songname = DupStr (name, me->songnamelen, 1);
      free (name);
    }
  else
    of.songname = DupStr (NULL, 0, 0);
  if ((mh->pMEDEXP) && (me->annotxt) && (me->annolen))
    {
      _mm_fseek (modreader, me->annotxt, SEEK_SET);
      ReadComment (me->annolen);
    }

  if (!AllocSamples ())
    return 0;
  q = of.samples;
  for (t = 0; t < of.numins; t++)
    {
      q->flags = SF_SIGNED;
      q->volume = 64;
      if (sa[t])
	{
	  _mm_fseek (modreader, sa[t], SEEK_SET);
	  s.length = _mm_read_M_ULONG (modreader);
	  s.type = _mm_read_M_SWORD (modreader);

	  if (s.type)
	    {
#ifdef MIKMOD_DEBUG
	      fputs ("\rNon-sample instruments not supported in MED loader yet\n", stderr);
#endif
	      if (!curious)
		{
		  _mm_errno = MMERR_MED_SYNTHSAMPLES;
		  return 0;
		}
	      s.length = 0;
	    }

	  if (_mm_eof (modreader))
	    {
	      _mm_errno = MMERR_LOADING_SAMPLEINFO;
	      return 0;
	    }

	  q->length = s.length;
	  q->seekpos = _mm_ftell (modreader);
	  q->loopstart = ms->sample[t].rep << 1;
	  q->loopend = q->loopstart + (ms->sample[t].replen << 1);

	  if (ms->sample[t].replen > 1)
	    q->flags |= SF_LOOP;

	  /* don't load sample if length>='MMD0'...
	     such kluges make libmikmod's code unique !!! */
	  if (q->length >= MMD0_string)
	    q->length = 0;
	}
      else
	q->length = 0;

      if ((mh->pMEDEXP) && (me->exp_smp) &&
	  (t < me->s_ext_entries) && (me->s_ext_entrsz >= 4))
	{
	  MEDINSTEXT ie;

	  _mm_fseek (modreader, me->exp_smp + t * me->s_ext_entrsz, SEEK_SET);
	  ie.hold = _mm_read_UBYTE (modreader);
	  ie.decay = _mm_read_UBYTE (modreader);
	  ie.suppress_midi_off = _mm_read_UBYTE (modreader);
	  ie.finetune = _mm_read_SBYTE (modreader);

	  q->speed = finetune[ie.finetune & 0xf];
	}
      else
	q->speed = 8363;

      if ((mh->pMEDEXP) && (me->iinfo) &&
	  (t < me->i_ext_entries) && (me->i_ext_entrsz >= 40))
	{
	  MEDINSTINFO ii;

	  _mm_fseek (modreader, me->iinfo + t * me->i_ext_entrsz, SEEK_SET);
	  _mm_read_UBYTES (ii.name, 40, modreader);
	  q->samplename = DupStr ((CHAR *) ii.name, 40, 1);
	}
      else
	q->samplename = NULL;

      q++;
    }

  if (mh->id == MMD0_string)
    {
      if (!LoadMMD0Patterns ())
	{
	  _mm_errno = MMERR_LOADING_PATTERN;
	  return 0;
	}
    }
  else if (mh->id == MMD1_string)
    {
      if (!LoadMMD1Patterns ())
	{
	  _mm_errno = MMERR_LOADING_PATTERN;
	  return 0;
	}
    }
  else
    {
      _mm_errno = MMERR_NOT_A_MODULE;
      return 0;
    }
  return 1;
}

/*========== Loader information */

MLOADER load_med =
{
  NULL,
  "MED",
  "MED (OctaMED)",
  MED_Init,
  MED_Test,
  MED_Load,
  MED_Cleanup,
  NULL
};

/* ex:set ts=4: */
