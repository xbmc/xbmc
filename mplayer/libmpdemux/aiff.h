/*
    AIFF Audio Header
    based on the Apple specification documented at 
    http://developer.apple.com/documentation/QuickTime/INMAC/SOUND/imsoundmgr.36.htm
    
    Copyright (c) 2004, Jake Luck <xbmc@10k.org>
    All rights reserved.
    BSD License
    http://www.opensource.org/licenses/bsd-license.php
*/

#ifndef __AIFF_HEADER_H
#define __AIFF_HEADER_H 1

typedef struct __attribute__((__packed__)) aiffformchunk_tag {
    unsigned char   fId[4];
    unsigned long   fSize;
    unsigned char   fFormat[4];
} AIFF_FORMCHUNK;   /* 12 bytes */

typedef struct __attribute__((__packed__)) aiffcommonchunk_tag {
    unsigned char   cId[4];
    unsigned long   cSize;
    unsigned short  cNumChannels;
    unsigned long   cSampleFrames;
    unsigned short  cSampleSize;
    unsigned char   cSampleRate[8];
    unsigned char   cPadd[2];
} AIFF_COMMONCHUNK; /* 26 bytes */

typedef struct __attribute__((__packed__)) aiffsounddatachunk_tag {
    unsigned char   sId[4];
    unsigned long   sSize;
    unsigned long   sOffset;
    unsigned long   sAlignSize;
} AIFF_SOUNDDATACHUNK;  /* 16 bytes */


typedef struct __attribute__((__packed__)) aiffheader_tag {
    AIFF_FORMCHUNK          f;
    AIFF_COMMONCHUNK        c;
    AIFF_SOUNDDATACHUNK     s;
} AIFF_HEADER;      /* 54 bytes */

AIFF_HEADER *get_aiff_header(stream_t *s);
void    print_aiff_header(AIFF_HEADER *h);
off_t   endpos_aiff_pcm(AIFF_HEADER *h);
int     samplerate_aiff_pcm(unsigned char *sr);

#endif
