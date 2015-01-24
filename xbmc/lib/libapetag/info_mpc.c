/********************************************************************
*    
* Copyright (c) 2002 Artur Polaczynski (Ar't)  All rights reserved.
*            <artii@o2.pl>        LGPL-2.1
*       $ArtId: info_mpc.c,v 1.12 2003/04/13 11:24:10 art Exp $
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
/*
    Some portions of code or/and ideas come from 
    winamp plugins, xmms plugins, mppdec decoder
    thanks:
    -Frank Klemm <Frank.Klemm@uni-jena.de> 
    -Andree Buschmann <Andree.Buschmann@web.de> 
    -Thomas Juerges <thomas.juerges@astro.ruhr-uni-bochum.de> 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_io.h"
#include "info_mpc.h"
#include "is_tag.h"
 
#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

/*
*.MPC,*.MP+,*.MPP
*/
static const char *
profile_stringify(unsigned int profile);    // profile is 0...15, where 7...13 is used


static const char *
profile_stringify(unsigned int profile)    // profile is 0...15, where 7...13 is used
{
    static const char na[] = "n.a.";
    static const char *Names[] = {
        na, "Experimental", na, na,
        na, na, na, "Telephone",
        "Thumb", "Radio", "Standard", "Xtreme",
        "Insane", "BrainDead", "BrainDead+", "BrainDead++"
    };

    return profile >=
        sizeof (Names) / sizeof (*Names) ? na : Names[profile];
}

int
read_file_header_fp(ape_file *fp, StreamInfoMpc * Info)
{

return 0;
}

/*
    return 0; Info has all info
    return 1; File not found
    return 2; no Mpc file
*/
int
info_mpc_read(const char *fn, StreamInfoMpc * Info)
{
    unsigned int HeaderData[16];
    ape_file *tmpFile = NULL;
    long SkipSizeID3;
    
    // load file
    tmpFile = ape_fopen(fn, "rb");
        
    if (tmpFile == NULL) 
        return 1;    // file not found or read-protected
    // skip id3v2 
    SkipSizeID3=is_id3v2(tmpFile);
    ape_fseek(tmpFile,SkipSizeID3 , SEEK_SET);
    ape_fread((void *) HeaderData, sizeof (int), 16, tmpFile);
    ape_fseek(tmpFile, 0, SEEK_END);
    Info->FileSize=ape_ftell(tmpFile);
    // stream size 
    Info->ByteLength = Info->FileSize-is_id3v1(tmpFile)-is_ape(tmpFile)-SkipSizeID3;
    
    ape_fclose(tmpFile);
    
    if (0 != memcmp(HeaderData, "MP+", 3))
        return 2; // no musepack file
    
    Info->StreamVersion = HeaderData[0] >> 24;
    if (Info->StreamVersion >= 7) {
        const long samplefreqs[4] = { 44100, 48000, 37800, 32000 };
        
        // read the file-header (SV7 and above)
        Info->Bitrate = 0;
        Info->Frames = HeaderData[1];
        Info->SampleFreq = samplefreqs[(HeaderData[2] >> 16) & 0x0003];
        Info->MaxBand = (HeaderData[2] >> 24) & 0x003F;
        Info->MS = (HeaderData[2] >> 30) & 0x0001;
        Info->Profile = (HeaderData[2] << 8) >> 28;
        Info->IS = (HeaderData[2] >> 31) & 0x0001;
        Info->BlockSize = 1;
        
        Info->EncoderVersion = (HeaderData[6] >> 24) & 0x00FF;
        Info->Channels = 2;
        // gain
        Info->EstPeakTitle = HeaderData[2] & 0xFFFF;    // read the ReplayGain data
        Info->GainTitle = (HeaderData[3] >> 16) & 0xFFFF;
        Info->PeakTitle = HeaderData[3] & 0xFFFF;
        Info->GainAlbum = (HeaderData[4] >> 16) & 0xFFFF;
        Info->PeakAlbum = HeaderData[4] & 0xFFFF;
        // gaples
        Info->IsTrueGapless = (HeaderData[5] >> 31) & 0x0001;    // true gapless: used?
        Info->LastFrameSamples = (HeaderData[5] >> 20) & 0x07FF;    // true gapless: valid samples for last frame
        
        if (Info->EncoderVersion == 0) {
            sprintf(Info->Encoder, "<= 1.05"); // Buschmann 1.7.x, Klemm <= 1.05
        } else {
            switch (Info->EncoderVersion % 10) {
            case 0:
                sprintf(Info->Encoder, "%u.%u",
                    Info->EncoderVersion / 100,
                    Info->EncoderVersion / 10 % 10);
                break;
            case 2:
            case 4:
            case 6:
            case 8:
                sprintf(Info->Encoder, "%u.%02u Beta",
                    Info->EncoderVersion / 100,
                    Info->EncoderVersion % 100);
                break;
            default:
                sprintf(Info->Encoder, "%u.%02u Alpha",
                    Info->EncoderVersion / 100,
                    Info->EncoderVersion % 100);
                break;
            }
        }
        // estimation, exact value needs too much time
        Info->Bitrate = (unsigned int)((long) (Info->ByteLength) * 8. * Info->SampleFreq / (1152 * Info->Frames - 576));
        
    } else {
        // read the file-header (SV6 and below)
        Info->Bitrate = ((HeaderData[0] >> 23) & 0x01FF) * 1000;    // read the file-header (SV6 and below)
        Info->MS = (HeaderData[0] >> 21) & 0x0001;
        Info->IS = (HeaderData[0] >> 22) & 0x0001;
        Info->StreamVersion = (HeaderData[0] >> 11) & 0x03FF;
        Info->MaxBand = (HeaderData[0] >> 6) & 0x001F;
        Info->BlockSize = (HeaderData[0]) & 0x003F;
        
        Info->Profile = 0;
        //gain
        Info->GainTitle = 0;    // not supported
        Info->PeakTitle = 0;
        Info->GainAlbum = 0;
        Info->PeakAlbum = 0;
        //gaples
        Info->LastFrameSamples = 0;
        Info->IsTrueGapless = 0;
        
        if (Info->StreamVersion >= 5)
            Info->Frames = HeaderData[1];    // 32 bit
        else
            Info->Frames = (HeaderData[1] >> 16);    // 16 bit
        
        Info->EncoderVersion = 0;
        Info->Encoder[0] = '\0';
#if 0
        if (Info->StreamVersion == 7)
            return ERROR_CODE_SV7BETA;    // are there any unsupported parameters used?
        if (Info->Bitrate != 0)
            return ERROR_CODE_CBR;
        if (Info->IS != 0)
            return ERROR_CODE_IS;
        if (Info->BlockSize != 1)
            return ERROR_CODE_BLOCKSIZE;
#endif
        if (Info->StreamVersion < 6)    // Bugfix: last frame was invalid for up to SV5
            Info->Frames -= 1;
        
        Info->SampleFreq = 44100;    // AB: used by all files up to SV7
        Info->Channels = 2;
    }
    
    Info->ProfileName=profile_stringify(Info->Profile);
    
    Info->Duration = (int) (Info->Frames * 1152 / (Info->SampleFreq/1000.0));
    return 0;
}
