/*
 * Copyright (C) 1999-2010 The L.A.M.E. project
 *
 * Initially written by Michael Hipp, see also AUTHORS and README.
 *  
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef INTERFACE_H_INCLUDED
#define INTERFACE_H_INCLUDED

#ifdef __cplusplus
extern  "C" {
#endif

#include "common.h"

    int     InitMP3(PMPSTR mp);
    int     decodeMP3(PMPSTR mp, unsigned char *inmemory, int inmemsize, char *outmemory,
                      int outmemsize, int *done);
    void    ExitMP3(PMPSTR mp);

/* added decodeMP3_unclipped to support returning raw floating-point values of samples. The representation
   of the floating-point numbers is defined in mpg123.h as #define real. It is 64-bit double by default. 
   No more than 1152 samples per channel are allowed. */
    int     decodeMP3_unclipped(PMPSTR mp, unsigned char *inmemory, int inmemsize, char *outmemory,
                                int outmemsize, int *done);

/* added remove_buf to support mpglib seeking */
    void    remove_buf(PMPSTR mp);

/* added audiodata_precedesframes to return the number of bitstream frames the audio data will precede the 
   current frame by for Layer 3 data. Aids seeking.
 */
    int     audiodata_precedesframes(PMPSTR mp);

/* Resets decoding. Aids seeking. */
    void    decode_reset(PMPSTR mp);

#ifdef __cplusplus
}
#endif
#endif
