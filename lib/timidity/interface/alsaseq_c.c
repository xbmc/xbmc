/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2004 Masanao Izumo <iz@onicos.co.jp>
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    alsaseq_c.c - ALSA sequencer server interface
        Copyright (c) 2000  Takashi Iwai <tiwai@suse.de>

    This interface provides an ALSA sequencer client which receives
    events and plays it in real-time.  On this mode, TiMidity works
    as a software (quasi-)real-time MIDI synth engine.

    See doc/C/README.alsaseq for more details.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/time.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <math.h>
#include <signal.h>

#if HAVE_ALSA_ASOUNDLIB_H
#include <alsa/asoundlib.h>
#else
#include <sys/asoundlib.h>
#endif

#include "timidity.h"
#include "common.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "recache.h"
#include "output.h"
#include "aq.h"
#include "timer.h"


void readmidi_read_init(void);

#define MAX_PORTS	16

#define TICKTIME_HZ	100

struct seq_context {
	snd_seq_t *handle;	/* The snd_seq handle to /dev/snd/seq */
	int client;		/* The client associated with this context */
	int num_ports;		/* number of ports */
	int port[MAX_PORTS];	/* created sequencer ports */
	int fd;			/* The file descriptor */
	int used;		/* number of current connection */
	int active;		/* */
	int queue;
	snd_seq_queue_status_t *q_status;
};

static struct seq_context alsactx;

#if SND_LIB_MAJOR > 0 || SND_LIB_MINOR >= 6
/* !! this is a dirty hack.  not sure to work in future !! */
static int snd_seq_file_descriptor(snd_seq_t *handle)
{
	int pfds = snd_seq_poll_descriptors_count(handle, POLLIN);
	if (pfds > 0) {
		struct pollfd pfd;
		if (snd_seq_poll_descriptors(handle, &pfd, 1, POLLIN) >= 0)
			return pfd.fd;
	}
	return -ENXIO;
}

static int alsa_seq_open(snd_seq_t **seqp)
{
	return snd_seq_open(seqp, "hw", SND_SEQ_OPEN_DUPLEX, 0);
}

static int alsa_create_port(snd_seq_t *seq, int index)
{
	char name[32];
	int port;

	sprintf(name, "TiMidity port %d", index);
	port = snd_seq_create_simple_port(seq, name,
					  SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
					  SND_SEQ_PORT_TYPE_MIDI_GENERIC);
	if (port < 0) {
		fprintf(stderr, "error in snd_seq_create_simple_port\n");
		return -1;
	}

	return port;
}

#else
static int alsa_seq_open(snd_seq_t **seqp)
{
	return snd_seq_open(seqp, SND_SEQ_OPEN_IN);
}

static int alsa_create_port(snd_seq_t *seq, int index)
{
	snd_seq_port_info_t pinfo;

	memset(&pinfo, 0, sizeof(pinfo));
	sprintf(pinfo.name, "TiMidity port %d", index);
	pinfo.capability = SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE;
	pinfo.type = SND_SEQ_PORT_TYPE_MIDI_GENERIC;
	strcpy(pinfo.group, SND_SEQ_GROUP_DEVICE);
	if (snd_seq_create_port(alsactx.handle, &pinfo) < 0) {
		fprintf(stderr, "error in snd_seq_create_simple_port\n");
		return -1;
	}
	return pinfo.port;
}

#endif

static void alsa_set_timestamping(struct seq_context *ctxp, int port)
{
#if HAVE_SND_SEQ_PORT_INFO_SET_TIMESTAMPING
	int q = 0;
	snd_seq_port_info_t *pinfo;

	if (ctxp->queue < 0) {
		q = snd_seq_alloc_queue(ctxp->handle);
		ctxp->queue = q;
		if (q < 0)
			return;
		if (snd_seq_queue_status_malloc(&ctxp->q_status) < 0) {
			fprintf(stderr, "no memory!\n");
			exit(1);
		}
	}

	snd_seq_port_info_alloca(&pinfo);
	if (snd_seq_get_port_info(ctxp->handle, port, pinfo) < 0)
		return;
	snd_seq_port_info_set_timestamping(pinfo, 1);
	snd_seq_port_info_set_timestamp_real(pinfo, 1);
	snd_seq_port_info_set_timestamp_queue(pinfo, q);
	if (snd_seq_set_port_info(ctxp->handle, port, pinfo) < 0)
		return;
#endif
}


static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static int ctl_read(int32 *valp);
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void ctl_event(CtlEvent *e);
static void ctl_pass_playing_list(int n, char *args[]);

/**********************************/
/* export the interface functions */

#define ctl alsaseq_control_mode

ControlMode ctl=
{
    "ALSA sequencer interface", 'A',
    1,0,0,
    0,
    ctl_open,
    ctl_close,
    ctl_pass_playing_list,
    ctl_read,
    cmsg,
    ctl_event
};

/* options */
int opt_realtime_priority = 0;
int opt_sequencer_ports = 4;

static int buffer_time_advance;
static long buffer_time_offset;
static long start_time_base;
static long cur_time_offset;
static long last_queue_offset;
static double rate_frac, rate_frac_nsec;
static FILE *outfp;

/*ARGSUSED*/
static int ctl_open(int using_stdin, int using_stdout)
{
	ctl.opened = 1;
	ctl.flags &= ~(CTLF_LIST_RANDOM|CTLF_LIST_SORT);
	if (using_stdout)
		outfp = stderr;
	else
		outfp = stdout;
	return 0;
}

static void ctl_close(void)
{
	if (!ctl.opened)
		return;
}

static int ctl_read(int32 *valp)
{
    return RC_NONE;
}

static int cmsg(int type, int verbosity_level, char *fmt, ...)
{
    va_list ap;

    if((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
       ctl.verbosity < verbosity_level)
	return 0;

    if(outfp == NULL)
	outfp = stderr;

    va_start(ap, fmt);
    vfprintf(outfp, fmt, ap);
    fputs(NLS, outfp);
    fflush(outfp);
    va_end(ap);

    return 0;
}

static void ctl_event(CtlEvent *e)
{
}

static RETSIGTYPE sig_timeout(int sig)
{
    signal(SIGALRM, sig_timeout); /* For SysV base */
    /* Expect EINTR */
}

static void doit(struct seq_context *ctxp);
static int do_sequencer(struct seq_context *ctxp);
static int start_sequencer(struct seq_context *ctxp);
static void stop_sequencer(struct seq_context *ctxp);
static void server_reset(void);

/* reset all when SIGHUP is received */
static RETSIGTYPE sig_reset(int sig)
{
	if (alsactx.active) {
		stop_sequencer(&alsactx);
		server_reset();
	}
	signal(SIGHUP, sig_reset);
}

/*
 * set the process to realtime privs
 */
static int set_realtime_priority(void)
{
	struct sched_param schp;
	int max_prio;

	if (opt_realtime_priority <= 0)
		return 0;

        memset(&schp, 0, sizeof(schp));
	max_prio = sched_get_priority_max(SCHED_FIFO);
	if (max_prio < opt_realtime_priority)
		opt_realtime_priority = max_prio;

	schp.sched_priority = opt_realtime_priority;
        if (sched_setscheduler(0, SCHED_FIFO, &schp) != 0) {
		printf("can't set sched_setscheduler - using normal priority\n");
                return -1;
        }
	/* drop root priv. */
	if (! geteuid() && getuid() != geteuid()) {
		if (setuid(getuid()))
			perror("dropping root priv");
	}
	printf("set SCHED_FIFO(%d)\n", opt_realtime_priority);
        return 0;
}

static void ctl_pass_playing_list(int n, char *args[])
{
	double btime;
	int i, j;

#ifdef SIGPIPE
	signal(SIGPIPE, SIG_IGN);    /* Handle broken pipe */
#endif /* SIGPIPE */

	printf("TiMidity starting in ALSA server mode\n");

	set_realtime_priority();

	if (alsa_seq_open(&alsactx.handle) < 0) {
		fprintf(stderr, "error in snd_seq_open\n");
		return;
	}
	alsactx.queue = -1;
	alsactx.client = snd_seq_client_id(alsactx.handle);
	alsactx.fd = snd_seq_file_descriptor(alsactx.handle);
	snd_seq_set_client_name(alsactx.handle, "TiMidity");
	snd_seq_set_client_pool_input(alsactx.handle, 1000); /* enough? */
	if (opt_sequencer_ports < 1)
		alsactx.num_ports = 1;
	else if (opt_sequencer_ports > MAX_PORTS)
		alsactx.num_ports = MAX_PORTS;
	else
		alsactx.num_ports = opt_sequencer_ports;

	printf("Opening sequencer port:");
	for (i = 0; i < alsactx.num_ports; i++) {
		int port;
		port = alsa_create_port(alsactx.handle, i);
		if (port < 0)
			return;
		alsactx.port[i] = port;
		alsa_set_timestamping(&alsactx, port);
		printf(" %d:%d", alsactx.client, alsactx.port[i]);
	}
	printf("\n");

	alsactx.used = 0;
	alsactx.active = 0;

	opt_realtime_playing = 1; /* Enable loading patch while playing */
	allocate_cache_size = 0; /* Don't use pre-calclated samples */
	current_keysig = (opt_init_keysig == 8) ? 0 : opt_init_keysig;
	note_key_offset = key_adjust;

	if (IS_STREAM_TRACE) {
		/* set the audio queue size as minimum as possible, since
		 * we don't have to use audio queue..
		 */
		play_mode->acntl(PM_REQ_GETFRAGSIZ, &buffer_time_advance);
		if (!(play_mode->encoding & PE_MONO))
			buffer_time_advance >>= 1;
		if (play_mode->encoding & PE_16BIT)
			buffer_time_advance >>= 1;

		btime = (double)buffer_time_advance / play_mode->rate;
		btime *= 1.01; /* to be sure */
		aq_set_soft_queue(btime, 0.0);
	} else {
		buffer_time_advance = 0;
	}
	rate_frac = (double)play_mode->rate / 1000000.0;
	rate_frac_nsec = (double)play_mode->rate / 1000000000.0;

	alarm(0);
	signal(SIGALRM, sig_timeout);
	signal(SIGINT, safe_exit);
	signal(SIGTERM, safe_exit);
	signal(SIGHUP, sig_reset);

	i = current_keysig + ((current_keysig < 8) ? 7 : -9), j = 0;
	while (i != 7)
		i += (i < 7) ? 5 : -7, j++;
	j += note_key_offset, j -= floor(j / 12.0) * 12;
	current_freq_table = j;

	play_mode->close_output();

	if (ctl.flags & CTLF_DAEMONIZE)
	{
		int pid = fork();
		FILE *pidf;
		switch (pid)
		{
			case 0:			// child is the daemon
				break;
			case -1:		// error status return
				exit(7);
			default:		// no error, doing well
				if ((pidf = fopen( "/var/run/timidity.pid", "w" )) != NULL )
					fprintf( pidf, "%d\n", pid );
				exit(0);
		}
	}

	for (;;) {
		server_reset();
		doit(&alsactx);
	}
}

/*
 * get the current time in usec from gettimeofday()
 */
static long get_current_time(void)
{
	struct timeval tv;
	long t;

	gettimeofday(&tv, NULL);
	t = tv.tv_sec * 1000000L + tv.tv_usec;
	return t - start_time_base;
}
	
/*
 * convert from snd_seq_real_time_t to sample count
 */
inline static long queue_time_to_position(const snd_seq_real_time_t *t)
{
	return (long)t->tv_sec * play_mode->rate + (long)(t->tv_nsec * rate_frac_nsec);
}

/*
 * get the current queue position in sample count
 */
static long get_current_queue_position(struct seq_context *ctxp)
{
#if HAVE_SND_SEQ_PORT_INFO_SET_TIMESTAMPING
	snd_seq_get_queue_status(ctxp->handle, ctxp->queue, ctxp->q_status);
	return queue_time_to_position(snd_seq_queue_status_get_real_time(ctxp->q_status));
#else
	return 0;
#endif
}

/*
 * update the current position from the event timestamp
 */
static void update_timestamp_from_event(snd_seq_event_t *ev)
{
	long t = queue_time_to_position(&ev->time.time) - last_queue_offset;
	if (t < 0) {
		// fprintf(stderr, "timestamp underflow! (delta=%d)\n", (int)t);
		t = 0;
	} else if (buffer_time_advance > 0 && t >= buffer_time_advance) {
		// fprintf(stderr, "timestamp overflow! (delta=%d)\n", (int)t);
		t = buffer_time_advance - 1;
	}
	t += buffer_time_offset;
	if (t >= cur_time_offset)
		cur_time_offset = t;
}

/*
 * update the current position from system time
 */
static void update_timestamp(void)
{
	cur_time_offset = (long)(get_current_time() * rate_frac);
}

static void seq_play_event(MidiEvent *ev)
{
	//JAVE  make channel -Q channels quiet, modified some code from readmidi.c
	int gch;
	gch = GLOBAL_CHANNEL_EVENT_TYPE(ev->type);

	if (gch || !IS_SET_CHANNELMASK(quietchannels, ev->channel)){
		//if its a global event or not a masked event
		ev->time = cur_time_offset;
		play_event(ev);
	}
}

static void stop_playing(void)
{
	if(upper_voices) {
		MidiEvent ev;
		ev.type = ME_EOT;
		ev.a = 0;
		ev.b = 0;
		seq_play_event(&ev);
		aq_flush(0);
	}
}

static void doit(struct seq_context *ctxp)
{
	for (;;) {
		while (snd_seq_event_input_pending(ctxp->handle, 1)) {
			if (do_sequencer(ctxp))
				goto __done;
		}
		if (ctxp->active) {
			MidiEvent ev;

			if (IS_STREAM_TRACE) {
				/* remember the last update position */
				if (ctxp->queue >= 0)
					last_queue_offset = get_current_queue_position(ctxp);
				/* advance the buffer position */
				buffer_time_offset += buffer_time_advance;
				ev.time = buffer_time_offset;
			} else {
				/* update the current position */
				if (ctxp->queue >= 0)
					cur_time_offset = get_current_queue_position(ctxp);
				else
					update_timestamp();
				ev.time = cur_time_offset;
			}
			ev.type = ME_NONE;
			play_event(&ev);
			aq_fill_nonblocking();
		}
		if (! ctxp->active || ! IS_STREAM_TRACE) {
			fd_set rfds;
			struct timeval timeout;
			FD_ZERO(&rfds);
			FD_SET(ctxp->fd, &rfds);
			timeout.tv_sec = 0;
			timeout.tv_usec = 10000; /* 10ms */
			if (select(ctxp->fd + 1, &rfds, NULL, NULL, &timeout) < 0)
				goto __done;
		}
	}

__done:
	if (ctxp->active) {
		stop_sequencer(ctxp);
	}
}

static void server_reset(void)
{
	readmidi_read_init();
	playmidi_stream_init();
	if (free_instruments_afterwards)
		free_instruments(0);
	reduce_voice_threshold = 0; /* Disable auto reduction voice */
	buffer_time_offset = 0;
}

static int start_sequencer(struct seq_context *ctxp)
{
	if (play_mode->open_output() < 0) {
		ctl.cmsg(CMSG_FATAL, VERB_NORMAL,
			 "Couldn't open %s (`%c')",
			 play_mode->id_name, play_mode->id_character);
		return 0;
	}
	ctxp->active = 1;

	buffer_time_offset = 0;
	last_queue_offset = 0;
	cur_time_offset = 0;
	if (ctxp->queue >= 0) {
		if (snd_seq_start_queue(ctxp->handle, ctxp->queue, NULL) < 0)
			ctxp->queue = -1;
		else
			snd_seq_drain_output(ctxp->handle);
	}
	if (ctxp->queue < 0) {
		start_time_base = 0;
		start_time_base = get_current_time();
	}

	return 1;
}

static void stop_sequencer(struct seq_context *ctxp)
{
	stop_playing();
	if (ctxp->queue >= 0) {
		snd_seq_stop_queue(ctxp->handle, ctxp->queue, NULL);
		snd_seq_drain_output(ctxp->handle);
	}
	play_mode->close_output();
	free_instruments(0);
	free_global_mblock();
	ctxp->used = 0;
	ctxp->active = 0;
}

#define NOTE_CHAN(ev)	((ev)->dest.port * 16 + (ev)->data.note.channel)
#define CTRL_CHAN(ev)	((ev)->dest.port * 16 + (ev)->data.control.channel)

static int do_sequencer(struct seq_context *ctxp)
{
	int n, ne, i;
	MidiEvent ev, evm[260];
	snd_seq_event_t *aevp;

	n = snd_seq_event_input(ctxp->handle, &aevp);
	if (n < 0 || aevp == NULL)
		return 0;

	if (ctxp->active && ctxp->queue >= 0)
		update_timestamp_from_event(aevp);
	else if (IS_STREAM_TRACE)
		cur_time_offset = buffer_time_offset;
	else
		update_timestamp();

	switch(aevp->type) {
	case SND_SEQ_EVENT_NOTEON:
		ev.channel = NOTE_CHAN(aevp);
		ev.a       = aevp->data.note.note;
		ev.b       = aevp->data.note.velocity;
		if (ev.b == 0)
			ev.type = ME_NOTEOFF;
		else
			ev.type = ME_NOTEON;
		seq_play_event(&ev);
		break;

	case SND_SEQ_EVENT_NOTEOFF:
		ev.channel = NOTE_CHAN(aevp);
		ev.a       = aevp->data.note.note;
		ev.b       = aevp->data.note.velocity;
		ev.type = ME_NOTEOFF;
		seq_play_event(&ev);
		break;

	case SND_SEQ_EVENT_KEYPRESS:
		ev.channel = NOTE_CHAN(aevp);
		ev.a       = aevp->data.note.note;
		ev.b       = aevp->data.note.velocity;
		ev.type = ME_KEYPRESSURE;
		seq_play_event(&ev);
		break;

	case SND_SEQ_EVENT_PGMCHANGE:
		ev.channel = CTRL_CHAN(aevp);
		ev.a = aevp->data.control.value;
		ev.type = ME_PROGRAM;
		seq_play_event(&ev);
		break;

	case SND_SEQ_EVENT_CONTROLLER:
		if(convert_midi_control_change(CTRL_CHAN(aevp),
					       aevp->data.control.param,
					       aevp->data.control.value,
					       &ev))
			seq_play_event(&ev);
		break;

	case SND_SEQ_EVENT_CONTROL14:
		if (aevp->data.control.param < 0 || aevp->data.control.param >= 32)
			break;
		if (! convert_midi_control_change(CTRL_CHAN(aevp),
						  aevp->data.control.param,
						  (aevp->data.control.value >> 7) & 0x7f,
						  &ev))
			break;
		seq_play_event(&ev);
		if (! convert_midi_control_change(CTRL_CHAN(aevp),
						  aevp->data.control.param + 32,
						  aevp->data.control.value & 0x7f,
						  &ev))
			break;
		seq_play_event(&ev);
		break;
		    
	case SND_SEQ_EVENT_PITCHBEND:
		ev.type    = ME_PITCHWHEEL;
		ev.channel = CTRL_CHAN(aevp);
		aevp->data.control.value += 0x2000;
		ev.a       = (aevp->data.control.value) & 0x7f;
		ev.b       = (aevp->data.control.value>>7) & 0x7f;
		seq_play_event(&ev);
		break;

	case SND_SEQ_EVENT_CHANPRESS:
		ev.type    = ME_CHANNEL_PRESSURE;
		ev.channel = CTRL_CHAN(aevp);
		ev.a       = aevp->data.control.value;
		seq_play_event(&ev);
		break;
		
	case SND_SEQ_EVENT_NONREGPARAM:
		/* Break it back into its controler values */
		ev.type = ME_NRPN_MSB;
		ev.channel = CTRL_CHAN(aevp);
		ev.a = (aevp->data.control.param >> 7) & 0x7f;
		seq_play_event(&ev);
		ev.type = ME_NRPN_LSB;
		ev.channel = CTRL_CHAN(aevp);
		ev.a = aevp->data.control.param & 0x7f;
		seq_play_event(&ev);
		ev.type = ME_DATA_ENTRY_MSB;
		ev.channel = CTRL_CHAN(aevp);
		ev.a = (aevp->data.control.value >> 7) & 0x7f;
		seq_play_event(&ev);
		ev.type = ME_DATA_ENTRY_LSB;
		ev.channel = CTRL_CHAN(aevp);
		ev.a = aevp->data.control.value & 0x7f;
		seq_play_event(&ev);
		break;

	case SND_SEQ_EVENT_REGPARAM:
		/* Break it back into its controler values */
		ev.type = ME_RPN_MSB;
		ev.channel = CTRL_CHAN(aevp);
		ev.a = (aevp->data.control.param >> 7) & 0x7f;
		seq_play_event(&ev);
		ev.type = ME_RPN_LSB;
		ev.channel = CTRL_CHAN(aevp);
		ev.a = aevp->data.control.param & 0x7f;
		seq_play_event(&ev);
		ev.type = ME_DATA_ENTRY_MSB;
		ev.channel = CTRL_CHAN(aevp);
		ev.a = (aevp->data.control.value >> 7) & 0x7f;
		seq_play_event(&ev);
		ev.type = ME_DATA_ENTRY_LSB;
		ev.channel = CTRL_CHAN(aevp);
		ev.a = aevp->data.control.value & 0x7f;
		seq_play_event(&ev);
		break;

	case SND_SEQ_EVENT_SYSEX:
		if (parse_sysex_event(aevp->data.ext.ptr + 1,
				 aevp->data.ext.len - 1, &ev))
			seq_play_event(&ev);
		if ((ne = parse_sysex_event_multi(aevp->data.ext.ptr + 1,
						  aevp->data.ext.len - 1, evm)) > 0)
			for (i = 0; i < ne; i++)
				seq_play_event(&evm[i]);
		break;

#if SND_LIB_MAJOR > 0 || SND_LIB_MINOR >= 6
#define snd_seq_addr_equal(a,b)	((a)->client == (b)->client && (a)->port == (b)->port)
	case SND_SEQ_EVENT_PORT_SUBSCRIBED:
		if (snd_seq_addr_equal(&aevp->data.connect.dest, &aevp->dest)) {
			if (! ctxp->active) {
				if (! start_sequencer(ctxp)) {
					snd_seq_free_event(aevp);
					return 0;
				}
			}
			ctxp->used++;
		}
		break;

	case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
		if (snd_seq_addr_equal(&aevp->data.connect.dest, &aevp->dest)) {
			if (ctxp->active) {
				ctxp->used--;
				if (ctxp->used <= 0) {
					snd_seq_free_event(aevp);
					return 1; /* quit now */
				}
			}
		}
		break;
#else
	case SND_SEQ_EVENT_PORT_USED:
		if (! ctxp->active) {
			if (! start_sequencer(ctxp)) {
				snd_seq_free_event(aevp);
				return 0;
			}
		}
		ctxp->used++;
		break;

	case SND_SEQ_EVENT_PORT_UNUSED:
		if (ctxp->active) {
			ctxp->used--;
			if (ctxp->used <= 0) {
				snd_seq_free_event(aevp);
				return 1; /* quit now */
			}
		}
		break;
#endif
		
	default:
		/*printf("Unsupported event %d\n", aevp->type);*/
		break;
	}
	snd_seq_free_event(aevp);
	return 0;
}

/*
 * interface_<id>_loader();
 */
ControlMode *interface_A_loader(void)
{
    return &ctl;
}
