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

  $Id: load_dsm.c,v 1.28 1999/10/25 16:31:41 miod Exp $

  DSIK internal format (DSM) module loader

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "unimod_priv.h"

/*========== Module structure */

#define DSM_MAXCHAN (16)
#define DSM_MAXORDERS (128)

typedef struct DSMSONG
  {
    CHAR songname[28];
    UWORD version;
    UWORD flags;
    ULONG reserved2;
    UWORD numord;
    UWORD numsmp;
    UWORD numpat;
    UWORD numtrk;
    UBYTE globalvol;
    UBYTE mastervol;
    UBYTE speed;
    UBYTE bpm;
    UBYTE panpos[DSM_MAXCHAN];
    UBYTE orders[DSM_MAXORDERS];
  }
DSMSONG;

typedef struct DSMINST
  {
    CHAR filename[13];
    UWORD flags;
    UBYTE volume;
    ULONG length;
    ULONG loopstart;
    ULONG loopend;
    ULONG reserved1;
    UWORD c2spd;
    UWORD period;
    CHAR samplename[28];
  }
DSMINST;

typedef struct DSMNOTE
  {
    UBYTE note, ins, vol, cmd, inf;
  }
DSMNOTE;

#define DSM_SURROUND (0xa4)

/*========== Loader variables */

static CHAR *SONGID = "SONG";
static CHAR *INSTID = "INST";
static CHAR *PATTID = "PATT";

static UBYTE blockid[4];
static ULONG blockln;
static ULONG blocklp;
static DSMSONG *mh = NULL;
static DSMNOTE *dsmbuf = NULL;

static CHAR DSM_Version[] = "DSIK DSM-format";

static unsigned char DSMSIG[4 + 4] =
{'R', 'I', 'F', 'F', 'D', 'S', 'M', 'F'};

/*========== Loader code */

BOOL 
DSM_Test (void)
{
  UBYTE id[12];

  if (!_mm_read_UBYTES (id, 12, modreader))
    return 0;
  if (!memcmp (id, DSMSIG, 4) && !memcmp (id + 8, DSMSIG + 4, 4))
    return 1;

  return 0;
}

BOOL 
DSM_Init (void)
{
  if (!(dsmbuf = (DSMNOTE *) _mm_malloc (DSM_MAXCHAN * 64 * sizeof (DSMNOTE))))
    return 0;
  if (!(mh = (DSMSONG *) _mm_calloc (1, sizeof (DSMSONG))))
    return 0;
  return 1;
}

void 
DSM_Cleanup (void)
{
  _mm_free (dsmbuf);
  _mm_free (mh);
}

static BOOL 
GetBlockHeader (void)
{
  /* make sure we're at the right position for reading the
     next riff block, no matter how many bytes read */
  _mm_fseek (modreader, blocklp + blockln, SEEK_SET);

  while (1)
    {
      _mm_read_UBYTES (blockid, 4, modreader);
      blockln = _mm_read_I_ULONG (modreader);
      if (_mm_eof (modreader))
	{
	  _mm_errno = MMERR_LOADING_HEADER;
	  return 0;
	}

      if (memcmp (blockid, SONGID, 4) && memcmp (blockid, INSTID, 4) &&
	  memcmp (blockid, PATTID, 4))
	{
#ifdef MIKMOD_DEBUG
	  fprintf (stderr, "\rDSM: Skipping unknown block type %4.4s\n", blockid);
#endif
	  _mm_fseek (modreader, blockln, SEEK_CUR);
	}
      else
	break;
    }

  blocklp = _mm_ftell (modreader);

  return 1;
}

static BOOL 
DSM_ReadPattern (void)
{
  int flag, row = 0;
  SWORD length;
  DSMNOTE *n;

  /* clear pattern data */
  memset (dsmbuf, 255, DSM_MAXCHAN * 64 * sizeof (DSMNOTE));
  length = _mm_read_I_SWORD (modreader);

  while (row < 64)
    {
      flag = _mm_read_UBYTE (modreader);
      if ((_mm_eof (modreader)) || (--length < 0))
	{
	  _mm_errno = MMERR_LOADING_PATTERN;
	  return 0;
	}

      if (flag)
	{
	  n = &dsmbuf[((flag & 0xf) * 64) + row];
	  if (flag & 0x80)
	    n->note = _mm_read_UBYTE (modreader);
	  if (flag & 0x40)
	    n->ins = _mm_read_UBYTE (modreader);
	  if (flag & 0x20)
	    n->vol = _mm_read_UBYTE (modreader);
	  if (flag & 0x10)
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
DSM_ConvertTrack (DSMNOTE * tr)
{
  int t;
  UBYTE note, ins, vol, cmd, inf;

  UniReset ();
  for (t = 0; t < 64; t++)
    {
      note = tr[t].note;
      ins = tr[t].ins;
      vol = tr[t].vol;
      cmd = tr[t].cmd;
      inf = tr[t].inf;

      if (ins != 0 && ins != 255)
	UniInstrument (ins - 1);
      if (note != 255)
	UniNote (note - 1);	/* normal note */
      if (vol < 65)
	UniPTEffect (0xc, vol);

      if (cmd != 255)
	{
	  if (cmd == 0x8)
	    {
	      if (inf == DSM_SURROUND)
		UniEffect (UNI_ITEFFECTS0, 0x91);
	      else if (inf <= 0x80)
		{
		  inf = (inf < 0x80) ? inf << 1 : 255;
		  UniPTEffect (cmd, inf);
		}
	    }
	  else if (cmd == 0xb)
	    {
	      if (inf <= 0x7f)
		UniPTEffect (cmd, inf);
	    }
	  else
	    {
	      /* Convert pattern jump from Dec to Hex */
	      if (cmd == 0xd)
		inf = (((inf & 0xf0) >> 4) * 10) + (inf & 0xf);
	      UniPTEffect (cmd, inf);
	    }
	}
      UniNewline ();
    }
  return UniDup ();
}

BOOL 
DSM_Load (BOOL curious)
{
  int t;
  DSMINST s;
  SAMPLE *q;
  int cursmp = 0, curpat = 0, track = 0;

  blocklp = 0;
  blockln = 12;

  if (!GetBlockHeader ())
    return 0;
  if (memcmp (blockid, SONGID, 4))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  _mm_read_UBYTES (mh->songname, 28, modreader);
  mh->version = _mm_read_I_UWORD (modreader);
  mh->flags = _mm_read_I_UWORD (modreader);
  mh->reserved2 = _mm_read_I_ULONG (modreader);
  mh->numord = _mm_read_I_UWORD (modreader);
  mh->numsmp = _mm_read_I_UWORD (modreader);
  mh->numpat = _mm_read_I_UWORD (modreader);
  mh->numtrk = _mm_read_I_UWORD (modreader);
  mh->globalvol = _mm_read_UBYTE (modreader);
  mh->mastervol = _mm_read_UBYTE (modreader);
  mh->speed = _mm_read_UBYTE (modreader);
  mh->bpm = _mm_read_UBYTE (modreader);
  _mm_read_UBYTES (mh->panpos, DSM_MAXCHAN, modreader);
  _mm_read_UBYTES (mh->orders, DSM_MAXORDERS, modreader);

  /* set module variables */
  of.initspeed = mh->speed;
  of.inittempo = mh->bpm;
  of.modtype = strdup (DSM_Version);
  of.numchn = mh->numtrk;
  of.numpat = mh->numpat;
  of.numtrk = of.numchn * of.numpat;
  of.songname = DupStr (mh->songname, 28, 1);	/* make a cstr of songname */
  of.reppos = 0;

  for (t = 0; t < DSM_MAXCHAN; t++)
    of.panning[t] = mh->panpos[t] == DSM_SURROUND ? PAN_SURROUND :
      mh->panpos[t] < 0x80 ? (mh->panpos[t] << 1) : 255;

  if (!AllocPositions (mh->numord))
    return 0;
  of.numpos = 0;
  for (t = 0; t < mh->numord; t++)
    {
      of.positions[of.numpos] = mh->orders[t];
      if (mh->orders[t] < 254)
	of.numpos++;
    }

  of.numins = of.numsmp = mh->numsmp;

  if (!AllocSamples ())
    return 0;
  if (!AllocTracks ())
    return 0;
  if (!AllocPatterns ())
    return 0;

  while (cursmp < of.numins || curpat < of.numpat)
    {
      if (!GetBlockHeader ())
	return 0;
      if (!memcmp (blockid, INSTID, 4) && cursmp < of.numins)
	{
	  q = &of.samples[cursmp];

	  /* try to read sample info */
	  _mm_read_UBYTES (s.filename, 13, modreader);
	  s.flags = _mm_read_I_UWORD (modreader);
	  s.volume = _mm_read_UBYTE (modreader);
	  s.length = _mm_read_I_ULONG (modreader);
	  s.loopstart = _mm_read_I_ULONG (modreader);
	  s.loopend = _mm_read_I_ULONG (modreader);
	  s.reserved1 = _mm_read_I_ULONG (modreader);
	  s.c2spd = _mm_read_I_UWORD (modreader);
	  s.period = _mm_read_I_UWORD (modreader);
	  _mm_read_UBYTES (s.samplename, 28, modreader);

	  q->samplename = DupStr (s.samplename, 28, 1);
	  q->seekpos = _mm_ftell (modreader);
	  q->speed = s.c2spd;
	  q->length = s.length;
	  q->loopstart = s.loopstart;
	  q->loopend = s.loopend;
	  q->volume = s.volume;

	  if (s.flags & 1)
	    q->flags |= SF_LOOP;
	  if (s.flags & 2)
	    q->flags |= SF_SIGNED;
	  /* (s.flags&4) means packed sample,
	     but did they really exist in dsm ? */
	  cursmp++;
	}
      else if (!memcmp (blockid, PATTID, 4) && curpat < of.numpat)
	{
	  DSM_ReadPattern ();
	  for (t = 0; t < of.numchn; t++)
	    if (!(of.tracks[track++] = DSM_ConvertTrack (&dsmbuf[t * 64])))
	      return 0;
	  curpat++;
	}
    }

  return 1;
}

CHAR *
DSM_LoadTitle (void)
{
  CHAR s[28];

  _mm_fseek (modreader, 12, SEEK_SET);
  if (!_mm_read_UBYTES (s, 28, modreader))
    return NULL;

  return (DupStr (s, 28, 1));
}

/*========== Loader information */

MLOADER load_dsm =
{
  NULL,
  "DSM",
  "DSM (DSIK internal format)",
  DSM_Init,
  DSM_Test,
  DSM_Load,
  DSM_Cleanup,
  DSM_LoadTitle
};


/* ex:set ts=4: */
