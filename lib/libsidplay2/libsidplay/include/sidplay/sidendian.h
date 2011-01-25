/***************************************************************************
                          sidendian.h  -  Improtant endian functions
                             -------------------
    begin                : Mon Jul 3 2000
    copyright            : (C) 2000 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/***************************************************************************
 *  $Log: sidendian.h,v $
 *  Revision 1.5  2001/07/03 22:44:13  s_a_white
 *  Added endian_16 to convert a 16 bit value to an array of 8s.
 *
 *  Revision 1.4  2001/05/07 16:27:24  s_a_white
 *  Fix optimisation issues with gcc 2.96.
 *
 *  Revision 1.3  2001/03/25 19:46:12  s_a_white
 *  _endian_h_ changed to _sidendian_h_.
 *
 *  Revision 1.2  2001/03/10 19:49:32  s_a_white
 *  Removed bad include.
 *
 *  Revision 1.1  2001/03/02 19:04:38  s_a_white
 *  Include structure modified for better compatibility
 *
 *  Revision 1.5  2001/01/07 16:01:33  s_a_white
 *  sidendian.h is now installed, therefore endian defines updated.
 *
 *  Revision 1.4  2000/12/13 17:53:01  s_a_white
 *  Fixes some of the endian calls.
 *
 *  Revision 1.3  2000/12/12 19:39:15  s_a_white
 *  Removed bad const.
 *
 *  Revision 1.2  2000/12/11 19:10:59  s_a_white
 *  AC99 Update.
 *
 ***************************************************************************/

#ifndef _sidendian_h_
#define _sidendian_h_

// NOTE: The optimisations in this file rely on the structure of memory
// e.g. 2 shorts being contained in 1 long.  Although these sizes are
// checked to make sure the optimisation is ok, gcc 2.96 (and above)
// introduced better optimisations.  This results in caching of values
// in internal registers and therefore writes to ram through the aliases
// not being reflected in the CPU regs.  The use of the volatile keyword
// fixes this.

#include "sidtypes.h"

#if defined(SID_WORDS_BIGENDIAN)
/* byte-order: HIHI..3210..LO */
#elif defined(SID_WORDS_LITTLEENDIAN)
/* byte-order: LO..0123..HIHI */
#else
  #error Please check source code configuration!
#endif

/*
Labeling:
0 - LO
1 - HI
2 - HILO
3 - HIHI
*/

///////////////////////////////////////////////////////////////////
// INT16 FUNCTIONS
///////////////////////////////////////////////////////////////////
// Set the lo byte (8 bit) in a word (16 bit)
inline void endian_16lo8 (uint_least16_t &word, uint8_t byte)
{
#if defined(SID_OPTIMISE_MEMORY_ACCESS)
#   if defined(SID_WORDS_LITTLEENDIAN)
    ((volatile uint8_t *) &word)[0] = byte;
#   else
    ((volatile uint8_t *) &word)[1] = byte;
#   endif
    word = *((volatile uint_least16_t *) &word);
#else
    word &= 0xff00;
    word |= byte;
#endif
}

// Get the lo byte (8 bit) in a word (16 bit)
inline uint8_t endian_16lo8 (uint_least16_t word)
{
    return (uint8_t) word;
}

// Set the hi byte (8 bit) in a word (16 bit)
inline void endian_16hi8 (uint_least16_t &word, uint8_t byte)
{
#if defined(SID_OPTIMISE_MEMORY_ACCESS)
#   if defined(SID_WORDS_LITTLEENDIAN)
    ((volatile uint8_t *) &word)[1] = byte;
#   else
    ((volatile uint8_t *) &word)[0] = byte;
#   endif
    word = *((volatile uint_least16_t *) &word);
#else
    word &= 0x00ff;
    word |= (uint_least16_t) byte << 8;
#endif
}

// Set the hi byte (8 bit) in a word (16 bit)
inline uint8_t endian_16hi8 (uint_least16_t word)
{
#if defined(SID_OPTIMISE_MEMORY_ACCESS)
#   if defined(SID_WORDS_LITTLEENDIAN)
    return ((uint8_t *) &word)[1];
#   else
    return ((uint8_t *) &word)[0];
#   endif
#else
    return (uint8_t) (word >> 8);
#endif
}

// Swap word endian.
inline void endian_16swap8 (uint_least16_t &word)
{
    uint8_t lo = endian_16lo8 (word);
    uint8_t hi = endian_16hi8 (word);
    endian_16lo8 (word, hi);
    endian_16hi8 (word, lo);
}

// Convert high-byte and low-byte to 16-bit word.
inline uint_least16_t endian_16 (uint8_t hi, uint8_t lo)
{
    uint_least16_t word;
    endian_16lo8 (word, lo);
    endian_16hi8 (word, hi);
    return word;
}

// Convert high-byte and low-byte to 16-bit little endian word.
inline void endian_16 (uint8_t ptr[2], uint_least16_t word)
{
#if defined(SID_OPTIMISE_MEMORY_ACCESS)
    *((uint_least16_t *) ptr) = word;
#else
#   if defined(SID_WORDS_BIGENDIAN)
    ptr[0] = endian_16hi8 (word);
    ptr[1] = endian_16lo8 (word);
#   else
    ptr[0] = endian_16lo8 (word);
    ptr[1] = endian_16hi8 (word);
#   endif
#endif
}

inline void endian_16 (char ptr[2], uint_least16_t word)
{
	endian_16 ((uint8_t *) ptr, word);
}

// Convert high-byte and low-byte to 16-bit little endian word.
inline uint_least16_t endian_little16 (const uint8_t ptr[2])
{
#if defined(SID_OPTIMISE_MEMORY_ACCESS) && \
    defined(SID_WORDS_LITTLEENDIAN)
    return *((uint_least16_t *) ptr);
#else
    return endian_16 (ptr[1], ptr[0]);
#endif
}

// Write a little-endian 16-bit word to two bytes in memory.
inline void endian_little16 (uint8_t ptr[2], uint_least16_t word)
{
#if defined(SID_OPTIMISE_MEMORY_ACCESS) && \
    defined(SID_WORDS_LITTLEENDIAN)
    *((uint_least16_t *) ptr) = word;
#else
    ptr[0] = endian_16lo8 (word);
    ptr[1] = endian_16hi8 (word);
#endif
}

// Convert high-byte and low-byte to 16-bit big endian word.
inline uint_least16_t endian_big16 (const uint8_t ptr[2])
{
#if defined(SID_OPTIMISE_MEMORY_ACCESS) && \
    defined(SID_WORDS_BIGENDIAN)
    return *((uint_least16_t *) ptr);
#else
    return endian_16 (ptr[0], ptr[1]);
#endif
}

// Write a little-big 16-bit word to two bytes in memory.
inline void endian_big16 (uint8_t ptr[2], uint_least16_t word)
{
#if defined(SID_OPTIMISE_MEMORY_ACCESS) && \
    defined(SID_WORDS_BIGENDIAN)
    *((uint_least16_t *) ptr) = word;
#else
    ptr[0] = endian_16hi8 (word);
    ptr[1] = endian_16lo8 (word);
#endif
}


///////////////////////////////////////////////////////////////////
// INT32 FUNCTIONS
///////////////////////////////////////////////////////////////////
// Set the lo word (16bit) in a dword (32 bit)
inline void endian_32lo16 (uint_least32_t &dword, uint_least16_t word)
{
#if defined(SID_OPTIMISE_MEMORY_ACCESS)
#   if defined(SID_WORDS_LITTLEENDIAN)
    ((volatile uint_least16_t *) &dword)[0] = word;
#   else
    ((volatile uint_least16_t *) &dword)[1] = word;
#   endif
    dword = *((volatile uint_least32_t *) &dword);
#else
    dword &= (uint_least32_t) 0xffff0000;
    dword |= word;
#endif
}

// Get the lo word (16bit) in a dword (32 bit)
inline uint_least16_t endian_32lo16 (uint_least32_t dword)
{
    return (uint_least16_t) dword & 0xffff;
}

// Set the hi word (16bit) in a dword (32 bit)
inline void endian_32hi16 (uint_least32_t &dword, uint_least16_t word)
{
#if defined(SID_OPTIMISE_MEMORY_ACCESS)
#   if defined(SID_WORDS_LITTLEENDIAN)
    ((volatile uint_least16_t *) &dword)[1] = word;
#   else
    ((volatile uint_least16_t *) &dword)[0] = word;
#   endif
    dword = *((volatile uint_least32_t *) &dword);
#else
    dword &= (uint_least32_t) 0x0000ffff;
    dword |= (uint_least32_t) word << 16;
#endif
}

// Get the hi word (16bit) in a dword (32 bit)
inline uint_least16_t endian_32hi16 (uint_least32_t dword)
{
#if defined(SID_OPTIMISE_MEMORY_ACCESS)
#   if defined(SID_WORDS_LITTLEENDIAN)
    return ((uint_least16_t *) &dword)[1];
#   else
    return ((uint_least16_t *) &dword)[0];
#   endif
#else
    return (uint_least16_t) (dword >> 16);
#endif
}

// Set the lo byte (8 bit) in a dword (32 bit)
inline void endian_32lo8 (uint_least32_t &dword, uint8_t byte)
{
#if defined(SID_OPTIMISE_MEMORY_ACCESS)
#   if defined(SID_WORDS_LITTLEENDIAN)
    ((volatile uint8_t *) &dword)[0] = byte;
#   else
    ((volatile uint8_t *) &dword)[3] = byte;
#   endif
    dword = *((volatile uint_least32_t *) &dword);
#else
    dword &= (uint_least32_t) 0xffffff00;
    dword |= (uint_least32_t) byte;
#endif
}

// Get the lo byte (8 bit) in a dword (32 bit)
inline uint8_t endian_32lo8 (uint_least32_t dword)
{
    return (uint8_t) dword;
}

// Set the hi byte (8 bit) in a dword (32 bit)
inline void endian_32hi8 (uint_least32_t &dword, uint8_t byte)
{
#if defined(SID_OPTIMISE_MEMORY_ACCESS)
#   if defined(SID_WORDS_LITTLEENDIAN)
    ((volatile uint8_t *) &dword)[1] = byte;
#   else
    ((volatile uint8_t *) &dword)[2] = byte;
#   endif
    dword = *((volatile uint_least32_t *) &dword);
#else
    dword &= (uint_least32_t) 0xffff00ff;
    dword |= (uint_least32_t) byte << 8;
#endif
}

// Get the hi byte (8 bit) in a dword (32 bit)
inline uint8_t endian_32hi8 (uint_least32_t dword)
{
#if defined(SID_OPTIMISE_MEMORY_ACCESS)
#   if defined(SID_WORDS_LITTLEENDIAN)
    return ((uint8_t *) &dword)[1];
#   else
    return ((uint8_t *) &dword)[2];
#   endif
#else
    return (uint8_t) (dword >> 8);
#endif
}

// Swap hi and lo words endian in 32 bit dword.
inline void endian_32swap16 (uint_least32_t &dword)
{
    uint_least16_t lo = endian_32lo16 (dword);
    uint_least16_t hi = endian_32hi16 (dword);
    endian_32lo16 (dword, hi);
    endian_32hi16 (dword, lo);
}

// Swap word endian.
inline void endian_32swap8 (uint_least32_t &dword)
{
    uint_least16_t lo, hi;
    lo = endian_32lo16 (dword);
    hi = endian_32hi16 (dword);
    endian_16swap8 (lo);
    endian_16swap8 (hi);
    endian_32lo16 (dword, hi);
    endian_32hi16 (dword, lo);
}

// Convert high-byte and low-byte to 32-bit word.
inline uint_least32_t endian_32 (uint8_t hihi, uint8_t hilo, uint8_t hi, uint8_t lo)
{
    uint_least32_t dword;
    uint_least16_t word;
    endian_32lo8  (dword, lo);
    endian_32hi8  (dword, hi);
    endian_16lo8  (word,  hilo);
    endian_16hi8  (word,  hihi);
    endian_32hi16 (dword, word);
    return dword;
}

// Convert high-byte and low-byte to 32-bit little endian word.
inline uint_least32_t endian_little32 (const uint8_t ptr[4])
{
#if defined(SID_OPTIMISE_MEMORY_ACCESS) && \
    defined(SID_WORDS_LITTLEENDIAN)
    return *((uint_least32_t *) ptr);
#else
    return endian_32 (ptr[3], ptr[2], ptr[1], ptr[0]);
#endif
}

// Write a little-endian 32-bit word to four bytes in memory.
inline void endian_little32 (uint8_t ptr[4], uint_least32_t dword)
{
#if defined(SID_OPTIMISE_MEMORY_ACCESS) && \
    defined(SID_WORDS_LITTLEENDIAN)
    *((uint_least32_t *) ptr) = dword;
#else
    uint_least16_t word;
    ptr[0] = endian_32lo8  (dword);
    ptr[1] = endian_32hi8  (dword);
    word   = endian_32hi16 (dword);
    ptr[2] = endian_16lo8  (word);
    ptr[3] = endian_16hi8  (word);
#endif
}

// Convert high-byte and low-byte to 32-bit big endian word.
inline uint_least32_t endian_big32 (const uint8_t ptr[4])
{
#if defined(SID_OPTIMISE_MEMORY_ACCESS) && \
    defined(SID_WORDS_BIGENDIAN)
    return *((uint_least32_t *) ptr);
#else
    return endian_32 (ptr[0], ptr[1], ptr[2], ptr[3]);
#endif
}

// Write a big-endian 32-bit word to four bytes in memory.
inline void endian_big32 (uint8_t ptr[4], uint_least32_t dword)
{
#if defined(SID_OPTIMISE_MEMORY_ACCESS) && \
    defined(SID_WORDS_BIGENDIAN)
    *((uint_least32_t *) ptr) = dword;
#else
    uint_least16_t word;
    word   = endian_32hi16 (dword);
    ptr[1] = endian_16lo8  (word);
    ptr[0] = endian_16hi8  (word);
    ptr[2] = endian_32hi8  (dword);
    ptr[3] = endian_32lo8  (dword);
#endif
}

#endif // _sidendian_h_


