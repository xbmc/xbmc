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

  $Id: load_imf.c,v 1.12 1999/10/25 16:31:41 miod Exp $

  Imago Orpheus (IMF) module loader

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "unimod_priv.h"

/*========== Module structure */

/* module header */
typedef struct IMFHEADER
  {
    CHAR songname[32];
    UWORD ordnum;
    UWORD patnum;
    UWORD insnum;
    UWORD flags;
    UBYTE initspeed;
    UBYTE inittempo;
    UBYTE mastervol;
    UBYTE mastermult;
    UBYTE orders[256];
  }
IMFHEADER;

/* channel settings */
typedef struct IMFCHANNEL
  {
    CHAR name[12];
    UBYTE chorus;
    UBYTE reverb;
    UBYTE pan;
    UBYTE status;
  }
IMFCHANNEL;

/* instrument header */
#define IMFNOTECNT (10*OCTAVE)
#define IMFENVCNT (16*2)
typedef struct IMFINSTHEADER
  {
    CHAR name[32];
    UBYTE what[IMFNOTECNT];
    UWORD volenv[IMFENVCNT];
    UWORD panenv[IMFENVCNT];
    UWORD pitenv[IMFENVCNT];
    UBYTE volpts;
    UBYTE volsus;
    UBYTE volbeg;
    UBYTE volend;
    UBYTE volflg;
    UBYTE panpts;
    UBYTE pansus;
    UBYTE panbeg;
    UBYTE panend;
    UBYTE panflg;
    UBYTE pitpts;
    UBYTE pitsus;
    UBYTE pitbeg;
    UBYTE pitend;
    UBYTE pitflg;
    UWORD volfade;
    UWORD numsmp;
    ULONG signature;
  }
IMFINSTHEADER;

/* sample header */
typedef struct IMFWAVHEADER
  {
    CHAR samplename[13];
    ULONG length;
    ULONG loopstart;
    ULONG loopend;
    ULONG samplerate;
    UBYTE volume;
    UBYTE pan;
    UBYTE flags;
  }
IMFWAVHEADER;

typedef struct IMFNOTE
  {
    UBYTE note, ins, eff1, dat1, eff2, dat2;
  }
IMFNOTE;

/*========== Loader variables */

static CHAR IMF_Version[] = "Imago Orpheus";

static IMFNOTE *imfpat = NULL;
static IMFHEADER *mh = NULL;

/*========== Loader code */

BOOL 
IMF_Test (void)
{
  UBYTE id[4];

  _mm_fseek (modreader, 0x3c, SEEK_SET);
  if (!_mm_read_UBYTES (id, 4, modreader))
    return 0;
  if (!memcmp (id, "IM10", 4))
    return 1;
  return 0;
}

BOOL 
IMF_Init (void)
{
  if (!(imfpat = (IMFNOTE *) _mm_malloc (32 * 256 * sizeof (IMFNOTE))))
    return 0;
  if (!(mh = (IMFHEADER *) _mm_malloc (sizeof (IMFHEADER))))
    return 0;

  return 1;
}

void 
IMF_Cleanup (void)
{
  FreeLinear ();

  _mm_free (imfpat);
  _mm_free (mh);
}

static BOOL 
IMF_ReadPattern (SLONG size, UWORD rows)
{
  int row = 0, flag, ch;
  IMFNOTE *n, dummy;

  /* clear pattern data */
  memset (imfpat, 255, 32 * 256 * sizeof (IMFNOTE));

  while ((size > 0) && (row < rows))
    {
      flag = _mm_read_UBYTE (modreader);
      size--;

      if (_mm_eof (modreader))
	{
	  _mm_errno = MMERR_LOADING_PATTERN;
	  return 0;
	}

      if (flag)
	{
	  ch = remap[flag & 31];

	  if (ch != -1)
	    n = &imfpat[256 * ch + row];
	  else
	    n = &dummy;

	  if (flag & 32)
	    {
	      n->note = _mm_read_UBYTE (modreader);
	      if (n->note >= 0xa0)
		n->note = 0xa0;	/* note off */
	      n->ins = _mm_read_UBYTE (modreader);
	      size -= 2;
	    }
	  if (flag & 64)
	    {
	      size -= 2;
	      n->eff2 = _mm_read_UBYTE (modreader);
	      n->dat2 = _mm_read_UBYTE (modreader);
	    }
	  if (flag & 128)
	    {
	      n->eff1 = _mm_read_UBYTE (modreader);
	      n->dat1 = _mm_read_UBYTE (modreader);
	      size -= 2;
	    }
	}
      else
	row++;
    }
  if ((size) || (row != rows))
    {
      _mm_errno = MMERR_LOADING_PATTERN;
      return 0;
    }
  return 1;
}

static void 
IMF_ProcessCmd (UBYTE eff, UBYTE inf)
{
  if ((eff) && (eff != 255))
    switch (eff)
      {
      case 0x01:		/* set tempo */
	UniEffect (UNI_S3MEFFECTA, inf);
	break;
      case 0x02:		/* set BPM */
	if (inf >= 0x20)
	  UniEffect (UNI_S3MEFFECTT, inf);
	break;
      case 0x03:		/* tone portamento */
	UniEffect (UNI_ITEFFECTG, inf);
	break;
      case 0x04:		/* porta + volslide */
	UniEffect (UNI_ITEFFECTG, inf);
	UniEffect (UNI_S3MEFFECTD, 0);
	break;
      case 0x05:		/* vibrato */
	UniPTEffect (0x4, inf);
	break;
      case 0x06:		/* vibrato + volslide */
	UniPTEffect (0x4, inf);
	UniEffect (UNI_S3MEFFECTD, 0);
	break;
      case 0x07:		/* fine vibrato */
	UniEffect (UNI_ITEFFECTU, inf);
	break;
      case 0x08:		/* tremolo */
	UniEffect (UNI_S3MEFFECTR, inf);
	break;
      case 0x09:		/* arpeggio */
	UniPTEffect (0x0, inf);
	break;
      case 0x0a:		/* panning */
	UniPTEffect (0x8, (inf >= 128) ? 255 : (inf << 1));
	break;
      case 0x0b:		/* pan slide */
	UniEffect (UNI_XMEFFECTP, inf);
	break;
      case 0x0c:		/* set channel volume */
	if (inf <= 64)
	  UniPTEffect (0xc, inf);
	break;
      case 0x0d:		/* volume slide */
	UniEffect (UNI_S3MEFFECTD, inf);
	break;
      case 0x0e:		/* fine volume slide */
	if (inf)
	  {
	    if (inf >> 4)
	      UniEffect (UNI_S3MEFFECTD, 0x0f | inf);
	    else
	      UniEffect (UNI_S3MEFFECTD, 0xf0 | inf);
	  }
	else
	  UniEffect (UNI_S3MEFFECTD, 0);
	break;
      case 0x0f:		/* set finetune */
	UniPTEffect (0xe, 0x50 | (inf >> 4));
	break;
#ifdef MIKMOD_DEBUG
      case 0x10:		/* note slide up */
      case 0x11:		/* not slide down */
	fprintf (stderr, "\rIMF effect 0x10/0x11 (note slide)"
		 " not implemented (eff=%2X inf=%2X)\n", eff, inf);
	break;
#endif
      case 0x12:		/* slide up */
	UniEffect (UNI_S3MEFFECTF, inf);
	break;
      case 0x13:		/* slide down */
	UniEffect (UNI_S3MEFFECTE, inf);
	break;
      case 0x14:		/* fine slide up */
	if (inf)
	  {
	    if (inf < 0x40)
	      UniEffect (UNI_S3MEFFECTF, 0xe0 | (inf / 4));
	    else
	      UniEffect (UNI_S3MEFFECTF, 0xf0 | (inf >> 4));
	  }
	else
	  UniEffect (UNI_S3MEFFECTF, 0);
	break;
      case 0x15:		/* fine slide down */
	if (inf)
	  {
	    if (inf < 0x40)
	      UniEffect (UNI_S3MEFFECTE, 0xe0 | (inf / 4));
	    else
	      UniEffect (UNI_S3MEFFECTE, 0xf0 | (inf >> 4));
	  }
	else
	  UniEffect (UNI_S3MEFFECTE, 0);
	break;
	/* 0x16 set filter cutoff (awe32) */
	/* 0x17 filter side + resonance (awe32) */
      case 0x18:		/* sample offset */
	UniPTEffect (0x9, inf);
	break;
#ifdef MIKMOD_DEBUG
      case 0x19:		/* set fine sample offset */
	fprintf (stderr, "\rIMF effect 0x19 (fine sample offset)"
		 " not implemented (inf=%2X)\n", inf);
	break;
#endif
      case 0x1a:		/* keyoff */
	UniWriteByte (UNI_KEYOFF);
	break;
      case 0x1b:		/* retrig */
	UniEffect (UNI_S3MEFFECTQ, inf);
	break;
      case 0x1c:		/* tremor */
	UniEffect (UNI_S3MEFFECTI, inf);
	break;
      case 0x1d:		/* position jump */
	UniPTEffect (0xb, inf);
	break;
      case 0x1e:		/* pattern break */
	UniPTEffect (0xd, (inf >> 4) * 10 + (inf & 0xf));
	break;
      case 0x1f:		/* set master volume */
	if (inf <= 64)
	  UniEffect (UNI_XMEFFECTG, inf);
	break;
      case 0x20:		/* master volume slide */
	UniEffect (UNI_XMEFFECTH, inf);
	break;
      case 0x21:		/* extended effects */
	switch (inf >> 4)
	  {
	  case 0x1:		/* set filter */
	  case 0x5:		/* vibrato waveform */
	  case 0x8:		/* tremolo waveform */
	    UniPTEffect (0xe, inf - 0x10);
	    break;
	  case 0xa:		/* pattern loop */
	    UniPTEffect (0xe, 0x60 | (inf & 0xf));
	    break;
	  case 0xb:		/* pattern delay */
	    UniPTEffect (0xe, 0xe0 | (inf & 0xf));
	    break;
	  case 0x3:		/* glissando */
	  case 0xc:		/* note cut */
	  case 0xd:		/* note delay */
	  case 0xf:		/* invert loop */
	    UniPTEffect (0xe, inf);
	    break;
	  case 0xe:		/* ignore envelope */
	    UniEffect (UNI_ITEFFECTS0, 0x77);	/* vol */
	    UniEffect (UNI_ITEFFECTS0, 0x79);	/* pan */
	    UniEffect (UNI_ITEFFECTS0, 0x7b);	/* pit */
	    break;
	  }
	break;
	/* 0x22 chorus (awe32) */
	/* 0x23 reverb (awe32) */
      }
}

static UBYTE *
IMF_ConvertTrack (IMFNOTE * tr, UWORD rows)
{
  int t;
  UBYTE note, ins;

  UniReset ();
  for (t = 0; t < rows; t++)
    {
      note = tr[t].note;
      ins = tr[t].ins;

      if ((ins) && (ins != 255))
	UniInstrument (ins - 1);
      if (note != 255)
	{
	  if (note == 0xa0)
	    {
	      UniPTEffect (0xc, 0);	/* Note cut */
	      if (tr[t].eff1 == 0x0c)
		tr[t].eff1 = 0;
	      if (tr[t].eff2 == 0x0c)
		tr[t].eff2 = 0;
	    }
	  else
	    UniNote (((note >> 4) * OCTAVE) + (note & 0xf));
	}

      IMF_ProcessCmd (tr[t].eff1, tr[t].dat1);
      IMF_ProcessCmd (tr[t].eff2, tr[t].dat2);
      UniNewline ();
    }
  return UniDup ();
}

BOOL 
IMF_Load (BOOL curious)
{
#define IMF_SMPINCR 64
  int t, u, track = 0;
  IMFCHANNEL channels[32];
  INSTRUMENT *d;
  SAMPLE *q;
  IMFWAVHEADER *wh = NULL, *s = NULL;
  ULONG *nextwav = NULL;
  UWORD wavcnt = 0;
  UBYTE id[4];

  /* try to read the module header */
  _mm_read_string (mh->songname, 32, modreader);
  mh->ordnum = _mm_read_I_UWORD (modreader);
  mh->patnum = _mm_read_I_UWORD (modreader);
  mh->insnum = _mm_read_I_UWORD (modreader);
  mh->flags = _mm_read_I_UWORD (modreader);
  _mm_fseek (modreader, 8, SEEK_CUR);
  mh->initspeed = _mm_read_UBYTE (modreader);
  mh->inittempo = _mm_read_UBYTE (modreader);
  mh->mastervol = _mm_read_UBYTE (modreader);
  mh->mastermult = _mm_read_UBYTE (modreader);
  _mm_fseek (modreader, 64, SEEK_SET);

  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  /* set module variables */
  of.songname = DupStr (mh->songname, 31, 1);
  of.modtype = strdup (IMF_Version);
  of.numpat = mh->patnum;
  of.numins = mh->insnum;
  of.reppos = 0;
  of.initspeed = mh->initspeed;
  of.inittempo = mh->inittempo;
  of.initvolume = mh->mastervol << 1;
  of.flags |= UF_INST;
  if (mh->flags & 1)
    of.flags |= UF_LINEAR;

  /* read channel information */
  of.numchn = 0;
  memset (remap, -1, 32 * sizeof (UBYTE));
  for (t = 0; t < 32; t++)
    {
      _mm_read_string (channels[t].name, 12, modreader);
      channels[t].chorus = _mm_read_UBYTE (modreader);
      channels[t].reverb = _mm_read_UBYTE (modreader);
      channels[t].pan = _mm_read_UBYTE (modreader);
      channels[t].status = _mm_read_UBYTE (modreader);
    }
  /* bug in Imago Orpheus ? If only channel 1 is enabled, in fact we have to
     enable 16 channels */
  if (!channels[0].status)
    {
      for (t = 1; t < 16; t++)
	if (channels[t].status != 1)
	  break;
      if (t == 16)
	for (t = 1; t < 16; t++)
	  channels[t].status = 0;
    }
  for (t = 0; t < 32; t++)
    {
      if (channels[t].status != 2)
	remap[t] = of.numchn++;
      else
	remap[t] = -1;
    }
  for (t = 0; t < 32; t++)
    if (remap[t] != -1)
      {
	of.panning[remap[t]] = channels[t].pan;
	of.chanvol[remap[t]] = channels[t].status ? 0 : 64;
      }

  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  /* read order list */
  _mm_read_UBYTES (mh->orders, 256, modreader);
  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  of.numpos = 0;
  for (t = 0; t < mh->ordnum; t++)
    if (mh->orders[t] != 0xff)
      of.numpos++;
  if (!AllocPositions (of.numpos))
    return 0;
  for (t = u = 0; t < mh->ordnum; t++)
    if (mh->orders[t] != 0xff)
      of.positions[u++] = mh->orders[t];

  /* load pattern info */
  of.numtrk = of.numpat * of.numchn;
  if (!AllocTracks ())
    return 0;
  if (!AllocPatterns ())
    return 0;

  for (t = 0; t < of.numpat; t++)
    {
      SLONG size;
      UWORD rows;

      size = (SLONG) _mm_read_I_UWORD (modreader);
      rows = _mm_read_I_UWORD (modreader);
      if ((rows > 256) || (size < 4))
	{
	  _mm_errno = MMERR_LOADING_PATTERN;
	  return 0;
	}

      of.pattrows[t] = rows;
      if (!IMF_ReadPattern (size - 4, rows))
	return 0;
      for (u = 0; u < of.numchn; u++)
	if (!(of.tracks[track++] = IMF_ConvertTrack (&imfpat[u * 256], rows)))
	  return 0;
    }

  /* load instruments */
  if (!AllocInstruments ())
    return 0;
  d = of.instruments;

  for (t = 0; t < of.numins; t++)
    {
      IMFINSTHEADER ih;

      memset (d->samplenumber, 0xff, INSTNOTES * sizeof (UWORD));

      /* read instrument header */
      _mm_read_string (ih.name, 32, modreader);
      d->insname = DupStr (ih.name, 31, 1);
      _mm_read_UBYTES (ih.what, IMFNOTECNT, modreader);
      _mm_fseek (modreader, 8, SEEK_CUR);
      _mm_read_I_UWORDS (ih.volenv, IMFENVCNT, modreader);
      _mm_read_I_UWORDS (ih.panenv, IMFENVCNT, modreader);
      _mm_read_I_UWORDS (ih.pitenv, IMFENVCNT, modreader);

#define IMF_FinishLoadingEnvelope(name)					\
		ih.name##pts=_mm_read_UBYTE(modreader);		\
		ih.name##sus=_mm_read_UBYTE(modreader);		\
		ih.name##beg=_mm_read_UBYTE(modreader);		\
		ih.name##end=_mm_read_UBYTE(modreader);		\
		ih.name##flg=_mm_read_UBYTE(modreader);		\
		_mm_read_UBYTE(modreader);						\
		_mm_read_UBYTE(modreader);						\
		_mm_read_UBYTE(modreader);

      IMF_FinishLoadingEnvelope (vol);
      IMF_FinishLoadingEnvelope (pan);
      IMF_FinishLoadingEnvelope (pit);

      ih.volfade = _mm_read_I_UWORD (modreader);
      ih.numsmp = _mm_read_I_UWORD (modreader);

      _mm_read_UBYTES (id, 4, modreader);
      if (memcmp (id, "II10", 4))
	{
	  if (nextwav)
	    free (nextwav);
	  if (wh)
	    free (wh);
	  _mm_errno = MMERR_LOADING_SAMPLEINFO;
	  return 0;
	}
      if ((ih.numsmp > 16) || (ih.volpts > IMFENVCNT / 2) || (ih.panpts > IMFENVCNT / 2) ||
	  (ih.pitpts > IMFENVCNT / 2) || (_mm_eof (modreader)))
	{
	  if (nextwav)
	    free (nextwav);
	  if (wh)
	    free (wh);
	  _mm_errno = MMERR_LOADING_SAMPLEINFO;
	  return 0;
	}

      for (u = 0; u < IMFNOTECNT; u++)
	d->samplenumber[u] = ih.what[u] > ih.numsmp ? 0xffff : ih.what[u] + of.numsmp;
      d->volfade = ih.volfade;

#define IMF_ProcessEnvelope(name) 										\
		for (u = 0; u < (IMFENVCNT >> 1); u++) {						\
			d->name##env[u].pos = ih.name##env[u << 1];				\
			d->name##env[u].val = ih.name##env[(u << 1)+ 1];		\
		}																\
		if (ih.name##flg&1) d->name##flg|=EF_ON;					\
		if (ih.name##flg&2) d->name##flg|=EF_SUSTAIN;				\
		if (ih.name##flg&4) d->name##flg|=EF_LOOP;					\
		d->name##susbeg=d->name##susend=ih.name##sus;				\
		d->name##beg=ih.name##beg;									\
		d->name##end=ih.name##end;									\
		d->name##pts=ih.name##pts;									\
																		\
		if ((d->name##flg&EF_ON)&&(d->name##pts<2))					\
			d->name##flg&=~EF_ON;

      IMF_ProcessEnvelope (vol);
      IMF_ProcessEnvelope (pan);
      IMF_ProcessEnvelope (pit);
#undef IMF_ProcessEnvelope

      if (ih.pitflg & 1)
	{
	  d->pitflg &= ~EF_ON;
#ifdef MIKMOD_DEBUG
	  fputs ("\rFilter envelopes not supported yet\n", stderr);
#endif
	}

      /* gather sample information */
      for (u = 0; u < ih.numsmp; u++, s++)
	{
	  /* allocate more room for sample information if necessary */
	  if (of.numsmp + u == wavcnt)
	    {
	      wavcnt += IMF_SMPINCR;
	      if (!(nextwav = realloc (nextwav, wavcnt * sizeof (ULONG))))
		{
		  if (wh)
		    free (wh);
		  _mm_errno = MMERR_OUT_OF_MEMORY;
		  return 0;
		}
	      if (!(wh = realloc (wh, wavcnt * sizeof (IMFWAVHEADER))))
		{
		  free (nextwav);
		  _mm_errno = MMERR_OUT_OF_MEMORY;
		  return 0;
		}
	      s = wh + (wavcnt - IMF_SMPINCR);
	    }

	  _mm_read_string (s->samplename, 13, modreader);
	  _mm_read_UBYTE (modreader);
	  _mm_read_UBYTE (modreader);
	  _mm_read_UBYTE (modreader);
	  s->length = _mm_read_I_ULONG (modreader);
	  s->loopstart = _mm_read_I_ULONG (modreader);
	  s->loopend = _mm_read_I_ULONG (modreader);
	  s->samplerate = _mm_read_I_ULONG (modreader);
	  s->volume = _mm_read_UBYTE (modreader) & 0x7f;
	  s->pan = _mm_read_UBYTE (modreader);
	  _mm_fseek (modreader, 14, SEEK_CUR);
	  s->flags = _mm_read_UBYTE (modreader);
	  _mm_fseek (modreader, 11, SEEK_CUR);
	  _mm_read_UBYTES (id, 4, modreader);
	  if (((memcmp (id, "IS10", 4)) && (memcmp (id, "IW10", 4))) ||
	      (_mm_eof (modreader)))
	    {
	      free (nextwav);
	      free (wh);
	      _mm_errno = MMERR_LOADING_SAMPLEINFO;
	      return 0;
	    }
	  nextwav[of.numsmp + u] = _mm_ftell (modreader);
	  _mm_fseek (modreader, s->length, SEEK_CUR);
	}

      of.numsmp += ih.numsmp;
      d++;
    }

  /* sanity check */
  if (!of.numsmp)
    {
      if (nextwav)
	free (nextwav);
      if (wh)
	free (wh);
      _mm_errno = MMERR_LOADING_SAMPLEINFO;
      return 0;
    }

  /* load samples */
  if (!AllocSamples ())
    {
      free (nextwav);
      free (wh);
      return 0;
    }
  if (!AllocLinear ())
    {
      free (nextwav);
      free (wh);
      return 0;
    }
  q = of.samples;
  s = wh;
  for (u = 0; u < of.numsmp; u++, s++, q++)
    {
      q->samplename = DupStr (s->samplename, 12, 1);
      q->length = s->length;
      q->loopstart = s->loopstart;
      q->loopend = s->loopend;
      q->volume = s->volume;
      q->speed = s->samplerate;
      if (of.flags & UF_LINEAR)
	q->speed = speed_to_finetune (s->samplerate << 1, u);
      q->panning = s->pan;
      q->seekpos = nextwav[u];

      q->flags |= SF_SIGNED;
      if (s->flags & 0x1)
	q->flags |= SF_LOOP;
      if (s->flags & 0x2)
	q->flags |= SF_BIDI;
      if (s->flags & 0x8)
	q->flags |= SF_OWNPAN;
      if (s->flags & 0x4)
	{
	  q->flags |= SF_16BITS;
	  q->length >>= 1;
	  q->loopstart >>= 1;
	  q->loopend >>= 1;
	}
    }

  d = of.instruments;
  s = wh;
  for (u = 0; u < of.numins; u++, d++)
    {
      for (t = 0; t < IMFNOTECNT; t++)
	{
	  if (d->samplenumber[t] >= of.numsmp)
	    d->samplenote[t] = 255;
	  else if (of.flags & UF_LINEAR)
	    {
	      int note = (int) d->samplenote[u] + noteindex[d->samplenumber[u]];
	      d->samplenote[u] = (note < 0) ? 0 : (note > 255 ? 255 : note);
	    }
	  else
	    d->samplenote[t] = t;
	}
    }

  free (wh);
  free (nextwav);
  return 1;
}

CHAR *
IMF_LoadTitle (void)
{
  CHAR s[31];

  _mm_fseek (modreader, 0, SEEK_SET);
  if (!_mm_read_UBYTES (s, 31, modreader))
    return NULL;

  return (DupStr (s, 31, 1));
}

/*========== Loader information */

MLOADER load_imf =
{
  NULL,
  "IMF",
  "IMF (Imago Orpheus)",
  IMF_Init,
  IMF_Test,
  IMF_Load,
  IMF_Cleanup,
  IMF_LoadTitle
};

/* ex:set ts=4: */
