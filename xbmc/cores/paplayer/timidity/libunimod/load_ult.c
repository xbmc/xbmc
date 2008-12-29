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

  $Id: load_ult.c,v 1.27 1999/10/25 16:31:41 miod Exp $

  Ultratracker (ULT) module loader

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "unimod_priv.h"

/*========== Module structure */

/* header */
typedef struct ULTHEADER
  {
    CHAR id[16];
    CHAR songtitle[32];
    UBYTE reserved;
  }
ULTHEADER;

/* sample information */
typedef struct ULTSAMPLE
  {
    CHAR samplename[32];
    CHAR dosname[12];
    SLONG loopstart;
    SLONG loopend;
    SLONG sizestart;
    SLONG sizeend;
    UBYTE volume;
    UBYTE flags;
    UWORD speed;
    SWORD finetune;
  }
ULTSAMPLE;

typedef struct ULTEVENT
  {
    UBYTE note, sample, eff, dat1, dat2;
  }
ULTEVENT;

/*========== Loader variables */

#define ULTS_16BITS     4
#define ULTS_LOOP       8
#define ULTS_REVERSE    16

#define ULT_VERSION_LEN 18
static CHAR ULT_Version[ULT_VERSION_LEN] = "Ultra Tracker v1.x";

static ULTEVENT ev;

/*========== Loader code */

BOOL 
ULT_Test (void)
{
  CHAR id[16];

  if (!_mm_read_string (id, 15, modreader))
    return 0;
  if (strncmp (id, "MAS_UTrack_V00", 14))
    return 0;
  if ((id[14] < '1') || (id[14] > '4'))
    return 0;
  return 1;
}

BOOL 
ULT_Init (void)
{
  return 1;
}

void 
ULT_Cleanup (void)
{
}

static UBYTE 
ReadUltEvent (ULTEVENT * event)
{
  UBYTE flag, rep = 1;

  flag = _mm_read_UBYTE (modreader);
  if (flag == 0xfc)
    {
      rep = _mm_read_UBYTE (modreader);
      event->note = _mm_read_UBYTE (modreader);
    }
  else
    event->note = flag;

  event->sample = _mm_read_UBYTE (modreader);
  event->eff = _mm_read_UBYTE (modreader);
  event->dat1 = _mm_read_UBYTE (modreader);
  event->dat2 = _mm_read_UBYTE (modreader);

  return rep;
}

BOOL 
ULT_Load (BOOL curious)
{
  int t, u, tracks = 0;
  SAMPLE *q;
  ULTSAMPLE s;
  ULTHEADER mh;
  UBYTE nos, noc, nop;

  /* try to read module header */
  _mm_read_string (mh.id, 15, modreader);
  _mm_read_string (mh.songtitle, 32, modreader);
  mh.reserved = _mm_read_UBYTE (modreader);

  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  ULT_Version[ULT_VERSION_LEN - 1] = '3' + (mh.id[14] - '1');
  of.modtype = DupStr (ULT_Version, ULT_VERSION_LEN, 1);
  of.initspeed = 6;
  of.inittempo = 125;
  of.reppos = 0;

  /* read songtext */
  if ((mh.id[14] > '1') && (mh.reserved))
    if (!ReadLinedComment (mh.reserved, 32))
      return 0;

  nos = _mm_read_UBYTE (modreader);
  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  of.songname = DupStr (mh.songtitle, 32, 1);
  of.numins = of.numsmp = nos;

  if (!AllocSamples ())
    return 0;
  q = of.samples;
  for (t = 0; t < nos; t++)
    {
      /* try to read sample info */
      _mm_read_string (s.samplename, 32, modreader);
      _mm_read_string (s.dosname, 12, modreader);
      s.loopstart = _mm_read_I_ULONG (modreader);
      s.loopend = _mm_read_I_ULONG (modreader);
      s.sizestart = _mm_read_I_ULONG (modreader);
      s.sizeend = _mm_read_I_ULONG (modreader);
      s.volume = _mm_read_UBYTE (modreader);
      s.flags = _mm_read_UBYTE (modreader);
      s.speed = (mh.id[14] >= '4') ? _mm_read_I_UWORD (modreader) : 8363;
      s.finetune = _mm_read_I_SWORD (modreader);

      if (_mm_eof (modreader))
	{
	  _mm_errno = MMERR_LOADING_SAMPLEINFO;
	  return 0;
	}

      q->samplename = DupStr (s.samplename, 32, 1);
      /* The correct formula for the coefficient would be
         pow(2,(double)s.finetume/OCTAVE/32768), but to avoid floating point
         here, we'll use a first order approximation here.
         1/567290 == Ln(2)/OCTAVE/32768 */
      q->speed = s.speed + s.speed * (((SLONG) s.speed * (SLONG) s.finetune) / 567290);
      q->length = s.sizeend - s.sizestart;
      q->volume = s.volume >> 2;
      q->loopstart = s.loopstart;
      q->loopend = s.loopend;
      q->flags = SF_SIGNED;
      if (s.flags & ULTS_LOOP)
	q->flags |= SF_LOOP;
      if (s.flags & ULTS_16BITS)
	{
	  s.sizeend += (s.sizeend - s.sizestart);
	  s.sizestart <<= 1;
	  q->flags |= SF_16BITS;
	  q->loopstart >>= 1;
	  q->loopend >>= 1;
	}
      q++;
    }

  if (!AllocPositions (256))
    return 0;
  for (t = 0; t < 256; t++)
    of.positions[t] = _mm_read_UBYTE (modreader);
  for (t = 0; t < 256; t++)
    if (of.positions[t] == 255)
      break;
  of.numpos = t;

  noc = _mm_read_UBYTE (modreader);
  nop = _mm_read_UBYTE (modreader);

  of.numchn = ++noc;
  of.numpat = ++nop;
  of.numtrk = of.numchn * of.numpat;
  if (!AllocTracks ())
    return 0;
  if (!AllocPatterns ())
    return 0;
  for (u = 0; u < of.numchn; u++)
    for (t = 0; t < of.numpat; t++)
      of.patterns[(t * of.numchn) + u] = tracks++;

  /* read pan position table for v1.5 and higher */
  if (mh.id[14] >= '3')
    for (t = 0; t < of.numchn; t++)
      of.panning[t] = _mm_read_UBYTE (modreader) << 4;

  for (t = 0; t < of.numtrk; t++)
    {
      int rep, row = 0;

      UniReset ();
      while (row < 64)
	{
	  rep = ReadUltEvent (&ev);

	  if (_mm_eof (modreader))
	    {
	      _mm_errno = MMERR_LOADING_TRACK;
	      return 0;
	    }

	  while (rep--)
	    {
	      UBYTE eff;
	      int offset;

	      if (ev.sample)
		UniInstrument (ev.sample - 1);
	      if (ev.note)
		UniNote (ev.note + 2 * OCTAVE - 1);

	      /* first effect - various fixes by Alexander Kerkhove and
	         Thomas Neumann */
	      eff = ev.eff >> 4;
	      switch (eff)
		{
		case 0x3:	/* tone portamento */
		  UniEffect (UNI_ITEFFECTG, ev.dat2);
		  break;
		case 0x5:
		  break;
		case 0x9:	/* sample offset */
		  offset = (ev.dat2 << 8) | ((ev.eff & 0xf) == 9 ? ev.dat1 : 0);
		  UniEffect (UNI_ULTEFFECT9, offset);
		  break;
		case 0xb:	/* panning */
		  UniPTEffect (8, ev.dat2 * 0xf);
		  break;
		case 0xc:	/* volume */
		  UniPTEffect (eff, ev.dat2 >> 2);
		  break;
		default:
		  UniPTEffect (eff, ev.dat2);
		  break;
		}

	      /* second effect */
	      eff = ev.eff & 0xf;
	      switch (eff)
		{
		case 0x3:	/* tone portamento */
		  UniEffect (UNI_ITEFFECTG, ev.dat1);
		  break;
		case 0x5:
		  break;
		case 0x9:	/* sample offset */
		  if ((ev.eff >> 4) != 9)
		    UniEffect (UNI_ULTEFFECT9, ((UWORD) ev.dat1) << 8);
		  break;
		case 0xb:	/* panning */
		  UniPTEffect (8, ev.dat1 * 0xf);
		  break;
		case 0xc:	/* volume */
		  UniPTEffect (eff, ev.dat1 >> 2);
		  break;
		default:
		  UniPTEffect (eff, ev.dat1);
		  break;
		}

	      UniNewline ();
	      row++;
	    }
	}
      if (!(of.tracks[t] = UniDup ()))
	return 0;
    }
  return 1;
}

CHAR *
ULT_LoadTitle (void)
{
  CHAR s[32];

  _mm_fseek (modreader, 15, SEEK_SET);
  if (!_mm_read_UBYTES (s, 32, modreader))
    return NULL;

  return (DupStr (s, 32, 1));
}

/*========== Loader information */

MLOADER load_ult =
{
  NULL,
  "ULT",
  "ULT (UltraTracker)",
  ULT_Init,
  ULT_Test,
  ULT_Load,
  ULT_Cleanup,
  ULT_LoadTitle
};


/* ex:set ts=4: */
