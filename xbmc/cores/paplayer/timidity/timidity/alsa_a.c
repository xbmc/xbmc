/* -*- c-file-style: "gnu" -*-
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>
    ALSA 0.[56] support by Katsuhiro Ueno <katsu@blue.sky.or.jp>
                rewritten by Takashi Iwai <tiwai@suse.de>

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

    alsa_a.c

    Functions to play sound on the ALSA audio driver

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

/*ALSA header file*/
#if HAVE_ALSA_ASOUNDLIB_H
#define ALSA_PCM_OLD_HW_PARAMS_API
#define ALSA_PCM_OLD_SW_PARAMS_API
#include <alsa/asoundlib.h>
#else
#include <sys/asoundlib.h>
#endif

#if SND_LIB_MAJOR > 0
#define ALSA_LIB  9
#elif defined(SND_LIB_MINOR)
#define ALSA_LIB  SND_LIB_MINOR
#else
#define ALSA_LIB  3
#endif

#if ALSA_LIB < 4
typedef void  snd_pcm_t;
#endif

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "timer.h"
#include "instrum.h"
#include "playmidi.h"
#include "miditrace.h"

static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static int output_data(char *buf, int32 nbytes);
static int acntl(int request, void *arg);
#if ALSA_LIB >= 5
static int detect(void);
#endif

/* export the playback mode */

#define dpm alsa_play_mode

PlayMode dpm = {
  DEFAULT_RATE, PE_16BIT|PE_SIGNED, PF_PCM_STREAM|PF_CAN_TRACE|PF_BUFF_FRAGM_OPT,
  -1,
  {0}, /* default: get all the buffer fragments you can */
  "ALSA pcm device", 's',
  "", /* here leave it empty so that the pcm device name can be given
       * via command line option.
       */
  open_output,
  close_output,
  output_data,
  acntl,
#if ALSA_LIB >= 5
  detect
#endif
};

/*************************************************************************/
/* We currently only honor the PE_MONO bit, the sample rate, and the
   number of buffer fragments. We try 16-bit signed data first, and
   then 8-bit unsigned if it fails. If you have a sound device that
   can't handle either, let me know. */


/*ALSA PCM handler*/
static snd_pcm_t* handle = NULL;
#if ALSA_LIB <= 5
static int card = 0;
static int device = 0;
#endif
static int total_bytes = -1;
static int frag_size = 0;
static int sample_shift = 0;
static int output_counter;

#if ALSA_LIB > 5
static char *alsa_device_name(void)
{
  if (dpm.name && *dpm.name)
    return dpm.name;
  else
    return "alsa pcm";
}
#else
static char *alsa_device_name(void)
{
  static char name[32];
  sprintf(name, "card%d/device%d", card, device);
  return name;
}
#endif

static void error_report (int snd_error)
{
  ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
	    alsa_device_name(), snd_strerror (snd_error));
}


#if ALSA_LIB < 6
/*return value == 0 sucess
               == -1 fails
 */
static int check_sound_cards (int* card__, int* device__,
			      const int32 extra_param[5])
{
  /*Search sound cards*/
  struct snd_ctl_hw_info ctl_hw_info;
  snd_pcm_info_t pcm_info;
  snd_ctl_t* ctl_handle;
  const char* env_sound_card = getenv ("TIMIDITY_SOUND_CARD");
  const char* env_pcm_device = getenv ("TIMIDITY_PCM_DEVICE");
  int tmp;

  /*specify card*/
  *card__ = 0;
  if (env_sound_card != NULL)
    *card__ = atoi (env_sound_card);
  /*specify device*/
  *device__ = 0;
  if (env_pcm_device != NULL)
    *device__ = atoi (env_pcm_device);

  tmp = snd_cards ();
  if (tmp == 0)
    {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "No sound card found.");
      return -1;
    }
  if (tmp < 0)
    {
      error_report (tmp);
      return -1;
    }

  if (*card__ < 0 || *card__ >= tmp)
    {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "There is %d sound cards."
		" %d is invalid sound card. assuming 0.",
		tmp, *card__);
      *card__ = 0;
    }

  tmp = snd_ctl_open (&ctl_handle, *card__);
  if (tmp < 0)
    {
      error_report (tmp);
      return -1;
    }

  /*check whether sound card has pcm device(s)*/
  tmp = snd_ctl_hw_info (ctl_handle, & ctl_hw_info);
  if (ctl_hw_info.pcmdevs == 0)
    {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		"%d-th sound card(%s) has no pcm device",
		ctl_hw_info.longname, *card__);
      snd_ctl_close (ctl_handle);
      return -1;
    }

  if (*device__ < 0 || *device__ >= ctl_hw_info.pcmdevs)
    {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		"%d-th sound cards(%s) has %d pcm device(s)."
		" %d is invalid pcm device. assuming 0.",
		*card__, ctl_hw_info.longname, ctl_hw_info.pcmdevs, *device__);
      *device__ = 0;

      if (ctl_hw_info.pcmdevs == 0)
	{/*sound card has no pcm devices*/
	  snd_ctl_close (ctl_handle);
	  return -1;
	}
    }

  /*check whether pcm device is able to playback*/
  tmp = snd_ctl_pcm_info(ctl_handle, *device__, &pcm_info);
  if (tmp < 0)
    {
      error_report (tmp);
      snd_ctl_close (ctl_handle);
      return -1;
    }

  if ((pcm_info.flags & SND_PCM_INFO_PLAYBACK) == 0)
    {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		"%d-th sound cards(%s), device=%d, "
		"type=%d, flags=%d, id=%s, name=%s,"
		" does not support playback",
		*card__, ctl_hw_info.longname, ctl_hw_info.pcmdevs,
		pcm_info.type, pcm_info.flags, pcm_info.id, pcm_info.name);
      snd_ctl_close (ctl_handle);
      return -1;
    }

  tmp = snd_ctl_close (ctl_handle);
  if (tmp < 0)
    {
      error_report (tmp);
      return -1;
    }

  return 0;
}
#endif


#if ALSA_LIB > 5
/*================================================================
 * ALSA API version 0.9.x
 *================================================================*/

static char *get_pcm_name(void)
{
  char *name;
  if (dpm.name && *dpm.name)
    return dpm.name;
  name = getenv("TIMIDITY_PCM_NAME");
  if (! name || ! *name)
    name = "default";
  return name;
}

static void error_handle(const char *file, int line, const char *func, int err, const char *fmt, ...)
{
}

static int detect(void)
{
  snd_pcm_t *pcm;
  snd_lib_error_set_handler(error_handle);
  if (snd_pcm_open(&pcm, get_pcm_name(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) < 0)
    return 0;
  snd_pcm_close(pcm);
  return 1; /* found */
}

/*return value == 0 sucess
               == 1 warning
               == -1 fails
 */
static int open_output(void)
{
  int orig_rate = dpm.rate;
  int ret_val = 0;
  int tmp, frags, r, pfds;
  int rate;
  snd_pcm_hw_params_t *pinfo;
  snd_pcm_sw_params_t *swpinfo;

  dpm.name = get_pcm_name();
  snd_lib_error_set_handler(NULL);
  tmp = snd_pcm_open(&handle, dpm.name, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK); /* avoid blocking by open */
  if (tmp < 0) {
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Can't open pcm device '%s'.", dpm.name);
    return -1;
  }
  snd_pcm_nonblock(handle, 0); /* set back to blocking mode */

  snd_pcm_hw_params_alloca(&pinfo);
  snd_pcm_sw_params_alloca(&swpinfo);

  if (snd_pcm_hw_params_any(handle, pinfo) < 0) {
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
	      "ALSA pcm '%s' can't initialize hw_params",
	      alsa_device_name());
    snd_pcm_close(handle);
    return -1;
  }

#ifdef LITTLE_ENDIAN
#define S16_FORMAT	SND_PCM_FORMAT_S16_LE
#define U16_FORMAT	SND_PCM_FORMAT_U16_LE
#else
#define S16_FORMAT	SND_PCM_FORMAT_S16_BE
#define U16_FORMAT	SND_PCM_FORMAT_U16_LE
#endif

  dpm.encoding &= ~(PE_ULAW|PE_ALAW|PE_BYTESWAP);
  /*check sample bit*/
  if (snd_pcm_hw_params_test_format(handle, pinfo, S16_FORMAT) < 0 &&
      snd_pcm_hw_params_test_format(handle, pinfo, U16_FORMAT) < 0)
    dpm.encoding &= ~PE_16BIT; /*force 8bit samples*/
  if (snd_pcm_hw_params_test_format(handle, pinfo, SND_PCM_FORMAT_U8) < 0 &&
      snd_pcm_hw_params_test_format(handle, pinfo, SND_PCM_FORMAT_S8) < 0)
    dpm.encoding |= PE_16BIT; /*force 16bit samples*/

  /*check format*/
  if (dpm.encoding & PE_16BIT) {
    /*16bit*/
    if (snd_pcm_hw_params_set_format(handle, pinfo, S16_FORMAT) == 0)
      dpm.encoding |= PE_SIGNED;
    else if (snd_pcm_hw_params_set_format(handle, pinfo, U16_FORMAT) == 0)
      dpm.encoding &= ~PE_SIGNED;
    else {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		"ALSA pcm '%s' doesn't support 16 bit sample width",
		alsa_device_name());
      snd_pcm_close(handle);
      return -1;
    }
  } else {
    /*8bit*/
    if (snd_pcm_hw_params_set_format(handle, pinfo, SND_PCM_FORMAT_U8) == 0)
      dpm.encoding &= ~PE_SIGNED;
    else if (snd_pcm_hw_params_set_format(handle, pinfo, SND_PCM_FORMAT_S8) == 0)
      dpm.encoding |= PE_SIGNED;
    else {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		"ALSA pcm '%s' doesn't support 8 bit sample width",
		alsa_device_name());
      snd_pcm_close(handle);
      return -1;
    }
  }

  if (snd_pcm_hw_params_set_access(handle, pinfo,
				   SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
	      "ALSA pcm '%s' doesn't support interleaved data",
	      alsa_device_name());
    snd_pcm_close(handle);
    return -1;
  }

  /*check rate*/
  r = snd_pcm_hw_params_get_rate_min(pinfo, NULL);
  if (r >= 0 && r > dpm.rate) {
    dpm.rate = r;
    ret_val = 1;
  }
  r = snd_pcm_hw_params_get_rate_max(pinfo, NULL);
  if (r >= 0 && r < dpm.rate) {
    dpm.rate = r;
    ret_val = 1;
  }
  if ((rate = snd_pcm_hw_params_set_rate_near(handle, pinfo, dpm.rate, 0)) < 0) {
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
	      "ALSA pcm '%s' can't set rate %d",
	      alsa_device_name(), dpm.rate);
    snd_pcm_close(handle);
    return -1;
  }

  /*check channels*/
  if (dpm.encoding & PE_MONO) {
    if (snd_pcm_hw_params_test_channels(handle, pinfo, 1) < 0)
      dpm.encoding &= ~PE_MONO;
  } else {
    if (snd_pcm_hw_params_test_channels(handle, pinfo, 2) < 0)
      dpm.encoding |= PE_MONO;
  }
    
  if (dpm.encoding & PE_MONO) {
    if (snd_pcm_hw_params_set_channels(handle, pinfo, 1) < 0) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		"ALSA pcm '%s' can't set mono channel",
		alsa_device_name());
      snd_pcm_close(handle);
      return -1;
    }
  } else {
    if (snd_pcm_hw_params_set_channels(handle, pinfo, 2) < 0) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		"ALSA pcm '%s' can't set stereo channels",
		alsa_device_name());
      snd_pcm_close(handle);
      return -1;
    }
  }

  sample_shift = 0;
  if (!(dpm.encoding & PE_MONO))
    sample_shift++;
  if (dpm.encoding & PE_16BIT)
    sample_shift++;

  /* Set buffer fragment size (in extra_param[1]) */
  if (dpm.extra_param[1] != 0)
    frag_size = dpm.extra_param[1];
  else
    frag_size = audio_buffer_size << sample_shift;

  /* Set buffer fragments (in extra_param[0]) */
  if (dpm.extra_param[0] == 0)
    frags = 4;
  else
    frags = dpm.extra_param[0];

  total_bytes = frag_size * frags;
  ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
	    "Requested buffer size %d, fragment size %d",
	    total_bytes, frag_size);
  if ((tmp = snd_pcm_hw_params_set_buffer_size_near(handle, pinfo, total_bytes >> sample_shift)) < 0) {
    ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
	      "ALSA pcm '%s' can't set buffer size %d",
	      alsa_device_name(), total_bytes);
    snd_pcm_close(handle);
    return -1;
  }

  if ((tmp = snd_pcm_hw_params_set_period_size_near(handle, pinfo, frag_size >> sample_shift, 0)) < 0) {
    ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
	      "ALSA pcm '%s' can't set period size %d",
	      alsa_device_name(), frag_size);
    snd_pcm_close(handle);
    return -1;
  }

  if (snd_pcm_hw_params(handle, pinfo) < 0) {
    snd_output_t *log;
    ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
	      "ALSA pcm '%s' can't set hw_params", alsa_device_name());
    snd_output_stdio_attach(&log, stderr, 0);
    snd_pcm_hw_params_dump(pinfo, log);
    snd_pcm_close(handle);
    return -1;
  }

  total_bytes = snd_pcm_hw_params_get_buffer_size(pinfo) << sample_shift;
  frag_size = snd_pcm_hw_params_get_period_size(pinfo, NULL) << sample_shift;
  ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
	    "ALSA pcm '%s' set buffer size %d, period size %d bytes",
	    alsa_device_name(), total_bytes, frag_size);
  tmp = snd_pcm_hw_params_get_rate(pinfo, NULL);
  if (tmp > 0 && tmp != dpm.rate) {
    dpm.rate = tmp;
    ret_val = 1;
  }
  if (orig_rate != dpm.rate) {
    ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
	      "Output rate adjusted to %d Hz (requested %d Hz)",
	      dpm.rate, orig_rate);
  }
  snd_pcm_sw_params_current(handle, swpinfo);
  snd_pcm_sw_params_set_start_threshold(handle, swpinfo, total_bytes >> sample_shift);
  snd_pcm_sw_params_set_stop_threshold(handle, swpinfo, total_bytes >> sample_shift);

  tmp = snd_pcm_prepare(handle);
  if (tmp < 0) {
    ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
	      "unable to prepare channel\n");
    snd_pcm_close(handle);
    return -1;
  }

  pfds = snd_pcm_poll_descriptors_count(handle);
  if (pfds > 1) {
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "too many poll descriptors: %s",
	      alsa_device_name());
    close_output ();
    return -1;
  } else if (pfds == 1) {
    struct pollfd pfd;
    if (snd_pcm_poll_descriptors(handle, &pfd, 1) >= 0)
      dpm.fd = pfd.fd;
    else
      dpm.fd = -1;
  } else
    dpm.fd = -1;

  output_counter = 0;

  return ret_val;
}

static void close_output(void)
{
  if (handle) {
    int ret = snd_pcm_close (handle);
    if (ret < 0)
      error_report (ret);
    handle = NULL;
  }

  dpm.fd = -1;
}

static int output_data(char *buf, int32 nbytes)
{
  int n;
  int nframes, shift;

  if (! handle)
    return -1;

  nframes = nbytes;
  shift = 0;
  if (!(dpm.encoding & PE_MONO))
    shift++;
  if (dpm.encoding & PE_16BIT)
    shift++;
  nframes >>= shift;
  
  while (nframes > 0) {
    n = snd_pcm_writei(handle, buf, nframes);
    if (n == -EAGAIN || (n >= 0 && n < nframes)) {
      snd_pcm_wait(handle, 1000);
    } else if (n == -EPIPE) {
      snd_pcm_status_t *status;
      snd_pcm_status_alloca(&status);
      if (snd_pcm_status(handle, status) < 0) {
	ctl->cmsg(CMSG_WARNING, VERB_DEBUG, "%s: cannot get status", alsa_device_name());
	return -1;
      }
      ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		"%s: underrun at %ld", alsa_device_name(), output_counter << sample_shift);
      snd_pcm_prepare(handle);
    } else if (n < 0) {
      ctl->cmsg(CMSG_WARNING, VERB_DEBUG,
		"%s: %s", alsa_device_name(),
		(n < 0) ? snd_strerror(n) : "write error");
      return -1;
    }
    if (n > 0) {
      nframes -= n;
      buf += n << shift;
      output_counter += n;
    }
  }

  return 0;
}

static int acntl(int request, void *arg)
{
  snd_pcm_status_t *status;
  snd_pcm_sframes_t delay;

  if (handle == NULL)
    return -1;

  switch (request) {
  case PM_REQ_GETFRAGSIZ:
    if (frag_size == 0)
      return -1;
    *((int *)arg) = frag_size;
    return 0;

  case PM_REQ_GETQSIZ:
    if (total_bytes == -1)
      return -1;
    *((int *)arg) = total_bytes;
    return 0;

  case PM_REQ_GETFILLABLE:
    if (total_bytes == -1)
      return -1;
    snd_pcm_status_alloca(&status);
    if (snd_pcm_status(handle, status) < 0)
      return -1;
    *((int *)arg) = snd_pcm_status_get_avail(status);
    return 0;
    
  case PM_REQ_GETFILLED:
    if (total_bytes == -1)
      return -1;
    if (snd_pcm_delay(handle, &delay) < 0)
      return -1;
    *((int *)arg) = delay;
    return 0;

  case PM_REQ_GETSAMPLES:
    if (total_bytes == -1)
      return -1;
    if (snd_pcm_delay(handle, &delay) < 0)
      return -1;
    *((int *)arg) = output_counter - delay;
    return 0;

  case PM_REQ_DISCARD:
    if (snd_pcm_drop(handle) < 0)
      return -1;
    if (snd_pcm_prepare(handle) < 0)
      return -1;
    output_counter = 0;
    return 0;

  case PM_REQ_FLUSH:
    if (snd_pcm_drain(handle) < 0)
      return -1;
    if (snd_pcm_prepare(handle) < 0)
      return -1;
    output_counter = 0;
    return 0;
  }
  return -1;
}


/* end ALSA API 0.9.x */


#elif ALSA_LIB == 5

/*================================================================
 * ALSA API version 0.5.x
 *================================================================*/

/*return value == 0 sucess
               == 1 warning
               == -1 fails
 */
static int set_playback_info (snd_pcm_t* handle__,
			      int32* encoding__, int32* rate__,
			      const int32 extra_param[5])
{
  int ret_val = 0;
  const int32 orig_rate = *rate__;
  int tmp;
  snd_pcm_channel_info_t   pinfo;
  snd_pcm_channel_params_t pparams;
  snd_pcm_channel_setup_t  psetup;

  memset (&pinfo, 0, sizeof (pinfo));
  memset (&pparams, 0, sizeof (pparams));
  pinfo.channel = SND_PCM_CHANNEL_PLAYBACK;
  tmp = snd_pcm_channel_info (handle__, &pinfo);
  if (tmp < 0)
    {
      error_report (tmp);
      return -1;
    }

  /*check sample bit*/
  if (!(pinfo.formats & ~(SND_PCM_FMT_S8 | SND_PCM_FMT_U8)))
    *encoding__ &= ~PE_16BIT; /*force 8bit samples*/
  if (!(pinfo.formats & ~(SND_PCM_FMT_S16 | SND_PCM_FMT_U16)))
    *encoding__ |= PE_16BIT; /*force 16bit samples*/

  /*check rate*/
  if (pinfo.min_rate > *rate__)
    *rate__ = pinfo.min_rate;
  if (pinfo.max_rate < *rate__)
    *rate__ = pinfo.max_rate;
  pparams.format.rate = *rate__;

  /*check channels*/
  if ((*encoding__ & PE_MONO) != 0 && pinfo.min_voices > 1)
    *encoding__ &= ~PE_MONO;
  if ((*encoding__ & PE_MONO) == 0 && pinfo.max_voices < 2)
    *encoding__ |= PE_MONO;

  if ((*encoding__ & PE_MONO) != 0)
    pparams.format.voices = 1; /*mono*/
  else
    pparams.format.voices = 2; /*stereo*/

  /*check format*/
  if ((*encoding__ & PE_16BIT) != 0)
    { /*16bit*/
      if ((pinfo.formats & SND_PCM_FMT_S16_LE) != 0)
	{
	  pparams.format.format = SND_PCM_SFMT_S16_LE;
	  *encoding__ |= PE_SIGNED;
	}
      else if ((pinfo.formats & SND_PCM_FMT_U16_LE) != 0)
	{
	  pparams.format.format = SND_PCM_SFMT_U16_LE;
	  *encoding__ &= ~PE_SIGNED;
	}
      else if ((pinfo.formats & SND_PCM_FMT_S16_BE) != 0)
	{
	  pparams.format.format = SND_PCM_SFMT_S16_BE;
	  *encoding__ |= PE_SIGNED;
	}
      else if ((pinfo.formats & SND_PCM_FMT_U16_BE) != 0)
	{
	  pparams.format.format = SND_PCM_SFMT_U16_BE;
	  *encoding__ &= ~PE_SIGNED;
	}
      else
	{
	  ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		    "%s doesn't support 16 bit sample width",
		    alsa_device_name());
	  return -1;
	}
    }
  else
    { /*8bit*/
      if ((pinfo.formats & SND_PCM_FMT_U8) != 0)
	{
	  pparams.format.format = SND_PCM_SFMT_U8;
	  *encoding__ &= ~PE_SIGNED;
	}
#if 0
      else if ((pinfo.formats & SND_PCM_FMT_S8) != 0)
	{
	  pcm_format.format = SND_PCM_SFMT_U16_LE;
	  *encoding__ |= PE_SIGNED;
	}
#endif
      else
	{
	  ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		    "%s doesn't support 8 bit sample width",
		    alsa_device_name());
	  return -1;
	}
    }


  sample_shift = 0;
  if (!(dpm.encoding & PE_MONO))
    sample_shift++;
  if (dpm.encoding & PE_16BIT)
    sample_shift++;

  /* Set buffer fragment size (in extra_param[1]) */
  if (extra_param[1] != 0)
    tmp = extra_param[1];
  else
    tmp = audio_buffer_size << sample_shift;

  /* Set buffer fragments (in extra_param[0]) */
  pparams.buf.block.frag_size = tmp;
  pparams.buf.block.frags_max = (extra_param[0] == 0) ? -1 : extra_param[0];
  pparams.buf.block.frags_min = 1;
  pparams.mode = SND_PCM_MODE_BLOCK;
  pparams.channel = SND_PCM_CHANNEL_PLAYBACK;
  pparams.start_mode = SND_PCM_START_DATA; /* .. should be START_FULL */
  pparams.stop_mode  = SND_PCM_STOP_STOP;
  pparams.format.interleave = 1;
  snd_pcm_channel_flush (handle__, SND_PCM_CHANNEL_PLAYBACK);
  tmp = snd_pcm_channel_params (handle__, &pparams);
  if (tmp < 0)
    {
      ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
		"%s doesn't support buffer fragments"
		":request size=%d, max=%d, min=%d\n",
		alsa_device_name(),
		pparams.buf.block.frag_size,
		pparams.buf.block.frags_max,
		pparams.buf.block.frags_min);
      return -1;
    }

  tmp = snd_pcm_channel_prepare (handle__, SND_PCM_CHANNEL_PLAYBACK);
  if (tmp < 0)
    {
      ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
		"unable to prepare channel\n");
      return -1;
    }

  memset (&psetup, 0, sizeof(psetup));
  psetup.channel = SND_PCM_CHANNEL_PLAYBACK;
  tmp = snd_pcm_channel_setup (handle__, &psetup);
  if (tmp == 0)
    {
      if(psetup.format.rate != orig_rate)
        {
	  ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		    "Output rate adjusted to %d Hz (requested %d Hz)",
		    psetup.format.rate, orig_rate);
	  dpm.rate = psetup.format.rate;
	  ret_val = 1;
	}
      frag_size = psetup.buf.block.frag_size;
      total_bytes = frag_size * psetup.buf.block.frags;
    }
  else
    {
      frag_size = 0;
      total_bytes = -1; /* snd_pcm_playback_status fails */
    }

  return ret_val;
}


static int detect(void)
{
  snd_pcm_t *pcm;
  if (snd_pcm_open(&pcm, card, device, SND_PCM_OPEN_PLAYBACK | SND_PCM_OPEN_NONBLOCK) < 0)
    return 0;
  snd_pcm_close(pcm);
  return 1; /* found */
}

static int open_output(void)
{
  int tmp, warnings=0;
  int ret;

  tmp = check_sound_cards (&card, &device, dpm.extra_param);
  if (tmp < 0)
    return -1;

  /* Open the audio device */
  ret = snd_pcm_open (&handle, card, device, SND_PCM_OPEN_PLAYBACK | SND_PCM_OPEN_NONBLOCK);
  if (ret < 0)
    {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
		alsa_device_name(), snd_strerror (ret));
      return -1;
    }

  snd_pcm_nonblock_mode(handle, 0); /* set back to blocking mode */
  /* They can't mean these */
  dpm.encoding &= ~(PE_ULAW|PE_ALAW|PE_BYTESWAP);
  warnings = set_playback_info (handle, &dpm.encoding, &dpm.rate,
				dpm.extra_param);
  if (warnings < 0)
    {
      close_output ();
      return -1;
    }

  dpm.fd = snd_pcm_file_descriptor (handle, SND_PCM_CHANNEL_PLAYBACK);
  output_counter = 0;
  return warnings;
}

static void close_output(void)
{
  int ret;

  if (handle == NULL)
    return;

  ret = snd_pcm_close (handle);
  if (ret < 0 && ret != -EINVAL) /* Maybe alsa-driver 0.5 has a bug */
    error_report (ret);
  handle = NULL;

  dpm.fd = -1;
}

static int output_data(char *buf, int32 nbytes)
{
  int n;
  snd_pcm_channel_status_t status;

  if (! handle)
    return -1;
  n = snd_pcm_write (handle, buf, nbytes);
  if (n <= 0)
    {
      memset (&status, 0, sizeof(status));
      status.channel = SND_PCM_CHANNEL_PLAYBACK;
      if (snd_pcm_channel_status(handle, &status) < 0)
	{
	  ctl->cmsg(CMSG_WARNING, VERB_DEBUG,
		    "%s: could not get channel status", alsa_device_name());
	  return -1;
	}
      if (status.status == SND_PCM_STATUS_UNDERRUN || n == -EPIPE)
	{
	  ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		    "%s: underrun at %d", alsa_device_name(), status.scount);
	  output_counter += status.scount;
	  snd_pcm_channel_flush (handle, SND_PCM_CHANNEL_PLAYBACK);
	  snd_pcm_channel_prepare (handle, SND_PCM_CHANNEL_PLAYBACK);
	  n = snd_pcm_write (handle, buf, nbytes);
	}
      if (n <= 0)
	{
	  ctl->cmsg(CMSG_WARNING, VERB_DEBUG,
		    "%s: %s", alsa_device_name(),
		    (n < 0) ? snd_strerror(n) : "write error");
	  if (n != -EPIPE)  /* buffer underrun is ignored */
	    return -1;
	}
    }
  return 0;
}


static int acntl(int request, void *arg)
{
  int i;
  snd_pcm_channel_status_t pstatus;
  memset (&pstatus, 0, sizeof (pstatus));
  pstatus.channel = SND_PCM_CHANNEL_PLAYBACK;

  if (handle == NULL)
    return -1;

  switch (request)
    {
      case PM_REQ_GETFRAGSIZ:
	if (frag_size == 0)
	  return -1;
	*((int *)arg) = frag_size;
	return 0;

      case PM_REQ_GETQSIZ:
	if (total_bytes == -1)
	  return -1;
	*((int *)arg) = total_bytes;
	return 0;

      case PM_REQ_GETFILLABLE:
	if (total_bytes == -1)
	  return -1;
	if (snd_pcm_channel_status(handle, &pstatus) < 0)
	  return -1;
	i = pstatus.free >> sample_shift;
	*((int *)arg) = i;
	return 0;

      case PM_REQ_GETFILLED:
	if (total_bytes == -1)
	  return -1;
	if (snd_pcm_channel_status(handle, &pstatus) < 0)
	  return -1;
	i = pstatus.count >> sample_shift;
	*((int *)arg) = i;
	return 0;

      case PM_REQ_GETSAMPLES:
	if (total_bytes == -1)
	  return -1;
	if (snd_pcm_channel_status(handle, &pstatus) < 0)
	  return -1;
	i = (output_counter + pstatus.scount) >> sample_shift;
	*((int *)arg) = i;
	return 0;

      case PM_REQ_DISCARD:
	if (snd_pcm_playback_drain(handle) < 0)
	  return -1;
	if (snd_pcm_channel_prepare(handle, SND_PCM_CHANNEL_PLAYBACK) < 0)
	  return -1;
	output_counter = 0;
	return 0;

      case PM_REQ_FLUSH:
	if (snd_pcm_channel_flush(handle, SND_PCM_CHANNEL_PLAYBACK) < 0)
	  return -1;
	if (snd_pcm_channel_prepare(handle, SND_PCM_CHANNEL_PLAYBACK) < 0)
	  return -1;
	output_counter = 0;
	return 0;
    }
    return -1;
}

/* end ALSA API 0.5.x */

#elif ALSA_LIB < 5

/*================================================================
 * ALSA API version 0.4.x
 *================================================================*/

/*return value == 0 sucess
               == 1 warning
               == -1 fails
 */
static int set_playback_info (snd_pcm_t* handle__,
			      int32* encoding__, int32* rate__,
			      const int32 extra_param[5])
{
  int ret_val = 0;
  const int32 orig_encoding = *encoding__;
  const int32 orig_rate = *rate__;
  int tmp;
  snd_pcm_playback_info_t pinfo;
  snd_pcm_format_t pcm_format;
  struct snd_pcm_playback_params pparams;
  struct snd_pcm_playback_status pstatus;
  memset (&pcm_format, 0, sizeof (pcm_format));

  memset (&pinfo, 0, sizeof (pinfo));
  memset (&pparams, 0, sizeof (pparams));
  tmp = snd_pcm_playback_info (handle__, &pinfo);
  if (tmp < 0)
    {
      error_report (tmp);
      return -1;
    }

  /*check sample bit*/
  if ((pinfo.flags & SND_PCM_PINFO_8BITONLY) != 0)
    *encoding__ &= ~PE_16BIT; /*force 8bit samples*/
  if ((pinfo.flags & SND_PCM_PINFO_16BITONLY) != 0)
    *encoding__ |= PE_16BIT; /*force 16bit samples*/

  /*check rate*/
  if (pinfo.min_rate > *rate__)
    *rate__ = pinfo.min_rate;
  if (pinfo.max_rate < *rate__)
    *rate__ = pinfo.max_rate;
  pcm_format.rate = *rate__;

  /*check channels*/
  if ((*encoding__ & PE_MONO) != 0 && pinfo.min_channels > 1)
    *encoding__ &= ~PE_MONO;
  if ((*encoding__ & PE_MONO) == 0 && pinfo.max_channels < 2)
    *encoding__ |= PE_MONO;

  if ((*encoding__ & PE_MONO) != 0)
    pcm_format.channels = 1; /*mono*/
  else
    pcm_format.channels = 2; /*stereo*/

  /*check format*/
  if ((*encoding__ & PE_16BIT) != 0)
    { /*16bit*/
      if ((pinfo.formats & SND_PCM_FMT_S16_LE) != 0)
	{
	  pcm_format.format = SND_PCM_SFMT_S16_LE;
	  *encoding__ |= PE_SIGNED;
	}
      else
	{
	  ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		    "%s doesn't support 16 bit sample width",
		    alsa_device_name());
	  return -1;
	}
    }
  else
    { /*8bit*/
      if ((pinfo.formats & SND_PCM_FMT_U8) != 0)
	{
	  pcm_format.format = SND_PCM_SFMT_U8;
	  *encoding__ &= ~PE_SIGNED;
	}
#if 0
      else if ((pinfo.formats & SND_PCM_FMT_S8) != 0)
	{
	  pcm_format.format = SND_PCM_SFMT_U16_LE;
	  *encoding__ |= PE_SIGNED;
	}
#endif
      else
	{
	  ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		    "%s doesn't support 8 bit sample width",
		    alsa_device_name());
	  return -1;
	}
    }


  tmp = snd_pcm_playback_format (handle__, &pcm_format);
  if (tmp < 0)
    {
      error_report (tmp);
      return -1;
    }
  /*check result of snd_pcm_playback_format*/
  if ((*encoding__ & PE_16BIT) != (orig_encoding & PE_16BIT ))
    {
      ctl->cmsg (CMSG_WARNING, VERB_VERBOSE,
		 "Sample width adjusted to %d bits",
		 ((*encoding__ & PE_16BIT) != 0)? 16:8);
      ret_val = 1;
    }
  if (((pcm_format.channels == 1)? PE_MONO:0) != (orig_encoding & PE_MONO))
    {
      ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Sound adjusted to %sphonic",
		((*encoding__ & PE_MONO) != 0)? "mono" : "stereo");
      ret_val = 1;
    }

  sample_shift = 0;
  if (!(dpm.encoding & PE_MONO))
    sample_shift++;
  if (dpm.encoding & PE_16BIT)
    sample_shift++;

  /* Set buffer fragment size (in extra_param[1]) */
  if (extra_param[1] != 0)
    tmp = extra_param[1];
  else
    tmp = audio_buffer_size << sample_shift;

  /* Set buffer fragments (in extra_param[0]) */
  pparams.fragment_size  = tmp;
  pparams.fragments_max  = (extra_param[0] == 0) ? -1 : extra_param[0];
  pparams.fragments_room = 1;
  tmp = snd_pcm_playback_params (handle__, &pparams);
  if (tmp < 0)
    {
      ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
		"%s doesn't support buffer fragments"
		":request size=%d, max=%d, room=%d\n",
		alsa_device_name(),
		pparams.fragment_size,
		pparams.fragments_max,
		pparams.fragments_room);
      ret_val = 1;
    }

  if (snd_pcm_playback_status(handle__, &pstatus) == 0)
    {
      if (pstatus.rate != orig_rate)
	{
	  ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		    "Output rate adjusted to %d Hz (requested %d Hz)",
		    pstatus.rate, orig_rate);
	  dpm.rate = pstatus.rate;
	  ret_val = 1;
	}
      frag_size = pstatus.fragment_size;
      total_bytes = pstatus.count;
    }
  else
    {
      frag_size = 0;
      total_bytes = -1; /* snd_pcm_playback_status fails */
    }

  return ret_val;
}

static int open_output(void)
{
  int tmp, warnings=0;
  int ret;

  tmp = check_sound_cards (&card, &device, dpm.extra_param);
  if (tmp < 0)
    return -1;

  /* Open the audio device */
  ret = snd_pcm_open (&handle, card, device, SND_PCM_OPEN_PLAYBACK);
  if (ret < 0)
    {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
		alsa_device_name(), snd_strerror (ret));
      return -1;
    }

  /* They can't mean these */
  dpm.encoding &= ~(PE_ULAW|PE_ALAW|PE_BYTESWAP);
  warnings = set_playback_info (handle, &dpm.encoding, &dpm.rate,
				dpm.extra_param);
  if (warnings < 0)
    {
      close_output ();
      return -1;
    }

  dpm.fd = snd_pcm_file_descriptor (handle);
  output_counter = 0;
  return warnings;
}

static void close_output(void)
{
  int ret;

  if (handle == NULL)
    return;

  ret = snd_pcm_close (handle);
  if (ret < 0)
    error_report (ret);
  handle = NULL;

  dpm.fd = -1;
}

static int output_data(char *buf, int32 nbytes)
{
  int n;

  if (! handle)
    return -1;
  while (nbytes > 0)
    {
      n = snd_pcm_write (handle, buf, nbytes);

      if (n < 0)
        {
	  ctl->cmsg(CMSG_WARNING, VERB_DEBUG,
		    "%s: %s", alsa_device_name(), snd_strerror(n));
	  if (n == -EWOULDBLOCK)
	    continue;
	  return -1;
	}
      buf += n;
      nbytes -= n;
      output_counter += n;
    }

  return 0;
}

static int acntl(int request, void *arg)
{
  int i;
  struct snd_pcm_playback_status pstatus;

  if (handle == NULL)
    return -1;

  switch (request)
    {
      case PM_REQ_GETFRAGSIZ:
	if (frag_size == 0)
	  return -1;
	*((int *)arg) = frag_size;
	return 0;

      case PM_REQ_GETQSIZ:
	if (total_bytes == -1)
	  return -1;
	*((int *)arg) = total_bytes;
	return 0;

      case PM_REQ_GETFILLABLE:
	if (total_bytes == -1)
	  return -1;
	if (snd_pcm_playback_status(handle, &pstatus) < 0)
	  return -1;
	i = pstatus.count >> sample_shift;
	*((int *)arg) = i;
	return 0;

      case PM_REQ_GETFILLED:
	if (total_bytes == -1)
	  return -1;
	if (snd_pcm_playback_status(handle, &pstatus) < 0)
	  return -1;
	i = pstatus.queue >> sample_shift;
	*((int *)arg) = i;
	return 0;

      case PM_REQ_GETSAMPLES:
	if (total_bytes == -1)
	  return -1;
	if (snd_pcm_playback_status(handle, &pstatus) < 0)
	  return -1;
	i = output_counter - pstatus.queue;
	i >>= sample_shift;
	*((int *)arg) = i;
	return 0;

      case PM_REQ_DISCARD:
	if (snd_pcm_drain_playback (handle) < 0)
	  return -1; /* error */
	output_counter = 0;
	return 0;

      case PM_REQ_FLUSH:
	if (snd_pcm_flush_playback(handle) < 0)
	  return -1; /* error */
	output_counter = 0;
	return 0;
    }
    return -1;
}

/* end ALSA API 0.4.x */

#endif

