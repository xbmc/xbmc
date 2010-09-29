/********************************************************************
*    
* Copyright (c) 2002 Artur Polaczynski (Ar't)  All rights reserved.
*            <artii@o2.pl>        LGPL-2.1
*       $ArtId: info_mac.c,v 1.15 2003/04/13 11:24:10 art Exp $
********************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_io.h"
#include "info_mac.h"
#include "is_tag.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

#define MAC_FORMAT_FLAG_8_BIT                 1    // 8-bit wave
#define MAC_FORMAT_FLAG_CRC                   2    // new CRC32 error detection
#define MAC_FORMAT_FLAG_HAS_PEAK_LEVEL        4    // u-long Peak_Level after the header
#define MAC_FORMAT_FLAG_24_BIT                8    // 24-bit wave
#define MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS    16    // number of seek elements after the peak level
#define MAC_FORMAT_FLAG_CREATE_WAV_HEADER    32    // wave header not stored

struct macHeader {
    char             id[4];               // should equal 'MAC '
    unsigned short   ver;                 // version number * 1000 (3.81 = 3810)
    unsigned short   compLevel;           // the compression level
    unsigned short   formatFlags;         // any format flags (for future use)
    unsigned short   channels;            // the number of channels (1 or 2)
    unsigned long    sampleRate;          // the sample rate (typically 44100)
    unsigned long    headerBytesWAV;      // the bytes after the MAC header that compose the WAV header
    unsigned long    terminatingBytesWAV; // the bytes after that raw data (for extended info)
    unsigned long    totalFrames;         // the number of frames in the file
    unsigned long    finalFrameBlocks;    // the number of samples in the final frame
    unsigned long    peakLevel;
    unsigned short   seekElements;
};
    

// local prototypes
static int 
monkey_samples_per_frame(unsigned int versionid, unsigned int compressionlevel);
static const char *
monkey_stringify(unsigned int profile);

static const char *
monkey_stringify(unsigned int profile)
{
        static const char na[] = "unknown";
        static const char *Names[] = {
                na, "Fast", "Normal", "High", "Extra-High", "Insane"
        };
        unsigned int profile2 = profile/1000;

        return (profile2 >= sizeof (Names) / sizeof (*Names)) ? na : Names[(profile2)];
}


static int 
monkey_samples_per_frame(unsigned int versionid, unsigned int compressionlevel) 
{
    if (versionid >= 3950) {
        return 294912; // 73728 * 4
    } else if (versionid >= 3900) {
        return 73728;
    } else if ((versionid >= 3800) && (compressionlevel == COMPRESSION_LEVEL_EXTRA_HIGH)) {
        return 73728;
    } else {
        return 9216;
    }
}    
    
/*
    return 0; Info has all info
    return 1; File not found
    return 2; no MAC file
*/
int
info_mac_read(const char *fn, StreamInfoMac * Info)
{
    unsigned int HeaderData[32];
    ape_file *tmpFile = NULL;
    long SkipSizeID3;
    struct macHeader * header;
    
    // load file
    tmpFile = ape_fopen(fn, "rb");
        
    if (tmpFile == NULL) 
        return 1;    // file not found or read-protected
    
    // skip id3v2 
    SkipSizeID3 = is_id3v2(tmpFile);
    ape_fseek(tmpFile, SkipSizeID3, SEEK_SET);
    ape_fread((void *) HeaderData, sizeof (int), 16, tmpFile);
    ape_fseek(tmpFile, 0, SEEK_END);
    Info->FileSize = ape_ftell(tmpFile);
    ape_fclose(tmpFile);
    
    if (0 != memcmp(HeaderData, "MAC", 3))
        return 2; // no monkeyAudio file
    
    header= (struct macHeader *) HeaderData;
    
    Info->Version         = Info->EncoderVersion = header->ver;
    Info->Channels        = header->channels;
    Info->SampleFreq      = header->sampleRate;
    Info->Flags           = header->formatFlags;
    Info->SamplesPerFrame = monkey_samples_per_frame(header->ver, header->compLevel);
    Info->BitsPerSample   = (header->formatFlags & MAC_FORMAT_FLAG_8_BIT) 
        ? 8 : ((header->formatFlags & MAC_FORMAT_FLAG_24_BIT) ? 24 : 16);
    
    Info->PeakLevel       = header->peakLevel;
//    Info->PeakRatio       = Info->PakLevel / pow(2, Info->bitsPerSample - 1);
    Info->Frames          = header->totalFrames;
    Info->Samples         = (Info->Frames - 1) * Info->SamplesPerFrame + 
        header->finalFrameBlocks;
    
    Info->Duration        = Info->SampleFreq > 0 ? 
        (int)(((float)Info->Samples / Info->SampleFreq)*1000) : 0;
    
    Info->Compresion      = header->compLevel;
    Info->CompresionName  = monkey_stringify(Info->Compresion);
    
    Info->UncompresedSize = Info->Samples * Info->Channels * 
        (Info->BitsPerSample / 8);
    
    Info->CompresionRatio = 
        (Info->UncompresedSize + header->headerBytesWAV) > 0 ?
        Info->FileSize / (float) (Info->UncompresedSize + 
        header->headerBytesWAV) : 0.0f ;
    
    Info->Bitrate         = Info->Duration > 0 ? (int)((((Info->Samples * 
        Info->Channels * Info->BitsPerSample) / (float) Info->Duration) *
        Info->CompresionRatio) * 1000) : 0;
    
    Info->PeakRatio=(float)(Info->ByteLength=0);
    return 0;
}
