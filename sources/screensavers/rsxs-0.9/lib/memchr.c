/* Copyright (C) 1991, 1993, 1996, 1997, 1999, 2000, 2003, 2004 Free
   Software Foundation, Inc.

   Based on strlen implementation by Torbjorn Granlund (tege@sics.se),
   with help from Dan Sahlin (dan@sics.se) and
   commentary by Jim Blandy (jimb@ai.mit.edu);
   adaptation to memchr suggested by Dick Karpinski (dick@cca.ucsf.edu),
   and implemented by Roland McGrath (roland@ai.mit.edu).

NOTE: The canonical source of this file is maintained with the GNU C Library.
Bugs can be reported to bug-glibc@prep.ai.mit.edu.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>

#include <stddef.h>

#if defined _LIBC
# include <memcopy.h>
#else
# define reg_char char
#endif

#include <limits.h>

#if HAVE_BP_SYM_H || defined _LIBC
# include <bp-sym.h>
#else
# define BP_SYM(sym) sym
#endif

#undef memchr
#undef __memchr

/* Search no more than N bytes of S for C.  */
void *
__memchr (void const *s, int c_in, size_t n)
{
  const unsigned char *char_ptr;
  const unsigned long int *longword_ptr;
  unsigned long int longword, magic_bits, charmask;
  unsigned reg_char c;
  int i;

  c = (unsigned char) c_in;

  /* Handle the first few characters by reading one character at a time.
     Do this until CHAR_PTR is aligned on a longword boundary.  */
  for (char_ptr = (const unsigned char *) s;
       n > 0 && (size_t) char_ptr % sizeof longword != 0;
       --n, ++char_ptr)
    if (*char_ptr == c)
      return (void *) char_ptr;

  /* All these elucidatory comments refer to 4-byte longwords,
     but the theory applies equally well to any size longwords.  */

  longword_ptr = (const unsigned long int *) char_ptr;

  /* Bits 31, 24, 16, and 8 of this number are zero.  Call these bits
     the "holes."  Note that there is a hole just to the left of
     each byte, with an extra at the end:

     bits:  01111110 11111110 11111110 11111111
     bytes: AAAAAAAA BBBBBBBB CCCCCCCC DDDDDDDD

     The 1-bits make sure that carries propagate to the next 0-bit.
     The 0-bits provide holes for carries to fall into.  */

  /* Set MAGIC_BITS to be this pattern of 1 and 0 bits.
     Set CHARMASK to be a longword, each of whose bytes is C.  */

  magic_bits = 0xfefefefe;
  charmask = c | (c << 8);
  charmask |= charmask << 16;
#if 0xffffffffU < ULONG_MAX
  magic_bits |= magic_bits << 32;
  charmask |= charmask << 32;
  if (8 < sizeof longword)
    for (i = 64; i < sizeof longword * 8; i *= 2)
      {
	magic_bits |= magic_bits << i;
	charmask |= charmask << i;
      }
#endif
  magic_bits = (ULONG_MAX >> 1) & (magic_bits | 1);

  /* Instead of the traditional loop which tests each character,
     we will test a longword at a time.  The tricky part is testing
     if *any of the four* bytes in the longword in question are zero.  */
  while (n >= sizeof longword)
    {
      /* We tentatively exit the loop if adding MAGIC_BITS to
	 LONGWORD fails to change any of the hole bits of LONGWORD.

	 1) Is this safe?  Will it catch all the zero bytes?
	 Suppose there is a byte with all zeros.  Any carry bits
	 propagating from its left will fall into the hole at its
	 least significant bit and stop.  Since there will be no
	 carry from its most significant bit, the LSB of the
	 byte to the left will be unchanged, and the zero will be
	 detected.

	 2) Is this worthwhile?  Will it ignore everything except
	 zero bytes?  Suppose every byte of LONGWORD has a bit set
	 somewhere.  There will be a carry into bit 8.  If bit 8
	 is set, this will carry into bit 16.  If bit 8 is clear,
	 one of bits 9-15 must be set, so there will be a carry
	 into bit 16.  Similarly, there will be a carry into bit
	 24.  If one of bits 24-30 is set, there will be a carry
	 into bit 31, so all of the hole bits will be changed.

	 The one misfire occurs when bits 24-30 are clear and bit
	 31 is set; in this case, the hole at bit 31 is not
	 changed.  If we had access to the processor carry flag,
	 we could close this loophole by putting the fourth hole
	 at bit 32!

	 So it ignores everything except 128's, when they're aligned
	 properly.

	 3) But wait!  Aren't we looking for C, not zero?
	 Good point.  So what we do is XOR LONGWORD with a longword,
	 each of whose bytes is C.  This turns each byte that is C
	 into a zero.  */

      longword = *longword_ptr++ ^ charmask;

      /* Add MAGIC_BITS to LONGWORD.  */
      if ((((longword + magic_bits)

	    /* Set those bits that were unchanged by the addition.  */
	    ^ ~longword)

	   /* Look at only the hole bits.  If any of the hole bits
	      are unchanged, most likely one of the bytes was a
	      zero.  */
	   & ~magic_bits) != 0)
	{
	  /* Which of the bytes was C?  If none of them were, it was
	     a misfire; continue the search.  */

	  const unsigned char *cp = (const unsigned char *) (longword_ptr - 1);

	  if (cp[0] == c)
	    return (void *) cp;
	  if (cp[1] == c)
	    return (void *) &cp[1];
	  if (cp[2] == c)
	    return (void *) &cp[2];
	  if (cp[3] == c)
	    return (void *) &cp[3];
	  if (4 < sizeof longword && cp[4] == c)
	    return (void *) &cp[4];
	  if (5 < sizeof longword && cp[5] == c)
	    return (void *) &cp[5];
	  if (6 < sizeof longword && cp[6] == c)
	    return (void *) &cp[6];
	  if (7 < sizeof longword && cp[7] == c)
	    return (void *) &cp[7];
	  if (8 < sizeof longword)
	    for (i = 8; i < sizeof longword; i++)
	      if (cp[i] == c)
		return (void *) &cp[i];
	}

      n -= sizeof longword;
    }

  char_ptr = (const unsigned char *) longword_ptr;

  while (n-- > 0)
    {
      if (*char_ptr == c)
	return (void *) char_ptr;
      else
	++char_ptr;
    }

  return 0;
}
#ifdef weak_alias
weak_alias (__memchr, BP_SYM (memchr))
#endif
