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

    ao_a.c
	Written by Iwata <b6330015@kit.jp>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <ao/ao.h>

#include "timidity.h"
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

/* export the playback mode */

#define dpm ao_play_mode

PlayMode dpm = {
  DEFAULT_RATE, PE_SIGNED|PE_16BIT, PF_PCM_STREAM,
  -1,
  {0}, /* default: get all the buffer fragments you can */
  "Libao mode", 'O',
  NULL, /* edit your ~/.libao */
  open_output,
  close_output,
  output_data,
  acntl
};

static ao_device *ao_device_ctx;
static ao_sample_format ao_sample_format_ctx;

void show_ao_device_info(FILE *fp)
{
  int driver_count;
  ao_info **devices;
  int i;

  ao_initialize();

  fputs("Output device name (ao only):" NLS
"  -o device    ", fp);

  devices  = ao_driver_info_list(&driver_count);
  if (driver_count < 1) {
	  fputs("*no device found*" NLS, fp);
  }
  else {
	  fputs("[ ", fp);
	  for(i = 0; i < driver_count; i++) {
		  if (devices[i]->type == AO_TYPE_LIVE)
			  fprintf(fp, "%s ", devices[i]->short_name);
	  }
	  fputs("]", fp);
  }
  ao_shutdown();
}


static int open_output(void)
{
  int driver_id;

  ao_initialize();

  if (dpm.name == NULL) {
    driver_id = ao_default_driver_id();
  }
  else {
    ao_info *device;

    driver_id = ao_driver_id(dpm.name);
    if ((device = ao_driver_info(driver_id)) == NULL) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: driver is not supported.",
		dpm.name);
      return -1;
    }
    if (device->type == AO_TYPE_FILE) {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: file output is not supported.",
		dpm.name);
      return -1;
    }
  }

  if (driver_id == -1) {
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
	      dpm.name, strerror(errno));
    return -1;
  }

  /* They can't mean these */
  dpm.encoding &= ~(PE_ULAW|PE_ALAW|PE_BYTESWAP);

  ao_sample_format_ctx.channels = (dpm.encoding & PE_MONO) ? 1 : 2;
  ao_sample_format_ctx.rate = dpm.rate;
  ao_sample_format_ctx.byte_format = AO_FMT_NATIVE;
  ao_sample_format_ctx.bits = (dpm.encoding & PE_24BIT) ? 24 : 0;
  ao_sample_format_ctx.bits = (dpm.encoding & PE_16BIT) ? 16 : 0;
  if (ao_sample_format_ctx.bits == 0)
    ao_sample_format_ctx.bits = 8;

  if ((ao_device_ctx = ao_open_live(driver_id, &ao_sample_format_ctx, NULL /* no options */)) == NULL) {
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
	      dpm.name, strerror(errno));
    return -1;
  }
  return 0;
}

static int output_data(char *buf, int32 nbytes)
{
  if (ao_play(ao_device_ctx, buf, nbytes) == 0) {
    ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "%s: %s",
	      dpm.name, strerror(errno));
    return -1;
  }
  return 0;
}

static void close_output(void)
{
  if (ao_device_ctx != NULL) {
    ao_close(ao_device_ctx);
    ao_device_ctx = NULL;
    ao_shutdown();
  }
}

static int acntl(int request, void *arg)
{
  switch(request) {
  case PM_REQ_DISCARD:
    ;;
    return 0;
  }
  return -1;
}
