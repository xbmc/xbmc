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

    bsd20_a.c
	Written by Yamate Keiichiro <keiich-y@is.aist-nara.ac.jp>
*/

/*
 *  BSD/OS 2.0 audio
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <i386/isa/sblast.h>

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

#define dpm bsdi_play_mode

PlayMode dpm = {
    DEFAULT_RATE, PE_SIGNED|PE_MONO, PF_PCM_STREAM|PF_CAN_TRACE,
    -1,
    {0}, /* default: get all the buffer fragments you can */
    "BSD/OS sblast dsp", 'd',
    "/dev/sb_dsp",
    open_output,
    close_output,
    output_data,
    acntl
};

static int open_output(void)
{
    int fd, tmp, warnings=0;

    if ((fd=open(dpm.name, O_RDWR | O_NDELAY, 0)) < 0)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
		  dpm.name, strerror(errno));
	return -1;
    }

    /* They can't mean these */
    dpm.encoding &= ~(PE_ULAW|PE_ALAW|PE_BYTESWAP);


    /* Set sample width to whichever the user wants. If it fails, try
       the other one. */

    if (dpm.encoding & PE_16BIT)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: sblast only 8bit",
		  dpm.name);
	close(fd);
	return -1;
    }

    tmp = ((~dpm.encoding) & PE_16BIT) ? PCM_8 : 0;
    ioctl(fd, DSP_IOCTL_COMPRESS, &tmp);
    dpm.encoding &= ~PE_SIGNED;

    /* Try stereo or mono, whichever the user wants. If it fails, try
       the other. */

    tmp=(dpm.encoding & PE_MONO) ? 0 : 1;
    ioctl(fd, DSP_IOCTL_STEREO, &tmp);

  /* Set the sample rate */

    tmp=dpm.rate * ((dpm.encoding & PE_MONO) ? 1 : 2);
    ioctl(fd, DSP_IOCTL_SPEED, &tmp);
    if (tmp != dpm.rate)
    {
	dpm.rate=tmp / ((dpm.encoding & PE_MONO) ? 1 : 2);;
	ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		  "Output rate adjusted to %d Hz", dpm.rate);
	warnings=1;
    }

    /* Older VoxWare drivers don't have buffer fragment capabilities */

    if (dpm.extra_param[0])
    {
	ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
		  "%s doesn't support buffer fragments", dpm.name);
	warnings=1;
    }

    tmp = 16384;
    ioctl(fd, DSP_IOCTL_BUFSIZE, &tmp);

    dpm.fd = fd;
    return warnings;
}

static int output_data(char *buf, int32 nbytes)
{
    while ((-1==write(dpm.fd, buf, nbytes)) && errno==EINTR)
	;
}

static void close_output(void)
{
    if(dpm.fd != -1)
    {
	close(dpm.fd);
	dpm.fd = -1;
    }
}

static int acntl(int request, void *arg)
{
    switch(request)
    {
      case PM_REQ_DISCARD:
	return ioctl(dpm.fd, DSP_IOCTL_RESET);
    }
    return -1;
}
