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

  $Id: munitrk.c,v 1.27 1999/10/25 16:31:41 miod Exp $

  All routines dealing with the manipulation of UNITRK streams

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "unimod_priv.h"

#include <string.h>

/* Unibuffer chunk size */
#define BUFPAGE  128

UWORD unioperands[UNI_LAST] =
{
  0,				/* not used */
  1,				/* UNI_NOTE */
  1,				/* UNI_INSTRUMENT */
  1,				/* UNI_PTEFFECT0 */
  1,				/* UNI_PTEFFECT1 */
  1,				/* UNI_PTEFFECT2 */
  1,				/* UNI_PTEFFECT3 */
  1,				/* UNI_PTEFFECT4 */
  1,				/* UNI_PTEFFECT5 */
  1,				/* UNI_PTEFFECT6 */
  1,				/* UNI_PTEFFECT7 */
  1,				/* UNI_PTEFFECT8 */
  1,				/* UNI_PTEFFECT9 */
  1,				/* UNI_PTEFFECTA */
  1,				/* UNI_PTEFFECTB */
  1,				/* UNI_PTEFFECTC */
  1,				/* UNI_PTEFFECTD */
  1,				/* UNI_PTEFFECTE */
  1,				/* UNI_PTEFFECTF */
  1,				/* UNI_S3MEFFECTA */
  1,				/* UNI_S3MEFFECTD */
  1,				/* UNI_S3MEFFECTE */
  1,				/* UNI_S3MEFFECTF */
  1,				/* UNI_S3MEFFECTI */
  1,				/* UNI_S3MEFFECTQ */
  1,				/* UNI_S3MEFFECTR */
  1,				/* UNI_S3MEFFECTT */
  1,				/* UNI_S3MEFFECTU */
  0,				/* UNI_KEYOFF */
  1,				/* UNI_KEYFADE */
  2,				/* UNI_VOLEFFECTS */
  1,				/* UNI_XMEFFECT4 */
  1,				/* UNI_XMEFFECTA */
  1,				/* UNI_XMEFFECTE1 */
  1,				/* UNI_XMEFFECTE2 */
  1,				/* UNI_XMEFFECTEA */
  1,				/* UNI_XMEFFECTEB */
  1,				/* UNI_XMEFFECTG */
  1,				/* UNI_XMEFFECTH */
  1,				/* UNI_XMEFFECTL */
  1,				/* UNI_XMEFFECTP */
  1,				/* UNI_XMEFFECTX1 */
  1,				/* UNI_XMEFFECTX2 */
  1,				/* UNI_ITEFFECTG */
  1,				/* UNI_ITEFFECTH */
  1,				/* UNI_ITEFFECTI */
  1,				/* UNI_ITEFFECTM */
  1,				/* UNI_ITEFFECTN */
  1,				/* UNI_ITEFFECTP */
  1,				/* UNI_ITEFFECTT */
  1,				/* UNI_ITEFFECTU */
  1,				/* UNI_ITEFFECTW */
  1,				/* UNI_ITEFFECTY */
  2,				/* UNI_ITEFFECTZ */
  1,				/* UNI_ITEFFECTS0 */
  2,				/* UNI_ULTEFFECT9 */
  2,				/* UNI_MEDSPEED */
  0,				/* UNI_MEDEFFECTF1 */
  0,				/* UNI_MEDEFFECTF2 */
  0				/* UNI_MEDEFFECTF3 */
};

/* Sparse description of the internal module format
   ------------------------------------------------

   A UNITRK stream is an array of bytes representing a single track of a pattern.
   It's made up of 'repeat/length' bytes, opcodes and operands (sort of a assembly
   language):

   rrrlllll
   [REP/LEN][OPCODE][OPERAND][OPCODE][OPERAND] [REP/LEN][OPCODE][OPERAND]..
   ^                                         ^ ^
   |-------ROWS 0 - 0+REP of a track---------| |-------ROWS xx - xx+REP of a track...

   The rep/len byte contains the number of bytes in the current row, _including_
   the length byte itself (So the LENGTH byte of row 0 in the previous example
   would have a value of 5). This makes it easy to search through a stream for a
   particular row. A track is concluded by a 0-value length byte.

   The upper 3 bits of the rep/len byte contain the number of times -1 this row
   is repeated for this track. (so a value of 7 means this row is repeated 8 times)

   Opcodes can range from 1 to 255 but currently only opcodes 1 to 52 are being
   used. Each opcode can have a different number of operands. You can find the
   number of operands to a particular opcode by using the opcode as an index into
   the 'unioperands' table.

 */

/*========== Reading routines */

static UBYTE *rowstart;		/* startadress of a row */
static UBYTE *rowend;		/* endaddress of a row (exclusive) */
static UBYTE *rowpc;		/* current unimod(tm) programcounter */


void 
UniSetRow (UBYTE * t)
{
  rowstart = t;
  rowpc = rowstart;
  rowend = t ? rowstart + (*(rowpc++) & 0x1f) : t;
}

UBYTE 
UniGetByte (void)
{
  return (rowpc < rowend) ? *(rowpc++) : 0;
}

UWORD 
UniGetWord (void)
{
  return ((UWORD) UniGetByte () << 8) | UniGetByte ();
}

void 
UniSkipOpcode (UBYTE op)
{
  if (op < UNI_LAST)
    {
      UWORD t = unioperands[op];

      while (t--)
	UniGetByte ();
    }
}

/* Finds the address of row number 'row' in the UniMod(tm) stream 't' returns
   NULL if the row can't be found. */
UBYTE *
UniFindRow (UBYTE * t, UWORD row)
{
  UBYTE c, l;

  if (t)
    while (1)
      {
	c = *t;			/* get rep/len byte */
	if (!c)
	  return NULL;		/* zero ? -> end of track.. */
	l = (c >> 5) + 1;	/* extract repeat value */
	if (l > row)
	  break;		/* reached wanted row? -> return pointer */
	row -= l;		/* haven't reached row yet.. update row */
	t += c & 0x1f;		/* point t to the next row */
      }
  return t;
}

/*========== Writing routines */

static UBYTE *unibuf;		/* pointer to the temporary unitrk buffer */
static UWORD unimax;		/* buffer size */

static UWORD unipc;		/* buffer cursor */
static UWORD unitt;		/* current row index */
static UWORD lastp;		/* previous row index */

/* Resets index-pointers to create a new track. */
void 
UniReset (void)
{
  unitt = 0;			/* reset index to rep/len byte */
  unipc = 1;			/* first opcode will be written to index 1 */
  lastp = 0;			/* no previous row yet */
  unibuf[0] = 0;		/* clear rep/len byte */
}

/* Expands the buffer */
static BOOL 
UniExpand (int wanted)
{
  if ((unipc + wanted) >= unimax)
    {
      UBYTE *newbuf;

      /* Expand the buffer by BUFPAGE bytes */
      newbuf = (UBYTE *) realloc (unibuf, (unimax + BUFPAGE) * sizeof (UBYTE));

      /* Check if realloc succeeded */
      if (newbuf)
	{
	  unibuf = newbuf;
	  unimax += BUFPAGE;
	  return 1;
	}
      else
	return 0;
    }
  return 1;
}

/* Appends one byte of data to the current row of a track. */
void 
UniWriteByte (UBYTE data)
{
  if (UniExpand (1))
    /* write byte to current position and update */
    unibuf[unipc++] = data;
}

void 
UniWriteWord (UWORD data)
{
  if (UniExpand (2))
    {
      unibuf[unipc++] = data >> 8;
      unibuf[unipc++] = data & 0xff;
    }
}

static BOOL 
MyCmp (UBYTE * a, UBYTE * b, UWORD l)
{
  UWORD t;

  for (t = 0; t < l; t++)
    if (*(a++) != *(b++))
      return 0;
  return 1;
}

/* Closes the current row of a unitrk stream (updates the rep/len byte) and sets
   pointers to start a new row. */
void 
UniNewline (void)
{
  UWORD n, l, len;

  n = (unibuf[lastp] >> 5) + 1;	/* repeat of previous row */
  l = (unibuf[lastp] & 0x1f);	/* length of previous row */

  len = unipc - unitt;		/* length of current row */

  /* Now, check if the previous and the current row are identical.. when they
     are, just increase the repeat field of the previous row */
  if (n < 8 && len == l && MyCmp (&unibuf[lastp + 1], &unibuf[unitt + 1], len - 1))
    {
      unibuf[lastp] += 0x20;
      unipc = unitt + 1;
    }
  else
    {
      if (UniExpand (unitt - unipc))
	{
	  /* current and previous row aren't equal... update the pointers */
	  unibuf[unitt] = len;
	  lastp = unitt;
	  unitt = unipc++;
	}
    }
}

/* Terminates the current unitrk stream and returns a pointer to a copy of the
   stream. */
UBYTE *
UniDup (void)
{
  UBYTE *d;

  if (!UniExpand (unitt - unipc))
    return NULL;
  unibuf[unitt] = 0;

  if (!(d = (UBYTE *) _mm_malloc (unipc)))
    return NULL;
  memcpy (d, unibuf, unipc);

  return d;
}

BOOL 
UniInit (void)
{
  unimax = BUFPAGE;

  if (!(unibuf = (UBYTE *) _mm_malloc (unimax * sizeof (UBYTE))))
    return 0;
  return 1;
}

void 
UniCleanup (void)
{
  if (unibuf)
    free (unibuf);
  unibuf = NULL;
}

/* ex:set ts=4: */
