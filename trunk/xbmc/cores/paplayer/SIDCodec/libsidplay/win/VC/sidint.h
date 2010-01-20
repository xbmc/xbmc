/***************************************************************************
                          sidint.h  -  AC99 types
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

/* Setup for Microsoft Visual C++ Version 5 */
#ifndef _sidint_h_
#define _sidint_h_

#include "sidconfig.h"

/* Wanted: Exactly 8-bit unsigned/signed (1 byte). */
typedef signed char        int8_t;
typedef unsigned char      uint8_t;

/* Small types.  */
/* Wanted: Atleast 8-bit unsigned/signed (1 byte). */
typedef signed char        int_least8_t;
typedef unsigned char      uint_least8_t;

/* Wanted: Atleast 16-bit unsigned/signed (2 bytes). */
#if SID_SIZEOF_SHORT_INT >= 2
typedef short int          int_least16_t;
typedef unsigned short int uint_least16_t;
#else
typedef int                int_least16_t;
typedef unsigned int       uint_least16_t;
#endif /* SID_SIZEOF_SHORT_INT */

/* Wanted: Atleast 32-bit unsigned/signed (4 bytes). */
#if SID_SIZEOF_INT >= 4
typedef int                int_least32_t;
typedef unsigned int       uint_least32_t;
#else
typedef long int           uint_least32_t;
typedef unsigned long int  uint_least32_t;
#endif /* SID_SIZEOF_INT */

/* Wanted: Atleast 32-bits but final size is determined
 * on which register size will provide best program
 * performance
 */
typedef uint_least32_t     uint_fast32_t;

#endif /* _sidint_h_ */
