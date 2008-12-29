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

  $Id: load_amf.c,v 1.28 1999/10/25 16:31:41 miod Exp $

  DMP Advanced Module Format loader

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "unimod_priv.h"

/*========== Module structure */

typedef struct AMFHEADER
  {
    UBYTE id[3];		/* AMF file marker */
    UBYTE version;		/* upper major, lower nibble minor version number */
    CHAR songname[32];		/* ASCIIZ songname */
    UBYTE numsamples;		/* number of samples saved */
    UBYTE numorders;
    UWORD numtracks;		/* number of tracks saved */
    UBYTE numchannels;		/* number of channels used  */
    SBYTE panpos[32];		/* voice pan positions */
    UBYTE songbpm;
    UBYTE songspd;
  }
AMFHEADER;

typedef struct AMFSAMPLE
  {
    UBYTE type;
    CHAR samplename[32];
    CHAR filename[13];
    ULONG offset;
    ULONG length;
    UWORD c2spd;
    UBYTE volume;
    ULONG reppos;
    ULONG repend;
  }
AMFSAMPLE;

typedef struct AMFNOTE
  {
    UBYTE note, instr, volume, fxcnt;
    UBYTE effect[3];
    SBYTE parameter[3];
  }
AMFNOTE;

/*========== Loader variables */

static AMFHEADER *mh = NULL;
#define AMFTEXTLEN 22
static CHAR AMF_Version[AMFTEXTLEN + 1] = "DSMI Module Format 0.0";
static AMFNOTE *track = NULL;

/*========== Loader code */

BOOL 
AMF_Test (void)
{
  UBYTE id[3], ver;

  if (!_mm_read_UBYTES (id, 3, modreader))
    return 0;
  if (memcmp (id, "AMF", 3))
    return 0;

  ver = _mm_read_UBYTE (modreader);
  if ((ver >= 10) && (ver <= 14))
    return 1;
  return 0;
}

BOOL 
AMF_Init (void)
{
  if (!(mh = (AMFHEADER *) _mm_malloc (sizeof (AMFHEADER))))
    return 0;
  if (!(track = (AMFNOTE *) _mm_calloc (64, sizeof (AMFNOTE))))
    return 0;

  return 1;
}

void 
AMF_Cleanup (void)
{
  _mm_free (mh);
  _mm_free (track);
}

static BOOL 
AMF_UnpackTrack (URL modreader)
{
  ULONG tracksize;
  UBYTE row, cmd;
  SBYTE arg;

  /* empty track */
  memset (track, 0, 64 * sizeof (AMFNOTE));

  /* read packed track */
  if (modreader)
    {
      tracksize = _mm_read_I_UWORD (modreader);
      tracksize += ((ULONG) _mm_read_UBYTE (modreader)) << 16;
      if (tracksize)
	while (tracksize--)
	  {
	    row = _mm_read_UBYTE (modreader);
	    cmd = _mm_read_UBYTE (modreader);
	    arg = _mm_read_SBYTE (modreader);
	    /* unexpected end of track */
	    if (!tracksize)
	      {
		if ((row == 0xff) && (cmd == 0xff) && (arg == -1))
		  break;
		/* the last triplet should be FF FF FF, but this is not
		   always the case... maybe a bug in m2amf ? 
		   else
		   return 0;
		 */

	      }
	    /* invalid row (probably unexpected end of row) */
	    if (row >= 64)
	      return 0;
	    if (cmd < 0x7f)
	      {
		/* note, vol */
		track[row].note = cmd;
		track[row].volume = (UBYTE) arg + 1;
	      }
	    else if (cmd == 0x7f)
	      {
		/* duplicate row */
		if ((arg < 0) && (row + arg >= 0))
		  {
		    memcpy (track + row, track + (row + arg), sizeof (AMFNOTE));
		  }
	      }
	    else if (cmd == 0x80)
	      {
		/* instr */
		track[row].instr = arg + 1;
	      }
	    else if (cmd == 0x83)
	      {
		/* volume without note */
		track[row].volume = (UBYTE) arg + 1;
	      }
	    else if (track[row].fxcnt < 3)
	      {
		/* effect, param */
		if (cmd > 0x97)
		  return 0;
		track[row].effect[track[row].fxcnt] = cmd & 0x7f;
		track[row].parameter[track[row].fxcnt] = arg;
		track[row].fxcnt++;
	      }
	    else
	      return 0;
	  }
    }
  return 1;
}

static UBYTE *
AMF_ConvertTrack (void)
{
  int row, fx4memory = 0;

  /* convert track */
  UniReset ();
  for (row = 0; row < 64; row++)
    {
      if (track[row].instr)
	UniInstrument (track[row].instr - 1);
      if (track[row].note > OCTAVE)
	UniNote (track[row].note - OCTAVE);

      /* AMF effects */
      while (track[row].fxcnt--)
	{
	  SBYTE inf = track[row].parameter[track[row].fxcnt];

	  switch (track[row].effect[track[row].fxcnt])
	    {
	    case 1:		/* Set speed */
	      UniEffect (UNI_S3MEFFECTA, inf);
	      break;
	    case 2:		/* Volume slide */
	      if (inf)
		{
		  UniWriteByte (UNI_S3MEFFECTD);
		  if (inf >= 0)
		    UniWriteByte ((inf & 0xf) << 4);
		  else
		    UniWriteByte ((-inf) & 0xf);
		}
	      break;
	      /* effect 3, set channel volume, done in UnpackTrack */
	    case 4:		/* Porta up/down */
	      if (inf)
		{
		  if (inf > 0)
		    {
		      UniEffect (UNI_S3MEFFECTE, inf);
		      fx4memory = UNI_S3MEFFECTE;
		    }
		  else
		    {
		      UniEffect (UNI_S3MEFFECTF, -inf);
		      fx4memory = UNI_S3MEFFECTF;
		    }
		}
	      else if (fx4memory)
		UniEffect (fx4memory, 0);
	      break;
	      /* effect 5, "Porta abs", not supported */
	    case 6:		/* Porta to note */
	      UniEffect (UNI_ITEFFECTG, inf);
	      break;
	    case 7:		/* Tremor */
	      UniEffect (UNI_S3MEFFECTI, inf);
	      break;
	    case 8:		/* Arpeggio */
	      UniPTEffect (0x0, inf);
	      break;
	    case 9:		/* Vibrato */
	      UniPTEffect (0x4, inf);
	      break;
	    case 0xa:		/* Porta + Volume slide */
	      UniPTEffect (0x3, 0);
	      if (inf)
		{
		  UniWriteByte (UNI_S3MEFFECTD);
		  if (inf >= 0)
		    UniWriteByte ((inf & 0xf) << 4);
		  else
		    UniWriteByte ((-inf) & 0xf);
		}
	      break;
	    case 0xb:		/* Vibrato + Volume slide */
	      UniPTEffect (0x4, 0);
	      if (inf)
		{
		  UniWriteByte (UNI_S3MEFFECTD);
		  if (inf >= 0)
		    UniWriteByte ((inf & 0xf) << 4);
		  else
		    UniWriteByte ((-inf) & 0xf);
		}
	      break;
	    case 0xc:		/* Pattern break (in hex) */
	      UniPTEffect (0xd, inf);
	      break;
	    case 0xd:		/* Pattern jump */
	      UniPTEffect (0xb, inf);
	      break;
	      /* effect 0xe, "Sync", not supported */
	    case 0xf:		/* Retrig */
	      UniEffect (UNI_S3MEFFECTQ, inf & 0xf);
	      break;
	    case 0x10:		/* Sample offset */
	      UniPTEffect (0x9, inf);
	      break;
	    case 0x11:		/* Fine volume slide */
	      if (inf)
		{
		  UniWriteByte (UNI_S3MEFFECTD);
		  if (inf >= 0)
		    UniWriteByte ((inf & 0xf) << 4 | 0xf);
		  else
		    UniWriteByte (0xf0 | ((-inf) & 0xf));
		}
	      break;
	    case 0x12:		/* Fine portamento */
	      if (inf)
		{
		  if (inf > 0)
		    {
		      UniEffect (UNI_S3MEFFECTE, 0xf0 | (inf & 0xf));
		      fx4memory = UNI_S3MEFFECTE;
		    }
		  else
		    {
		      UniEffect (UNI_S3MEFFECTF, 0xf0 | ((-inf) & 0xf));
		      fx4memory = UNI_S3MEFFECTF;
		    }
		}
	      else if (fx4memory)
		UniEffect (fx4memory, 0);
	      break;
	    case 0x13:		/* Delay note */
	      UniPTEffect (0xe, 0xd0 | (inf & 0xf));
	      break;
	    case 0x14:		/* Note cut */
	      UniPTEffect (0xc, 0);
	      track[row].volume = 0;
	      break;
	    case 0x15:		/* Set tempo */
	      UniEffect (UNI_S3MEFFECTT, inf);
	      break;
	    case 0x16:		/* Extra fine portamento */
	      if (inf)
		{
		  if (inf > 0)
		    {
		      UniEffect (UNI_S3MEFFECTE, 0xe0 | ((inf >> 2) & 0xf));
		      fx4memory = UNI_S3MEFFECTE;
		    }
		  else
		    {
		      UniEffect (UNI_S3MEFFECTF, 0xe0 | (((-inf) >> 2) & 0xf));
		      fx4memory = UNI_S3MEFFECTF;
		    }
		}
	      else if (fx4memory)
		UniEffect (fx4memory, 0);
	      break;
	    case 0x17:		/* Panning */
	      if (inf > 64)
		UniEffect (UNI_ITEFFECTS0, 0x91);	/* surround */
	      else
		UniPTEffect (0x8, (inf == 64) ? 255 : (inf + 64) << 1);
	      break;
	    }

	}
      if (track[row].volume)
	UniVolEffect (VOL_VOLUME, track[row].volume - 1);
      UniNewline ();
    }
  return UniDup ();
}

BOOL 
AMF_Load (BOOL curious)
{
  int t, u, realtrackcnt, realsmpcnt;
  AMFSAMPLE s;
  SAMPLE *q;
  UWORD *track_remap;
  ULONG samplepos;
  int channel_remap[16];

  /* try to read module header  */
  _mm_read_UBYTES (mh->id, 3, modreader);
  mh->version = _mm_read_UBYTE (modreader);
  _mm_read_string (mh->songname, 32, modreader);
  mh->numsamples = _mm_read_UBYTE (modreader);
  mh->numorders = _mm_read_UBYTE (modreader);
  mh->numtracks = _mm_read_I_UWORD (modreader);
  mh->numchannels = _mm_read_UBYTE (modreader);
  if ((!mh->numchannels) || (mh->numchannels > (mh->version >= 12 ? 32 : 16)))
    {
      _mm_errno = MMERR_NOT_A_MODULE;
      return 0;
    }

  if (mh->version >= 11)
    {
      memset (mh->panpos, 0, 32);
      _mm_read_SBYTES (mh->panpos, (mh->version >= 13) ? 32 : 16, modreader);
    }
  else
    _mm_read_UBYTES (channel_remap, 16, modreader);

  if (mh->version >= 13)
    {
      mh->songbpm = _mm_read_UBYTE (modreader);
      if (mh->songbpm < 32)
	{
	  _mm_errno = MMERR_NOT_A_MODULE;
	  return 0;
	}
      mh->songspd = _mm_read_UBYTE (modreader);
      if (mh->songspd > 32)
	{
	  _mm_errno = MMERR_NOT_A_MODULE;
	  return 0;
	}
    }
  else
    {
      mh->songbpm = 125;
      mh->songspd = 6;
    }

  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  /* set module variables */
  of.initspeed = mh->songspd;
  of.inittempo = mh->songbpm;
  AMF_Version[AMFTEXTLEN - 3] = '0' + (mh->version / 10);
  AMF_Version[AMFTEXTLEN - 1] = '0' + (mh->version % 10);
  of.modtype = strdup (AMF_Version);
  of.numchn = mh->numchannels;
  of.numtrk = mh->numorders * mh->numchannels;
  if (mh->numtracks > of.numtrk)
    of.numtrk = mh->numtracks;
  of.songname = DupStr (mh->songname, 32, 1);
  of.numpos = mh->numorders;
  of.numpat = mh->numorders;
  of.reppos = 0;
  of.flags |= UF_S3MSLIDES;
  for (t = 0; t < 32; t++)
    {
      if (mh->panpos[t] > 64)
	of.panning[t] = PAN_SURROUND;
      else if (mh->panpos[t] == 64)
	of.panning[t] = 255;
      else
	of.panning[t] = (mh->panpos[t] + 64) << 1;
    }
  of.numins = of.numsmp = mh->numsamples;

  if (!AllocPositions (of.numpos))
    return 0;
  for (t = 0; t < of.numpos; t++)
    of.positions[t] = t;

  if (!AllocTracks ())
    return 0;
  if (!AllocPatterns ())
    return 0;

  /* read AMF order table */
  for (t = 0; t < of.numpat; t++)
    {
      if (mh->version >= 14)
	/* track size */
	of.pattrows[t] = _mm_read_I_UWORD (modreader);
      if (mh->version >= 10)
	_mm_read_I_UWORDS (of.patterns + (t * of.numchn), of.numchn, modreader);
      else
	for (u = 0; u < of.numchn; u++)
	  of.patterns[t * of.numchn + channel_remap[u]] = _mm_read_I_UWORD (modreader);
    }
  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  /* read sample information */
  if (!AllocSamples ())
    return 0;
  q = of.samples;
  for (t = 0; t < of.numins; t++)
    {
      /* try to read sample info */
      s.type = _mm_read_UBYTE (modreader);
      _mm_read_string (s.samplename, 32, modreader);
      _mm_read_string (s.filename, 13, modreader);
      s.offset = _mm_read_I_ULONG (modreader);
      s.length = _mm_read_I_ULONG (modreader);
      s.c2spd = _mm_read_I_UWORD (modreader);
      if (s.c2spd == 8368)
	s.c2spd = 8363;
      s.volume = _mm_read_UBYTE (modreader);
      if (mh->version >= 11)
	{
	  s.reppos = _mm_read_I_ULONG (modreader);
	  s.repend = _mm_read_I_ULONG (modreader);
	}
      else
	{
	  s.reppos = _mm_read_I_UWORD (modreader);
	  s.repend = s.length;
	}

      if (_mm_eof (modreader))
	{
	  _mm_errno = MMERR_LOADING_SAMPLEINFO;
	  return 0;
	}

      q->samplename = DupStr (s.samplename, 32, 1);
      q->speed = s.c2spd;
      q->volume = s.volume;
      if (s.type)
	{
	  q->seekpos = s.offset;
	  q->length = s.length;
	  q->loopstart = s.reppos;
	  q->loopend = s.repend;
	  if ((s.repend - s.reppos) > 2)
	    q->flags |= SF_LOOP;
	}
      q++;
    }

  /* read track table */
  if (!(track_remap = _mm_calloc (mh->numtracks + 1, sizeof (UWORD))))
    return 0;
  _mm_read_I_UWORDS (track_remap + 1, mh->numtracks, modreader);
  if (_mm_eof (modreader))
    {
      free (track_remap);
      _mm_errno = MMERR_LOADING_TRACK;
      return 0;
    }

  for (realtrackcnt = t = 0; t <= mh->numtracks; t++)
    if (realtrackcnt < track_remap[t])
      realtrackcnt = track_remap[t];
  for (t = 0; t < of.numpat * of.numchn; t++)
    of.patterns[t] = (of.patterns[t] <= mh->numtracks) ?
      track_remap[of.patterns[t]] - 1 : realtrackcnt;

  free (track_remap);

  /* unpack tracks */
  for (t = 0; t < realtrackcnt; t++)
    {
      if (_mm_eof (modreader))
	{
	  _mm_errno = MMERR_LOADING_TRACK;
	  return 0;
	}
      if (!AMF_UnpackTrack (modreader))
	{
	  _mm_errno = MMERR_LOADING_TRACK;
	  return 0;
	}
      if (!(of.tracks[t] = AMF_ConvertTrack ()))
	return 0;
    }
  /* add en extra void track */
  UniReset ();
  for (t = 0; t < 64; t++)
    UniNewline ();
  of.tracks[realtrackcnt++] = UniDup ();
  for (t = realtrackcnt; t < of.numtrk; t++)
    of.tracks[t] = NULL;

  /* compute sample offsets */
  samplepos = _mm_ftell (modreader);
  for (realsmpcnt = t = 0; t < of.numsmp; t++)
    if (realsmpcnt < of.samples[t].seekpos)
      realsmpcnt = of.samples[t].seekpos;
  for (t = 1; t <= realsmpcnt; t++)
    {
      q = of.samples;
      while (q->seekpos != t)
	q++;
      q->seekpos = samplepos;
      samplepos += q->length;
    }

  return 1;
}

CHAR *
AMF_LoadTitle (void)
{
  CHAR s[32];

  _mm_fseek (modreader, 4, SEEK_SET);
  if (!_mm_read_UBYTES (s, 32, modreader))
    return NULL;

  return (DupStr (s, 32, 1));
}

/*========== Loader information */

MLOADER load_amf =
{
  NULL,
  "AMF",
  "AMF (DSMI Advanced Module Format)",
  AMF_Init,
  AMF_Test,
  AMF_Load,
  AMF_Cleanup,
  AMF_LoadTitle
};

/* ex:set ts=4: */
