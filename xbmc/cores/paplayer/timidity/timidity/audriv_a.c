/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    audriv_audio.c

    Functions to play sound on audriv_*.
    Written by Masanao Izumo <mo@goice.co.jp>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <fcntl.h>
#include <stdio.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif /* HAVE_SYS_IOCTL_H */
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "aenc.h"
#include "audriv.h"
#include "instrum.h"
#include "playmidi.h"
#include "miditrace.h"

static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static int output_data(char *buf, int32 nbytes);
static int acntl(int request, void *arg);

/* export the playback mode */

#define dpm audriv_play_mode
PlayMode dpm = {
  DEFAULT_RATE, PE_16BIT|PE_SIGNED, PF_PCM_STREAM|PF_CAN_TRACE,
  -1,
  {0,0,0,0,0}, /* no extra parameters so far */

#if defined(AU_DEC)
  "DEC audio device",
#elif defined(sgi)
  "SGI audio device",
#elif defined(AU_NONE)
  "Psedo audio device (No audio)",
#else
  "Audriv audio device",
#endif

  'd', "",
  open_output,
  close_output,
  output_data,
  acntl
};

/********** Audio_Init **********************
  This just does some generic initialization.  The actual audio
  output device open is done in the Audio_On routine so that we
  can get a device which matches the requested sample rate & format
 *****/

static int Audio_Init(void)
{
    static int init_flag = False;

    if(init_flag)
	return 0;
    if(!audriv_setup_audio())
    {
	ctl->cmsg(CMSG_FATAL, VERB_NORMAL, "%s", audriv_errmsg);
	return -1;
    }
    audriv_set_noblock_write(0);
    init_flag = True;
    return 0;
}

static int audio_init_open(void)
{
    int ch, enc;

    if(audriv_set_play_sample_rate(dpm.rate) == False)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Sample rate %d is not supported", dpm.rate);
	return -1;
    }

    if(dpm.encoding & PE_MONO)/* Mono samples */
	ch = 1;
    else
	ch = 2;

    if(audriv_set_play_channels(ch) == False)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Channel %d is not supported", ch);
	return -1;
    }

    if(dpm.encoding & PE_ULAW)
    {
	enc = AENC_G711_ULAW;
	dpm.encoding &= ~PE_16BIT;
    }
    else if(dpm.encoding & PE_ALAW)
    {
	enc = AENC_G711_ALAW;
	dpm.encoding &= ~PE_16BIT;
    }
    else if(dpm.encoding & PE_16BIT)
    {
#if defined(LITTLE_ENDIAN)
	if(dpm.encoding & PE_BYTESWAP)
	    if(dpm.encoding & PE_SIGNED)
		enc = AENC_SIGWORDB;
	    else
		enc = AENC_UNSIGWORDL;
	else
	    if(dpm.encoding & PE_SIGNED)
		enc = AENC_SIGWORDL;
	    else
		enc = AENC_UNSIGWORDL;
#else /* BIG_ENDIAN */
	if(dpm.encoding & PE_BYTESWAP)
	    if(dpm.encoding & PE_SIGNED)
		enc = AENC_SIGWORDL;
	    else
		enc = AENC_UNSIGWORDB;
	else
	    if(dpm.encoding & PE_SIGNED)
		enc = AENC_SIGWORDB;
	    else
		enc = AENC_UNSIGWORDB;
#endif
    }
    else
    {
	/* 8 bit */
	if(dpm.encoding & PE_SIGNED)
	    enc = AENC_SIGBYTE;
	else
	    enc = AENC_UNSIGBYTE;
    }

    if(audriv_set_play_encoding(enc) == False)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Output format is not supported: %s", AENC_NAME(enc));
	return -1;
    }

    return 0;
}

/********** Audio_On **********************
 * Turn On Audio Stream.
 *
 *****/
int Audio_On(void)
{
    static int init_flag = 0;

    if(!init_flag)
    {
	int i;

	i = audio_init_open();
	if(i)
	    return i;
	init_flag = 1;
    }

    if(!audriv_play_open())
    {
	ctl->cmsg(CMSG_FATAL, VERB_NORMAL, "%s", audriv_errmsg);
	ctl->close();
	exit(1);
    }

    return 0;
}

/* Open the audio device */
static int open_output(void)
{
    int i;
    if((i = Audio_Init()) != 0)
	return i;
    if((i = Audio_On()) != 0)
	return i;
    dpm.fd = 0;
    return 0;
}

/* Output of audio data from timidity */
static int output_data(char *buf, int32 nbytes)
{
    int n;

    while(nbytes > 0)
    {
	if((n = audriv_write(buf, nbytes)) == -1)
	    return -1;
	buf += n;
	nbytes -= n;
    }
    return 0;
}

/* close output device */
static void close_output(void)
{
    if(dpm.fd != -1)
    {
	audriv_play_close();
	dpm.fd = -1;
    }
}

static int acntl(int request, void *arg)
{
    switch(request)
    {
      case PM_REQ_DISCARD:
	audriv_play_stop(); /* Reset audriv's sample counter */
	Audio_On();
	return 0;
    }
    return -1;
}
