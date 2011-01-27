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


    sun_a.c - Functions to play sound on a Sun and NetBSD /dev/audio. 
				Written by Masanao Izumo <mo@goice.co.jp>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#ifndef NO_STRING_H
#include <string.h>
#endif
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/filio.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif /* HAVE_STROPTS_H */

#if defined(HAVE_SYS_AUDIOIO_H)
#include <sys/audioio.h>
#elif defined(HAVE_SUN_AUDIOIO_H)
#include <sun/audioio.h>
#endif

#include "timidity.h"
#include "common.h"
#include "aenc.h"
#include "output.h"
#include "controls.h"

#if defined(__NetBSD__) /* NetBSD */
#ifdef LITTLE_ENDIAN
#define AUDIO_LINEAR_TAG	AUDIO_ENCODING_SLINEAR_LE
#else
#define AUDIO_LINEAR_TAG	AUDIO_ENCODING_SLINEAR_BE
#endif
#else /* Sun */
#define AUDIO_LINEAR_TAG	AUDIO_ENCODING_LINEAR
#endif

#ifdef LITTLE_ENDIAN
#define SUNAUDIO_AENC_SIGWORD	AENC_SIGWORDL
#else
#define SUNAUDIO_AENC_SIGWORD	AENC_SIGWORDB
#endif

#define AUDIO_DEV    "/dev/audio"
#define AUDIO_CTLDEV "/dev/audioctl"


static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static int output_data(char *buf, int32 bytes);
static int acntl(int request, void *arg);
static int output_counter, play_samples_offset;

/* export the playback mode */
#define dpm sun_play_mode
PlayMode dpm = {
    DEFAULT_RATE, PE_16BIT|PE_SIGNED,
    PF_PCM_STREAM|PF_CAN_TRACE,
    -1,
    {0,0,0,0,0}, /* no extra parameters so far */

#if defined(sun)
    "Sun audio device",
#elif defined(__NetBSD__)
    "NetBSD audio device",
#else
    AUDIO_DEV,
#endif

    'd',
    AUDIO_DEV,
    open_output,
    close_output,
    output_data,
    acntl
};
static int audioctl_fd = -1;

static int sun_audio_encoding(int enc)
{
    if(enc & PE_ULAW)
	return AUDIO_ENCODING_ULAW;
    if(enc & PE_ALAW)
	return AUDIO_ENCODING_ALAW;
#ifdef AUDIO_ENCODING_LINEAR8
    if(!(enc & PE_16BIT))
	return AUDIO_ENCODING_LINEAR8;
#else
    if(!(enc & PE_16BIT))
	return 105;
#endif /* AUDIO_ENCODING_LINEAR8 */
    return AUDIO_LINEAR_TAG;
}

static int sun_audio_getinfo(audio_info_t *auinfo)
{
    AUDIO_INITINFO(auinfo);
    return ioctl(audioctl_fd, AUDIO_GETINFO, auinfo);
}

static int sun_audio_setinfo(audio_info_t *auinfo)
{
    return ioctl(dpm.fd, AUDIO_SETINFO, auinfo);
}

static int open_output(void)
{
    int include_enc = 0, exclude_enc = PE_BYTESWAP;
    struct stat sb;
    audio_info_t auinfo;
    char *audio_dev, *audio_ctl_dev, *tmp_audio;


    /* See if the AUDIODEV environment variable is defined, and set the
       audio device accordingly  - Lalit Chhabra 23/Oct/2001 */
    if((audio_dev  = getenv("AUDIODEV")) != NULL)
    {
      dpm.id_name = safe_malloc(strlen(audio_dev)+1);
      dpm.name = safe_malloc(strlen(audio_dev)+1);
      strcpy(dpm.name, audio_dev);
      strcpy(dpm.id_name, audio_dev);

      tmp_audio = safe_malloc(strlen(audio_dev) + 3);
      audio_ctl_dev = safe_malloc(strlen(audio_dev) + 3);

      strcpy(tmp_audio, audio_dev);
      strcpy(audio_ctl_dev, strcat(tmp_audio, "ctl"));
    }
    else
    {
      audio_ctl_dev = safe_malloc(strlen(AUDIO_CTLDEV) + 3);
      strcpy(audio_ctl_dev, AUDIO_CTLDEV);
    }

    output_counter = play_samples_offset = 0;

    /* Open the audio device */
    if((audioctl_fd = open(audio_ctl_dev, O_RDWR)) < 0)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "%s: %s", audio_ctl_dev, strerror(errno));
	return -1;
    }

/* ############## */
#if 0
    if((dpm.fd = open(dpm.name, O_WRONLY | O_NDELAY)) == -1)
    {
	ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
		  "%s: %s", dpm.name, strerror(errno));
	if(errno == EBUSY)
	{
	    if((dpm.fd = open(dpm.name, O_WRONLY)) == -1)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: %s", dpm.name, strerror(errno));
		close_output();
		return -1;
	    }
	}
    }
#endif

    if((dpm.fd = open(dpm.name, O_WRONLY)) == -1)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "%s: %s", dpm.name, strerror(errno));
	close_output();
	return -1;
    }

    if(stat(dpm.name, &sb) < 0)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "%s: %s", dpm.name, strerror(errno));
	close_output();
	return -1;
    }

    if(!S_ISCHR(sb.st_mode))
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "%s: Not a audio device", dpm.name);
	close_output();
	return -1;
    }

    if(sun_audio_getinfo(&auinfo) < 0)
    { 
	/* from Francesco Zanichelli's */
	/* If it doesn't give info, it probably won't take requests
	    either. Assume it's an old device that does 8kHz uLaw only.

	      Disclaimer: I don't know squat about the various Sun audio
		  devices, so if this is not what we should do, I'll gladly
		      accept modifications. */
	ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "Cannot inquire %s", dpm.name);
	include_enc = PE_ULAW|PE_ALAW|PE_MONO;
	exclude_enc = PE_SIGNED|PE_16BIT|PE_BYTESWAP;
	dpm.encoding = validate_encoding(dpm.encoding,
					 include_enc, exclude_enc);
	if(dpm.rate != 8000)
	    ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		      "Sample rate is changed %d to 8000",
		      dpm.rate);
	dpm.rate = 8000;
	return 1;
    }

    if(!(dpm.encoding & PE_16BIT))
	exclude_enc |= PE_SIGNED; /* Always unsigned */
    dpm.encoding = validate_encoding(dpm.encoding, include_enc, exclude_enc);

    AUDIO_INITINFO(&auinfo);
    auinfo.play.sample_rate = dpm.rate;
    auinfo.play.channels = (dpm.encoding & PE_MONO) ? 1 : 2;
    auinfo.play.encoding = sun_audio_encoding(dpm.encoding);
    auinfo.play.precision = (dpm.encoding & PE_16BIT) ? 16 : 8;

    if(sun_audio_setinfo(&auinfo) == -1)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "rate=%d, channels=%d, precision=%d, encoding=%s",
		  auinfo.play.sample_rate, auinfo.play.channels,
		  auinfo.play.precision, output_encoding_string(dpm.encoding));
	close_output();
	return -1;
    }

    return 0;
}

static void close_output(void)
{
    if(dpm.fd != -1)
    {
	close(dpm.fd);
	dpm.fd = -1;
    }
    if(audioctl_fd != -1)
    {
	close(audioctl_fd);
	audioctl_fd = -1;
    }
}

int output_data(char *buff, int32 nbytes)
{
    int n;

  retry_write:
    n = write(dpm.fd, buff, nbytes);

    if(n < 0)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
		  dpm.name, strerror(errno));
	if(errno == EINTR || errno == EAGAIN)
	    goto retry_write;
	return -1;
    }

    output_counter += n;

    return n;
}


#if !defined(I_FLUSH) || !defined(FLUSHW)
static void null_proc(){}
static int sun_discard_playing(void)
{
    void (* orig_alarm_handler)();

    orig_alarm_handler = signal(SIGALRM, null_proc);
    ualarm(10000, 10000);
    close_output();
    ualarm(0, 0);
    signal(SIGALRM, orig_alarm_handler);
    return open_output();
}
#else
static int sun_discard_playing(void)
{
    if(ioctl(dpm.fd, I_FLUSH, FLUSHW) < 0)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: (ioctl) %s",
		  dpm.name, strerror(errno));
	return -1;
    }
    return 0;
}
#endif

static int acntl(int request, void *arg)
{
    audio_info_t auinfo;
    int i;

    switch(request)
    {
      case PM_REQ_GETFILLED:
	if(ioctl(audioctl_fd, AUDIO_GETINFO, &auinfo) < 0)
	    return -1;
#ifdef __NetBSD__
	*((int *)arg) = auinfo.play.seek;
#else
	if(auinfo.play.samples == play_samples_offset)
	    return -1; /* auinfo.play.samples is not active */
	i = output_counter;
	if(!(dpm.encoding & PE_MONO)) i >>= 1;
	if(dpm.encoding & PE_16BIT) i >>= 1;
	*((int *)arg) = i - (auinfo.play.samples - play_samples_offset);
#endif
	return 0;

      case PM_REQ_GETSAMPLES:
	if(ioctl(audioctl_fd, AUDIO_GETINFO, &auinfo) < 0)
	    return -1;
	if(auinfo.play.samples == play_samples_offset)
	    return -1; /* auinfo.play.samples is not active */
	*((int *)arg) = auinfo.play.samples - play_samples_offset;
	return 0;

      case PM_REQ_DISCARD:
	if(ioctl(audioctl_fd, AUDIO_GETINFO, &auinfo) != -1)
	    play_samples_offset = auinfo.play.samples;
	output_counter = 0;
	return sun_discard_playing();

      case PM_REQ_FLUSH:
	if(ioctl(audioctl_fd, AUDIO_GETINFO, &auinfo) != -1)
	    play_samples_offset = auinfo.play.samples;
	output_counter = 0;
	return 0;

      case PM_REQ_RATE: {
	  audio_info_t auinfo;
	  int rate;

	  rate = *(int *)arg;
	  AUDIO_INITINFO(&auinfo);
	  auinfo.play.sample_rate = rate;
	  if(sun_audio_setinfo(&auinfo) == -1)
	      return -1;
	  play_mode->rate = rate;
	  return 0;
        }
    }
    return -1;
}
