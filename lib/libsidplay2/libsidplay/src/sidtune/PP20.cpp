/*
 * /home/ms/files/source/libsidtune/RCS/PP20.cpp,v
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

#include "PP20.h"

#include <string.h>
#ifdef PP20_HAVE_EXCEPTIONS
#include <new>
#endif

/* Read a big-endian 32-bit word from four bytes in memory.
   No endian-specific optimizations applied. */
inline udword_ppt readBEdword(const ubyte_ppt ptr[4])
{
    return ( (((udword_ppt)ptr[0])<<24) + (((udword_ppt)ptr[1])<<16) +
             (((udword_ppt)ptr[2])<<8) + ((udword_ppt)ptr[3]) );
}

const char _pp20_txt_packeddatacorrupt[] = "PowerPacker: Packed data is corrupt";
const char _pp20_txt_unrecognized[]    = "PowerPacker: Unrecognized compression method";
const char _pp20_txt_uncompressed[] = "Not compressed with PowerPacker (PP20)";
const char _pp20_txt_notenoughmemory[] = "Not enough free memory";
const char _pp20_txt_fast[] = "PowerPacker: fast compression";
const char _pp20_txt_mediocre[] = "PowerPacker: mediocre compression";
const char _pp20_txt_good[] = "PowerPacker: good compression";
const char _pp20_txt_verygood[] = "PowerPacker: very good compression";
const char _pp20_txt_best[] = "PowerPacker: best compression";
const char _pp20_txt_na[] = "N/A";

const char* PP20::PP_ID = "PP20";

PP20::PP20()
{
    statusString = _pp20_txt_uncompressed;
}

bool PP20::isCompressed(const void* source, const udword_ppt size)
{
    // Check minimum input size, PP20 ID, and efficiency table.
    if ( size<8 )
    {
        return false;
    }
    // We hope that every file with a valid signature and a valid
    // efficiency table is PP-compressed actually.
    const char* idPtr = (const char*)source;
    if ( strncmp(idPtr,PP_ID,4) != 0 )
    {
        statusString = _pp20_txt_uncompressed;
        return false;
    }
    return checkEfficiency(idPtr+4);
}

bool PP20::checkEfficiency(const void* source)
{
    const udword_ppt PP_BITS_FAST = 0x09090909;
    const udword_ppt PP_BITS_MEDIOCRE = 0x090a0a0a;
    const udword_ppt PP_BITS_GOOD = 0x090a0b0b;
    const udword_ppt PP_BITS_VERYGOOD = 0x090a0c0c;
    const udword_ppt PP_BITS_BEST = 0x090a0c0d;

    // Copy efficiency table.
    memcpy(efficiency,(const ubyte_ppt*)source,4);
    udword_ppt eff = readBEdword(efficiency);
    if (( eff != PP_BITS_FAST ) &&
        ( eff != PP_BITS_MEDIOCRE ) &&
        ( eff != PP_BITS_GOOD ) &&
        ( eff != PP_BITS_VERYGOOD ) &&
        ( eff != PP_BITS_BEST ))
    {
        statusString = _pp20_txt_unrecognized;
        return false;
    }

    // Define string describing compression encoding used.
    switch ( eff)
    {
        case PP_BITS_FAST:
            statusString = _pp20_txt_fast;
            break;
         case PP_BITS_MEDIOCRE:
            statusString = _pp20_txt_mediocre;
            break;
         case PP_BITS_GOOD:
            statusString = _pp20_txt_good;
            break;
         case PP_BITS_VERYGOOD:
            statusString = _pp20_txt_verygood;
            break;
         case PP_BITS_BEST:
            statusString = _pp20_txt_best;
            break;
    }
    
    return true;
}

// Move four bytes to Motorola big-endian double-word.
inline void PP20::bytesTOdword()
{
    readPtr -= 4;
    if ( readPtr < sourceBeg )
    {
        statusString = _pp20_txt_packeddatacorrupt;
        globalError = true;
    }
    else 
    {
        current = readBEdword(readPtr);
    }
}

inline udword_ppt PP20::readBits(int count)
{
    udword_ppt data = 0;
    // read 'count' bits of packed data
    for (; count > 0; count--)
    {
        // equal to shift left
        data += data;
        // merge bit 0
        data |= (current&1);
        current >>= 1;
        if (--bits == 0)
        {
            bytesTOdword();
            bits = 32;
        }
    }
    return data;
}

inline void PP20::bytes()
{
    udword_ppt count, add;
    count = (add = readBits(2));
    while (add == 3)
    {
        add = readBits(2);
        count += add;
    }
    for ( ++count; count > 0 ; count--)
    {
        if (writePtr > destBeg)
        {
            *(--writePtr) = (ubyte_ppt)readBits(8);
        }
        else
        {
            statusString = _pp20_txt_packeddatacorrupt;
            globalError = true;
        }
    }
}

inline void PP20::sequence()
{
    udword_ppt offset, add;
    udword_ppt length = readBits(2);  // is length-2
    int offsetBitLen = (int)efficiency[length];
    length += 2;
    if ( length != 5 )
        offset = readBits( offsetBitLen );
    else
    {
        if ( readBits(1) == 0 )
            offsetBitLen = 7;
        offset = readBits( offsetBitLen );
        add = readBits(3);
        length += add;
        while ( add == 7 )
        {
            add = readBits(3);
            length += add;
        }
    }
    for ( ; length > 0 ; length-- )
    {
        if ( writePtr > destBeg )
        {
            --writePtr;
            *writePtr = *(writePtr+1+offset);
        }
        else
        {
            statusString = _pp20_txt_packeddatacorrupt;
            globalError = true;
        }
    }
}

udword_ppt PP20::decompress(const void* source, 
                                  udword_ppt size,
                                  ubyte_ppt** destRef)
{
    globalError = false;  // assume no error
    
    sourceBeg = (const ubyte_ppt*)source;
    readPtr = sourceBeg;

    if ( !isCompressed(readPtr,size) )
    {
        return 0;
    }
    
    // Uncompressed size is stored at end of source file.
    // Backwards decompression.
    readPtr += (size-4);

    udword_ppt lastDword = readBEdword(readPtr);
    // Uncompressed length in bits 31-8 of last dword.
    udword_ppt outputLen = lastDword>>8;
    
    // Allocate memory for output data.
    ubyte_ppt* dest;
#ifdef PP20_HAVE_EXCEPTIONS
    if (( dest = new(std::nothrow) ubyte_ppt[outputLen]) == 0 )
#else
    if (( dest = new ubyte_ppt[outputLen]) == 0 )
#endif
    {
        statusString = _pp20_txt_notenoughmemory;
        return 0;
    }
  
    // Lowest dest. address for range-checks.
    destBeg = dest;
    // Put destptr to end of uncompressed data.
    writePtr = dest+outputLen;

    // Read number of unused bits in 1st data dword
    // from lowest bits 7-0 of last dword.
    bits = 32 - (lastDword&0xFF);
    
    // Main decompression loop.
    bytesTOdword();
    if ( bits != 32 )
        current >>= (32-bits);
    do
    {
        if ( readBits(1) == 0 )  
            bytes();
        if ( writePtr > dest )  
            sequence();
        if ( globalError )
        {
            // statusString already set.
            outputLen = 0;  // unsuccessful decompression
            break;
        }
    } while ( writePtr > dest );

    // Finished.

    if (outputLen > 0)  // successful
    {
        // Free any previously existing destination buffer.
        if ( *destRef != 0 )
        {
            delete[] *destRef;
        }
        *destRef = dest;
    }
    else
    {
        delete[] dest;
    }

    return outputLen;
}
