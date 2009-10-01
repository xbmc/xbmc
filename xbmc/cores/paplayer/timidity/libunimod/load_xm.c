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

  $Id: load_xm.c,v 1.32 1999/10/25 16:31:41 miod Exp $

  Fasttracker (XM) module loader

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
extern char *safe_strdup(const char *s);

#include "unimod_priv.h"

/*========== Module structure */

typedef struct XMHEADER
  {
    CHAR id[17];		/* ID text: 'Extended module: ' */
    CHAR songname[21];		/* Module name */
    CHAR trackername[20];	/* Tracker name */
    UWORD version;		/* Version number */
    ULONG headersize;		/* Header size */
    UWORD songlength;		/* Song length (in patten order table) */
    UWORD restart;		/* Restart position */
    UWORD numchn;		/* Number of channels (2,4,6,8,10,...,32) */
    UWORD numpat;		/* Number of patterns (max 256) */
    UWORD numins;		/* Number of instruments (max 128) */
    UWORD flags;
    UWORD tempo;		/* Default tempo */
    UWORD bpm;			/* Default BPM */
    UBYTE orders[256];		/* Pattern order table  */
  }
XMHEADER;

typedef struct XMINSTHEADER
  {
    ULONG size;			/* Instrument size */
    CHAR name[22];		/* Instrument name */
    UBYTE type;			/* Instrument type (always 0) */
    UWORD numsmp;		/* Number of samples in instrument */
    ULONG ssize;
  }
XMINSTHEADER;

#define XMENVCNT (12*2)
#define XMNOTECNT (8*OCTAVE)
typedef struct XMPATCHHEADER
  {
    UBYTE what[XMNOTECNT];	/*  Sample number for all notes */
    UWORD volenv[XMENVCNT];	/*  Points for volume envelope */
    UWORD panenv[XMENVCNT];	/*  Points for panning envelope */
    UBYTE volpts;		/*  Number of volume points */
    UBYTE panpts;		/*  Number of panning points */
    UBYTE volsus;		/*  Volume sustain point */
    UBYTE volbeg;		/*  Volume loop start point */
    UBYTE volend;		/*  Volume loop end point */
    UBYTE pansus;		/*  Panning sustain point */
    UBYTE panbeg;		/*  Panning loop start point */
    UBYTE panend;		/*  Panning loop end point */
    UBYTE volflg;		/*  Volume type: bit 0: On; 1: Sustain; 2: Loop */
    UBYTE panflg;		/*  Panning type: bit 0: On; 1: Sustain; 2: Loop */
    UBYTE vibflg;		/*  Vibrato type */
    UBYTE vibsweep;		/*  Vibrato sweep */
    UBYTE vibdepth;		/*  Vibrato depth */
    UBYTE vibrate;		/*  Vibrato rate */
    UWORD volfade;		/*  Volume fadeout */
  }
XMPATCHHEADER;

typedef struct XMWAVHEADER
  {
    ULONG length;		/* Sample length */
    ULONG loopstart;		/* Sample loop start */
    ULONG looplength;		/* Sample loop length */
    UBYTE volume;		/* Volume  */
    SBYTE finetune;		/* Finetune (signed byte -128..+127) */
    UBYTE type;			/* Loop type */
    UBYTE panning;		/* Panning (0-255) */
    SBYTE relnote;		/* Relative note number (signed byte) */
    UBYTE reserved;
    CHAR samplename[22];	/* Sample name */
    UBYTE vibtype;		/* Vibrato type */
    UBYTE vibsweep;		/* Vibrato sweep */
    UBYTE vibdepth;		/* Vibrato depth */
    UBYTE vibrate;		/* Vibrato rate */
  }
XMWAVHEADER;

typedef struct XMPATHEADER
  {
    ULONG size;			/* Pattern header length  */
    UBYTE packing;		/* Packing type (always 0) */
    UWORD numrows;		/* Number of rows in pattern (1..256) */
    SWORD packsize;		/* Packed patterndata size */
  }
XMPATHEADER;

typedef struct XMNOTE
  {
    UBYTE note, ins, vol, eff, dat;
  }
XMNOTE;

/*========== Loader variables */

static XMNOTE *xmpat = NULL;
static XMHEADER *mh = NULL;

/* increment unit for sample array reallocation */
#define XM_SMPINCR 64
static ULONG *nextwav = NULL;
static XMWAVHEADER *wh = NULL, *s = NULL;

/*========== Loader code */

BOOL 
XM_Test (void)
{
  UBYTE id[38];

  if (!_mm_read_UBYTES (id, 38, modreader))
    return 0;
  if (memcmp (id, "Extended Module: ", 17))
    return 0;
  if (id[37] == 0x1a)
    return 1;
  return 0;
}

BOOL 
XM_Init (void)
{
  if (!(mh = (XMHEADER *) _mm_malloc (sizeof (XMHEADER))))
    return 0;
  return 1;
}

void 
XM_Cleanup (void)
{
  _mm_free (mh);
}

static int 
XM_ReadNote (XMNOTE * n)
{
  UBYTE cmp, result = 1;

  memset (n, 0, sizeof (XMNOTE));
  cmp = _mm_read_UBYTE (modreader);

  if (cmp & 0x80)
    {
      if (cmp & 1)
	{
	  result++;
	  n->note = _mm_read_UBYTE (modreader);
	}
      if (cmp & 2)
	{
	  result++;
	  n->ins = _mm_read_UBYTE (modreader);
	}
      if (cmp & 4)
	{
	  result++;
	  n->vol = _mm_read_UBYTE (modreader);
	}
      if (cmp & 8)
	{
	  result++;
	  n->eff = _mm_read_UBYTE (modreader);
	}
      if (cmp & 16)
	{
	  result++;
	  n->dat = _mm_read_UBYTE (modreader);
	}
    }
  else
    {
      n->note = cmp;
      n->ins = _mm_read_UBYTE (modreader);
      n->vol = _mm_read_UBYTE (modreader);
      n->eff = _mm_read_UBYTE (modreader);
      n->dat = _mm_read_UBYTE (modreader);
      result += 4;
    }
  return result;
}

static UBYTE *
XM_Convert (XMNOTE * xmtrack, UWORD rows)
{
  int t;
  UBYTE note, ins, vol, eff, dat;

  UniReset ();
  for (t = 0; t < rows; t++)
    {
      note = xmtrack->note;
      ins = xmtrack->ins;
      vol = xmtrack->vol;
      eff = xmtrack->eff;
      dat = xmtrack->dat;

      if (note)
	{
	  if (note > XMNOTECNT)
	    UniEffect (UNI_KEYFADE, 0);
	  else
	    UniNote (note - 1);
	}
      if (ins)
	UniInstrument (ins - 1);

      switch (vol >> 4)
	{
	case 0x6:		/* volslide down */
	  if (vol & 0xf)
	    UniEffect (UNI_XMEFFECTA, vol & 0xf);
	  break;
	case 0x7:		/* volslide up */
	  if (vol & 0xf)
	    UniEffect (UNI_XMEFFECTA, vol << 4);
	  break;

	  /* volume-row fine volume slide is compatible with protracker
	     EBx and EAx effects i.e. a zero nibble means DO NOT SLIDE, as
	     opposed to 'take the last sliding value'. */
	case 0x8:		/* finevol down */
	  UniPTEffect (0xe, 0xb0 | (vol & 0xf));
	  break;
	case 0x9:		/* finevol up */
	  UniPTEffect (0xe, 0xa0 | (vol & 0xf));
	  break;
	case 0xa:		/* set vibrato speed */
	  UniPTEffect (0x4, vol << 4);
	  break;
	case 0xb:		/* vibrato */
	  UniPTEffect (0x4, vol & 0xf);
	  break;
	case 0xc:		/* set panning */
	  UniPTEffect (0x8, vol << 4);
	  break;
	case 0xd:		/* panning slide left (only slide when data not zero) */
	  if (vol & 0xf)
	    UniEffect (UNI_XMEFFECTP, vol & 0xf);
	  break;
	case 0xe:		/* panning slide right (only slide when data not zero) */
	  if (vol & 0xf)
	    UniEffect (UNI_XMEFFECTP, vol << 4);
	  break;
	case 0xf:		/* tone porta */
	  UniPTEffect (0x3, vol << 4);
	  break;
	default:
	  if ((vol >= 0x10) && (vol <= 0x50))
	    UniPTEffect (0xc, vol - 0x10);
	}

      switch (eff)
	{
	case 0x4:
	  UniEffect (UNI_XMEFFECT4, dat);
	  break;
	case 0xa:
	  UniEffect (UNI_XMEFFECTA, dat);
	  break;
	case 0xe:		/* Extended effects */
	  switch (dat >> 4)
	    {
	    case 0x1:		/* XM fine porta up */
	      UniEffect (UNI_XMEFFECTE1, dat & 0xf);
	      break;
	    case 0x2:		/* XM fine porta down */
	      UniEffect (UNI_XMEFFECTE2, dat & 0xf);
	      break;
	    case 0xa:		/* XM fine volume up */
	      UniEffect (UNI_XMEFFECTEA, dat & 0xf);
	      break;
	    case 0xb:		/* XM fine volume down */
	      UniEffect (UNI_XMEFFECTEB, dat & 0xf);
	      break;
	    default:
	      UniPTEffect (eff, dat);
	    }
	  break;
	case 'G' - 55:		/* G - set global volume */
	  UniEffect (UNI_XMEFFECTG, dat > 64 ? 64 : dat);
	  break;
	case 'H' - 55:		/* H - global volume slide */
	  UniEffect (UNI_XMEFFECTH, dat);
	  break;
	case 'K' - 55:		/* K - keyOff and KeyFade */
	  UniEffect (UNI_KEYFADE, dat);
	  break;
	case 'L' - 55:		/* L - set envelope position */
	  UniEffect (UNI_XMEFFECTL, dat);
	  break;
	case 'P' - 55:		/* P - panning slide */
	  UniEffect (UNI_XMEFFECTP, dat);
	  break;
	case 'R' - 55:		/* R - multi retrig note */
	  UniEffect (UNI_S3MEFFECTQ, dat);
	  break;
	case 'T' - 55:		/* T - Tremor */
	  UniEffect (UNI_S3MEFFECTI, dat);
	  break;
	case 'X' - 55:
	  switch (dat >> 4)
	    {
	    case 1:		/* X1 - Extra Fine Porta up */
	      UniEffect (UNI_XMEFFECTX1, dat & 0xf);
	      break;
	    case 2:		/* X2 - Extra Fine Porta down */
	      UniEffect (UNI_XMEFFECTX2, dat & 0xf);
	      break;
	    }
	  break;
	default:
	  if (eff <= 0xf)
	    {
	      /* the pattern jump destination is written in decimal,
	         but it seems some poor tracker software writes them
	         in hexadecimal... (sigh) */
	      if (eff == 0xd)
		/* don't change anything if we're sure it's in hexa */
		if ((((dat & 0xf0) >> 4) <= 9) && ((dat & 0xf) <= 9))
		  /* otherwise, convert from dec to hex */
		  dat = (((dat & 0xf0) >> 4) * 10) + (dat & 0xf);
	      UniPTEffect (eff, dat);
	    }
	  break;
	}
      UniNewline ();
      xmtrack++;
    }
  return UniDup ();
}

static BOOL 
LoadPatterns (BOOL dummypat)
{
  int t, u, v, numtrk;

  if (!AllocTracks ())
    return 0;
  if (!AllocPatterns ())
    return 0;

  numtrk = 0;
  for (t = 0; t < mh->numpat; t++)
    {
      XMPATHEADER ph;

      ph.size = _mm_read_I_ULONG (modreader);
      if (ph.size < (mh->version == 0x0102 ? 8 : 9))
	{
	  _mm_errno = MMERR_LOADING_PATTERN;
	  return 0;
	}
      ph.packing = _mm_read_UBYTE (modreader);
      if (ph.packing)
	{
	  _mm_errno = MMERR_LOADING_PATTERN;
	  return 0;
	}
      if (mh->version == 0x0102)
	ph.numrows = _mm_read_UBYTE (modreader) + 1;
      else
	ph.numrows = _mm_read_I_UWORD (modreader);
      ph.packsize = _mm_read_I_UWORD (modreader);

      ph.size -= (mh->version == 0x0102 ? 8 : 9);
      if (ph.size)
	_mm_fseek (modreader, ph.size, SEEK_CUR);

      of.pattrows[t] = ph.numrows;

      if (ph.numrows)
	{
	  if (!(xmpat = (XMNOTE *) _mm_calloc (ph.numrows * of.numchn, sizeof (XMNOTE))))
	    return 0;

	  /* when packsize is 0, don't try to load a pattern.. it's empty. */
	  if (ph.packsize)
	    for (u = 0; u < ph.numrows; u++)
	      for (v = 0; v < of.numchn; v++)
		{
		  if (!ph.packsize)
		    break;

		  ph.packsize -= XM_ReadNote (&xmpat[(v * ph.numrows) + u]);
		  if (ph.packsize < 0)
		    {
		      free (xmpat);
		      xmpat = NULL;
		      _mm_errno = MMERR_LOADING_PATTERN;
		      return 0;
		    }
		}

	  if (ph.packsize)
	    {
	      _mm_fseek (modreader, ph.packsize, SEEK_CUR);
	    }

	  if (_mm_eof (modreader))
	    {
	      free (xmpat);
	      xmpat = NULL;
	      _mm_errno = MMERR_LOADING_PATTERN;
	      return 0;
	    }

	  for (v = 0; v < of.numchn; v++)
	    of.tracks[numtrk++] = XM_Convert (&xmpat[v * ph.numrows], ph.numrows);

	  free (xmpat);
	  xmpat = NULL;
	}
      else
	{
	  for (v = 0; v < of.numchn; v++)
	    of.tracks[numtrk++] = XM_Convert (NULL, ph.numrows);
	}
    }

  if (dummypat)
    {
      of.pattrows[t] = 64;
      if (!(xmpat = (XMNOTE *) _mm_calloc (64 * of.numchn, sizeof (XMNOTE))))
	return 0;
      for (v = 0; v < of.numchn; v++)
	of.tracks[numtrk++] = XM_Convert (&xmpat[v * 64], 64);
      free (xmpat);
      xmpat = NULL;
    }

  return 1;
}

static BOOL 
LoadInstruments (void)
{
  int t, u;
  INSTRUMENT *d;
  long next = 0;
  UWORD wavcnt = 0;

  if (!AllocInstruments ())
    return 0;
  d = of.instruments;
  for (t = 0; t < of.numins; t++, d++)
    {
      XMINSTHEADER ih;
      long headend;

      memset (d->samplenumber, 0xff, INSTNOTES * sizeof (UWORD));

      /* read instrument header */
      headend = _mm_ftell (modreader);
      ih.size = _mm_read_I_ULONG (modreader);
      headend += ih.size;
      _mm_read_string (ih.name, 22, modreader);
      ih.type = _mm_read_UBYTE (modreader);
      ih.numsmp = _mm_read_I_UWORD (modreader);

      d->insname = DupStr (ih.name, 22, 1);

      if ((SWORD) ih.size > 29)
	{
	  ih.ssize = _mm_read_I_ULONG (modreader);
	  if (((SWORD) ih.numsmp > 0) && (ih.numsmp <= XMNOTECNT))
	    {
	      XMPATCHHEADER pth;
	      int p;

	      _mm_read_UBYTES (pth.what, XMNOTECNT, modreader);
	      _mm_read_I_UWORDS (pth.volenv, XMENVCNT, modreader);
	      _mm_read_I_UWORDS (pth.panenv, XMENVCNT, modreader);
	      pth.volpts = _mm_read_UBYTE (modreader);
	      pth.panpts = _mm_read_UBYTE (modreader);
	      pth.volsus = _mm_read_UBYTE (modreader);
	      pth.volbeg = _mm_read_UBYTE (modreader);
	      pth.volend = _mm_read_UBYTE (modreader);
	      pth.pansus = _mm_read_UBYTE (modreader);
	      pth.panbeg = _mm_read_UBYTE (modreader);
	      pth.panend = _mm_read_UBYTE (modreader);
	      pth.volflg = _mm_read_UBYTE (modreader);
	      pth.panflg = _mm_read_UBYTE (modreader);
	      pth.vibflg = _mm_read_UBYTE (modreader);
	      pth.vibsweep = _mm_read_UBYTE (modreader);
	      pth.vibdepth = _mm_read_UBYTE (modreader);
	      pth.vibrate = _mm_read_UBYTE (modreader);
	      pth.volfade = _mm_read_I_UWORD (modreader);

	      /* read the remainder of the header
	         (2 bytes for 1.03, 22 for 1.04) */
	      for (u = headend - _mm_ftell (modreader); u; u--)
		_mm_read_UBYTE (modreader);

	      /* #@!$&% fix for K_OSPACE.XM and possibly others */
	      if(pth.volpts > XMENVCNT/2)
	        pth.volpts = XMENVCNT/2;
	      if(pth.panpts > XMENVCNT/2)
	        pth.panpts = XMENVCNT/2;

	      if ((_mm_eof (modreader)) || (pth.volpts > XMENVCNT / 2) || (pth.panpts > XMENVCNT / 2))
		{
		  if (nextwav)
		    {
		      free (nextwav);
		      nextwav = NULL;
		    }
		  if (wh)
		    {
		      free (wh);
		      wh = NULL;
		    }
		  _mm_errno = MMERR_LOADING_SAMPLEINFO;
		  return 0;
		}

	      for (u = 0; u < XMNOTECNT; u++)
		d->samplenumber[u] = pth.what[u] + of.numsmp;
	      d->volfade = pth.volfade;

#define XM_ProcessEnvelope(name) 											\
 				for (u = 0; u < (XMENVCNT / 2); u++) {						\
 					d->name##env[u].pos = pth.name##env[2*u];		\
 					d->name##env[u].val = pth.name##env[2*u + 1];	\
 				}															\
				memcpy(d->name##env,pth.name##env,XMENVCNT);			\
				if (pth.name##flg&1) d->name##flg|=EF_ON;				\
				if (pth.name##flg&2) d->name##flg|=EF_SUSTAIN;			\
				if (pth.name##flg&4) d->name##flg|=EF_LOOP;				\
				d->name##susbeg=d->name##susend=pth.name##sus;		\
				d->name##beg=pth.name##beg;								\
				d->name##end=pth.name##end;								\
				d->name##pts=pth.name##pts;								\
																			\
				/* scale envelope */										\
				for (p=0;p<XMENVCNT/2;p++)									\
					d->name##env[p].val<<=2;								\
																			\
				if ((d->name##flg&EF_ON)&&(d->name##pts<2))				\
					d->name##flg&=~EF_ON;

	      XM_ProcessEnvelope (vol);
	      XM_ProcessEnvelope (pan);
#undef XM_ProcessEnvelope

	      /* Samples are stored outside the instrument struct now, so we
	         have to load them all into a temp area, count the of.numsmp
	         along the way and then do an AllocSamples() and move
	         everything over */
	      if (mh->version > 0x0103)
		next = 0;
	      for (u = 0; u < ih.numsmp; u++, s++)
		{
		  /* Allocate more room for sample information if necessary */
		  if (of.numsmp + u == wavcnt)
		    {
		      wavcnt += XM_SMPINCR;
		      if (!(nextwav = realloc (nextwav, wavcnt * sizeof (ULONG))))
			{
			  if (wh)
			    {
			      free (wh);
			      wh = NULL;
			    }
			  _mm_errno = MMERR_OUT_OF_MEMORY;
			  return 0;
			}
		      if (!(wh = realloc (wh, wavcnt * sizeof (XMWAVHEADER))))
			{
			  free (nextwav);
			  nextwav = NULL;
			  _mm_errno = MMERR_OUT_OF_MEMORY;
			  return 0;
			}
		      s = wh + (wavcnt - XM_SMPINCR);
		    }

		  s->length = _mm_read_I_ULONG (modreader);
		  s->loopstart = _mm_read_I_ULONG (modreader);
		  s->looplength = _mm_read_I_ULONG (modreader);
		  s->volume = _mm_read_UBYTE (modreader);
		  s->finetune = _mm_read_SBYTE (modreader);
		  s->type = _mm_read_UBYTE (modreader);
		  s->panning = _mm_read_UBYTE (modreader);
		  s->relnote = _mm_read_SBYTE (modreader);
		  s->vibtype = pth.vibflg;
		  s->vibsweep = pth.vibsweep;
		  s->vibdepth = pth.vibdepth * 4;
		  s->vibrate = pth.vibrate;
		  s->reserved = _mm_read_UBYTE (modreader);
		  _mm_read_string (s->samplename, 22, modreader);

		  nextwav[of.numsmp + u] = next;
		  next += s->length;

		  if (_mm_eof (modreader))
		    {
		      free (nextwav);
		      free (wh);
		      nextwav = NULL;
		      wh = NULL;
		      _mm_errno = MMERR_LOADING_SAMPLEINFO;
		      return 0;
		    }
		}

	      if (mh->version > 0x0103)
		{
		  for (u = 0; u < ih.numsmp; u++)
		    nextwav[of.numsmp++] += _mm_ftell (modreader);
		  _mm_fseek (modreader, next, SEEK_CUR);
		}
	      else
		of.numsmp += ih.numsmp;
	    }
	  else
	    {
	      /* read the remainder of the header */
	      for (u = headend - _mm_ftell (modreader); u; u--)
		_mm_read_UBYTE (modreader);

	      if (_mm_eof (modreader))
		{
		  free (nextwav);
		  free (wh);
		  nextwav = NULL;
		  wh = NULL;
		  _mm_errno = MMERR_LOADING_SAMPLEINFO;
		  return 0;
		}
	    }
	}
    }

  /* sanity check */
  if (!of.numsmp)
    {
      if (nextwav)
	{
	  free (nextwav);
	  nextwav = NULL;
	}
      if (wh)
	{
	  free (wh);
	  wh = NULL;
	}
      _mm_errno = MMERR_LOADING_SAMPLEINFO;
      return 0;
    }

  return 1;
}

BOOL 
XM_Load (BOOL curious)
{
  INSTRUMENT *d;
  SAMPLE *q;
  int t, u;
  BOOL dummypat = 0;
  char tracker[21], modtype[60];

  /* try to read module header */
  _mm_read_string (mh->id, 17, modreader);
  _mm_read_string (mh->songname, 21, modreader);
  _mm_read_string (mh->trackername, 20, modreader);
  mh->version = _mm_read_I_UWORD (modreader);
  if ((mh->version < 0x102) || (mh->version > 0x104))
    {
      _mm_errno = MMERR_NOT_A_MODULE;
      return 0;
    }
  mh->headersize = _mm_read_I_ULONG (modreader);
  mh->songlength = _mm_read_I_UWORD (modreader);
  mh->restart = _mm_read_I_UWORD (modreader);
  mh->numchn = _mm_read_I_UWORD (modreader);
  mh->numpat = _mm_read_I_UWORD (modreader);
  mh->numins = _mm_read_I_UWORD (modreader);
  mh->flags = _mm_read_I_UWORD (modreader);
  mh->tempo = _mm_read_I_UWORD (modreader);
  mh->bpm = _mm_read_I_UWORD (modreader);
  if (!mh->bpm)
    {
      _mm_errno = MMERR_NOT_A_MODULE;
      return 0;
    }
  _mm_read_UBYTES (mh->orders, 256, modreader);

  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  /* set module variables */
  of.initspeed = mh->tempo;
  of.inittempo = mh->bpm;
  strncpy (tracker, mh->trackername, 20);
  tracker[20] = 0;
  for (t = 20; (tracker[t] <= ' ') && (t >= 0); t--)
    tracker[t] = 0;

  /* some modules have the tracker name empty */
  if (!tracker[0])
    strcpy (tracker, "Unknown tracker");

#ifdef HAVE_SNPRINTF
  snprintf (modtype, 60, "%s (XM format %d.%02d)",
	    tracker, mh->version >> 8, mh->version & 0xff);
#else
  sprintf (modtype, "%s (XM format %d.%02d)",
	   tracker, mh->version >> 8, mh->version & 0xff);
#endif
  of.modtype = safe_strdup (modtype);
  of.numchn = mh->numchn;
  of.numpat = mh->numpat;
  of.numtrk = (UWORD) of.numpat * of.numchn;	/* get number of channels */
  of.songname = DupStr (mh->songname, 20, 1);
  of.numpos = mh->songlength;	/* copy the songlength */
  of.reppos = mh->restart < mh->songlength ? mh->restart : 0;
  of.numins = mh->numins;
  of.flags |= UF_XMPERIODS | UF_INST | UF_BGSLIDES | UF_NOWRAP | UF_FT2QUIRKS;
  if (mh->flags & 1)
    of.flags |= UF_LINEAR;

  memset (of.chanvol, 64, of.numchn);	/* store channel volumes */

  if (!AllocPositions (of.numpos + 1))
    return 0;
  for (t = 0; t < of.numpos; t++)
    of.positions[t] = mh->orders[t];

  /* We have to check for any pattern numbers in the order list greater than
     the number of patterns total. If one or more is found, we set it equal to
     the pattern total and make a dummy pattern to workaround the problem */
  for (t = 0; t < of.numpos; t++)
    {
      if (of.positions[t] >= of.numpat)
	{
	  of.positions[t] = of.numpat;
	  dummypat = 1;
	}
    }
  if (dummypat)
    {
      of.numpat++;
      of.numtrk += of.numchn;
    }

  if (mh->version < 0x0104)
    {
      if (!LoadInstruments ())
	return 0;
      if (!LoadPatterns (dummypat))
	return 0;
      for (t = 0; t < of.numsmp; t++)
	nextwav[t] += _mm_ftell (modreader);
    }
  else
    {
      if (!LoadPatterns (dummypat))
	return 0;
      if (!LoadInstruments ())
	return 0;
    }

  if (!AllocSamples ())
    {
      free (nextwav);
      free (wh);
      nextwav = NULL;
      wh = NULL;
      return 0;
    }
  q = of.samples;
  s = wh;
  for (u = 0; u < of.numsmp; u++, q++, s++)
    {
      q->samplename = DupStr (s->samplename, 22, 1);
      q->length = s->length;
      q->loopstart = s->loopstart;
      q->loopend = s->loopstart + s->looplength;
      q->volume = s->volume;
      q->speed = s->finetune + 128;
      q->panning = s->panning;
      q->seekpos = nextwav[u];
      q->vibtype = s->vibtype;
      q->vibsweep = s->vibsweep;
      q->vibdepth = s->vibdepth;
      q->vibrate = s->vibrate;

      if (s->type & 0x10)
	{
	  q->length >>= 1;
	  q->loopstart >>= 1;
	  q->loopend >>= 1;
	}

      q->flags |= SF_OWNPAN;
      if (s->type & 0x3)
	q->flags |= SF_LOOP;
      if (s->type & 0x2)
	q->flags |= SF_BIDI;
      if (s->type & 0x10)
	q->flags |= SF_16BITS;
      q->flags |= SF_DELTA | SF_SIGNED;
    }

  d = of.instruments;
  s = wh;
  for (u = 0; u < of.numins; u++, d++)
    for (t = 0; t < XMNOTECNT; t++)
      {
	if (d->samplenumber[t] >= of.numsmp)
	  d->samplenote[t] = 255;
	else
	  {
	    int note = t + s[d->samplenumber[t]].relnote;
	    d->samplenote[t] = (note < 0) ? 0 : note;
	  }
      }

  free (wh);
  free (nextwav);
  wh = NULL;
  nextwav = NULL;
  return 1;
}

CHAR *
XM_LoadTitle (void)
{
  CHAR s[21];

  _mm_fseek (modreader, 17, SEEK_SET);
  if (!_mm_read_UBYTES (s, 21, modreader))
    return NULL;

  return (DupStr (s, 21, 1));
}

/*========== Loader information */

MLOADER load_xm =
{
  NULL,
  "XM",
  "XM (FastTracker 2)",
  XM_Init,
  XM_Test,
  XM_Load,
  XM_Cleanup,
  XM_LoadTitle
};

/* ex:set ts=4: */
