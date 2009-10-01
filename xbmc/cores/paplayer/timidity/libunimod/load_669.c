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

  $Id: load_669.c,v 1.30 1999/10/25 16:31:41 miod Exp $

  Composer 669 module loader

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "unimod_priv.h"

/*========== Module structure */

/* header */
typedef struct S69HEADER
  {
    UBYTE marker[2];
    CHAR message[108];
    UBYTE nos;
    UBYTE nop;
    UBYTE looporder;
    UBYTE orders[0x80];
    UBYTE tempos[0x80];
    UBYTE breaks[0x80];
  }
S69HEADER;

/* sample information */
typedef struct S69SAMPLE
  {
    CHAR filename[13];
    SLONG length;
    SLONG loopbeg;
    SLONG loopend;
  }
S69SAMPLE;

/* encoded note */
typedef struct S69NOTE
  {
    UBYTE a, b, c;
  }
S69NOTE;

/*========== Loader variables */

/* current pattern */
static S69NOTE *s69pat = NULL;
/* Module header */
static S69HEADER *mh = NULL;

/* file type identification */
static CHAR *S69_Version[] =
{
  "Composer 669",
  "Extended 669"
};

/*========== Loader code */

BOOL 
S69_Test (void)
{
  UBYTE buf[0x80];

  if (!_mm_read_UBYTES (buf, 2, modreader))
    return 0;
  /* look for id */
  if (!memcmp (buf, "if", 2) || !memcmp (buf, "JN", 2))
    {
      int i;

      /* skip song message */
      _mm_fseek (modreader, 108, SEEK_CUR);
      /* sanity checks */
      if (_mm_read_UBYTE (modreader) > 64)
	return 0;
      if (_mm_read_UBYTE (modreader) > 128)
	return 0;
      if (_mm_read_UBYTE (modreader) > 127)
	return 0;
      /* check order table */
      if (!_mm_read_UBYTES (buf, 0x80, modreader))
	return 0;
      for (i = 0; i < 0x80; i++)
	if ((buf[i] >= 0x80) && (buf[i] != 0xff))
	  return 0;
      /* check tempos table */
      if (!_mm_read_UBYTES (buf, 0x80, modreader))
	return 0;
      for (i = 0; i < 0x80; i++)
	if ((!buf[i]) || (buf[i] > 32))
	  return 0;
      /* check pattern length table */
      if (!_mm_read_UBYTES (buf, 0x80, modreader))
	return 0;
      for (i = 0; i < 0x80; i++)
	if (buf[i] > 0x3f)
	  return 0;
    }
  else
    return 0;

  return 1;
}

BOOL 
S69_Init (void)
{
  if (!(s69pat = (S69NOTE *) _mm_malloc (64 * 8 * sizeof (S69NOTE))))
    return 0;
  if (!(mh = (S69HEADER *) _mm_malloc (sizeof (S69HEADER))))
    return 0;

  return 1;
}

void 
S69_Cleanup (void)
{
  _mm_free (s69pat);
  _mm_free (mh);
}

static BOOL 
S69_LoadPatterns (void)
{
  int track, row, channel;
  UBYTE note, inst, vol, effect, lastfx, lastval;
  S69NOTE *cur;
  int tracks = 0;

  if (!AllocPatterns ())
    return 0;
  if (!AllocTracks ())
    return 0;

  for (track = 0; track < of.numpat; track++)
    {
      /* set pattern break locations */
      of.pattrows[track] = mh->breaks[track] + 1;

      /* load the 669 pattern */
      cur = s69pat;
      for (row = 0; row < 64; row++)
	{
	  for (channel = 0; channel < 8; channel++, cur++)
	    {
	      cur->a = _mm_read_UBYTE (modreader);
	      cur->b = _mm_read_UBYTE (modreader);
	      cur->c = _mm_read_UBYTE (modreader);
	    }
	}

      if (_mm_eof (modreader))
	{
	  _mm_errno = MMERR_LOADING_PATTERN;
	  return 0;
	}

      /* translate the pattern */
      for (channel = 0; channel < 8; channel++)
	{
	  UniReset ();
	  /* set pattern tempo */
	  UniPTEffect (0xf, 78);
	  UniPTEffect (0xf, mh->tempos[track]);

	  lastfx = 0xff, lastval = 0;

	  for (row = 0; row <= mh->breaks[track]; row++)
	    {
	      int a, b, c;

	      /* fetch the encoded note */
	      a = s69pat[(row * 8) + channel].a;
	      b = s69pat[(row * 8) + channel].b;
	      c = s69pat[(row * 8) + channel].c;

	      /* decode it */
	      note = a >> 2;
	      inst = ((a & 0x3) << 4) | ((b & 0xf0) >> 4);
	      vol = b & 0xf;

	      if (a < 0xff)
		{
		  if (a < 0xfe)
		    {
		      UniInstrument (inst);
		      UniNote (note + 2 * OCTAVE);
		      lastfx = 0xff;	/* reset background effect memory */
		    }
		  UniPTEffect (0xc, vol << 2);
		}

	      if ((c != 0xff) || (lastfx != 0xff))
		{
		  if (c == 0xff)
		    c = lastfx, effect = lastval;
		  else
		    effect = c & 0xf;

		  switch (c >> 4)
		    {
		    case 0:	/* porta up */
		      UniPTEffect (0x1, effect);
		      lastfx = c, lastval = effect;
		      break;
		    case 1:	/* porta down */
		      UniPTEffect (0x2, effect);
		      lastfx = c, lastval = effect;
		      break;
		    case 2:	/* porta to note */
		      UniPTEffect (0x3, effect);
		      lastfx = c, lastval = effect;
		      break;
		    case 3:	/* frequency adjust */
		      /* DMP converts this effect to S3M FF1. Why not ? */
		      UniEffect (UNI_S3MEFFECTF, 0xf0 | effect);
		      break;
		    case 4:	/* vibrato */
		      UniPTEffect (0x4, effect);
		      lastfx = c, lastval = effect;
		      break;
		    case 5:	/* set speed */
		      if (effect)
			UniPTEffect (0xf, effect);
		      else if (mh->marker[0] != 0x69)
			{
#ifdef MIKMOD_DEBUG
			  fprintf (stderr, "\r669: unsupported super fast tempo at pat=%d row=%d chan=%d\n",
				   track, row, channel);
#endif
			}
		      break;
		    }
		}
	      UniNewline ();
	    }
	  if (!(of.tracks[tracks++] = UniDup ()))
	    return 0;
	}
    }

  return 1;
}

BOOL 
S69_Load (BOOL curious)
{
  int i;
  SAMPLE *current;
  S69SAMPLE sample;

  /* module header */
  _mm_read_UBYTES (mh->marker, 2, modreader);
  _mm_read_UBYTES (mh->message, 108, modreader);
  mh->nos = _mm_read_UBYTE (modreader);
  mh->nop = _mm_read_UBYTE (modreader);
  mh->looporder = _mm_read_UBYTE (modreader);
  _mm_read_UBYTES (mh->orders, 0x80, modreader);
  for (i = 0; i < 0x80; i++)
    if ((mh->orders[i] >= 0x80) && (mh->orders[i] != 0xff))
      {
	_mm_errno = MMERR_NOT_A_MODULE;
	return 1;
      }
  _mm_read_UBYTES (mh->tempos, 0x80, modreader);
  for (i = 0; i < 0x80; i++)
    if ((!mh->tempos[i]) || (mh->tempos[i] > 32))
      {
	_mm_errno = MMERR_NOT_A_MODULE;
	return 1;
      }
  _mm_read_UBYTES (mh->breaks, 0x80, modreader);
  for (i = 0; i < 0x80; i++)
    if (mh->breaks[i] > 0x3f)
      {
	_mm_errno = MMERR_NOT_A_MODULE;
	return 1;
      }

  /* set module variables */
  of.initspeed = 4;
  of.inittempo = 78;
  of.songname = DupStr (mh->message, 36, 1);
  of.modtype = strdup (S69_Version[memcmp (mh->marker, "JN", 2) == 0]);
  of.numchn = 8;
  of.numpat = mh->nop;
  of.numins = of.numsmp = mh->nos;
  of.numtrk = of.numchn * of.numpat;
  of.flags = UF_XMPERIODS | UF_LINEAR;

  for (i = 35; (i >= 0) && (mh->message[i] == ' '); i--)
    mh->message[i] = 0;
  for (i = 36 + 35; (i >= 36 + 0) && (mh->message[i] == ' '); i--)
    mh->message[i] = 0;
  for (i = 72 + 35; (i >= 72 + 0) && (mh->message[i] == ' '); i--)
    mh->message[i] = 0;
  if ((mh->message[0]) || (mh->message[36]) || (mh->message[72]))
    if ((of.comment = (CHAR *) _mm_malloc (3 * (36 + 1) + 1)))
      {
	strncpy (of.comment, mh->message, 36);
	strcat (of.comment, "\r");
	if (mh->message[36])
	  strncat (of.comment, mh->message + 36, 36);
	strcat (of.comment, "\r");
	if (mh->message[72])
	  strncat (of.comment, mh->message + 72, 36);
	strcat (of.comment, "\r");
	of.comment[3 * (36 + 1)] = 0;
      }

  if (!AllocPositions (0x80))
    return 0;
  for (i = 0; i < 0x80; i++)
    {
      if (mh->orders[i] >= mh->nop)
	break;
      of.positions[i] = mh->orders[i];
    }
  of.numpos = i;
  of.reppos = mh->looporder < of.numpos ? mh->looporder : 0;

  if (!AllocSamples ())
    return 0;
  current = of.samples;

  for (i = 0; i < of.numins; i++)
    {
      /* sample information */
      _mm_read_UBYTES ((UBYTE *) sample.filename, 13, modreader);
      sample.length = _mm_read_I_SLONG (modreader);
      sample.loopbeg = _mm_read_I_SLONG (modreader);
      sample.loopend = _mm_read_I_SLONG (modreader);
      if (sample.loopend == 0xfffff)
	sample.loopend = 0;

      if ((sample.length < 0) || (sample.loopbeg < -1) || (sample.loopend < -1))
	{
	  _mm_errno = MMERR_LOADING_HEADER;
	  return 0;
	}

      current->samplename = DupStr (sample.filename, 13, 1);
      current->seekpos = 0;
      current->speed = 0;
      current->length = sample.length;
      current->loopstart = sample.loopbeg;
      current->loopend = (sample.loopend < sample.length) ? sample.loopend : sample.length;
      current->flags = (sample.loopbeg < sample.loopend) ? SF_LOOP : 0;
      current->volume = 64;

      current++;
    }

  if (!S69_LoadPatterns ())
    return 0;

  return 1;
}

CHAR *
S69_LoadTitle (void)
{
  CHAR s[36];

  _mm_fseek (modreader, 2, SEEK_SET);
  if (!_mm_read_UBYTES (s, 36, modreader))
    return NULL;

  return (DupStr (s, 36, 1));
}

/*========== Loader information */

MLOADER load_669 =
{
  NULL,
  "669",
  "669 (Composer 669, Unis 669)",
  S69_Init,
  S69_Test,
  S69_Load,
  S69_Cleanup,
  S69_LoadTitle
};

/* ex:set ts=4: */
