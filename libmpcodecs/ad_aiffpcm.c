/*
	AIFF Audio Decoder
	
	Copyright (c) 2004, Jake Luck <xbmc@10k.org>
	All rights reserved.
	BSD License
	http://www.opensource.org/licenses/bsd-license.php
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "ad_internal.h"

static ad_info_t info = 
{
	"Uncompressed AIFF PCM audio decoder",
	"aiffpcm",
	"Jake Luck",
	"",
	""
};

LIBAD_EXTERN(aiffpcm)

#include "aiff.h"

static int aiff_decode_factor;
    
static int preinit(sh_audio_t *sh)
{
  	return 1;
}

static int init(sh_audio_t *sh)
{
	AIFF_FORMCHUNK			*f;
	AIFF_COMMONCHUNK		*c;
	AIFF_SOUNDDATACHUNK		*s;
	int		                n;
	
	f = &(sh->ah->f);
	c = &(sh->ah->c);
	s = &(sh->ah->s);

    switch (c->cSampleSize)
    {
        case 16:
            sh->sample_format = AFMT_S16_BE;
            break;
        case 8:
            sh->sample_format = AFMT_S8;
            break;
        default:
            break;
    }
    
    sh->samplerate = samplerate_aiff_pcm(c->cSampleRate);
  	sh->samplesize = c->cSampleSize >> 3;  	
  	sh->channels = c->cNumChannels;
  	sh->i_bps = sh->samplesize * sh->samplerate * sh->channels ;
  
    aiff_decode_factor = sh->samplesize * sh->channels - 1;
        
    return 1;
}

static void uninit(sh_audio_t *sh)
{
	return;
}

static int control(sh_audio_t *sh, int cmd, void* arg, ...)
{
	return CONTROL_UNKNOWN;
}

static int decode_audio(sh_audio_t *sh, unsigned char *buf, int minlen, int maxlen)
{
    int             i;
    int             n;
    unsigned char   x;
    
    n = (minlen + aiff_decode_factor) & (~aiff_decode_factor);
    n = demux_read_data(sh->ds, buf, n);
  
#ifndef WORDS_BIGENDIAN
    for (i = 0 ; i < n; i = i + 2)
    {
        x = buf[i];
        buf[i]=buf[i+1];
        buf[i+1]=x;
    }
#endif

    return  n;
}

