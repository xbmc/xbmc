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

  $Id: unimod_priv.h,v 1.23 1999/10/25 16:31:41 miod Exp $

  MikMod sound library internal definitions

==============================================================================*/

#ifndef _UNIMOD_PRIV_H
#define _UNIMOD_PRIV_H

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(HAVE_MALLOC_H) && !defined(_MSC_VER) && !defined(__BORLANDC__) && !defined(__WATCOMC__)
#include <malloc.h>
#endif
#include <stdarg.h>
#if defined(__OS2__)||defined(__EMX__)||defined(__W32__)
#ifndef strcasecmp
#define strcasecmp(s,t) stricmp(s,t)
#endif
#endif

#include "unimod.h"
#include "url.h"

#ifdef __W32__
#pragma warning(disable:4761)
#endif

/*========== Error handling */

#define _mm_errno ML_errno

/*========== Memory allocation */

extern void *_mm_malloc (size_t);
extern void *_mm_calloc (size_t, size_t);
#define _mm_free(p) do { if (p) free(p); p = NULL; } while(0)

/*========== Portable file I/O */

#define _mm_read_SBYTE(x)	((SBYTE)url_getc(x))
#define _mm_read_UBYTE(x)	((UBYTE)url_getc(x))
#define _mm_read_SBYTES(x,y,z)	url_nread(z,(void *)x,y)
#define _mm_read_UBYTES(x,y,z)	url_nread(z,(void *)x,y)
#define _mm_fseek(x,y,z)	url_seek(x,y,z)
#define _mm_ftell(x)		url_tell(x)
#define _mm_eof(x)		url_eof(x)
#define _mm_rewind(x)		_mm_fseek(x,0,SEEK_SET)

extern int _mm_read_string (CHAR *, int, URL);

extern SWORD _mm_read_M_SWORD (URL);
extern SWORD _mm_read_I_SWORD (URL);
extern UWORD _mm_read_M_UWORD (URL);
extern UWORD _mm_read_I_UWORD (URL);

extern SLONG _mm_read_M_SLONG (URL);
extern SLONG _mm_read_I_SLONG (URL);
extern ULONG _mm_read_M_ULONG (URL);
extern ULONG _mm_read_I_ULONG (URL);

extern int _mm_read_M_SWORDS (SWORD *, int, URL);
extern int _mm_read_I_SWORDS (SWORD *, int, URL);
extern int _mm_read_M_UWORDS (UWORD *, int, URL);
extern int _mm_read_I_UWORDS (UWORD *, int, URL);

extern int _mm_read_M_SLONGS (SLONG *, int, URL);
extern int _mm_read_I_SLONGS (SLONG *, int, URL);
extern int _mm_read_M_ULONGS (ULONG *, int, URL);
extern int _mm_read_I_ULONGS (ULONG *, int, URL);


/*========== Loaders */

typedef struct MLOADER
{
  struct MLOADER *next;
  CHAR *type;
  CHAR *version;
  BOOL (*Init) (void);
  BOOL (*Test) (void);
  BOOL (*Load) (BOOL);
  void (*Cleanup) (void);
  CHAR *(*LoadTitle) (void);
}
MLOADER;

/* internal loader variables: */
extern URL modreader;
extern UWORD finetune[16];
extern MODULE of;		/* static unimod loading space */

extern SBYTE remap[64];	/* for removing empty channels */
extern UBYTE *poslookup;	/* lookup table for pattern jumps after
				 blank pattern removal */
extern UBYTE poslookupcnt;
extern UWORD *origpositions;

extern BOOL filters;		/* resonant filters in use */
extern UBYTE activemacro;	/* active midi macro number for Sxx */
extern UBYTE filtermacros[16];	/* midi macros settings */
extern FILTER filtersettings[256];	/* computed filter settings */

extern int *noteindex;

/* tracker identifiers */
#define STM_NTRACKERS 3
extern CHAR *STM_Signatures[];
extern CHAR *STM_Version[];


/*========== Internal loader interface */

extern BOOL ReadComment (UWORD);
extern BOOL ReadLinedComment (UWORD, UWORD);
extern BOOL AllocPositions (int);
extern BOOL AllocPatterns (void);
extern BOOL AllocTracks (void);
extern BOOL AllocInstruments (void);
extern BOOL AllocSamples (void);
extern CHAR *DupStr (CHAR *, UWORD, BOOL);

/* loader utility functions */
extern int *AllocLinear (void);
extern void FreeLinear (void);
extern int speed_to_finetune (ULONG, int);
extern void S3MIT_ProcessCmd (UBYTE, UBYTE, BOOL);
extern void S3MIT_CreateOrders (BOOL);

#ifdef __cplusplus
}
#endif

#endif

/* ex:set ts=4: */
