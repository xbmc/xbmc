/***************************************************************************
                          sidtypes.h  -  type definition file
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

#ifndef _sidtypes_h_
#define _sidtypes_h_

#include "sidint.h"
#include "sidconfig.h"

#if SID_SIZEOF_CHAR == 1
#   if (SID_SIZEOF_SHORT_INT == 2) || (SID_SIZEOF_INT == 2)
#       if (SID_SIZEOF_INT == 4) || (SID_SIZEOF_LONG_INT == 4)
//#           define SID_OPTIMISE_MEMORY_ACCESS
#       endif
#   endif
#endif

#if SID_SIZEOF_CHAR != 1
#   error Code cannot work correctly on this platform as no real 8 bit data type supported!
#endif

#ifndef SID_HAVE_BOOL
#   ifdef SID_HAVE_STDBOOL_H
#       include <stdbool.h>
#   else
        typedef int   bool;
#       define  true  1
#       define  false 0
#   endif /* SID_HAVE_STDBOOL_H */
#endif /* HAVE_BOOL */

/* Custom types */
typedef int sid_fc_t[2];
typedef struct
{
    sid_fc_t       cutoff[0x800];
    uint_least16_t points;
} sid_filter_t;
#define sid_filter_t sid_filter_t

typedef unsigned int uint;
typedef float    float32_t;
typedef double   float64_t;

#define SID_FOREVER for(;;)
#define SID_SWAP(x,y) ((x)^=(y)^=(x)^=(y))

#endif /* _sidtypes_h_ */
