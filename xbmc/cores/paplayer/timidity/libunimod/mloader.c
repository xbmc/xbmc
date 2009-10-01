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

  $Id: mloader.c,v 1.33 1999/10/25 16:31:41 miod Exp $

  These routines are used to access the available module loaders

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

#include "unimod_priv.h"

#include <string.h>

URL modreader;
MODULE of;
BOOL ML_8bitsamples;
BOOL ML_monosamples;

static MLOADER *firstloader = NULL;

UWORD finetune[16] =
{
  8363, 8413, 8463, 8529, 8581, 8651, 8723, 8757,
  7895, 7941, 7985, 8046, 8107, 8169, 8232, 8280
};

/* This is a handle of sorts attached to any sample registered with
   SL_RegisterSample. */
typedef struct SAMPLOAD
  {
    struct SAMPLOAD *next;

    ULONG length;		/* length of sample (in samples!) */
    ULONG loopstart;		/* repeat position (relative to start, in samples) */
    ULONG loopend;		/* repeat end */
    UWORD infmt, outfmt;
    int scalefactor;
    SAMPLE *sample;
    URL reader;
  }
SAMPLOAD;

static int sl_rlength;
static SWORD sl_old;
static SWORD *sl_buffer = NULL;
static SAMPLOAD *musiclist = NULL;

/* size of the loader buffer in words */
#define SLBUFSIZE 2048

/* max # of KB to be devoted to samples */
/* #define MAX_SAMPLESPACE 1024 */

/* IT-Compressed status structure */
typedef struct ITPACK
  {
    UWORD bits;			/* current number of bits */
    UWORD bufbits;		/* bits in buffer */
    SWORD last;			/* last output */
    UBYTE buf;			/* bit buffer */
  }
ITPACK;

#ifdef MAX_SAMPLESPACE
static void SL_HalveSample (SAMPLOAD *);
#endif
static void SL_Sample8to16 (SAMPLOAD *);
static void SL_Sample16to8 (SAMPLOAD *);
static void SL_SampleSigned (SAMPLOAD *);
static void SL_SampleUnsigned (SAMPLOAD *);
static SAMPLOAD *SL_RegisterSample (SAMPLE *, URL);
static SWORD *SL_Load (SAMPLOAD *);
static BOOL SL_Init (SAMPLOAD *);
static void SL_Exit (SAMPLOAD *);

BOOL 
SL_Init (SAMPLOAD * s)
{
  if (!sl_buffer)
    if (!(sl_buffer = _mm_malloc (SLBUFSIZE * sizeof (SWORD))))
      return 0;

  sl_rlength = s->length;
  if (s->infmt & SF_16BITS)
    sl_rlength >>= 1;
  sl_old = 0;

  return 1;
}

void 
SL_Exit (SAMPLOAD * s)
{
  if (sl_rlength > 0)
    _mm_fseek (s->reader, sl_rlength, SEEK_CUR);
  if (sl_buffer)
    {
      free (sl_buffer);
      sl_buffer = NULL;
    }
}

/* unpack a 8bit IT packed sample */
static BOOL 
read_itcompr8 (ITPACK * status, URL reader, SWORD * sl_buffer, UWORD count, UWORD * incnt)
{
  SWORD *dest = sl_buffer, *end = sl_buffer + count;
  UWORD x, y, needbits, havebits, new_count = 0;
  UWORD bits = status->bits;
  UWORD bufbits = status->bufbits;
  SBYTE last = status->last;
  UBYTE buf = status->buf;

  while (dest < end)
    {
      needbits = new_count ? 3 : bits;
      x = havebits = 0;
      while (needbits)
	{
	  /* feed buffer */
	  if (!bufbits)
	    {
	      if ((*incnt)--)
		buf = _mm_read_UBYTE (reader);
	      else
		buf = 0;
	      bufbits = 8;
	    }
	  /* get as many bits as necessary */
	  y = needbits < bufbits ? needbits : bufbits;
	  x |= (buf & ((1 << y) - 1)) << havebits;
	  buf >>= y;
	  bufbits -= y;
	  needbits -= y;
	  havebits += y;
	}
      if (new_count)
	{
	  new_count = 0;
	  if (++x >= bits)
	    x++;
	  bits = x;
	  continue;
	}
      if (bits < 7)
	{
	  if (x == (1 << (bits - 1)))
	    {
	      new_count = 1;
	      continue;
	    }
	}
      else if (bits < 9)
	{
	  y = (0xff >> (9 - bits)) - 4;
	  if ((x > y) && (x <= y + 8))
	    {
	      if ((x -= y) >= bits)
		x++;
	      bits = x;
	      continue;
	    }
	}
      else if (bits < 10)
	{
	  if (x >= 0x100)
	    {
	      bits = x - 0x100 + 1;
	      continue;
	    }
	}
      else
	{
	  /* error in compressed data... */
	  _mm_errno = MMERR_ITPACK_INVALID_DATA;
	  return 0;
	}

      if (bits < 8)		/* extend sign */
	x = ((SBYTE) (x << (8 - bits))) >> (8 - bits);
      *(dest++) = (last += x) << 8;	/* convert to 16 bit */
    }
  status->bits = bits;
  status->bufbits = bufbits;
  status->last = last;
  status->buf = buf;
  return dest - sl_buffer;
}

/* unpack a 16bit IT packed sample */
static BOOL 
read_itcompr16 (ITPACK * status, URL reader, SWORD * sl_buffer, UWORD count, UWORD * incnt)
{
  SWORD *dest = sl_buffer, *end = sl_buffer + count;
  SLONG x, y, needbits, havebits, new_count = 0;
  UWORD bits = status->bits;
  UWORD bufbits = status->bufbits;
  SWORD last = status->last;
  UBYTE buf = status->buf;

  while (dest < end)
    {
      needbits = new_count ? 4 : bits;
      x = havebits = 0;
      while (needbits)
	{
	  /* feed buffer */
	  if (!bufbits)
	    {
	      if ((*incnt)--)
		buf = _mm_read_UBYTE (reader);
	      else
		buf = 0;
	      bufbits = 8;
	    }
	  /* get as many bits as necessary */
	  y = needbits < bufbits ? needbits : bufbits;
	  x |= (buf & ((1 << y) - 1)) << havebits;
	  buf >>= y;
	  bufbits -= y;
	  needbits -= y;
	  havebits += y;
	}
      if (new_count)
	{
	  new_count = 0;
	  if (++x >= bits)
	    x++;
	  bits = x;
	  continue;
	}
      if (bits < 7)
	{
	  if (x == (1 << (bits - 1)))
	    {
	      new_count = 1;
	      continue;
	    }
	}
      else if (bits < 17)
	{
	  y = (0xffff >> (17 - bits)) - 8;
	  if ((x > y) && (x <= y + 16))
	    {
	      if ((x -= y) >= bits)
		x++;
	      bits = x;
	      continue;
	    }
	}
      else if (bits < 18)
	{
	  if (x >= 0x10000)
	    {
	      bits = x - 0x10000 + 1;
	      continue;
	    }
	}
      else
	{
	  /* error in compressed data... */
	  _mm_errno = MMERR_ITPACK_INVALID_DATA;
	  return 0;
	}

      if (bits < 16)		/* extend sign */
	x = ((SWORD) (x << (16 - bits))) >> (16 - bits);
      *(dest++) = (last += x);
    }
  status->bits = bits;
  status->bufbits = bufbits;
  status->last = last;
  status->buf = buf;
  return dest - sl_buffer;
}

static BOOL 
SL_LoadInternal (void *buffer, UWORD infmt, UWORD outfmt, int scalefactor, ULONG length, URL reader)
{
  SBYTE *bptr = (SBYTE *) buffer;
  SWORD *wptr = (SWORD *) buffer;
  int stodo, t, u;

  int result, c_block = 0;	/* compression bytes until next block */
  ITPACK status;
  UWORD incnt;

  while (length)
    {
      stodo = (length < SLBUFSIZE) ? length : SLBUFSIZE;

      if (infmt & SF_ITPACKED)
	{
	  sl_rlength = 0;
	  if (!c_block)
	    {
	      status.bits = (infmt & SF_16BITS) ? 17 : 9;
	      status.last = status.bufbits = 0;
	      incnt = _mm_read_I_UWORD (reader);
	      c_block = (infmt & SF_16BITS) ? 0x4000 : 0x8000;
	      if (infmt & SF_DELTA)
		sl_old = 0;
	    }
	  if (infmt & SF_16BITS)
	    {
	      if (!(result = read_itcompr16 (&status, reader, sl_buffer, stodo, &incnt)))
		return 1;
	    }
	  else
	    {
	      if (!(result = read_itcompr8 (&status, reader, sl_buffer, stodo, &incnt)))
		return 1;
	    }
	  if (result != stodo)
	    {
	      _mm_errno = MMERR_ITPACK_INVALID_DATA;
	      return 1;
	    }
	  c_block -= stodo;
	}
      else
	{
	  if (infmt & SF_16BITS)
	    {
	      if (infmt & SF_BIG_ENDIAN)
		_mm_read_M_SWORDS (sl_buffer, stodo, reader);
	      else
		_mm_read_I_SWORDS (sl_buffer, stodo, reader);
	    }
	  else
	    {
	      /* Always convert to 16 bits for internal use */
	      SBYTE *src;
	      SWORD *dest;

	      _mm_read_UBYTES (sl_buffer, stodo, reader);
	      src = (SBYTE *) sl_buffer;
	      dest = sl_buffer;
	      src += stodo;
	      dest += stodo;

	      for (t = 0; t < stodo; t++)
		{
		  src--;
		  dest--;
		  *dest = (*src) << 8;
		}
	    }
	  sl_rlength -= stodo;
	}

      if (infmt & SF_DELTA)
	for (t = 0; t < stodo; t++)
	  {
	    sl_buffer[t] += sl_old;
	    sl_old = sl_buffer[t];
	  }

      if ((infmt ^ outfmt) & SF_SIGNED)
	for (t = 0; t < stodo; t++)
	  sl_buffer[t] ^= 0x8000;

      /* Dithering... */
      if ((infmt & SF_STEREO) && !(outfmt & SF_STEREO))
	{
	  /* dither stereo to mono, average together every two samples */
	  SLONG avgval;
	  int idx = 0;

	  t = 0;
	  while (t < stodo && length)
	    {
	      avgval = sl_buffer[t++];
	      avgval += sl_buffer[t++];
	      sl_buffer[idx++] = avgval >> 1;
	      length -= 2;
	    }
	  stodo = idx;
	}
      else if (scalefactor)
	{
	  int idx = 0;
	  SLONG scaleval;

	  /* Sample Scaling... average values for better results. */
	  t = 0;
	  while (t < stodo && length)
	    {
	      scaleval = 0;
	      for (u = scalefactor; u && t < stodo; u--, t++)
		scaleval += sl_buffer[t];
	      sl_buffer[idx++] = scaleval / (scalefactor - u);
	      length--;
	    }
	  stodo = idx;
	}
      else
	length -= stodo;

      if (outfmt & SF_16BITS)
	{
	  for (t = 0; t < stodo; t++)
	    *(wptr++) = sl_buffer[t];
	}
      else
	{
	  for (t = 0; t < stodo; t++)
	    *(bptr++) = sl_buffer[t] >> 8;
	}
    }
  return 0;
}

static SWORD *
SL_Load (struct SAMPLOAD *sload)
{
  SAMPLE *s = sload->sample;
  SWORD *data;
  ULONG t, length, loopstart, loopend;

  length = s->length;
  loopstart = s->loopstart;
  loopend = s->loopend;

  if (!(data = (SWORD *) _mm_malloc ((length + 20) << 1)))
    {
      _mm_errno = MMERR_SAMPLE_TOO_BIG;
      return NULL;
    }

  /* read sample into buffer */
  if (SL_LoadInternal (data, sload->infmt, sload->outfmt,
		       sload->scalefactor, length, sload->reader))
    return NULL;

  /* Unclick sample */
  if (s->flags & SF_LOOP)
    {
      if (s->flags & SF_BIDI)
	for (t = 0; t < 16; t++)
	  data[loopend + t] = data[(loopend - t) - 1];
      else
	for (t = 0; t < 16; t++)
	  data[loopend + t] = data[t + loopstart];
    }
  else
    for (t = 0; t < 16; t++)
      data[t + length] = 0;

  return data;
}

/* Registers a sample for loading when SL_LoadSamples() is called. */
SAMPLOAD *
SL_RegisterSample (SAMPLE * s, URL reader)
{
  SAMPLOAD *news, *cruise;

  cruise = musiclist;

  /* Allocate and add structure to the END of the list */
  if (!(news = (SAMPLOAD *) _mm_malloc (sizeof (SAMPLOAD))))
    return NULL;

  if (cruise)
    {
      while (cruise->next)
	cruise = cruise->next;
      cruise->next = news;
    }
  else
    musiclist = news;

  news->infmt = s->flags & SF_FORMATMASK;
  news->outfmt = news->infmt;
  news->reader = reader;
  news->sample = s;
  news->length = s->length;
  news->loopstart = s->loopstart;
  news->loopend = s->loopend;

  if (ML_monosamples)
    {
      news->outfmt &= ~SF_STEREO;
    }

  if (ML_8bitsamples)
    {
      SL_SampleUnsigned (news);
      SL_Sample16to8 (news);
    }
  else
    {
      SL_SampleSigned (news);
      SL_Sample8to16 (news);
    }
  

  return news;
}

static void 
FreeSampleList ()
{
  SAMPLOAD *old, *s = musiclist;

  while (s)
    {
      old = s;
      s = s->next;
      free (old);
    }
  musiclist = NULL;
}

/* Returns the total amount of memory required by the musiclist queue. */
#ifdef MAX_SAMPLESPACE
static ULONG 
SampleTotal ()
{
  int total = 0;
  SAMPLOAD *samplist = musiclist;
  SAMPLE *s;

  while (samplist)
    {
      s = samplist->sample;
      s->flags = (s->flags & ~SF_FORMATMASK) | samplist->outfmt;

      total += (s->length * ((s->flags & SF_16BITS) ? 2 : 1)) + 16;

      samplist = samplist->next;
    }

  return total;
}

static ULONG 
RealSpeed (SAMPLOAD * s)
{
  return (s->sample->speed / (s->scalefactor ? s->scalefactor : 1));
}
#endif

BOOL 
SL_LoadSamples (void)
{
  SAMPLOAD *s;

  if (!musiclist)
    return 0;

#ifdef MAX_SAMPLESPACE
  while (SampleTotal () > (MAX_SAMPLESPACE * 1024))
    {
      /* First Pass - check for any 16 bit samples */
      s = musiclist;
      while (s)
	{
	  if (s->outfmt & SF_16BITS)
	    {
	      SL_Sample16to8 (s);
	      break;
	    }
	  s = s->next;
	}
      /* Second pass (if no 16bits found above) is to take the sample with
         the highest speed and dither it by half. */
      if (!s)
	{
	  SAMPLOAD *c2smp = NULL;
	  ULONG maxsize, speed;

	  s = musiclist;
	  speed = 0;
	  while (s)
	    {
	      if ((s->sample->length) && (RealSpeed (s) > speed))
		{
		  speed = RealSpeed (s);
		  c2smp = s;
		}
	      s = s->next;
	    }
	  if (c2smp)
	    SL_HalveSample (c2smp);
	}
    }
#endif

  /* Samples dithered, now load them ! */
  s = musiclist;
  while (s)
    {
      /* sample has to be loaded ? -> increase number of samples, allocate
         memory and load sample. */
      if (s->sample->length)
	{
	  if (s->sample->seekpos)
	    _mm_fseek (s->reader, s->sample->seekpos, SEEK_SET);

	  /* Call the sample load routine of the driver module. It has to
	     return a pointer to sample dat). */
	  if (SL_Init (s))
	    {
	      s->sample->data = SL_Load (s);
	      SL_Exit (s);
	    }
	  s->sample->flags = (s->sample->flags & ~SF_FORMATMASK) | s->outfmt;
	  if (s->sample->data == NULL)
	    {
	      FreeSampleList (musiclist);
	      return 1;
	    }
	}
      s = s->next;
    }

  FreeSampleList (musiclist);
  return 0;
}

void 
SL_Sample16to8 (SAMPLOAD * s)
{
  s->outfmt &= ~SF_16BITS;
  s->sample->flags = (s->sample->flags & ~SF_FORMATMASK) | s->outfmt;
}

void 
SL_Sample8to16 (SAMPLOAD * s)
{
  s->outfmt |= SF_16BITS;
  s->sample->flags = (s->sample->flags & ~SF_FORMATMASK) | s->outfmt;
}

void 
SL_SampleSigned (SAMPLOAD * s)
{
  s->outfmt |= SF_SIGNED;
  s->sample->flags = (s->sample->flags & ~SF_FORMATMASK) | s->outfmt;
}

void 
SL_SampleUnsigned (SAMPLOAD * s)
{
  s->outfmt &= ~SF_SIGNED;
  s->sample->flags = (s->sample->flags & ~SF_FORMATMASK) | s->outfmt;
}

#ifdef MAX_SAMPLESPACE
void 
SL_HalveSample (SAMPLOAD * s)
{
  s->scalefactor = 2;		/* this is a divisor */
  s->sample->divfactor = 1;	/* this is a shift count */
  s->sample->length = s->length / s->scalefactor;
  s->sample->loopstart = s->loopstart / s->scalefactor;
  s->sample->loopend = s->loopend / s->scalefactor;
}
#endif


CHAR *
ML_InfoLoader (void)
{
  int len = 0;
  MLOADER *l;
  CHAR *list = NULL;

  /* compute size of buffer */
  for (l = firstloader; l; l = l->next)
    len += 1 + (l->next ? 1 : 0) + strlen (l->version);

  if (len)
    if ((list = _mm_malloc (len * sizeof (CHAR))))
      {
	list[0] = 0;
	/* list all registered module loders */
	for (l = firstloader; l; l = l->next)
	  sprintf (list, (l->next) ? "%s%s\n" : "%s%s", list, l->version);
      }
  return list;
}

BOOL 
ReadComment (UWORD len)
{
  if (len)
    {
      int i;

      if (!(of.comment = (CHAR *) _mm_malloc (len + 1)))
	return 0;
      _mm_read_UBYTES (of.comment, len, modreader);

      /* translate IT linefeeds */
      for (i = 0; i < len; i++)
	if (of.comment[i] == '\r')
	  of.comment[i] = '\n';

      of.comment[len] = 0;	/* just in case */
    }
  if (!of.comment[0])
    {
      free (of.comment);
      of.comment = NULL;
    }
  return 1;
}

BOOL 
ReadLinedComment (UWORD lines, UWORD linelen)
{
  CHAR *tempcomment, *line, *storage;
  UWORD total = 0, t, len = lines * linelen;
  int i;

  if (lines)
    {
      if (!(tempcomment = (CHAR *) _mm_malloc (len + 1)))
	return 0;
      if (!(storage = (CHAR *) _mm_malloc (linelen + 1)))
	{
	  free (tempcomment);
	  return 0;
	}
      _mm_read_UBYTES (tempcomment, len, modreader);

      /* compute message length */
      for (line = tempcomment, total = t = 0; t < lines; t++, line += linelen)
	{
	  for (i = linelen; (i >= 0) && (line[i] == ' '); i--)
	    line[i] = 0;
	  for (i = 0; i < linelen; i++)
	    if (!line[i])
	      break;
	  total += 1 + i;
	}

      if (total > lines)
	{
	  if (!(of.comment = (CHAR *) _mm_malloc (total + 1)))
	    {
	      free (storage);
	      free (tempcomment);
	      return 0;
	    }

	  /* convert message */
	  for (line = tempcomment, t = 0; t < lines; t++, line += linelen)
	    {
	      for (i = 0; i < linelen; i++)
		if (!(storage[i] = line[i]))
		  break;
	      storage[i] = 0;	/* if (i==linelen) */
	      strcat (of.comment, storage);
	      strcat (of.comment, "\r");
	    }
	  free (storage);
	  free (tempcomment);
	}
    }
  return 1;
}

BOOL 
AllocPositions (int total)
{
  if (!total)
    {
      _mm_errno = MMERR_NOT_A_MODULE;
      return 0;
    }
  if (!(of.positions = _mm_calloc (total, sizeof (UWORD))))
    return 0;
  return 1;
}

BOOL 
AllocPatterns (void)
{
  int s, t, tracks = 0;

  if ((!of.numpat) || (!of.numchn))
    {
      _mm_errno = MMERR_NOT_A_MODULE;
      return 0;
    }
  /* Allocate track sequencing array */
  if (!(of.patterns = (UWORD *) _mm_calloc ((ULONG) (of.numpat + 1) * of.numchn, sizeof (UWORD))))
    return 0;
  if (!(of.pattrows = (UWORD *) _mm_calloc (of.numpat + 1, sizeof (UWORD))))
    return 0;

  for (t = 0; t <= of.numpat; t++)
    {
      of.pattrows[t] = 64;
      for (s = 0; s < of.numchn; s++)
	of.patterns[(t * of.numchn) + s] = tracks++;
    }

  return 1;
}

BOOL 
AllocTracks (void)
{
  if (!of.numtrk)
    {
      _mm_errno = MMERR_NOT_A_MODULE;
      return 0;
    }
  if (!(of.tracks = (UBYTE **) _mm_calloc (of.numtrk, sizeof (UBYTE *))))
    return 0;
  return 1;
}

BOOL 
AllocInstruments (void)
{
  int t, n;

  if (!of.numins)
    {
      _mm_errno = MMERR_NOT_A_MODULE;
      return 0;
    }
  if (!(of.instruments = (INSTRUMENT *) _mm_calloc (of.numins, sizeof (INSTRUMENT))))
    return 0;

  for (t = 0; t < of.numins; t++)
    {
      for (n = 0; n < INSTNOTES; n++)
	{
	  /* Init note / sample lookup table */
	  of.instruments[t].samplenote[n] = n;
	  of.instruments[t].samplenumber[n] = t;
	}
      of.instruments[t].globvol = 64;
    }
  return 1;
}

BOOL 
AllocSamples (void)
{
  UWORD u;

  if (!of.numsmp)
    {
      _mm_errno = MMERR_NOT_A_MODULE;
      return 0;
    }
  if (!(of.samples = (SAMPLE *) _mm_calloc (of.numsmp, sizeof (SAMPLE))))
    return 0;

  for (u = 0; u < of.numsmp; u++)
    {
      of.samples[u].panning = 128;	/* center */
      of.samples[u].data = NULL;
      of.samples[u].globvol = 64;
      of.samples[u].volume = 64;
    }
  return 1;
}

static BOOL 
ML_LoadSamples (void)
{
  SAMPLE *s;
  int u;

  for (u = of.numsmp, s = of.samples; u; u--, s++)
    if (s->length)
      SL_RegisterSample (s, modreader);

  return 1;
}

/* Creates a CSTR out of a character buffer of 'len' bytes, but strips any
   terminating non-printing characters like 0, spaces etc.                    */
CHAR *
DupStr (CHAR * s, UWORD len, BOOL strict)
{
  UWORD t;
  CHAR *d = NULL;

  /* Scan for last printing char in buffer [includes high ascii up to 254] */
  while (len)
    {
      if (s[len - 1] > 0x20)
	break;
      len--;
    }

  /* Scan forward for possible NULL character */
  if (strict)
    {
      for (t = 0; t < len; t++)
	if (!s[t])
	  break;
      if (t < len)
	len = t;
    }

  /* When the buffer wasn't completely empty, allocate a cstring and copy the
     buffer into that string, except for any control-chars */
  if ((d = (CHAR *) _mm_malloc (sizeof (CHAR) * (len + 1))))
    {
      for (t = 0; t < len; t++)
	d[t] = (s[t] < 32) ? '.' : s[t];
      d[len] = 0;
    }
  return d;
}

static void 
ML_XFreeSample (SAMPLE * s)
{
  if (s->data)
    free (s->data);
  if (s->samplename)
    free (s->samplename);
}

static void 
ML_XFreeInstrument (INSTRUMENT * i)
{
  if (i->insname)
    free (i->insname);
}

void 
ML_Free (MODULE * mf)
{
  UWORD t;

  if (!mf)
    return;

  if (mf->songname)
    free (mf->songname);
  if (mf->comment)
    free (mf->comment);

  if (mf->modtype)
    free (mf->modtype);
  if (mf->positions)
    free (mf->positions);
  if (mf->patterns)
    free (mf->patterns);
  if (mf->pattrows)
    free (mf->pattrows);

  if (mf->tracks)
    {
      for (t = 0; t < mf->numtrk; t++)
	if (mf->tracks[t])
	  free (mf->tracks[t]);
      free (mf->tracks);
    }
  if (mf->instruments)
    {
      for (t = 0; t < mf->numins; t++)
	ML_XFreeInstrument (&mf->instruments[t]);
      free (mf->instruments);
    }
  if (mf->samples)
    {
      for (t = 0; t < mf->numsmp; t++)
	if (mf->samples[t].length)
	  ML_XFreeSample (&mf->samples[t]);
      free (mf->samples);
    }
  memset (mf, 0, sizeof (MODULE));
  if (mf != &of)
    free (mf);
}

static MODULE *
ML_AllocUniMod (void)
{
  return (_mm_malloc (sizeof (MODULE)));
}

CHAR *
ML_LoadTitle (URL reader)
{
  MLOADER *l;

  modreader = reader;
  _mm_errno = 0;

  /* Try to find a loader that recognizes the module */
  for (l = firstloader; l; l = l->next)
    {
      _mm_rewind (modreader);
      if (l->Test ())
	break;
    }

  if (!l)
    {
      _mm_errno = MMERR_NOT_A_MODULE;
      return NULL;
    }

  return l->LoadTitle ();
}

/* Check if it is a module given a reader */
BOOL 
ML_Test (URL reader)
{
  MLOADER *l;

  modreader = reader;
  _mm_errno = 0;

  /* Try to find a loader that recognizes the module */
  for (l = firstloader; l; l = l->next)
    {
      _mm_rewind (modreader);
      if (l->Test ())
	return 1;
    }
  return 0;
}

/* Loads a module given a reader */
MODULE *
ML_Load (URL reader, int maxchan, BOOL curious)
{
  int t;
  MLOADER *l;
  BOOL ok;
  MODULE *mf;

  modreader = reader;
  _mm_errno = 0;

  /* Try to find a loader that recognizes the module */
  for (l = firstloader; l; l = l->next)
    {
      _mm_rewind (modreader);
      if (l->Test ())
	break;
    }

  if (!l)
    {
      _mm_errno = MMERR_NOT_A_MODULE;
      _mm_rewind (modreader);
      return NULL;
    }

  /* init unitrk routines */
  if (!UniInit ())
    {
      _mm_rewind (modreader);
      return NULL;
    }

  /* load the song using the song's loader variable */
  memset (&of, 0, sizeof (MODULE));
  of.initvolume = 128;

  /* init panning array */
  for (t = 0; t < 64; t++)
    of.panning[t] = ((t + 1) & 2) ? 255 : 0;
  for (t = 0; t < 64; t++)
    of.chanvol[t] = 64;

  /* init module loader and load the header / patterns */
  if (l->Init ())
    {
      _mm_rewind (modreader);
      ok = l->Load (curious);
    }
  else
    ok = 0;

  /* free loader and unitrk allocations */
  l->Cleanup ();
  UniCleanup ();

  if (!ok)
    {
      ML_Free (&of);
      _mm_rewind (modreader);
      return NULL;
    }

  if (!ML_LoadSamples ())
    {
      ML_Free (&of);
      _mm_rewind (modreader);
      return NULL;
    }

  if (!(mf = ML_AllocUniMod ()))
    {
      ML_Free (&of);
      return NULL;
    }

  /* Copy the static MODULE contents into the dynamic MODULE struct. */
  memcpy (mf, &of, sizeof (MODULE));

  if (maxchan > 0)
    {
      if (!(mf->flags & UF_NNA) && (mf->numchn < maxchan))
	maxchan = mf->numchn;
      else if ((mf->numvoices) && (mf->numvoices < maxchan))
	maxchan = mf->numvoices;

      if (maxchan < mf->numchn)
	mf->flags |= UF_NNA;
    }
  if (SL_LoadSamples ())
    {
      ML_Free (mf);
      return NULL;
    }
  return mf;
}


void 
ML_RegisterAllLoaders (void)
{
  MLOADER *last = NULL;

  if (firstloader)
    return;

#define LOADER(fmt)	{ 			\
	extern MLOADER fmt; 			\
	if (!last)				\
		firstloader = &fmt;		\
	else					\
		last->next = &fmt;		\
	last = &fmt;				\
}


  /* Most likely first */
  LOADER (load_xm);
  LOADER (load_s3m);
  LOADER (load_mod);
  LOADER (load_it);

  /* Then the others in alphabetic order */
  LOADER (load_669);
  LOADER (load_amf);
  LOADER (load_dsm);
  LOADER (load_far);
  LOADER (load_gdm);
  LOADER (load_imf);
  LOADER (load_med);
  LOADER (load_mtm);
  LOADER (load_okt);
  LOADER (load_stm);
  LOADER (load_stx);
  LOADER (load_ult);
  LOADER (load_uni);

  /* must be last! */
  LOADER (load_m15);
}
/* ex:set ts=4: */
