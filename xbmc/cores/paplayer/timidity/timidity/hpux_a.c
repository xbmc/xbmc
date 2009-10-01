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

    hpux_audio.c written by Vincent Pagel pagel@loria.fr

    Functions to play sound on HPUX stations V0.1 1995 March 1

    HPUX allows you to connect to a remote sound server through a socket
    ( put the name in the string "server"). Not compulsory to play the
    sound on the machine running timidity

    Exemple : if I'm on the console of 'exupery' and that I've opened a
    remote connection to 'yeager' , the command line becomes :

    yeager 1% timidity  -o exupery -Od jazzy.mid

    And the sound travels through the net !

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <Alib.h>
#include <CUlib.h>

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

static Audio    *audio;		      /* Audio Connection */
static AErrorHandler   prevHandler;  /* pointer to previous error handler */
static AudioAttributes  SourceAttribs, PlayAttribs;
static AudioAttrMask   SourceAttribsMask, PlayAttribsMask;
static AGainEntry      gainEntry[4];
static ATransID  xid;	/* Socket for connection with audio stream */
static SStream audioStream;
static SSPlayParams    streamParams;
static int streamSocket;
static long status;
static int data_format;
static long   seekOffset, data_length;
static AByteOrder      byte_order, play_byte_order;
static int useIntSpeaker;

/* export the playback mode */
#define DEFAULT_HP_ENCODING PE_16BIT|PE_SIGNED

#define dpm hpux_nplay_mode
PlayMode dpm = {
    DEFAULT_RATE, DEFAULT_HP_ENCODING, PF_PCM_STREAM|PF_CAN_TRACE,
    -1,
    {0}, /* default: get all the buffer fragments you can */
    "HPUX network audio (Alib)", 'A',
    "", /* THIS STRING IS THE NAME OF THE AUDIO SERVER (default =none)*/

    open_output,
    close_output,
    output_data,
    acntl
};

/*
 * error handler for player
 */
long myHandler( Audio  * audio, AErrorEvent  * err_event  )
{
    char    errorbuff[132];

    AGetErrorText(audio, err_event->error_code, errorbuff, 131);
    ctl->cmsg(CMSG_ERROR,VERB_NORMAL,"HPUX Audio error:%s", errorbuff );
    ctl->close();
    exit(1);
}

static int open_output(void)
{
    char *pSpeaker;	/* Environment SPEAKER variable */
    int warnings=0;
    int i;

    if(dpm.encoding & PE_ALAW)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "%s: A-Law not supported in this version", dpm.name);
	return -1;
    }

/* replace default error handler */
    prevHandler = ASetErrorHandler(myHandler);

/*
 *  open audio connection
 */
    audio = AOpenAudio( dpm.name, NULL );

    PlayAttribsMask = 0;
    SourceAttribsMask = 0;

    /* User defined sample rate */
    SourceAttribs.attr.sampled_attr.sampling_rate = dpm.rate;
    SourceAttribsMask = (PlayAttribsMask | ASSamplingRateMask);
    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Playing rate : %i", dpm.rate);

    /* User wants mono or Stereo ?  */
    SourceAttribs.attr.sampled_attr.channels = (dpm.encoding & PE_MONO) ? 1 : 2;
    SourceAttribsMask = (SourceAttribsMask | ASChannelsMask);
    if (dpm.encoding & PE_MONO)
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Sound is mono");
    else
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Sound is stereo dolby fx");

    if (dpm.encoding & PE_ULAW )
	{ data_format= AFFRawMuLaw; /* Ignore the rest signed/unsigned 16/8 */
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Sound format Ulaw");
	}
    else if (dpm.encoding & PE_16BIT )
	{ /* HP700's DO NOT SUPPORT unsigned 16bits */
	    if (! (dpm.encoding & PE_SIGNED))
		{ ctl->cmsg(CMSG_WARNING, VERB_NORMAL,"No unsigned 16bit format");
		dpm.encoding |= PE_SIGNED;
		warnings=1;
		}
	     ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Sound format Linear 16bits");
	    data_format= AFFRawLin16;
	}
    else
	{ if (dpm.encoding & PE_SIGNED)
	   {
	       data_format=AFFRawLin8;
	       ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Sound format Linear signed 8bits");
	   }
	else
	  {
	      data_format=AFFRawLin8Offset;
	      ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Sound format Linear unsigned 8bits");
	  }
	};

    AChooseSourceAttributes(audio, NULL,NULL, data_format,
			    SourceAttribsMask, &SourceAttribs,
			    &seekOffset,&data_length, &byte_order, NULL );

    AChoosePlayAttributes(audio, &SourceAttribs, PlayAttribsMask,
			  &PlayAttribs, &play_byte_order,NULL);

    /* Match the source and play audio parameters and see if all are accepted */
    if (PlayAttribs.attr.sampled_attr.sampling_rate!=
	SourceAttribs.attr.sampled_attr.sampling_rate)
	{ ctl->cmsg(CMSG_WARNING, VERB_NORMAL,"Unsupported sample rate %i replaced by %i",
	       SourceAttribs.attr.sampled_attr.sampling_rate,
	       PlayAttribs.attr.sampled_attr.sampling_rate );
	warnings=1;
	dpm.rate = PlayAttribs.attr.sampled_attr.sampling_rate;
	}

    if (PlayAttribs.attr.sampled_attr.channels!=
	SourceAttribs.attr.sampled_attr.channels)
	{ ctl->cmsg(CMSG_WARNING, VERB_NORMAL,"Unsupported STEREO -> going back mono");
	dpm.encoding |= PE_MONO;
	warnings=1;
	}

    if (PlayAttribs.attr.sampled_attr.data_format !=
	SourceAttribs.attr.sampled_attr.data_format )
	{ ctl->cmsg(CMSG_ERROR, VERB_NORMAL,"Audio device can't play this format, try another one");
	return -1;
	}

    /*
     * Traditionnaly on HPUX, the SPEAKER environment variable is EXTERNAL if we use
     * the headphone jack and INTERNAL if we use the internal speaker
     */
    pSpeaker = getenv( "SPEAKER" );         /* get user speaker preference */
    if ( pSpeaker )
	useIntSpeaker = ( (*pSpeaker == 'i') || (*pSpeaker == 'I') );
    else
	/* SPEAKER environment variable not found - use internal speaker */
	useIntSpeaker = 1;

    /* Tune the stereo */
    switch(PlayAttribs.attr.sampled_attr.channels )
	{
	case 1:
	    gainEntry[0].u.o.out_ch = AOCTMono;
	    gainEntry[0].gain = AUnityGain;
	    gainEntry[0].u.o.out_dst = AODTDefaultOutput;
	    break;
	case 2:
	default:    /* assume no more than 2 channels... for the moment !!! */
	    gainEntry[0].u.o.out_ch = AOCTLeft;
	    gainEntry[0].gain = AUnityGain;
	    gainEntry[0].u.o.out_dst = AODTDefaultOutput;
	    gainEntry[1].u.o.out_ch = AOCTRight;
	    gainEntry[1].gain = AUnityGain;
	    gainEntry[1].u.o.out_dst = AODTDefaultOutput;
	    break;
	}
    streamParams.gain_matrix.type = AGMTOutput;       /* gain matrix */
    streamParams.gain_matrix.num_entries = PlayAttribs.attr.sampled_attr.channels;
    streamParams.gain_matrix.gain_entries = gainEntry;
    streamParams.play_volume = AUnityGain;            /* play volume */
    streamParams.priority = APriorityNormal;          /* normal priority */
    streamParams.event_mask = 0;                      /* don't solicit any events */

/* create an audio stream */
    xid = APlaySStream( audio, ~0, &PlayAttribs, &streamParams,
			&audioStream, NULL );
/* create a stream socket */
    streamSocket = socket( AF_INET, SOCK_STREAM, 0 );
    if( streamSocket < 0 )
	{ ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Audio Socket creation failed" );
	return -1;
	}

    i = 128 * 1024;
    status = setsockopt(streamSocket, SOL_SOCKET, SO_SNDBUF, &i, sizeof(int));
    if(status < 0)
    {
	ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
		  "Warning: SO_SNDBUF: size=%d is failed", i);
	warnings=1;
    }

/* connect the stream socket to the audio stream port */
    status = connect( streamSocket, (struct sockaddr *)&audioStream.tcp_sockaddr,
		      sizeof(struct sockaddr_in) );
    if(status<0)
	{  ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Audio Connect failed" );
	return -1;
	}
    dpm.fd=0;
    return(warnings);
}

static int output_data(char *buf, int32 nbytes)
{
    return write( streamSocket, buf, nbytes );
}

static void close_output(void)
{
    if(dpm.fd != -1)
    {
	close( streamSocket );
	ASetCloseDownMode( audio, AKeepTransactions, NULL );
	ACloseAudio( audio, NULL );
	dpm.fd = -1;
    }
}

static int acntl(int request, void *arg)
{
    switch(request)
    {
      case PM_REQ_DISCARD:
	/* Must be defined this request but I don't know how to do */
/*	fprintf(stderr,
		"hpux_a.c: acntl(): PM_REQ_DISCARD is not implemented.");
 */
	return -1;
    }
    return -1;
}
