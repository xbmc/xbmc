/*
    AIFF Audio Utilities
    
    Copyright (c) 2004, Jake Luck <xbmc@10k.org>
    All rights reserved.
    BSD License
    http://www.opensource.org/licenses/bsd-license.php
*/


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "stream.h"
#include "aiff.h"
#include "bswap.h"

AIFF_HEADER * 
get_aiff_header(stream_t *s)
{
    AIFF_HEADER             *h;
    AIFF_FORMCHUNK          *fc;
    AIFF_COMMONCHUNK        *cc;
    AIFF_SOUNDDATACHUNK     *sc;
    
    h = (AIFF_HEADER *)malloc(sizeof(AIFF_HEADER));
    if (h == NULL)      goto abort;

    fc = &(h->f);
    cc = &(h->c);
    sc = &(h->s);

    /* read form chunk */
    if ( stream_read(s, (char *)&(fc->fId), 4) != 4 )      goto abort;
    if ( stream_read(s, (char *)&(fc->fSize), 4) != 4)     goto abort;
    if ( stream_read(s, (char *)&(fc->fFormat), 4) != 4)   goto abort;
    
    /* read common chunk */
    if ( stream_read(s, (char *)&(cc->cId), 4) != 4)               goto abort;
    if ( stream_read(s, (char *)&(cc->cSize), 4) != 4)             goto abort;
    if ( stream_read(s, (char *)&(cc->cNumChannels), 2) != 2)      goto abort;
    if ( stream_read(s, (char *)&(cc->cSampleFrames), 4) != 4)     goto abort;
    if ( stream_read(s, (char *)&(cc->cSampleSize), 2) != 2)       goto abort;
    if ( stream_read(s, (char *)&(cc->cSampleRate[0]), 4) != 4)    goto abort;
    if ( stream_read(s, (char *)&(cc->cSampleRate[4]), 4) != 4)    goto abort;
    if ( stream_read(s, (char *)&(cc->cPadd), 2) != 2)             goto abort;
    
    /* read sound data chunk */
    if ( stream_read(s, (char *)&(sc->sId), 4) != 4)           goto abort;
    if ( stream_read(s, (char *)&(sc->sSize), 4) != 4)         goto abort;
    if ( stream_read(s, (char *)&(sc->sOffset), 4) != 4)       goto abort;
    if ( stream_read(s, (char *)&(sc->sAlignSize), 4) != 4)    goto abort;
    
    /* fix endian issue */
    fc->fSize        = be2me_32(fc->fSize);

    cc->cSize        = be2me_32(cc->cSize);
    cc->cNumChannels = be2me_16(cc->cNumChannels);
    cc->cSampleFrames= be2me_32(cc->cSampleFrames);
    cc->cSampleSize  = be2me_16(cc->cSampleSize);

    sc->sSize        = be2me_32(sc->sSize);
    sc->sOffset      = be2me_32(sc->sOffset);
    sc->sAlignSize   = be2me_32(sc->sAlignSize);

    return h;
        
abort:
    if (h != NULL)  free(h);
    return NULL;
}

/*
    demuxer->movi_end uses absolute file location
    
    ssnd chunksize already includes
        its offset field (4 bytes)
        its align field (4 bytes)
    
    hence 
    sounddata_end = ssnd_chunksize - partial ssnd header + headersize
*/
off_t   
endpos_aiff_pcm(AIFF_HEADER *h)
{
    AIFF_FORMCHUNK          *f;
    AIFF_COMMONCHUNK        *c;
    AIFF_SOUNDDATACHUNK     *s;
    int         n;
    off_t       w;
    
    f = &(h->f);
    c = &(h->c);
    s = &(h->s);
    
    w = s->sSize - 8 + 54;
    
    return w;
}

/*  
    convert the extended 8 byte sample rate to int
*/
int
samplerate_aiff_pcm(unsigned char *sr)
{
    int     r;
    
    if      ( memcmp(sr, "\x40\x0E\xAC\x44\x00\x00\x00\x00", 8) == 0 )
        r = 44100;
    else if ( memcmp(sr, "\x40\x0E\xBB\x80\x00\x00\x00\x00", 8) == 0 )
        r = 48000;    
    else if ( memcmp(sr, "\x40\x0D\xFA\x00\x00\x00\x00\x00", 8) == 0 )
        r = 32000; 
    else if ( memcmp(sr, "\x40\x0D\xBB\x80\x00\x00\x00\x00", 8) == 0 )
        r = 24000; 
    else if ( memcmp(sr, "\x40\x0D\xAC\x44\x00\x00\x00\x00", 8) == 0 )
        r = 22050; 
    else if ( memcmp(sr, "\x40\x0C\xFA\x00\x00\x00\x00\x00", 8) == 0 )
        r = 16000; 
    else if ( memcmp(sr, "\x40\x0C\xAC\x44\x00\x00\x00\x00", 8) == 0 )
        r = 11025; 
    else if ( memcmp(sr, "\x40\x0B\xFA\x00\x00\x00\x00\x00", 8) == 0 )
        r = 8000; 
        
    return r;
}

void 
print_aiff_header(AIFF_HEADER *h)
{
    AIFF_FORMCHUNK          *f;
    AIFF_COMMONCHUNK        *c;
    AIFF_SOUNDDATACHUNK     *s;
    int     n;
    
    f = &(h->f);
    c = &(h->c);
    s = &(h->s);

    printf("---------------------------------------\n");
    printf("FORM id %c%c%c%c\n", f->fId[0], f->fId[1], f->fId[2], f->fId[3]);
    printf("FORM size : %d\n",f->fSize);
    printf("FORM format %c%c%c%c\n\n", f->fFormat[0], f->fFormat[1], 
                                       f->fFormat[2], f->fFormat[3]);
    
    printf("COMMON id %c%c%c%c\n", c->cId[0], c->cId[1], c->cId[2], c->cId[3]);
    printf("COMMON size : %d\n", c->cSize);
    printf("COMMON channel: %d\n", c->cNumChannels);
    printf("COMMON sample frames : %d\n", c->cSampleFrames);
    printf("COMMON sample size : %d\n", c->cSampleSize);
    printf("COMMON sample rate : %08X %08X\n", *((unsigned int *)(&(c->cSampleRate[0]))),
                                               *((unsigned int *)(&(c->cSampleRate[4]))));
    printf("COMMON pad %c%c\n\n", c->cPadd[0], c->cPadd[1]);
    
    printf("SSND id %c%c%c%c\n", s->sId[0], s->sId[1], s->sId[2], s->sId[3]);
    printf("SSND size : %d\n", s->sSize);
    printf("SSND offset : %d\n", s->sOffset);
    printf("SSND alignsize : %d\n", s->sAlignSize);
    printf("---------------------------------------\n");
    printf("SSND  sample bytes: %d\n", s->sSize - 8);
    printf("Total sample bytes: %d\n", c->cNumChannels * c->cSampleFrames * 
                                       c->cSampleSize>>3);

}

