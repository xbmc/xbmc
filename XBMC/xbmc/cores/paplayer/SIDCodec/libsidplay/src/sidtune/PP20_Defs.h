/*
 * /home/ms/files/source/libsidtune/RCS/PP20_Defs.h,v
 *
 * PowerPacker (AMIGA) "PP20" format decompressor.
 * Copyright (C) Michael Schwendt <mschwendt@yahoo.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PP_DECOMPRESSOR_DEFS_H
#define PP_DECOMPRESSOR_DEFS_H

#include "sidtypes.h"

#ifdef HAVE_EXCEPTIONS
  #define PP20_HAVE_EXCEPTIONS
#else
  #undef PP20_HAVE_EXCEPTIONS
#endif

// Wanted: 8-bit unsigned.
typedef uint_least8_t ubyte_ppt;

// Wanted: 32-bit unsigned.
typedef uint_least32_t udword_ppt;

#endif  /* PP_DECOMPRESSOR_DEFS_H */
