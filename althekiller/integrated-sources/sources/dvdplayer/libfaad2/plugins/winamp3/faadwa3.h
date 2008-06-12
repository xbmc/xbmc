/*
cnv_FAAD - MP4-AAC decoder plugin for Winamp3
Copyright (C) 2002 Antonio Foranna

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

The author can be contacted at:
kreel@tiscali.it
*/

#ifndef _AACPCM_H
#define _AACPCM_H

#include <studio/services/svc_mediaconverter.h>
#include <studio/services/servicei.h>
#include <studio/corecb.h>
#include <studio/wac.h>
#include <attribs/cfgitemi.h>
#include <attribs/attrint.h>

#include <mp4.h>
#include "..\..\..\faac\include\faac.h"
#include <faad.h>
extern "C" {
#include <aacinfo.h>    // get_AAC_format()
}
#include "Defines.h"



// -----------------------------------------------------------------------------------------------



class AacPcm : public svc_mediaConverterI
{
public:
    AacPcm();
    virtual ~AacPcm();

    // service
    static const char *getServiceName() { return FILES_SUPPORT " to PCM converter"; }

    virtual int canConvertFrom(svc_fileReader *reader, const char *name, const char *chunktype) {
        if(name && (!STRICMP(Std::extension(name),"aac")|| !STRICMP(Std::extension(name),"mp4"))) return 1; // only accepts *.aac and *.mp4 files
        return 0;
    }
    virtual const char *getConverterTo() { return "PCM"; }

    virtual int getInfos(MediaInfo *infos);

    virtual int processData(MediaInfo *infos, ChunkList *chunk_list, bool *killswitch);

    virtual int getLatency(void) { return 0; }

    // callbacks

    virtual int corecb_onSeeked(int newpos)
    {
/*      if(!IsSeekable)
        {
            newpos_ms=-1;
            return 0;
        }*/
        newpos_ms=newpos;
        return 0;
    }

// Raw AAC
    BOOL            FindBitrate;

private:

//MP4
    MP4FileHandle   mp4File;
    MP4SampleId     sampleId,
                    numSamples;
    int             track;
    BYTE            type;

// AAC
    FILE            *aacFile;
    DWORD           Samplerate;
    BYTE            Channels;
    DWORD           bps;
    DWORD           src_size; // aac filesize
    BYTE            *buffer;
    long            tagsize;
    DWORD           *seek_table;
    int             seek_table_length;
    bool            BlockSeeking;

// GLOBAL
    faacDecHandle   hDecoder;
    faadAACInfo     file_info;
    faacDecFrameInfo    frameInfo;
    DWORD           len_ms;         // length of file in milliseconds
    DWORD           bytes_read;     // from file
    DWORD           bytes_consumed; // by faadDecDecode
    long            bytes_into_buffer;
//  DWORD           dst_size;
    long            newpos_ms;
    BOOL            IsSeekable;
    bool            IsAAC;
};
#endif

