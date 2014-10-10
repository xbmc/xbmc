/* Iterating through multibyte strings: macros for multi-byte encodings.
   Copyright (C) 2001, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by Bruno Haible <bruno@clisp.org>.  */

/* The macros in this file implement forward iteration through a
   multi-byte string, without knowing its length a-priori.

   With these macros, an iteration loop that looks like

      char *iter;
      for (iter = buf; *iter != '\0'; iter++)
        {
          do_something (*iter);
        }

   becomes

      mbui_iterator_t iter;
      for (mbui_init (iter, buf); mbui_avail (iter); mbui_advance (iter))
        {
          do_something (mbui_cur_ptr (iter), mb_len (mbui_cur (iter)));
        }

   The benefit of these macros over plain use of mbrtowc is:
   - Handling of invalid multibyte sequences is possible without
     making the code more complicated, while still preserving the
     invalid multibyte sequences.

   Compared to mbiter.h, the macros here don't need to know the string's
   length a-priori.  The downside is that at each step, the look-ahead
   that guards against overrunning the terminating '\0' is more expensive.
   The mbui_* macros are therefore suitable when there is a high probability
   that only the first few multibyte characters need to be inspected.
   Whereas the mbi_* macros are better if usually the iteration runs
   through the entire string.

   mbui_iterator_t
     is a type usable for variable declarations.

   mbui_init (iter, startptr)
     initializes the iterator, starting at startptr.

   mbui_avail (iter)
     returns true if there are more multibyte chracters available before
     the end of string is reached. In this case, mbui_cur (iter) is
     initialized to the next multibyte chracter.

   mbui_advance (iter)
     advances the iterator by one multibyte character.

   mbui_cur (iter)
     returns the current multibyte character, of type mbchar_t.  All the
     macros defined in mbchar.h can be used on it.

   mbui_cur_ptr (iter)
     return a pointer to the beginning of the current multibyte character.

   mbui_reloc (iter, ptrdiff)
     relocates iterator when the string is moved by ptrdiff bytes.

   Here are the function prototypes of the macros.

   extern void		mbui_init (mbui_iterator_t iter, const char *startptr);
   extern bool		mbui_avail (mbui_iterator_t iter);
   extern void		mbui_advance (mbui_iterator_t iter);
   extern mbchar_t	mbui_cur (mbui_iterator_t iter);
   extern const char *	mbui_cur_ptr (mbui_iterator_t iter);
   extern void		mbui_reloc (mbui_iterator_t iter, ptrdiff_t ptrdiff);
 */

#ifndef _MBUITER_H
#define _MBUITER_H 1

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

/* Tru64 with Desktop Toolkit C has a bug: <stdio.h> must be included before
   <wchar.h>.
   BSD/OS 4.1 has a bug: <stdio.h> and <time.h> must be included before
   <wchar.h>.  */
#include <stdio.h>
#include <time.h>
#include <wchar.h>

#include "mbchar.h"
#include "strnlen1.h"

struct mbuiter_multi
{
  bool in_shift;	/* true if next byte may not be interpreted as ASCII */
  mbstate_t state;	/* if in_shift: current shift state */
  bool next_done;	/* true if mbui_avail has already filled the following */
  struct mbchar cur;	/* the current character:
	const char *cur.ptr		pointer to current character
	The following are only valid after mbui_avail.
	size_t cur.bytes		number of bytes of current character
	bool cur.wc_valid		true if wc is a valid wide character
	wchar_t cur.wc			if wc_valid: the current character
	*/
};

static inline void
mbuiter_multi_next (struct mbuiter_multi *iter)
{
  if (iter->next_done)
    return;
  if (iter->in_shift)
    goto with_shift;
  /* Handle most ASCII characters quickly, without calling mbrtowc().  */
  if (is_basic (*iter->cur.ptr))
    {
      /* These characters are part of the basic character set.  ISO C 99
	 guarantees that their wide character code is identical to their
	 char code.  */
      iter->cur.bytes = 1;
      iter->cur.wc = *iter->cur.ptr;
      iter->cur.wc_valid = true;
    }
  else
    {
      assert (mbsinit (&iter->state));
      iter->in_shift = true;
    with_shift:
      iter->cur.bytes = mbrtowc (&iter->cur.wc, iter->cur.ptr,
				 strnlen1 (iter->cur.ptr, MB_CUR_MAX),
				 &iter->state);
      if (iter->cur.bytes == (size_t) -1)
	{
	  /* An invalid multibyte sequence was encountered.  */
	  iter->cur.bytes = 1;
	  iter->cur.wc_valid = false;
	  /* Whether to set iter->in_shift = false and reset iter->state
	     or not is not very important; the string is bogus anyway.  */
	}
      else if (iter->cur.bytes == (size_t) -2)
	{
	  /* An incomplete multibyte character at the end.  */
	  iter->cur.bytes = strlen (iter->cur.ptr);
	  iter->cur.wc_valid = false;
	  /* Whether to set iter->in_shift = false and reset iter->state
	     or not is not important; the string end is reached anyway.  */
	}
      else
	{
	  if (iter->cur.bytes == 0)
	    {
	      /* A null wide character was encountered.  */
	      iter->cur.bytes = 1;
	      assert (*iter->cur.ptr == '\0');
	      assert (iter->cur.wc == 0);
	    }
	  iter->cur.wc_valid = true;

	  /* When in the initial state, we can go back treating ASCII
	     characters more quickly.  */
	  if (mbsinit (&iter->state))
	    iter->in_shift = false;
	}
    }
  iter->next_done = true;
}

static inline void
mbuiter_multi_reloc (struct mbuiter_multi *iter, ptrdiff_t ptrdiff)
{
  iter->cur.ptr += ptrdiff;
}

/* Iteration macros.  */
typedef struct mbuiter_multi mbui_iterator_t;
#define mbui_init(iter, startptr) \
  ((iter).cur.ptr = (startptr), \
   (iter).in_shift = false, memset (&(iter).state, '\0', sizeof (mbstate_t)), \
   (iter).next_done = false)
#define mbui_avail(iter) \
  (mbuiter_multi_next (&(iter)), !mb_isnul ((iter).cur))
#define mbui_advance(iter) \
  ((iter).cur.ptr += (iter).cur.bytes, (iter).next_done = false)

/* Access to the current character.  */
#define mbui_cur(iter) (iter).cur
#define mbui_cur_ptr(iter) (iter).cur.ptr

/* Relocation.  */
#define mbui_reloc(iter, ptrdiff) mbuiter_multi_reloc (&iter, ptrdiff)

#endif /* _MBUITER_H */
