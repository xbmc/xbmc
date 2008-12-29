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

    oss_a.c  (linux_a.c)

    Functions to play sound on the OSS audio driver (Linux or FreeBSD)

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#if defined(HAVE_SYS_SOUNDCARD_H)
#include <sys/soundcard.h>
#elif defined(linux)
#include <sys/ioctl.h> /* new with 1.2.0? Didn't need this under 1.1.64 */
#include <linux/soundcard.h>
#elif defined(__FreeBSD__)
#include <machine/soundcard.h>
#include <sys/filio.h>
#elif defined(HAVE_SOUNDCARD_H)
#include <soundcard.h>
#else
#include <sys/soundcard.h>
#endif /* HAVE_SYS_SOUNDCARD_H */

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "timer.h"
#include "instrum.h"
#include "playmidi.h"
#include "miditrace.h"

#ifndef AFMT_S16_NE
#ifdef LITTLE_ENDIAN
#define AFMT_S16_NE AFMT_S16_LE
#else
#define AFMT_S16_NE AFMT_S16_BE
#endif
#endif /* AFMT_S16_NE */

static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static int output_data(char *buf, int32 nbytes);
static int acntl(int request, void *arg);
static int detect(void);
static int output_counter;
static int total_bytes; /* Maximum buffer size in bytes */

/* export the playback mode */

#define dpm oss_play_mode

PlayMode dpm = {
    DEFAULT_RATE,
    PE_16BIT|PE_SIGNED,
    PF_PCM_STREAM|PF_CAN_TRACE|PF_BUFF_FRAGM_OPT,
    -1,
    {0}, /* default: get all the buffer fragments you can */
    "dsp device", 'd',
#ifdef OSS_DEVICE
    OSS_DEVICE,
#else
    "/dev/dsp",
#endif
    open_output,
    close_output,
    output_data,
    acntl,
    detect
};

static int detect(void)
{
    int fd;
    fd = open(dpm.name, O_WRONLY | O_NONBLOCK);
    if (fd < 0)
	return 0;
    close(fd);
    return 1; /* found */
}

/*************************************************************************/
/* We currently only honor the PE_MONO bit, the sample rate, and the
   number of buffer fragments. We try 16-bit signed data first, and
   then 8-bit unsigned if it fails. If you have a sound device that
   can't handle either, let me know. */

static int open_output(void)
{
    int fd, tmp, i, warnings = 0;
    int include_enc, exclude_enc;
#ifdef SNDCTL_DSP_GETOSPACE
    audio_buf_info info;
#endif /* SNDCTL_DSP_GETOSPACE */

    /* Open the audio device */
    fd = open(dpm.name, O_WRONLY);
    if(fd < 0)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
		  dpm.name, strerror(errno));
	return -1;
    }

    /*
     * Modified: Fri Nov 20 1998
     *
     * Reported from http://www.ife.ee.ethz.ch/~sailer/linux/pciaudio.html
     *   by Thomas Sailer <sailer@ife.ee.ethz.ch>
     * OSS/Free sets nonblocking mode with an ioctl, unlike the rest of the
     * kernel, which uses the O_NONBLOCK flag. I want to regularize that API,
     * and this trips on Timidity.
     */
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NDELAY);

    include_enc = 0;
    exclude_enc = PE_ULAW|PE_ALAW|PE_BYTESWAP; /* They can't mean these */
    if(dpm.encoding & PE_16BIT)
	include_enc |= PE_SIGNED;
    else
	exclude_enc |= PE_SIGNED;
    dpm.encoding = validate_encoding(dpm.encoding, include_enc, exclude_enc);

    /* Set sample width to whichever the user wants. If it fails, try
       the other one. */

    i = tmp = (dpm.encoding & PE_16BIT) ? AFMT_S16_NE : AFMT_U8;
    if(ioctl(fd, SNDCTL_DSP_SAMPLESIZE, &tmp) < 0 || tmp != i)
    {
	/* Try the other one */
	i = tmp = (dpm.encoding & PE_16BIT) ? AFMT_U8 : AFMT_S16_NE;
	if(ioctl(fd, SNDCTL_DSP_SAMPLESIZE, &tmp) < 0 || tmp != i)
	{
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		      "%s doesn't support 16- or 8-bit sample width",
		      dpm.name);
	    close(fd);
	    return -1;
	}
	ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		  "Sample width adjusted to %d bits", tmp);
	dpm.encoding ^= PE_16BIT;
	warnings = 1;
    }

    /* Try stereo or mono, whichever the user wants. If it fails, try
       the other. */

    i = tmp = (dpm.encoding & PE_MONO) ? 0 : 1;
    if((ioctl(fd, SNDCTL_DSP_STEREO, &tmp) < 0) || tmp != i)
    {
	i = tmp = (dpm.encoding & PE_MONO) ? 1 : 0;

	if((ioctl(fd, SNDCTL_DSP_STEREO, &tmp) < 0) || tmp != i)
	{
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		      "%s doesn't support mono or stereo samples",
		      dpm.name);
	    close(fd);
	    return -1;
	}
	if(tmp == 0) dpm.encoding |= PE_MONO;
	else dpm.encoding &= ~PE_MONO;
	ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Sound adjusted to %sphonic",
		  (tmp==0) ? "mono" : "stereo");
	warnings = 1;
    }


    /* Set the sample rate */

    tmp = dpm.rate;
    if(ioctl(fd, SNDCTL_DSP_SPEED, &tmp) < 0)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "%s doesn't support a %d Hz sample rate",
		  dpm.name, dpm.rate);
	close(fd);
	return -1;
    }
    if(tmp != dpm.rate)
    {
	ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		  "Output rate adjusted to %d Hz (requested %d Hz)",
		  tmp, dpm.rate);
	dpm.rate = tmp;
	warnings = 1;
    }

    /* Older VoxWare drivers don't have buffer fragment capabilities */
#ifdef SNDCTL_DSP_SETFRAGMENT
    /* Set buffer fragments (in extra_param[0]) */

    tmp = audio_buffer_bits;
    if(!(dpm.encoding & PE_MONO)) tmp++;
    if(dpm.encoding & PE_16BIT) tmp++;
    i = tmp;
    tmp |= (dpm.extra_param[0] << 16);
    if(ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &tmp) < 0)
    {
	ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
		  "%s doesn't support %d-byte buffer fragments (%d)",
		  dpm.name, 1<<i, i);
	/* It should still work in some fashion. We should use a
	   secondary buffer anyway -- 64k isn't enough. */
	warnings = 1;
    }
#else
    if(dpm.extra_param[0])
    {
	ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
		  "%s doesn't support buffer fragments", dpm.name);
	warnings = 1;
    }
#endif

#ifdef SNDCTL_DSP_GETOSPACE
    if(ioctl(fd, SNDCTL_DSP_GETOSPACE, &info) != -1) {
	total_bytes = info.fragstotal * info.fragsize;
	ctl->cmsg(CMSG_INFO, VERB_NOISY, "Audio device buffer: %d x %d bytes",
		  info.fragstotal, info.fragsize);
    }
    else
#endif /* SNDCTL_DSP_GETOSPACE */
	total_bytes = -1; /* Unknown */

    dpm.fd = fd;
    output_counter = 0;

    return warnings;
}

static int output_data(char *buf, int32 nbytes)
{
    int n;

    while(nbytes > 0)
    {
	if((n = write(dpm.fd, buf, nbytes)) == -1)
	{
#if 0
	    if(errno != XXX) /* I don't know what error should be ignored.
			      * XXX := EWOULDBLOCK??
			      */
	    {
		usleep(100000);
		continue;
	    }
	    ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		      "%s: %s", dpm.name, strerror(errno));
	    return -1;
#endif

	    /* Now, Ignore error */
	    usleep(100000);
	    continue;
	}
	buf += n;
	nbytes -= n;
#ifdef SNDCTL_DSP_GETODELAY
	output_counter += n;
#endif
    }

    return 0;
}

static void close_output(void)
{
    if(dpm.fd == -1)
	return;
    close(dpm.fd);
    dpm.fd = -1;
}

static int acntl(int request, void *arg)
{
#ifndef SNDCTL_DSP_GETODELAY
# ifdef SNDCTL_DSP_GETOSPACE
    audio_buf_info info;
# endif /* SNDCTL_DSP_GETOSPACE */
    count_info cinfo;
#endif /* SNDCTL_DSP_GETODELAY */
    int i;

    switch(request)
    {
      case PM_REQ_RATE:
	i = *(int *)arg; /* sample rate in and out */
	if(ioctl(dpm.fd, SNDCTL_DSP_SPEED, &i) < 0)
	    return -1;
	play_mode->rate = i;
	return 0;

      case PM_REQ_GETQSIZ:
	if(total_bytes <= 0)
	    return -1;
	*((int *)arg) = total_bytes;
	return 0;

#ifdef SNDCTL_DSP_GETODELAY
      case PM_REQ_DISCARD:
	output_counter = 0;
	return ioctl(dpm.fd, SNDCTL_DSP_RESET, NULL);

      case PM_REQ_FLUSH:
	output_counter = 0;
	return ioctl(dpm.fd, SNDCTL_DSP_SYNC, NULL);

      case PM_REQ_GETFILLED:
	if(total_bytes <= 0 || ioctl(dpm.fd, SNDCTL_DSP_GETODELAY, &i) == -1)
	    return -1;
	if (i > total_bytes) i = total_bytes;
	if(!(dpm.encoding & PE_MONO)) i >>= 1;
	if(dpm.encoding & PE_16BIT) i >>= 1;
	*((int *)arg) = i;
	return 0;

      case PM_REQ_GETFILLABLE:
	if(total_bytes <= 0 || ioctl(dpm.fd, SNDCTL_DSP_GETODELAY, &i) == -1)
	    return -1;
	if (i > total_bytes) i = 0;
        else i = total_bytes - i;
	if(!(dpm.encoding & PE_MONO)) i >>= 1;
	if(dpm.encoding & PE_16BIT) i >>= 1;
	*((int *)arg) = i;
	return 0;

      case PM_REQ_GETSAMPLES:
	if(ioctl(dpm.fd, SNDCTL_DSP_GETODELAY, &i) == -1)
	    return -1;
	i = output_counter - i;
	if(!(dpm.encoding & PE_MONO)) i >>= 1;
	if(dpm.encoding & PE_16BIT) i >>= 1;
	*((int *)arg) = i;
	return 0;

#else /* SNDCTL_DSP_GETODELAY */
      case PM_REQ_DISCARD:
	if(ioctl(dpm.fd, SNDCTL_DSP_RESET, NULL) == -1)
	    return -1;
	if(ioctl(dpm.fd, SNDCTL_DSP_GETOPTR, &cinfo) == -1)
	    return -1;
	output_counter = cinfo.bytes;
	return 0;

      case PM_REQ_FLUSH:
	if(ioctl(dpm.fd, SNDCTL_DSP_SYNC, NULL) == -1)
	    return -1;
	if(ioctl(dpm.fd, SNDCTL_DSP_GETOPTR, &cinfo) == -1)
	    return -1;
	output_counter = cinfo.bytes;
	return 0;

# ifdef SNDCTL_DSP_GETOSPACE
      case PM_REQ_GETFILLABLE:
	if(total_bytes <= 0 || ioctl(dpm.fd, SNDCTL_DSP_GETOSPACE, &info) == -1)
	    return -1;
	if (info.bytes > total_bytes) i = total_bytes;
	else i = info.bytes;
	if(!(dpm.encoding & PE_MONO)) i >>= 1;
	if(dpm.encoding & PE_16BIT) i >>= 1;
	*((int *)arg) = i;
	return 0;

      case PM_REQ_GETFILLED:
	if(total_bytes <= 0 || ioctl(dpm.fd, SNDCTL_DSP_GETOSPACE, &info) == -1)
	    return -1;
	if (info.bytes > total_bytes) i = 0;
	else i = total_bytes - info.bytes;
	if(!(dpm.encoding & PE_MONO)) i >>= 1;
	if(dpm.encoding & PE_16BIT) i >>= 1;
	*((int *)arg) = i;
	return 0;
# endif /* SNDCTL_DSP_GETOSPACE */

      case PM_REQ_GETSAMPLES:
	if(ioctl(dpm.fd, SNDCTL_DSP_GETOPTR, &cinfo) == -1)
	    return -1;
	i = cinfo.bytes - output_counter;
	if(!(dpm.encoding & PE_MONO)) i >>= 1;
	if(dpm.encoding & PE_16BIT) i >>= 1;
	*((int *)arg) = i;
	return 0;
#endif /* SNDCTL_DSP_GETODELAY */
    }
    return -1;
}
