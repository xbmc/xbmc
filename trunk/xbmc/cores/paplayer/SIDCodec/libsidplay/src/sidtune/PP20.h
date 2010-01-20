/*
 * /home/ms/files/source/libsidtune/RCS/PP20.h,v
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

#ifndef PP_DECOMPRESSOR_H
#define PP_DECOMPRESSOR_H

#include "PP20_Defs.h"

class PP20
{
 public:
    
    PP20();

    bool isCompressed(const void* source, const udword_ppt size);
    
    // If successful, allocates a new buffer containing the
    // uncompresse data and returns the uncompressed length.
    // Else, returns 0.
    udword_ppt decompress(const void* source, 
                                udword_ppt size,
                                ubyte_ppt** destRef);
    
    const char* getStatusString()  { return statusString; }
    
 private:
    bool checkEfficiency(const void* source);
    
    void bytesTOdword();
    udword_ppt readBits(int count);
    void bytes();
    void sequence();

    static const char* PP_ID;

    ubyte_ppt efficiency[4];
    
    const ubyte_ppt* sourceBeg;
    const ubyte_ppt* readPtr;
    
    const ubyte_ppt* destBeg;
    ubyte_ppt* writePtr;
    
    udword_ppt current;            // compressed data longword
    int bits;                   // number of bits in 'current' to evaluate
    
    bool globalError;           // exception-free version of code

    const char* statusString;
};

#endif  /* PP_DECOMPRESSOR_H */
