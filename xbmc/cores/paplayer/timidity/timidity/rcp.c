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
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
/* rcp.c - written by Masanao Izumo <mo@goice.co.jp> */

#include <stdio.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "controls.h"

#define RCP_MAXCHANNELS 32
/* #define RCP_LOOP_CONT_LIMIT 16 */
#define RCP_LOOP_TIME_LIMIT 600

struct NoteList
{
    int32 gate;			/* Note length */
    int   ch;			/* channel */
    int   note;			/* Note number */
    struct NoteList *next;	/* next note */
};

struct RCPNoteTracer
{
    int gfmt;			/*! RCP format (1 if G36 or G18) */
    int32 at;			/*! current time */
    int32 tempo;		/*! current tempo (sync with current_tempo) */
    int32 tempo_to;		/*! tempo gradate to */
    int tempo_grade;		/*! tempo gradation slope */
    int tempo_step;		/*! tempo gradation step */
    struct NoteList *notes;	/*! note list */
    MBlockList pool;		/*! memory pool for notes */
    struct NoteList *freelist;	/*! free note list */
};

#define SETMIDIEVENT(e, at, t, ch, pa, pb) \
    { (e).time = (at); (e).type = (t); \
      (e).channel = (uint8)(ch); (e).a = (uint8)(pa); (e).b = (uint8)(pb); }

#define MIDIEVENT(at, t, ch, pa, pb) \
    { MidiEvent event; SETMIDIEVENT(event, at, t, ch, pa, pb); \
      readmidi_add_event(&event); }

static int read_rcp_track(struct timidity_file *tf, int trackno, int gfmt);
static int preprocess_sysex(uint8* ex, int ch, int gt, int vel);

/* Note Tracer */
static void ntr_init(struct RCPNoteTracer *ntr, int gfmt, int32 at);
static void ntr_end(struct RCPNoteTracer *ntr);
static void ntr_incr(struct RCPNoteTracer *ntr, int step);
static void ntr_note_on(struct RCPNoteTracer *ntr,
			int ch, int note, int velo, int gate);
static void ntr_wait_all_off(struct RCPNoteTracer *ntr);
#define ntr_at(ntr) ((ntr).at)

#define USER_EXCLUSIVE_LENGTH 24
#define MAX_EXCLUSIVE_LENGTH 1024
static uint8 user_exclusive_data[8][USER_EXCLUSIVE_LENGTH];
static int32 init_tempo;
static int32 init_keysig;
static int play_bias;

#define TEMPO_GRADATION_SKIP		2
#define TEMPO_GRADATION_GRADE		600

int read_rcp_file(struct timidity_file *tf, char *magic0, char *fn)
{
    char buff[361], *p;
    int ntrack, timebase1, timebase2, i, len, gfmt;

    strncpy(buff, magic0, 4);
    if(tf_read(buff + 4, 1, 32-4, tf) != 32-4)
	return 1;
    len = strlen(fn);
    if(strncmp(buff, "RCM-PC98V2.0(C)COME ON MUSIC", 28) == 0)
    {
	/* RCP or R36 */
	gfmt = 0;
	if(check_file_extension(fn, ".r36", 1))
	    current_file_info->file_type = IS_R36_FILE;
	else
	    current_file_info->file_type = IS_RCP_FILE;
    }
    else if(strncmp(buff, "COME ON MUSIC RECOMPOSER RCP3.0", 31) == 0)
    {
	/* G36 or G18 */
	gfmt = 1;
	if(check_file_extension(fn, ".g18", 1))
	    current_file_info->file_type = IS_G18_FILE;
	else
	    current_file_info->file_type = IS_G36_FILE;
    }
    else
	return 1;

    /* title */
    if(tf_read(buff, 1, 64, tf) != 64)
	return 1;
    if(current_file_info->seq_name == NULL)
    {
	buff[64] = '\0';
	for(len = 63; len >= 0; len--)
	{
	    if(buff[len] == ' ')
		buff[len] = '\0';
	    else if(buff[len] != '\0')
		break;
	}

	len = SAFE_CONVERT_LENGTH(len + 1);
	p = (char *)new_segment(&tmpbuffer, len);
	code_convert(buff, p, len, NULL, NULL);
	current_file_info->seq_name = (char *)safe_strdup(p);
	reuse_mblock(&tmpbuffer);
    }
    current_file_info->format = 1;

    if(!gfmt) /* RCP or R36 */
    {
	if(tf_read(buff, 1, 336, tf) != 336)
	    return 1;
#ifndef __BORLANDC__
	buff[336] = '\0';
	for(len = 335; len >= 0; len--)
	{
	    if(buff[len] == ' ')
		buff[len] = '\0';
	    else if(buff[len] != '\0')
		break;
	}
	len = SAFE_CONVERT_LENGTH(len + 1);

	p = (char *)new_segment(&tmpbuffer, len);
	code_convert(buff, p, len, NULL, NULL);
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "Memo: %s", p);
	reuse_mblock(&tmpbuffer);
#else
	buff[336] = '\0';
	{
	char tmp[30];
	int i;
	for(i=0;i<336;i+=28)
	{
	strncpy(tmp,buff+i,28);
	tmp[28] = '\0';
	for(len=28; len>=0; len--)
	{
	    if(tmp[len] == ' ')
			tmp[len] = '\0';
	    else if(tmp[len] != '\0')
			break;
	}
	if(tmp[0]=='\0')
		continue;
	len = SAFE_CONVERT_LENGTH(len + 1);

	p = (char *)new_segment(&tmpbuffer, len);
	code_convert(tmp, p, len, NULL, NULL);
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "Memo: %s", p);
	reuse_mblock(&tmpbuffer);
	}
	}
#endif

	skip(tf, 16);		/* 0x40 */

	timebase1 = tf_getc(tf);
	init_tempo = tf_getc(tf); /* tempo */
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Tempo %d", init_tempo);
	if(init_tempo < 8 || init_tempo > 250)
	    init_tempo = 120;

	/* Time Signature: numerator, denominator, Key Signature */
	current_file_info->time_sig_n = tf_getc(tf);
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Time signature(n) %d",
		  current_file_info->time_sig_n);
	current_file_info->time_sig_d = tf_getc(tf);
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Time signature(d) %d",
		  current_file_info->time_sig_d);
	init_keysig = tf_getc(tf);
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Key signature %d", init_keysig);
	if (init_keysig < 0 || init_keysig >= 32)
		init_keysig = 0;

	play_bias = (int)(signed char)tf_getc(tf);
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Play bias %d", play_bias);
	if(play_bias < -36 || play_bias > 36)
	    play_bias = 0;

	skip(tf, 12);		/* cm6 */
	skip(tf, 4);		/* reserved */
	skip(tf, 12);		/* gsd */
	skip(tf, 4);		/* reserved */

	if((ntrack = tf_getc(tf)) == EOF)
	    return 1;
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Number of tracks %d", ntrack);
	if(ntrack != 18 && ntrack != 36)
	    ntrack = 18;
	timebase2 = tf_getc(tf);
	skip(tf, 14);		/* reserved */
	skip(tf, 16);		/*  */

	skip(tf, 32 * (14 + 2)); /* rhythm definition */
    }
    else /* G36 or G18 */
    {
	skip(tf, 64);	/* reserved */

	/* memo */
	if(tf_read(buff, 1, 360, tf) != 360)
	    return 1;
	buff[360] = '\0';
	for(len = 359; len >= 0; len--)
	{
	    if(buff[len] == ' ')
		buff[len] = '\0';
	    else if(buff[len] != '\0')
		break;
	}
	len = SAFE_CONVERT_LENGTH(len + 1);
	p = (char *)new_segment(&tmpbuffer, len);
	code_convert(buff, p, len, NULL, NULL);
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "Memo: %s", p);
	reuse_mblock(&tmpbuffer);

	/* Number of tracks */
	ntrack = tf_getc(tf);
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Number of tracks %d", ntrack);
	if(ntrack != 18 && ntrack != 36)
	    ntrack = 18;
	skip(tf, 1);
	timebase1 = tf_getc(tf);
	timebase2 = tf_getc(tf);
	init_tempo = tf_getc(tf); /* tempo */
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Tempo %d", init_tempo);
	if(init_tempo < 8 || init_tempo > 250)
	    init_tempo = 120;
	skip(tf, 1); /* ?? */

	/* Time Signature: numerator, denominator, Key Signature */
	current_file_info->time_sig_n = tf_getc(tf);
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Time signature(n) %d",
		  current_file_info->time_sig_n);
	current_file_info->time_sig_d = tf_getc(tf);
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Time signature(n) %d",
		  current_file_info->time_sig_d);
	init_keysig = tf_getc(tf);
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Key signature %d", init_keysig);
	if (init_keysig < 0 || init_keysig >= 32)
		init_keysig = 0;

	play_bias = (int)(signed char)tf_getc(tf);
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Play bias %d", play_bias);
	if(play_bias < -36 || play_bias > 36)
	    play_bias = 0;
	skip(tf, 6 + 16 + 112);	/* reserved */
	skip(tf, 12);		/* gsd */
	skip(tf, 4);
	skip(tf, 12);		/* gsd */
	skip(tf, 4);
	skip(tf, 12);		/* cm6 */
	skip(tf, 4);
	skip(tf, 80);		/* reserved */
	skip(tf, 128 * (14 + 2)); /* rhythm definition */
    }

    /* SysEx data */
    for(i = 0; i < 8; i++)
    {
	int mid;
	skip(tf, 24);	/* memo */
	if(tf_read(user_exclusive_data[i], 1, USER_EXCLUSIVE_LENGTH, tf)
	   != USER_EXCLUSIVE_LENGTH)
	    return 1;
	mid = user_exclusive_data[i][0];
	if(mid > 0 && mid < 0x7e &&
	   (current_file_info->mid == 0 || current_file_info->mid >= 0x7e))
	    current_file_info->mid = mid;
    }

    current_file_info->divisions = (timebase1 | (timebase2 << 8));
    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "divisions %d",
	      current_file_info->divisions);
    current_file_info->format = 1;
    current_file_info->tracks = ntrack;

    if(!IS_URL_SEEK_SAFE(tf->url))
	if((tf->url = url_cache_open(tf->url, 1)) == NULL)
	    return 1;

    for(i = 0; i < ntrack; i++)
	if(read_rcp_track(tf, i, gfmt))
	{
	    if(ignore_midi_error)
		return 0;
	    return 1;
	}
    return 0;
}

static void rcp_tempo_set(int32 at, int32 tempo)
{
    int lo, mid, hi;
    
    lo = (tempo & 0xff);
    mid = ((tempo >> 8) & 0xff);
    hi = ((tempo >> 16) & 0xff);
    MIDIEVENT(at, ME_TEMPO, lo, hi, mid);
}

static int32 rcp_tempo_change(struct RCPNoteTracer *ntr, int a, int b)
{
    int32 tempo;
    
    tempo = (int32)((uint32)60000000 * 64 / (init_tempo * a));	/* 6*10^7 / (ini*a/64) */
    ntr->tempo_grade = b;
    if (b != 0)
    {
	ntr->tempo_to = tempo;
	ntr->tempo_step = 0;
	ntr->tempo_grade *= TEMPO_GRADATION_GRADE * TEMPO_GRADATION_SKIP;
	tempo = ntr->tempo;	/* unchanged */
    }
    else
    {
	ntr->tempo = tempo;
	rcp_tempo_set(ntr_at(*ntr), tempo);
    }
    return tempo;
}

static void rcp_timesig_change(int32 at)
{
	int n, d;
	
	n = current_file_info->time_sig_n;
	d = current_file_info->time_sig_d;
	MIDIEVENT(at, ME_TIMESIG, 0, n, d);
}

static void rcp_keysig_change(int32 at, int32 key)
{
	int8 sf, mi;
	
	if (key < 8)
		sf = key, mi = 0;
	else if (key < 16)
		sf = 8 - key, mi = 0;
	else if (key < 24)
		sf = key - 16, mi = 1;
	else
		sf = 24 - key, mi = 1;
	MIDIEVENT(at, ME_KEYSIG, 0, sf, mi);
}

static char *rcp_cmd_name(int cmd)
{
    if(cmd < 0x80)
    {
	static char name[16];
	sprintf(name, "NoteOn %d", cmd);
	return name;
    }

    switch(cmd)
    {
      case 0x90: return "UserExclusive0";
      case 0x91: return "UserExclusive1";
      case 0x92: return "UserExclusive2";
      case 0x93: return "UserExclusive3";
      case 0x94: return "UserExclusive4";
      case 0x95: return "UserExclusive5";
      case 0x96: return "UserExclusive6";
      case 0x97: return "UserExclusive7";
      case 0x98: return "ChannelExclusive";
      case 0xc0: return "DX7 function";
      case 0xc1: return "DX parameter";
      case 0xc2: return "DX RERF";
      case 0xc3: return "TX function";
      case 0xc5: return "FB-01 P parameter";
      case 0xc6: return "FB-01 S System";
      case 0xc7: return "TX81Z V VCED";
      case 0xc8: return "TX81Z A ACED";
      case 0xc9: return "TX81Z P PCED";
      case 0xca: return "TX81Z S System";
      case 0xcb: return "TX81Z E EFFECT";
      case 0xcc: return "DX7-2 R REMOTE SW";
      case 0xcd: return "DX7-2 A ACED";
      case 0xce: return "DX7-2 P PCED";
      case 0xcf: return "TX802 P PCED";
      case 0xd0: return "YamahaBase";
      case 0xd1: return "YamahaPara";
      case 0xd2: return "YamahaDevice";
      case 0xd3: return "XGPara";
      case 0xdc: return "MKS-7";
      case 0xdd: return "RolandBase";
      case 0xde: return "RolandPara";
      case 0xdf: return "RolandDevice";
      case 0xe1: return "BnkLPrg";
      case 0xe2: return "Bank&ProgCng";
      case 0xe5: return "KeyScan";
      case 0xe6: return "ChChange";
      case 0xe7: return "TempoChange";
      case 0xea: return "ChannelAfterTouch";
      case 0xeb: return "ControlChange";
      case 0xec: return "ProgChange";
      case 0xed: return "AfterTouch";
      case 0xee: return "PitchBend";
      case 0xf5: return "KeyChange";
      case 0xf6: return "Comment";
      case 0xf7: return "2ndEvent";
      case 0xf8: return "LoopEnd";
      case 0xf9: return "LoopStart";
      case 0xfc: return "SameMeasure";
      case 0xfd: return "MeasureEnd";
      case 0xfe: return "EndOfTrack";
    }
    return "Unknown";
}

static int rcp_parse_sysex_event(int32 at, uint8 *val, int32 len)
{
    MidiEvent ev, evm[260];
    int ne, i;

    if(len == 0) {return 0;}

    if(parse_sysex_event(val, len, &ev))
    {
	ev.time = at;
	readmidi_add_event(&ev);
    }
	if ((ne = parse_sysex_event_multi(val, len, evm)) > 0)
		for (i = 0; i < ne; i++) {
			evm[i].time = at;
			readmidi_add_event(&evm[i]);
		}
    
    return 0;
}

#define MAX_STACK_DEPTH 16
static int read_rcp_track(struct timidity_file *tf, int trackno, int gfmt)
{
    int32 current_tempo;
    int size;
    int ch;
    long last_point, cmdlen;
    int key_offset;
    struct RCPNoteTracer ntr;
    struct
    {
	long start_at;		/* loop start time */
	long loop_start;	/* loop start seek point */
	int  count;		/* loop count */
    } stack[MAX_STACK_DEPTH];
    int sp;
    int i, len;
    uint8 sysex[MAX_EXCLUSIVE_LENGTH];

    int roland_base_init = 0;
    int roland_dev_init = 0;
    int roland_base_addr0 = 0;
    int roland_base_addr1 = 0;
    int roland_device_id = 0x10;
    int roland_model_id = 0x16;

    int yamaha_base_init = 0;
    int yamaha_dev_init = 0;
    int yamaha_base_addr0 = 0;
    int yamaha_base_addr1 = 0;
    int yamaha_device_id = 0x10;	/* ??? */
    int yamaha_model_id = 0x16;		/* ??? */

    unsigned char cs;		/* check sum */

    long same_measure = -1;	/* Meassure to stack pointer */
    long track_top, data_top;
    int time_offset;
    char buff[64], *p;
    int ret = 0;

    /*
     * Read Track Header
     */

    track_top = tf_tell(tf);
    ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY, "Track top: %d", track_top);

    size = tf_getc(tf);
    size |= (tf_getc(tf) << 8);

    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Track size %d", size);
    last_point = size + tf_tell(tf) - 2;

    if(gfmt)
    {
	skip(tf, 2);
	cmdlen = 6;
    }
    else
	cmdlen = 4;

    i = tf_getc(tf);		/* Track Number */
    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Track number %d", i);
    skip(tf, 1);		/* Rhythm */

    if((ch = tf_getc(tf)) == 0xff) /* No playing */
    {
	tf_seek(tf, last_point, SEEK_SET);
	return 0;
    }

    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Track channel %d", ch);

    if(ch >= RCP_MAXCHANNELS)
	ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		  "RCP: Invalid channel: %d", ch);

    /* Key offset */
    key_offset = tf_getc(tf);
    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Key offset %d", key_offset);
    if(key_offset > 64)
	key_offset -= 128;
    key_offset += play_bias;

    /* Time offset */
    time_offset = (int)(signed char)tf_getc(tf);
    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Time offset %d", time_offset);
    if(time_offset < -99 || 99 < time_offset)
    {
	ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		  "RCP: Invalid time offset: %d", time_offset);
	if(time_offset < -99)
	    time_offset = -99;
	else
	    time_offset = 99;
    }

    i = tf_getc(tf);		/* mode */
    if(i == 1)
    {
	/* Mute */
	tf_seek(tf, last_point, SEEK_SET);
	return 0;
    }

    /* Comment */
    tf_read(buff, 1, 36, tf);
    buff[36] = '\0';
    for(len = 35; len >= 0; len--)
    {
	if(buff[len] == ' ')
	    buff[len] = '\0';
	else if(buff[len] != '\0')
	    break;
    }
    len = SAFE_CONVERT_LENGTH(len+1);
    p = (char *)new_segment(&tmpbuffer, len);
    code_convert(buff, p, len, NULL, NULL);
    if(*p)
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "RCP Track name: %s", p);
    reuse_mblock(&tmpbuffer);

    /*
     * End of Track Header
     */

    sp = 0;
    ntr.tempo = current_tempo = 60000000 / init_tempo;
    ntr_init(&ntr, gfmt, readmidi_set_track(trackno, 1));
    if(trackno == 0 && init_tempo != 120)
	ntr.tempo = current_tempo = rcp_tempo_change(&ntr, 64, 0);
	if (trackno == 0) {
		rcp_timesig_change(ntr_at(ntr));
		rcp_keysig_change(ntr_at(ntr), init_keysig);
	}
    ntr_incr(&ntr, time_offset);

    data_top = tf_tell(tf);
    while(last_point >= tf_tell(tf) + cmdlen)
    {
	int cmd, a, b, step, gate;
	int st1, gt1, st2, gt2;

	if(readmidi_error_flag)
	{
	    ret = -1;
	    break;
	}

	if(!gfmt)
	{
	    cmd = tf_getc(tf);
	    step = tf_getc(tf);
	    gate = a = tf_getc(tf);
	    if((b = tf_getc(tf)) == EOF)
	    {
		ret = -1;
		break;
	    }
	    st1 = gt1 = st2 = gt2 = 0;

	    if(ctl->verbosity >= VERB_DEBUG_SILLY)
	    {
		ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
			  "[%d] %d %s: ch=%d step=%d a=%d b=%d sp=%d",
			  tf_tell(tf) - 4 - track_top,
			  ntr_at(ntr), rcp_cmd_name(cmd), ch, step, a, b, sp);
	    }
	}
	else
	{
	    cmd = tf_getc(tf);
	    b = tf_getc(tf);
	    st1 = tf_getc(tf);
	    st2 = tf_getc(tf);
	    a = gt1 = tf_getc(tf);
	    if((gt2 = tf_getc(tf)) == EOF)
	    {
		ret = -1;
		break;
	    }

	    step = st1 + (st2 << 8);
	    gate = gt1 + (gt2 << 8);

	    if(ctl->verbosity >= VERB_DEBUG_SILLY)
	    {
		ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
			  "[%d] %d %s: ch=%d step=%d gate=%d a=%d b=%d sp=%d",
			  tf_tell(tf) - 6 - track_top,
			  ntr_at(ntr), rcp_cmd_name(cmd), ch,
			  step, gate, a, b, sp);
	    }
	}

	if(cmd < 0x80)		/* Note event */
	{
	    if(gate > 0)
	    {
		int note = cmd + key_offset;
		ntr_note_on(&ntr, ch, note & 0x7f, b, gate);
	    }
	    ntr_incr(&ntr, step);
	    continue;
	}

	/* command */
	switch(cmd)
	{
	  case 0x90:	/* User Exclusive 1 */
	  case 0x91:	/* User Exclusive 2 */
	  case 0x92:	/* User Exclusive 3 */
	  case 0x93:	/* User Exclusive 4 */
	  case 0x94:	/* User Exclusive 5 */
	  case 0x95:	/* User Exclusive 6 */
	  case 0x96:	/* User Exclusive 7 */
	  case 0x97:	/* User Exclusive 8 */
	    memcpy(sysex, user_exclusive_data[cmd - 0x90],
		   USER_EXCLUSIVE_LENGTH);
	    sysex[USER_EXCLUSIVE_LENGTH] = 0xf7;
	    len = preprocess_sysex(sysex, ch, a, b);
	    rcp_parse_sysex_event(ntr_at(ntr), sysex, len);
	    ntr_incr(&ntr, step);
	    break;

	  case 0x98:	/* Channel Exclusive */
	    len = 0;
	    while(tf_getc(tf) == 0xf7 && len + 6 < sizeof(sysex))
	    {
		if(gfmt)
		{
		    if(tf_read(sysex + len, 1, 5, tf) != 5)
		    {
			ret = -1;
			goto end_of_track;
		    }
		    len += 5;
		}
		else
		{
		    tf_getc(tf); /* 0x00 */
		    if(tf_read(sysex + len, 1, 2, tf) != 2)
		    {
			ret = -1;
			goto end_of_track;
		    }
		    len += 2;
		}
	    }
	    tf_seek(tf, -1, SEEK_CUR);
	    sysex[len] = 0xf7;
	    len = preprocess_sysex(sysex, ch, a, b);
	    rcp_parse_sysex_event(ntr_at(ntr), sysex, len);
	    ntr_incr(&ntr, step);
	    break;

	  case 0xd0:	/* Yamaha base */
	    a &= 0x7f;
	    b &= 0x7f;
	    yamaha_base_addr0 = a;
	    yamaha_base_addr1 = b;
	    yamaha_base_init = 1;
	    ntr_incr(&ntr, step);
	    break;

	  case 0xd1:	/* Yamaha para */
	    a &= 0x7f;
	    b &= 0x7f;
	    if(!yamaha_base_init)
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
			  "YamPara used before initializing YamBase");
	    if(!yamaha_dev_init)
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
			  "YamPara used before initializing YamDev#");
	    yamaha_base_init = yamaha_dev_init = 1;
	    sysex[0] = 0x43;
	    sysex[1] = yamaha_device_id;
	    sysex[2] = yamaha_model_id;
	    sysex[3] = 0x12;
	    cs = 0;
	    cs += sysex[4] = yamaha_base_addr0;
	    cs += sysex[5] = yamaha_base_addr1;
	    cs += sysex[6] = a;
	    cs += sysex[7] = b;
	    sysex[8] = 128 - (cs & 0x7f);
	    sysex[9] = 0xf7;
		rcp_parse_sysex_event(ntr_at(ntr), sysex, 10);
	    ntr_incr(&ntr, step);
	    break;

	  case 0xd2:	/* Yamaha device */
	    a &= 0x7f;
	    b &= 0x7f;
	    yamaha_device_id = a;
	    yamaha_model_id = b;
	    yamaha_dev_init = 1;
	    ntr_incr(&ntr, step);
	    break;

	  case 0xd3:	/* XG para */
	    /* ?? */
	    a &= 0x7f;
	    b &= 0x7f;
	    sysex[0] = 0x43;
	    sysex[1] = yamaha_device_id;
	    sysex[2] = yamaha_model_id;
	    sysex[3] = 0x12;
	    cs = 0;
	    cs += sysex[4] = yamaha_base_addr0;
	    cs += sysex[5] = yamaha_base_addr1;
	    cs += sysex[6] = a;
	    cs += sysex[7] = b;
	    sysex[8] = 128 - (cs & 0x7f);
	    sysex[9] = 0xf7;
		rcp_parse_sysex_event(ntr_at(ntr), sysex, 10);
	    ntr_incr(&ntr, step);
	    break;

	  case 0xdd:	/* Roland base */
	    a &= 0x7f;
	    b &= 0x7f;
	    roland_base_addr0 = a;
	    roland_base_addr1 = b;
	    roland_base_init = 1;
	    ntr_incr(&ntr, step);
	    break;

	  case 0xde:	/* Roland para */
	    a &= 0x7f;
	    b &= 0x7f;
	    if(!roland_base_init)
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
			  "RolPara used before initializing RolBase");
	    if(!roland_dev_init)
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
			  "RolPara used before initializing RolDev#");
	    roland_base_init = roland_dev_init = 1;
	    sysex[0] = 0x41;
	    sysex[1] = roland_device_id;
	    sysex[2] = roland_model_id;
	    sysex[3] = 0x12;
	    cs = 0;
	    cs += sysex[4] = roland_base_addr0;
	    cs += sysex[5] = roland_base_addr1;
	    cs += sysex[6] = a;
	    cs += sysex[7] = b;
	    sysex[8] = 128 - (cs & 0x7f);
	    sysex[9] = 0xf7;
		rcp_parse_sysex_event(ntr_at(ntr), sysex, 10);
	    ntr_incr(&ntr, step);
	    break;

	  case 0xdf:	/* Roland device */
	    a &= 0x7f;
	    b &= 0x7f;
	    roland_device_id = a;
	    roland_model_id = b;
	    roland_dev_init = 1;
	    ntr_incr(&ntr, step);
	    break;

	  case 0xe1:	/* BnkLPrg */
	    ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		      "BnkLPrg is not supported: 0x%02x 0x%02x", a, b);
	    ntr_incr(&ntr, step);
	    break;

	  case 0xe2:	/* bank & program change */
	    readmidi_add_ctl_event(ntr_at(ntr), ch, 0, b); /*Change MSB Bank*/
	    MIDIEVENT(ntr_at(ntr), ME_PROGRAM, ch, a & 0x7f, 0);
	    ntr_incr(&ntr, step);
	    break;

	  case 0xe5:	/* key scan */
#if 0
	    switch(a)
	    {
	      case 12: /* suspend playing */
	      case 18: /* increase play bias */
	      case 23: /* stop playing */
	      case 32: /* show main screen */
	      case 33: /* show 11th track */
	      case 34: /* show 12th track */
	      case 35: /* show 13th track */
	      case 36: /* show 14th track */
	      case 37: /* show 15th track */
	      case 38: /* show 16th track */
	      case 39: /* show 17th track */
	      case 40: /* show 18th track */
	      case 48: /* show 10th track */
	      case 49: /* show 1st track */
	      case 50: /* show 2nd track */
	      case 51: /* show 3rd track */
	      case 52: /* show 4th track */
	      case 53: /* show 5th track */
	      case 54: /* show 6th track */
	      case 55: /* show 7th track */
	      case 56: /* show 8th track */
	      case 57: /* show 9th track */
	      case 61: /* mute 1st track */
		break;
	    }
#endif
	    ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		      "Key scan 0x%02x 0x%02x is not supported", a, b);
	    ntr_incr(&ntr, step);
	    break;

	  case 0xe6:	/* MIDI channel change */
	    ch = a - 1;
	    if(ch == 0) /* ##?? */
		ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "MIDI channel off");
	    else if(ch >= RCP_MAXCHANNELS)
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
			  "RCP: Invalid channel: %d", ch);
	    ntr_incr(&ntr, step);
	    break;

	  case 0xe7:	/* tempo change */
	    if(a == 0)
	    {
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
			  "Invalid tempo change\n");
		a = 64;
	    }
	    current_tempo = rcp_tempo_change(&ntr, a, b);
	    ntr_incr(&ntr, step);
	    break;

	  case 0xea:	/* channel after touch (channel pressure) */
	    a &= 0x7f;
	    MIDIEVENT(ntr_at(ntr), ME_CHANNEL_PRESSURE, ch, a, 0);
	    ntr_incr(&ntr, step);
	    break;

	  case 0xeb:	/* control change */
	    readmidi_add_ctl_event(ntr_at(ntr), ch, a, b);
	    ntr_incr(&ntr, step);
	    break;

	  case 0xec:	/* program change */
	    a &= 0x7f;
	    MIDIEVENT(ntr_at(ntr), ME_PROGRAM, ch, a, 0);
	    ntr_incr(&ntr, step);
	    break;

	  case 0xed:	/* after touch polyphonic (polyphonic key pressure) */
	    a &= 0x7f;
	    b &= 0x7f;
	    MIDIEVENT(ntr_at(ntr), ME_KEYPRESSURE, ch, a, b);
	    ntr_incr(&ntr, step);
	    break;

	  case 0xee:	/* pitch bend */
	    a &= 0x7f;
	    b &= 0x7f;
	    MIDIEVENT(ntr_at(ntr), ME_PITCHWHEEL, ch, a, b);
	    ntr_incr(&ntr, step);
	    break;

	case 0xf5:	/* key change */
		if (step < 0 || step >= 32) {
			ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Invalid key change\n");
			step = 0;
		}
		rcp_keysig_change(ntr_at(ntr), step);
		break;

	  case 0xf6:	/* comment */
	    len = 0;
	    if(!gfmt)
	    {
		buff[len++] = a;
		buff[len++] = b;
	    }
	    else
	    {
		buff[len++] = b;
		buff[len++] = st1;
		buff[len++] = st2;
		buff[len++] = gt1;
		buff[len++] = gt2;
	    }

	    while(tf_getc(tf) == 0xf7 && len + 6 < sizeof(buff))
	    {
		if(gfmt)
		{
		    if(tf_read(buff + len, 1, 5, tf) != 5)
		    {
			ret = -1;
			goto end_of_track;
		    }
		    len += 5;
		}
		else
		{
		    tf_getc(tf); /* 0x00 */
		    if(tf_read(buff + len, 1, 2, tf) != 2)
		    {
			ret = -1;
			goto end_of_track;
		    }
		    len += 2;
		}
	    }
	    tf_seek(tf, -1, SEEK_CUR);

	    if(ctl->verbosity >= VERB_VERBOSE || opt_trace_text_meta_event)
	    {
		buff[len] = '\0';
		for(len--; len >= 0; len--)
		{
		    if(buff[len] == ' ')
			buff[len] = '\0';
		    else if(buff[len] != '\0')
			break;
		}

		if(opt_trace_text_meta_event)
		{
		    MidiEvent ev;
		    if(readmidi_make_string_event(ME_TEXT, buff, &ev, 1)
		       != NULL)
		    {
			ev.time = ntr_at(ntr);
			readmidi_add_event(&ev);
		    }
		}
		else if(ctl->verbosity >= VERB_VERBOSE)
		{
		    len = SAFE_CONVERT_LENGTH(len+1);
		    p = (char *)new_segment(&tmpbuffer, len);
		    code_convert(buff, p, len, NULL, NULL);
		    ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "Comment: %s", p);
		    reuse_mblock(&tmpbuffer);
		}
	    }
	    break;

	  case 0xf7:	/* 2nd Event */
	    ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		      "Something's wrong: 2nd Event is appeared");
	    break;

	  case 0xf8:	/* loop end */
	    if(ctl->verbosity >= VERB_DEBUG_SILLY)
		ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
			  "Loop end at %d",
			  tf_tell(tf) - track_top - cmdlen);
	    if(sp == 0)
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
			  "Misplaced loop end (ch=%d)", ch);
	    else if(sp > MAX_STACK_DEPTH)
		sp--;		/* through deeply loop */
	    else
	    {
		int top;

		top = sp - 1;

		if(same_measure == top)
		{
		    sp = same_measure;
		    tf_seek(tf, stack[sp].loop_start, SEEK_SET);
		    same_measure = -1;
		    goto break_loop_end;
		}

		if((!gfmt && step == 0xff) || (gfmt && step == 0xffff))
		{
		    sp = top;
		    goto break_loop_end;
		}

		if(stack[top].count >= step)
		    sp = top;
#ifdef RCP_LOOP_CONT_LIMIT
		else if(stack[top].count >= RCP_LOOP_CONT_LIMIT)
		{
		    ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
			      "Loop limit exceeded (ch=%d)", ch);
		    sp = top;
		}
#endif /* RCP_LOOP_CONT_LIMIT */
#ifdef RCP_LOOP_TIME_LIMIT
		else if((current_tempo / 1000000.0) *
			(ntr_at(ntr) - stack[top].start_at)
			/ current_file_info->divisions
			> RCP_LOOP_TIME_LIMIT)
		{
		    ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
			      "Loop time exceeded (ch=%d)", ch);
		    sp = top;
		}
#endif /* RCP_LOOP_TIME_LIMIT */
		else
		{
		    stack[top].count++;
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
			      "Jump to %d (cnt=%d)",
			      stack[top].loop_start - track_top,
			      stack[top].count);
		    tf_seek(tf, stack[top].loop_start, SEEK_SET);
		}
	    }
	  break_loop_end:
	    break;

	  case 0xf9:	/* loop start */
#if 1
	    if(!gfmt && step == 0xff) /* ##?? */
		continue;
#endif
	    if(ctl->verbosity >= VERB_DEBUG_SILLY)
		ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
			  "Loop start at %d",
			  tf_tell(tf) - track_top - cmdlen);

	    if(sp >= MAX_STACK_DEPTH)
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
			  "Too deeply nested %d (ch=%d, Ignored this loop)",
			  ch, sp);
	    else
	    {
		stack[sp].start_at = ntr_at(ntr);
		stack[sp].loop_start = tf_tell(tf);
		stack[sp].count = 1;
	    }
	    sp++;
	    break;

	  case 0xfc:	/* same measure */
	    if(ctl->verbosity >= VERB_DEBUG_SILLY)
		ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
			  "Same measure at %d",
			  tf_tell(tf) - track_top - cmdlen);
	    if(sp >= MAX_STACK_DEPTH)
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
			  "Too deeply nested %d (ch=%d)", sp, ch);
	    else
	    {
		long jmp, nextflag;

		if(same_measure >= 0)
		{
		    sp = same_measure;
		    tf_seek(tf, stack[sp].loop_start, SEEK_SET);
		    same_measure = -1;
		    goto break_same_measure;
		}

		nextflag = 0;

	      next_same_measure:
		if(!gfmt)
		{
		    jmp = (long)a | ((long)b << 8);

		    if(jmp & 0x03) /* ##?? */
		    {
			/* What do these two bits mean? */
			/* Clear them here. */
			ctl->cmsg(CMSG_WARNING, VERB_DEBUG,
				  "Jump %d is changed to %d",
				  jmp, jmp & ~3);
			jmp &= ~3;		/* jmp=(jmp/4)*4 */
		    }
		}
		else
		{
#if 0
		    if(b != 0)
		    {
			/* ##?? */
		    }
#endif
		    jmp = gate;
		    if(current_file_info->file_type == IS_G36_FILE)
			jmp = jmp * 6 - 242;
		    else
			jmp -= (jmp + 2) % 6; /* (jmp+2)%6==0 */
		}

		if(jmp < data_top - track_top ||
		   jmp >= last_point - track_top)
		{
		    ctl->cmsg(CMSG_WARNING, VERB_NOISY,
			      "RCP Invalid same measure: %d (ch=%d)", jmp, ch);
		    break;
		}

		if(nextflag == 0)
		{
		    stack[sp].start_at = ntr_at(ntr);
		    stack[sp].loop_start = tf_tell(tf);
		    same_measure = sp;
		    sp++;
		}

		ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY, "Jump to %d", jmp);
		tf_seek(tf, track_top + jmp, SEEK_SET);

		if(tf_getc(tf) == 0xfc)
		{
		    nextflag = 1;
		    if(!gfmt)
		    {
			tf_getc(tf); /* step */
			a = tf_getc(tf);
			b = tf_getc(tf);
		    }
		    else
		    {
			b = tf_getc(tf);
			tf_getc(tf); /* st1 */
			tf_getc(tf); /* st2 */
			a = gt1 = tf_getc(tf);
			gate = gt1 + (gt2 << 8);
		    }
		    goto next_same_measure;
		}
		tf_seek(tf, -1, SEEK_CUR);
	    }
	  break_same_measure:
	    break;

	  case 0xfd:	/* measure end */
	    if(same_measure >= 0)
	    {
		sp = same_measure;
		tf_seek(tf, stack[sp].loop_start, SEEK_SET);
		same_measure = -1;
	    }
	    break;

	  case 0xfe:	/* end of track */
	    goto end_of_track;

	  case 0xc0:	/* DX7 function */
	  case 0xc1:	/* DX parameter */
	  case 0xc2:	/* DX RERF */
	  case 0xc3:	/* TX function */
	  case 0xc5:	/* FB-01 P parameter */
	  case 0xc6:	/* FB-01 S System */
	  case 0xc7:	/* TX81Z V VCED */
	  case 0xc8:	/* TX81Z A ACED */
	  case 0xc9:	/* TX81Z P PCED */
	  case 0xca:	/* TX81Z S System */
	  case 0xcb:	/* TX81Z E EFFECT */
	  case 0xcc:	/* DX7-2 R REMOTE SW */
	  case 0xcd:	/* DX7-2 A ACED */
	  case 0xce:	/* DX7-2 P PCED */
	  case 0xcf:	/* TX802 P PCED */
	  case 0xdc:	/* MKS-7 */
	    if(!gfmt)
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
			  "RCP %s is not unsupported: "
			  "0x%02x 0x%02x 0x%02x 0x%02x (ch=%d)",
			  rcp_cmd_name(cmd), cmd, step, a, b, ch);
	    else
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
			  "RCP %s is not unsupported: "
			  "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x (ch=%d)",
			  rcp_cmd_name(cmd), cmd, b, st1, st2, gt1, gt2, ch);
	    ntr_incr(&ntr, step);
	    break;
	  default:
	    if(!gfmt)
	    {
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
			  "RCP Unknown Command: 0x%02x 0x%02x 0x%02x 0x%02x "
			  "(ch=%d)",
			  cmd, step, a, b, ch);
		/* ##?? */
		if(cmd == 0xe9 && step == 0xf0 && a == 0xe0 && b == 0x80)
		{
		    ctl->cmsg(CMSG_ERROR, VERB_VERBOSE,
			      "RCP e9 f0 e0 80 is end of track ??");
		    goto end_of_track;
		}
	    }
	    else
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
			  "RCP Unknown Command: "
			  "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x (ch=%d)",
			  cmd, b, st1, st2, gt1, gt2, ch);
#if 0
	    ntr_incr(&ntr, step);
#endif
	    break;
	}
    }

  end_of_track:
    /* All note off */
    ntr_wait_all_off(&ntr);
    ntr_end(&ntr);
    tf_seek(tf, last_point, SEEK_SET);
    return ret;
}

static void ntr_init(struct RCPNoteTracer *ntr, int gfmt, int32 at)
{
    memset(ntr, 0, sizeof(*ntr));
    init_mblock(&ntr->pool);
    ntr->gfmt = gfmt;
    ntr->at = at;
    ntr->tempo_grade = 0;
}

static void ntr_end(struct RCPNoteTracer *ntr)
{
    reuse_mblock(&ntr->pool);
}

static void ntr_add(struct RCPNoteTracer *ntr, int ch, int note, int gate)
{
    struct NoteList *p;

    if(ntr->freelist)
    {
	p = ntr->freelist;
	ntr->freelist = ntr->freelist->next;
    }
    else
	p = (struct NoteList *)new_segment(&ntr->pool,
					   sizeof(struct NoteList));
    p->gate = gate;
    p->ch = ch;
    p->note = note;
    p->next = ntr->notes;
    ntr->notes = p;
}

static void ntr_note_on(struct RCPNoteTracer *ntr,
			int ch, int note, int velo, int gate)
{
    struct NoteList *p;

    for(p = ntr->notes; p != NULL; p = p->next)
	if(p->ch == ch && p->note == note)
	{
	    p->gate = gate;
	    return;
	}

    MIDIEVENT(ntr->at, ME_NOTEON, ch, note, velo);
    ntr_add(ntr, ch, note, gate);
}

static void rcp_tempo_gradate(struct RCPNoteTracer *ntr, int step)
{
    int tempo_grade, tempo_step, tempo, diff, sign, at;
    
    if (step <= 0 || (tempo_grade = ntr->tempo_grade) == 0)
    	return;
    tempo_step = ntr->tempo_step - step;
    if (tempo_step <= 0)
    {
	tempo = ntr->tempo;
	diff = ntr->tempo_to - tempo;
	sign = (diff < 0) ? -1 : 1;
	diff *= sign;	/* abs(diff) */
	at = ntr_at(*ntr);
	while (tempo_step <= 0 && diff != 0)
	{
	    if (tempo_grade > diff)
		tempo_grade = diff;
	    tempo += sign * tempo_grade;
	    diff -= tempo_grade;
	    rcp_tempo_set(at, tempo);
	    at += TEMPO_GRADATION_SKIP;
	    tempo_step += TEMPO_GRADATION_SKIP;
	}
	ntr->tempo = tempo;
	if (diff == 0)
	    ntr->tempo_grade = 0;
    }
    ntr->tempo_step = tempo_step;
}

static void ntr_incr(struct RCPNoteTracer *ntr, int step)
{
    if(step < 0) {
	struct NoteList *p;
	ntr->at += step;
	for(p = ntr->notes; p != NULL; p = p->next)
	    p->gate -= step;
	return;
    }

    rcp_tempo_gradate(ntr, step);

    while(step >= 0)
    {
	int32 mingate;
	struct NoteList *p, *q;

	if(ntr->notes == NULL)
	{
	    ntr->at += step;
	    return;
	}

	q = NULL;
	p = ntr->notes;
	mingate = step;
	while(p)
	{
	    struct NoteList *next;

	    next = p->next;
	    if(p->gate == 0)
	    {
		if(ctl->verbosity >= VERB_DEBUG_SILLY)
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
			      "NoteOff %d at %d", p->note, ntr->at);
		MIDIEVENT(ntr->at, ME_NOTEOFF, p->ch, p->note, 0);
		p->next = ntr->freelist;
		ntr->freelist = p;
	    }
	    else
	    {
		if(mingate > p->gate)
		    mingate = p->gate;
		p->next = q;
		q = p;
	    }
	    p = next;
	}
	ntr->notes = q;

 	if(step == 0)
	    return;

	step -= mingate;
	ntr->at += mingate;

	for(p = ntr->notes; p != NULL; p = p->next)
	    p->gate -= mingate;
    }
}

static void ntr_wait_all_off(struct RCPNoteTracer *ntr)
{
    while(ntr->notes)
    {
	int32 mingate;
	struct NoteList *p, *q;

	mingate = 256;
	q = NULL;
	p = ntr->notes;
	while(p)
	{
	    struct NoteList *next;

	    next = p->next;
	    if(p->gate == 0)
	    {
		if(ctl->verbosity >= VERB_DEBUG_SILLY)
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
			      "NoteOff %d", p->note);
		MIDIEVENT(ntr->at, ME_NOTEOFF, p->ch, p->note, 0);
		p->next = ntr->freelist;
		ntr->freelist = p;
	    }
	    else
	    {
		if(mingate > p->gate)
		    mingate = p->gate;
		p->next = q;
		q = p;
	    }
	    p = next;
	}
	ntr->notes = q;
	for(p = ntr->notes; p != NULL; p = p->next)
	    p->gate -= mingate;
	rcp_tempo_gradate(ntr, mingate);
	ntr->at += mingate;
    }
}

static int preprocess_sysex(uint8 *ex, int ch, int gt, int vel)
{
    uint8 cs = 0;		/* check sum */
    int len = 0;
    int i;

    for(i = 0; i < MAX_EXCLUSIVE_LENGTH && ex[i] != 0xf7; i++)
    {
	switch(ex[i])
	{
	  case 0x80: /* gt */
	    cs += ex[len++] = gt;
	    break;
	  case 0x81: /* ve */
	    cs += ex[len++] = vel;
	    break;
	  case 0x82: /* ch */
	    cs += ex[len++] = ch;
	    break;
	  case 0x83: /* (Clear Sum) */
#if 0
	    if(ex[0] == 0x41 && len == 3 &&
	       ex[len - 2] == 0x10 && ex[len - 1] == 0x12) /* ##?? */
	    {
		ex[len - 1] = 0x42;
		ex[len++] = 0x12;
	    }
#endif
	    cs = 0;
	    break;
	  case 0x84: /* (Send Sum) */
	    ex[len++] = 128 - (cs & 0x7f);
	    break;
	  default:
	    cs += ex[len++] = ex[i];
	    break;
	}
    }
    ex[len++] = 0xf7;
    return len;
}
