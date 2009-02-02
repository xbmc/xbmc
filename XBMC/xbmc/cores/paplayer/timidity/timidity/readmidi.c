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
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"
#include "controls.h"
#include "strtab.h"
#include "memb.h"
#include "wrd.h"
#include "tables.h"
#include "reverb.h"
#include <math.h>


// oldnemesis: turn off MIDI cache, we're not downloading/converting anything
#define NO_MIDI_CACHE
#undef SMFCONV

/* rcp.c */
//int read_rcp_file(struct timidity_file *tf, char *magic0, char *fn);

/* mld.c */
//extern int read_mfi_file(struct timidity_file *tf);
//extern char *get_mfi_file_title(struct timidity_file *tf);

#define MAX_MIDI_EVENT ((MAX_SAFE_MALLOC_SIZE / sizeof(MidiEvent)) - 1)
#define MARKER_START_CHAR	'('
#define MARKER_END_CHAR		')'

static uint8 rhythm_part[2];	/* for GS */
static uint8 drum_setup_xg[16] = { 9, 9, 9, 9, 9, 9, 9, 9,
				   9, 9, 9, 9, 9, 9, 9, 9 };	/* for XG */

enum
{
    CHORUS_ST_NOT_OK = 0,
    CHORUS_ST_OK
};

#ifdef ALWAYS_TRACE_TEXT_META_EVENT
int opt_trace_text_meta_event = 1;
#else
int opt_trace_text_meta_event = 0;
#endif /* ALWAYS_TRACE_TEXT_META_EVENT */

int opt_default_mid = 0;
int opt_system_mid = 0;
int ignore_midi_error = 1;
ChannelBitMask quietchannels;
struct midi_file_info *current_file_info = NULL;
int readmidi_error_flag = 0;
int readmidi_wrd_mode = 0;
int play_system_mode = DEFAULT_SYSTEM_MODE;

static MidiEventList *evlist, *current_midi_point;
static int32 event_count;
static MBlockList mempool;
static StringTable string_event_strtab;
static int current_read_track;
static int karaoke_format, karaoke_title_flag;
static struct midi_file_info *midi_file_info = NULL;
static char **string_event_table = NULL;
static int    string_event_table_size = 0;
int    default_channel_program[256];
static MidiEvent timesig[256];

void init_delay_status_gs(void);
void init_chorus_status_gs(void);
void init_reverb_status_gs(void);
void init_eq_status_gs(void);
void init_insertion_effect_gs(void);
void init_multi_eq_xg(void);
static void init_all_effect_xg(void);

/* MIDI ports will be merged in several channels in the future. */
int midi_port_number;

/* These would both fit into 32 bits, but they are often added in
   large multiples, so it's simpler to have two roomy ints */
static int32 sample_increment, sample_correction; /*samples per MIDI delta-t*/

#define SETMIDIEVENT(e, at, t, ch, pa, pb) \
    { (e).time = (at); (e).type = (t); \
      (e).channel = (uint8)(ch); (e).a = (uint8)(pa); (e).b = (uint8)(pb); }

#define MIDIEVENT(at, t, ch, pa, pb) \
    { MidiEvent event; SETMIDIEVENT(event, at, t, ch, pa, pb); \
      readmidi_add_event(&event); }

#if MAX_CHANNELS <= 16
#define MERGE_CHANNEL_PORT(ch) ((int)(ch))
#define MERGE_CHANNEL_PORT2(ch, port) ((int)(ch))
#else
#define MERGE_CHANNEL_PORT(ch) ((int)(ch) | (midi_port_number << 4))
#define MERGE_CHANNEL_PORT2(ch, port) ((int)(ch) | ((int)port << 4))
#endif

#define alloc_midi_event() \
    (MidiEventList *)new_segment(&mempool, sizeof(MidiEventList))

typedef struct _UserDrumset {
	int8 bank;
	int8 prog;
	int8 play_note;
	int8 level;
	int8 assign_group;
	int8 pan;
	int8 reverb_send_level;
	int8 chorus_send_level;
	int8 rx_note_off;
	int8 rx_note_on;
	int8 delay_send_level;
	int8 source_map;
	int8 source_prog;
	int8 source_note;
	struct _UserDrumset *next;
} UserDrumset;

UserDrumset *userdrum_first = (UserDrumset *)NULL;
UserDrumset *userdrum_last = (UserDrumset *)NULL; 

void init_userdrum();
UserDrumset *get_userdrum(int bank, int prog);
void recompute_userdrum(int bank, int prog);
void recompute_userdrum_altassign(int bank,int group);

typedef struct _UserInstrument {
	int8 bank;
	int8 prog;
	int8 source_map;
	int8 source_bank;
	int8 source_prog;
	int8 vibrato_rate;
	int8 vibrato_depth;
	int8 cutoff_freq;
	int8 resonance;
	int8 env_attack;
	int8 env_decay;
	int8 env_release;
	int8 vibrato_delay;
	struct _UserInstrument *next;
} UserInstrument;

UserInstrument *userinst_first = (UserInstrument *)NULL;
UserInstrument *userinst_last = (UserInstrument *)NULL; 

void init_userinst();
UserInstrument *get_userinst(int bank, int prog);
void recompute_userinst(int bank, int prog);
void recompute_userinst_altassign(int bank,int group);

int32 readmidi_set_track(int trackno, int rewindp)
{
    current_read_track = trackno;
    memset(&chorus_status_gs.text, 0, sizeof(struct chorus_text_gs_t));
    if(karaoke_format == 1 && current_read_track == 2)
	karaoke_format = 2; /* Start karaoke lyric */
    else if(karaoke_format == 2 && current_read_track == 3)
	karaoke_format = 3; /* End karaoke lyric */
    midi_port_number = 0;

    if(evlist == NULL)
	return 0;
    if(rewindp)
	current_midi_point = evlist;
    else
    {
	/* find the last event in the list */
	while(current_midi_point->next != NULL)
	    current_midi_point = current_midi_point->next;
    }
    return current_midi_point->event.time;
}

void readmidi_add_event(MidiEvent *a_event)
{
    MidiEventList *newev;
    int32 at;

    if(event_count++ == MAX_MIDI_EVENT)
    {
	if(!readmidi_error_flag)
	{
	    readmidi_error_flag = 1;
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		      "Maxmum number of events is exceeded");
	}
	return;
    }

    at = a_event->time;
    newev = alloc_midi_event();
    newev->event = *a_event;	/* assign by value!!! */
    if(at < 0)	/* for safety */
	at = newev->event.time = 0;

    if(at >= current_midi_point->event.time)
    {
	/* Forward scan */
	MidiEventList *next = current_midi_point->next;
	while (next && (next->event.time <= at))
	{
	    current_midi_point = next;
	    next = current_midi_point->next;
	}
	newev->prev = current_midi_point;
	newev->next = next;
	current_midi_point->next = newev;
	if (next)
	    next->prev = newev;
    }
    else
    {
	/* Backward scan -- symmetrical to the one above */
	MidiEventList *prev = current_midi_point->prev;
	while (prev && (prev->event.time > at)) {
	    current_midi_point = prev;
	    prev = current_midi_point->prev;
	}
	newev->prev = prev;
	newev->next = current_midi_point;
	current_midi_point->prev = newev;
	if (prev)
	    prev->next = newev;
    }
    current_midi_point = newev;
}

void readmidi_add_ctl_event(int32 at, int ch, int a, int b)
{
    MidiEvent ev;

    if(convert_midi_control_change(ch, a, b, &ev))
    {
	ev.time = at;
	readmidi_add_event(&ev);
    }
    else
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "(Control ch=%d %d: %d)", ch, a, b);
}

char *readmidi_make_string_event(int type, char *string, MidiEvent *ev,
				 int cnv)
{
    char *text;
    int len;
    StringTableNode *st;
    int a, b;

    if(string_event_strtab.nstring == 0)
	put_string_table(&string_event_strtab, "", 0);
    else if(string_event_strtab.nstring == 0x7FFE)
    {
	SETMIDIEVENT(*ev, 0, type, 0, 0, 0);
	return NULL; /* Over flow */
    }
    a = (string_event_strtab.nstring & 0xff);
    b = ((string_event_strtab.nstring >> 8) & 0xff);

    len = strlen(string);
    if(cnv)
    {
	text = (char *)new_segment(&tmpbuffer, SAFE_CONVERT_LENGTH(len) + 1);
	code_convert(string, text + 1, SAFE_CONVERT_LENGTH(len), NULL, NULL);
    }
    else
    {
	text = (char *)new_segment(&tmpbuffer, len + 1);
	memcpy(text + 1, string, len);
	text[len + 1] = '\0';
    }

    st = put_string_table(&string_event_strtab, text, strlen(text + 1) + 1);
    reuse_mblock(&tmpbuffer);

    text = st->string;
    *text = type;
    SETMIDIEVENT(*ev, 0, type, 0, a, b);
    return text;
}

static char *readmidi_make_lcd_event(int type, const uint8 *data, MidiEvent *ev)
{
    char *text;
    int len;
    StringTableNode *st;
    int a, b, i;

    if(string_event_strtab.nstring == 0)
	put_string_table(&string_event_strtab, "", 0);
    else if(string_event_strtab.nstring == 0x7FFE)
    {
	SETMIDIEVENT(*ev, 0, type, 0, 0, 0);
	return NULL; /* Over flow */
    }
    a = (string_event_strtab.nstring & 0xff);
    b = ((string_event_strtab.nstring >> 8) & 0xff);

    len = 128;
    
	text = (char *)new_segment(&tmpbuffer, len + 2);

    for( i=0; i<64; i++){
	const char tbl[]= "0123456789ABCDEF";
	text[1+i*2  ]=tbl[data[i]>>4];
	text[1+i*2+1]=tbl[data[i]&0xF];
    }
    text[len + 1] = '\0';
    
    
    st = put_string_table(&string_event_strtab, text, strlen(text + 1) + 1);
    reuse_mblock(&tmpbuffer);

    text = st->string;
    *text = type;
    SETMIDIEVENT(*ev, 0, type, 0, a, b);
    return text;
}

/* Computes how many (fractional) samples one MIDI delta-time unit contains */
static void compute_sample_increment(int32 tempo, int32 divisions)
{
  double a;
  a = (double) (tempo) * (double) (play_mode->rate) * (65536.0/1000000.0) /
    (double)(divisions);

  sample_correction = (int32)(a) & 0xFFFF;
  sample_increment = (int32)(a) >> 16;

  ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Samples per delta-t: %d (correction %d)",
       sample_increment, sample_correction);
}

/* Read variable-length number (7 bits per byte, MSB first) */
static int32 getvl(struct timidity_file *tf)
{
    int32 l;
    int c;

    errno = 0;
    l = 0;

    /* 1 */
    if((c = tf_getc(tf)) == EOF)
	goto eof;
    if(!(c & 0x80)) return l | c;
    l = (l | (c & 0x7f)) << 7;

    /* 2 */
    if((c = tf_getc(tf)) == EOF)
	goto eof;
    if(!(c & 0x80)) return l | c;
    l = (l | (c & 0x7f)) << 7;

    /* 3 */
    if((c = tf_getc(tf)) == EOF)
	goto eof;
    if(!(c & 0x80)) return l | c;
    l = (l | (c & 0x7f)) << 7;

    /* 4 */
    if((c = tf_getc(tf)) == EOF)
	goto eof;
    if(!(c & 0x80)) return l | c;

    /* 5 */
    if((c = tf_getc(tf)) == EOF)
	goto eof;
    if(!(c & 0x80)) return l | c;

    /* Error */
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
	      "%s: Illegal variable-length quantity format.",
	      current_filename);
    return -2;

  eof:
    if(errno)
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "%s: read_midi_event: %s",
		  current_filename, strerror(errno));
    else
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Warning: %s: Too shorten midi file.",
		  current_filename);
    return -1;
}

static char *add_karaoke_title(char *s1, char *s2)
{
    char *ks;
    int k1, k2;

    if(s1 == NULL)
	return safe_strdup(s2);

    k1 = strlen(s1);
    k2 = strlen(s2);
    if(k2 == 0)
	return s1;
    ks = (char *)safe_malloc(k1 + k2 + 2);
    memcpy(ks, s1, k1);
    ks[k1++] = ' ';
    memcpy(ks + k1, s2, k2 + 1);
    free(s1);
	s1 = NULL;

    return ks;
}


/* Print a string from the file, followed by a newline. Any non-ASCII
   or unprintable characters will be converted to periods. */
static char *dumpstring(int type, int32 len, char *label, int allocp,
			struct timidity_file *tf)
{
    char *si, *so;
    int s_maxlen = SAFE_CONVERT_LENGTH(len);
    int llen, solen;

    if(len <= 0)
    {
	ctl->cmsg(CMSG_TEXT, VERB_VERBOSE, "%s", label);
	return NULL;
    }

    si = (char *)new_segment(&tmpbuffer, len + 1);
    so = (char *)new_segment(&tmpbuffer, s_maxlen);

    if(len != tf_read(si, 1, len, tf))
    {
	reuse_mblock(&tmpbuffer);
	return NULL;
    }
    si[len]='\0';

    if(type == 1 &&
       current_read_track == 1 &&
       current_file_info->format == 1 &&
       strncmp(si, "@KMIDI", 6) == 0)
	karaoke_format = 1;

    code_convert(si, so, s_maxlen, NULL, NULL);

    llen = strlen(label);
    solen = strlen(so);
    if(llen + solen >= MIN_MBLOCK_SIZE)
	so[MIN_MBLOCK_SIZE - llen - 1] = '\0';

    ctl->cmsg(CMSG_TEXT, VERB_VERBOSE, "%s%s", label, so);

    if(allocp)
    {
	so = safe_strdup(so);
	reuse_mblock(&tmpbuffer);
	return so;
    }
    reuse_mblock(&tmpbuffer);
    return NULL;
}

static uint16 gs_convert_master_vol(int vol)
{
    double v;

    if(vol >= 0x7f)
	return 0xffff;
    v = (double)vol * (0xffff/127.0);
    if(v >= 0xffff)
	return 0xffff;
    return (uint16)v;
}

static uint16 gm_convert_master_vol(uint16 v1, uint16 v2)
{
    return (((v1 & 0x7f) | ((v2 & 0x7f) << 7)) << 2) | 3;
}

static void check_chorus_text_start(void)
{
	struct chorus_text_gs_t *p = &(chorus_status_gs.text);
    if(p->status != CHORUS_ST_OK && p->voice_reserve[17] &&
       p->macro[2] && p->pre_lpf[2] && p->level[2] &&
       p->feed_back[2] && p->delay[2] && p->rate[2] &&
       p->depth[2] && p->send_level[2])
    {
	ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Chorus text start");
	p->status = CHORUS_ST_OK;
    }
}

int convert_midi_control_change(int chn, int type, int val, MidiEvent *ev_ret)
{
    switch(type)
    {
      case   0: type = ME_TONE_BANK_MSB; break;
      case   1: type = ME_MODULATION_WHEEL; break;
      case   2: type = ME_BREATH; break;
      case   4: type = ME_FOOT; break;
      case   5: type = ME_PORTAMENTO_TIME_MSB; break;
      case   6: type = ME_DATA_ENTRY_MSB; break;
      case   7: type = ME_MAINVOLUME; break;
      case   8: type = ME_BALANCE; break;
      case  10: type = ME_PAN; break;
      case  11: type = ME_EXPRESSION; break;
      case  32: type = ME_TONE_BANK_LSB; break;
      case  37: type = ME_PORTAMENTO_TIME_LSB; break;
      case  38: type = ME_DATA_ENTRY_LSB; break;
      case  64: type = ME_SUSTAIN; break;
      case  65: type = ME_PORTAMENTO; break;
      case  66: type = ME_SOSTENUTO; break;
      case  67: type = ME_SOFT_PEDAL; break;
      case  68: type = ME_LEGATO_FOOTSWITCH; break;
      case  69: type = ME_HOLD2; break;
      case  71: type = ME_HARMONIC_CONTENT; break;
      case  72: type = ME_RELEASE_TIME; break;
      case  73: type = ME_ATTACK_TIME; break;
      case  74: type = ME_BRIGHTNESS; break;
      case  84: type = ME_PORTAMENTO_CONTROL; break;
      case  91: type = ME_REVERB_EFFECT; break;
      case  92: type = ME_TREMOLO_EFFECT; break;
      case  93: type = ME_CHORUS_EFFECT; break;
      case  94: type = ME_CELESTE_EFFECT; break;
      case  95: type = ME_PHASER_EFFECT; break;
      case  96: type = ME_RPN_INC; break;
      case  97: type = ME_RPN_DEC; break;
      case  98: type = ME_NRPN_LSB; break;
      case  99: type = ME_NRPN_MSB; break;
      case 100: type = ME_RPN_LSB; break;
      case 101: type = ME_RPN_MSB; break;
      case 120: type = ME_ALL_SOUNDS_OFF; break;
      case 121: type = ME_RESET_CONTROLLERS; break;
      case 123: type = ME_ALL_NOTES_OFF; break;
      case 126: type = ME_MONO; break;
      case 127: type = ME_POLY; break;
      default: type = -1; break;
    }

    if(type != -1)
    {
	if(val > 127)
	    val = 127;
	ev_ret->type    = type;
	ev_ret->channel = chn;
	ev_ret->a       = val;
	ev_ret->b       = 0;
	return 1;
    }
    return 0;
}

static int block_to_part(int block, int port)
{
	int p;
	p = block & 0x0F;
	if(p == 0) {p = 9;}
	else if(p <= 9) {p--;}
	return MERGE_CHANNEL_PORT2(p, port);
}

/* Map XG types onto GS types.  XG should eventually have its own tables */
static int set_xg_reverb_type(int msb, int lsb)
{
	int type = 4;

	if ((msb == 0x00) ||
	    (msb >= 0x05 && msb <= 0x0F) ||
	    (msb >= 0x14))			/* NO EFFECT */
	{
		ctl->cmsg(CMSG_INFO,VERB_NOISY,"XG Set Reverb Type (NO EFFECT %d %d)", msb, lsb);
		return -1;
	}

	switch(msb)
	{
	    case 0x01:
		type = 3;			/* Hall 1 */
		break;
	    case 0x02:
		type = 0;			/* Room 1 */
		break;
	    case 0x03:
		type = 3;			/* Stage 1 -> Hall 1 */
	    case 0x04:
		type = 5;			/* Plate */
		break;
	    default:
		type = 4;			/* unsupported -> Hall 2 */
	    break;
	}
	if (lsb == 0x01)
	{
	    switch(msb)
	    {
		case 0x01:
		    type = 4;			/* Hall 2 */
		    break;
		case 0x02:
		    type = 1;			/* Room 2 */
		    break;
		case 0x03:
		    type = 4;			/* Stage 2 -> Hall 2 */
		    break;
		default:
		    break;
	    }
	}
	if (lsb == 0x02 && msb == 0x02)
	    type = 2;				/* Room 3 */

	ctl->cmsg(CMSG_INFO,VERB_NOISY,"XG Set Reverb Type (%d)", type);
	return type;
}

/* Map XG types onto GS types.  XG should eventually have its own tables */
static int set_xg_chorus_type(int msb, int lsb)
{
	int type = 2;

	if ((msb >= 0x00 && msb <= 0x40) ||
	    (msb >= 0x45 && msb <= 0x47) ||
	    (msb >= 0x49))			/* NO EFFECT */
	{
		ctl->cmsg(CMSG_INFO,VERB_NOISY,"XG Set Chorus Type (NO EFFECT %d %d)", msb, lsb);
		return -1;
	}

	switch(msb)
	{
	    case 0x41:
		type = 0;			/* Chorus 1 */
		break;
	    case 0x42:
		type = 0;			/* Celeste 1 -> Chorus 1 */
		break;
	    case 0x43:
		type = 5;
		break;
	    default:
		type = 2;			/* unsupported -> Chorus 3 */
	    break;
	}
	if (lsb == 0x01)
	{
	    switch(msb)
	    {
		case 0x41:
		    type = 1;			/* Chorus 2 */
		    break;
		case 0x42:
		    type = 1;			/* Celeste 2 -> Chorus 2 */
		    break;
		default:
		    break;
	    }
	}
	else if (lsb == 0x02)
	{
	    switch(msb)
	    {
		case 0x41:
		    type = 2;			/* Chorus 3 */
		    break;
		case 0x42:
		    type = 2;			/* Celeste 3 -> Chorus 3 */
		    break;
		default:
		    break;
	    }
	}
	else if (lsb == 0x08)
	{
	    switch(msb)
	    {
		case 0x41:
		    type = 3;			/* Chorus 4 */
		    break;
		case 0x42:
		    type = 3;			/* Celeste 4 -> Chorus 4 */
		    break;
		default:
		    break;
	    }
	}

	ctl->cmsg(CMSG_INFO,VERB_NOISY,"XG Set Chorus Type (%d)", type);
	return type;
}

/* XG SysEx parsing function by Eric A. Welsh
 * Also handles GS patch+bank changes
 *
 * This function provides basic support for XG Bulk Dump and Parameter
 * Change SysEx events
 */
int parse_sysex_event_multi(uint8 *val, int32 len, MidiEvent *evm)
{
    int num_events = 0;				/* Number of events added */
    uint32 channel_tt;
    int i, j;
    static uint8 xg_reverb_type_msb = 0x01, xg_reverb_type_lsb = 0x00;
    static uint8 xg_chorus_type_msb = 0x41, xg_chorus_type_lsb = 0x00;

    if(current_file_info->mid == 0 || current_file_info->mid >= 0x7e)
	current_file_info->mid = val[0];

    /* Effect 1 or Multi EQ */
    if(len >= 8 &&
       val[0] == 0x43 && /* Yamaha ID */
       val[2] == 0x4C && /* XG Model ID */
       ((val[1] <  0x10 && val[5] == 0x02) ||	/* Bulk Dump*/
        (val[1] >= 0x10 && val[3] == 0x02)))	/* Parameter Change */
    {
	uint8 addhigh, addmid, addlow;		/* Addresses */
	uint8 *body;				/* SysEx body */
	int ent, v;				/* Entry # of sub-event */
	uint8 *body_end;			/* End of SysEx body */

	if (val[1] < 0x10)	/* Bulk Dump */
	{
	    addhigh = val[5];
	    addmid = val[6];
	    addlow = val[7];
	    body = val + 8;
	    body_end = val + len - 3;
	}
	else			/* Parameter Change */
	{
	    addhigh = val[3];
	    addmid = val[4];
	    addlow = val[5];
	    body = val + 6;
	    body_end = val + len - 2;
	}

	/* set the SYSEX_XG_MSB info */
	SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_MSB, 0, addhigh, addmid);
	num_events++;

	for (ent = addlow; body <= body_end; body++, ent++) {
	  if(addmid == 0x01) {	/* Effect 1 */
	    switch(ent) {
		case 0x00:	/* Reverb Type MSB */
		    xg_reverb_type_msb = *body;
#if 0	/* XG specific reverb is not supported yet, use GS instead */
		    SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
#endif
		    break;

		case 0x01:	/* Reverb Type LSB */
		    xg_reverb_type_lsb = *body;
#if 0	/* XG specific reverb is not supported yet, use GS instead */
		    SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
#else
		    v = set_xg_reverb_type(xg_reverb_type_msb, xg_reverb_type_lsb);
		    if (v >= 0) {
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_GS_LSB, 0, v, 0x05);
			num_events++;
		    }
#endif
		    break;

		case 0x0C:	/* Reverb Return */
		    SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
		    break;

		case 0x20:	/* Chorus Type MSB */
		    xg_chorus_type_msb = *body;
#if 0	/* XG specific chorus is not supported yet, use GS instead */
		    SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
#endif
		    break;

		case 0x21:	/* Chorus Type LSB */
		    xg_chorus_type_lsb = *body;
#if 0	/* XG specific chorus is not supported yet, use GS instead */
		    SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
#else
		    v = set_xg_chorus_type(xg_chorus_type_msb, xg_chorus_type_lsb);
		    if (v >= 0) {
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_GS_LSB, 0, v, 0x0D);
			num_events++;
		    }
#endif
		    break;

		case 0x2C:	/* Chorus Return */
		    SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
		    break;

		default:
		    SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
		    break;
	    }
	  }
	  else if(addmid == 0x40) {	/* Multi EQ */
	    switch(ent) {
		case 0x00:	/* EQ type */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
		    break;

		case 0x01:	/* EQ gain1 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
			break;

		case 0x02:	/* EQ frequency1 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
			break;

		case 0x03:	/* EQ Q1 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
			break;

		case 0x04:	/* EQ shape1 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
			break;

		case 0x05:	/* EQ gain2 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
			break;

		case 0x06:	/* EQ frequency2 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
			break;

		case 0x07:	/* EQ Q2 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
			break;

		case 0x09:	/* EQ gain3 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
			break;

		case 0x0A:	/* EQ frequency3 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
			break;

		case 0x0B:	/* EQ Q3 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
			break;

		case 0x0D:	/* EQ gain4 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
			break;

		case 0x0E:	/* EQ frequency4 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
			break;

		case 0x0F:	/* EQ Q4 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
			break;

		case 0x11:	/* EQ gain5 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
			break;

		case 0x12:	/* EQ frequency5 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
			break;

		case 0x13:	/* EQ Q5 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
			break;

		case 0x14:	/* EQ shape5 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			num_events++;
			break;

		default:
		    	break;
	    }
	  }
	}
    }

    /* Effect 2 (Insertion Effects) */
    else if(len >= 8 &&
       val[0] == 0x43 && /* Yamaha ID */
       val[2] == 0x4C && /* XG Model ID */
       ((val[1] <  0x10 && val[5] == 0x03) ||	/* Bulk Dump*/
        (val[1] >= 0x10 && val[3] == 0x03)))	/* Parameter Change */
    {
	uint8 addhigh, addmid, addlow;		/* Addresses */
	uint8 *body;				/* SysEx body */
	int ent;				/* Entry # of sub-event */
	uint8 *body_end;			/* End of SysEx body */

	if (val[1] < 0x10)	/* Bulk Dump */
	{
	    addhigh = val[5];
	    addmid = val[6];
	    addlow = val[7];
	    body = val + 8;
	    body_end = val + len - 3;
	}
	else			/* Parameter Change */
	{
	    addhigh = val[3];
	    addmid = val[4];
	    addlow = val[5];
	    body = val + 6;
	    body_end = val + len - 2;
	}

	/* set the SYSEX_XG_MSB info */
	SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_MSB, 0, addhigh, addmid);
	num_events++;

	for (ent = addlow; body <= body_end; body++, ent++) {
	    SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, 0, *body, ent);
			 num_events++;
	}
    }

    /* XG Multi Part Data parameter change */
    else if(len >= 10 &&
       val[0] == 0x43 && /* Yamaha ID */
       val[2] == 0x4C && /* XG Model ID */
       ((val[1] <  0x10 && val[5] == 0x08 &&	/* Bulk Dump */
         val[4] == 0x29 || val[4] == 0x3F) ||	/* Blocks 1 or 2 */
        (val[1] >= 0x10 && val[3] == 0x08)))	/* Parameter Change */
    {
	uint8 addhigh, addmid, addlow;		/* Addresses */
	uint8 *body;				/* SysEx body */
	uint8 p;				/* Channel part number [0..15] */
	int ent;				/* Entry # of sub-event */
	uint8 *body_end;			/* End of SysEx body */

	if (val[1] < 0x10)	/* Bulk Dump */
	{
	    addhigh = val[5];
	    addmid = val[6];
	    addlow = val[7];
	    body = val + 8;
	    p = addmid;
	    body_end = val + len - 3;
	}
	else			/* Parameter Change */
	{
	    addhigh = val[3];
	    addmid = val[4];
	    addlow = val[5];
	    body = val + 6;
	    p = addmid;
	    body_end = val + len - 2;
	}

	/* set the SYSEX_XG_MSB info */
	SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_MSB, p, addhigh, addmid);
	num_events++;

	for (ent = addlow; body <= body_end; body++, ent++) {
	    switch(ent) {
		case 0x00:	/* Element Reserve */
/*			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Element Reserve is not supported. (CH:%d VAL:%d)", p, *body); */
		    break;

		case 0x01:	/* bank select MSB */
		    SETMIDIEVENT(evm[num_events], 0, ME_TONE_BANK_MSB, p, *body, SYSEX_TAG);
		    num_events++;
		    break;

		case 0x02:	/* bank select LSB */
		    SETMIDIEVENT(evm[num_events], 0, ME_TONE_BANK_LSB, p, *body, SYSEX_TAG);
		    num_events++;
		    break;

		case 0x03:	/* program number */
		    SETMIDIEVENT(evm[num_events], 0, ME_PROGRAM, p, *body, SYSEX_TAG);
		    num_events++;
		    break;

		case 0x04:	/* Rcv CHANNEL */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, p, *body, 0x99);
			num_events++;
		    break;

		case 0x05:	/* mono/poly mode */
			if(*body == 0) {SETMIDIEVENT(evm[num_events], 0, ME_MONO, p, 0, SYSEX_TAG);}
			else {SETMIDIEVENT(evm[num_events], 0, ME_POLY, p, 0, SYSEX_TAG);}
			num_events++;
		    break;

		case 0x06:	/* Same Note Number Key On Assign */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, p, *body, ent);
			num_events++;
		    break;

		case 0x07:	/* Part Mode */
			drum_setup_xg[*body] = p;
			SETMIDIEVENT(evm[num_events], 0, ME_DRUMPART, p, *body, SYSEX_TAG);
			num_events++;
		    break;

		case 0x08:	/* note shift */
		    SETMIDIEVENT(evm[num_events], 0, ME_KEYSHIFT, p, *body, SYSEX_TAG);
		    num_events++;
		    break;

		case 0x09:	/* Detune 1st bit */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Detune 1st bit is not supported. (CH:%d VAL:%d)", p, *body); 
		    break;

		case 0x0A:	/* Detune 2nd bit */
		    ctl->cmsg(CMSG_INFO, VERB_NOISY, "Detune 2nd bit is not supported. (CH:%d VAL:%d)", p, *body); 
		    break;

		case 0x0B:	/* volume */
		    SETMIDIEVENT(evm[num_events], 0, ME_MAINVOLUME, p, *body, SYSEX_TAG);
		    num_events++;
		    break;

		case 0x0C:	/* Velocity Sense Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_GS_LSB, p, *body, 0x21);
			num_events++;
			break;

		case 0x0D:	/* Velocity Sense Offset */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_GS_LSB, p, *body, 0x22);
			num_events++;
			break;

		case 0x0E:	/* pan */
		    if(*body == 0) {
			SETMIDIEVENT(evm[num_events], 0, ME_RANDOM_PAN, p, 0, SYSEX_TAG);
		    }
		    else {
			SETMIDIEVENT(evm[num_events], 0, ME_PAN, p, *body, SYSEX_TAG);
		    }
		    num_events++;
		    break;

		case 0x0F:	/* Note Limit Low */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x42);
			num_events++;
		    break;

		case 0x10:	/* Note Limit High */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x43);
			num_events++;
			break;

		case 0x11:	/* Dry Level */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, p, *body, ent);
			num_events++;
			break;

		case 0x12:	/* chorus send */
		    SETMIDIEVENT(evm[num_events], 0, ME_CHORUS_EFFECT, p, *body, SYSEX_TAG);
		    num_events++;
		    break;

		case 0x13:	/* reverb send */
		    SETMIDIEVENT(evm[num_events], 0, ME_REVERB_EFFECT, p, *body, SYSEX_TAG);
		    num_events++;
		    break;

		case 0x14:	/* Variation Send */
		    SETMIDIEVENT(evm[num_events], 0, ME_CELESTE_EFFECT, p, *body, SYSEX_TAG);
		    num_events++;
		    break;

		case 0x15:	/* Vibrato Rate */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, p, 0x08, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, p, *body, SYSEX_TAG);
			num_events += 3;
		    break;

		case 0x16:	/* Vibrato Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, p, 0x09, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, p, *body, SYSEX_TAG);
			num_events += 3;
		    break;

		case 0x17:	/* Vibrato Delay */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, p, 0x0A, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, p, *body, SYSEX_TAG);
			num_events += 3;
		    break;

		case 0x18:	/* Filter Cutoff Frequency */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, p, 0x20, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, p, *body, SYSEX_TAG);
			num_events += 3;
		    break;

		case 0x19:	/* Filter Resonance */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, p, 0x21, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, p, *body, SYSEX_TAG);
			num_events += 3;
		    break;

		case 0x1A:	/* EG Attack Time */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, p, 0x63, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, p, *body, SYSEX_TAG);
			num_events += 3;
		    break;

		case 0x1B:	/* EG Decay Time */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, p, 0x64, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, p, *body, SYSEX_TAG);
			num_events += 3;
		    break;

		case 0x1C:	/* EG Release Time */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, p, 0x66, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, p, *body, SYSEX_TAG);
			num_events += 3;
		    break;

		case 0x1D:	/* MW Pitch Control */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x16);
			num_events++;
			break;

		case 0x1E:	/* MW Filter Control */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x17);
			num_events++;
			break;

		case 0x1F:	/* MW Amplitude Control */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x18);
			num_events++;
			break;

		case 0x20:	/* MW LFO PMod Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x1A);
			num_events++;
			break;

		case 0x21:	/* MW LFO FMod Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x1B);
			num_events++;
			break;

		case 0x22:	/* MW LFO AMod Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x1C);
			num_events++;
			break;

		case 0x23:	/* bend pitch control */
		    SETMIDIEVENT(evm[num_events], 0, ME_RPN_MSB, p, 0, SYSEX_TAG);
		    SETMIDIEVENT(evm[num_events + 1], 0, ME_RPN_LSB, p, 0, SYSEX_TAG);
		    SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, p, (*body - 0x40) & 0x7F, SYSEX_TAG);
		    num_events += 3;
		    break;

		case 0x24:	/* Bend Filter Control */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x22);
			num_events++;
			break;

		case 0x25:	/* Bend Amplitude Control */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x23);
			num_events++;
			break;

		case 0x26:	/* Bend LFO PMod Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x25);
			num_events++;
			break;

		case 0x27:	/* Bend LFO FMod Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x26);
			num_events++;
			break;

		case 0x28:	/* Bend LFO AMod Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x27);
			num_events++;
			break;

		case 0x30:	/* Rcv Pitch Bend */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x48);
			num_events++;
			break;

		case 0x31:	/* Rcv Channel Pressure */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x49);
			num_events++;
			break;

		case 0x32:	/* Rcv Program Change */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x4A);
			num_events++;
			break;

		case 0x33:	/* Rcv Control Change */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x4B);
			num_events++;
			break;

		case 0x34:	/* Rcv Poly Pressure */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x4C);
			num_events++;
			break;

		case 0x35:	/* Rcv Note Message */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x4D);
			num_events++;
			break;

		case 0x36:	/* Rcv RPN */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x4E);
			num_events++;
			break;

		case 0x37:	/* Rcv NRPN */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x4F);
			num_events++;
			break;

		case 0x38:	/* Rcv Modulation */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x50);
			num_events++;
			break;

		case 0x39:	/* Rcv Volume */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x51);
			num_events++;
			break;

		case 0x3A:	/* Rcv Pan */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x52);
			num_events++;
			break;

		case 0x3B:	/* Rcv Expression */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x53);
			num_events++;
			break;

		case 0x3C:	/* Rcv Hold1 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x54);
			num_events++;
			break;

		case 0x3D:	/* Rcv Portamento */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x55);
			num_events++;
			break;

		case 0x3E:	/* Rcv Sostenuto */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x56);
			num_events++;
			break;

		case 0x3F:	/* Rcv Soft */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x57);
			num_events++;
			break;

		case 0x40:	/* Rcv Bank Select */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x58);
			num_events++;
			break;

		case 0x41:	/* scale tuning */
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46:
		case 0x47:
		case 0x48:
		case 0x49:
		case 0x4a:
		case 0x4b:
		case 0x4c:
		    SETMIDIEVENT(evm[num_events], 0, ME_SCALE_TUNING, p, ent - 0x41, *body - 64);
		    num_events++;
		    ctl->cmsg(CMSG_INFO, VERB_NOISY, "Scale Tuning %s (CH:%d %d cent)",
			      note_name[ent - 0x41], p, *body - 64);
		    break;

		case 0x4D:	/* CAT Pitch Control */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x00);
			num_events++;
			break;

		case 0x4E:	/* CAT Filter Control */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x01);
			num_events++;
			break;

		case 0x4F:	/* CAT Amplitude Control */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x02);
			num_events++;
			break;

		case 0x50:	/* CAT LFO PMod Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x04);
			num_events++;
			break;

		case 0x51:	/* CAT LFO FMod Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x05);
			num_events++;
			break;

		case 0x52:	/* CAT LFO AMod Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x06);
			num_events++;
			break;

		case 0x53:	/* PAT Pitch Control */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x0B);
			num_events++;
			break;

		case 0x54:	/* PAT Filter Control */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x0C);
			num_events++;
			break;

		case 0x55:	/* PAT Amplitude Control */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x0D);
			num_events++;
			break;

		case 0x56:	/* PAT LFO PMod Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x0F);
			num_events++;
			break;

		case 0x57:	/* PAT LFO FMod Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x10);
			num_events++;
			break;

		case 0x58:	/* PAT LFO AMod Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x11);
			num_events++;
			break;
		
		case 0x59:	/* AC1 Controller Number */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "AC1 Controller Number is not supported. (CH:%d VAL:%d)", p, *body); 
			break;

		case 0x5A:	/* AC1 Pitch Control */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x2C);
			num_events++;
			break;

		case 0x5B:	/* AC1 Filter Control */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x2D);
			num_events++;
			break;

		case 0x5C:	/* AC1 Amplitude Control */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x2E);
			num_events++;
			break;

		case 0x5D:	/* AC1 LFO PMod Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x30);
			num_events++;
			break;

		case 0x5E:	/* AC1 LFO FMod Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x31);
			num_events++;
			break;

		case 0x5F:	/* AC1 LFO AMod Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x32);
			num_events++;
			break;

		case 0x60:	/* AC2 Controller Number */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "AC2 Controller Number is not supported. (CH:%d VAL:%d)", p, *body); 
			break;

		case 0x61:	/* AC2 Pitch Control */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x37);
			num_events++;
			break;

		case 0x62:	/* AC2 Filter Control */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x38);
			num_events++;
			break;

		case 0x63:	/* AC2 Amplitude Control */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x39);
			num_events++;
			break;

		case 0x64:	/* AC2 LFO PMod Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x3B);
			num_events++;
			break;

		case 0x65:	/* AC2 LFO FMod Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x3C);
			num_events++;
			break;

		case 0x66:	/* AC2 LFO AMod Depth */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x3D);
			num_events++;
			break;

		case 0x67:	/* Portamento Switch */
			SETMIDIEVENT(evm[num_events], 0, ME_PORTAMENTO, p, *body, SYSEX_TAG);
		    num_events++;

		case 0x68:	/* Portamento Time */
			SETMIDIEVENT(evm[num_events], 0, ME_PORTAMENTO_TIME_MSB, p, *body, SYSEX_TAG);
		    num_events++;

		case 0x69:	/* Pitch EG Initial Level */
		    ctl->cmsg(CMSG_INFO, VERB_NOISY, "Pitch EG Initial Level is not supported. (CH:%d VAL:%d)", p, *body); 
		    break;

		case 0x6A:	/* Pitch EG Attack Time */
		    ctl->cmsg(CMSG_INFO, VERB_NOISY, "Pitch EG Attack Time is not supported. (CH:%d VAL:%d)", p, *body); 
		    break;

		case 0x6B:	/* Pitch EG Release Level */
		    ctl->cmsg(CMSG_INFO, VERB_NOISY, "Pitch EG Release Level is not supported. (CH:%d VAL:%d)", p, *body); 
		    break;

		case 0x6C:	/* Pitch EG Release Time */
		    ctl->cmsg(CMSG_INFO, VERB_NOISY, "Pitch EG Release Time is not supported. (CH:%d VAL:%d)", p, *body); 
		    break;

		case 0x6D:	/* Velocity Limit Low */
		    SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x44);
			num_events++;
			break;

		case 0x6E:	/* Velocity Limit High */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, p, *body, 0x45);
			num_events++;
		    break;

		case 0x70:	/* Bend Pitch Low Control */
		    ctl->cmsg(CMSG_INFO, VERB_NOISY, "Bend Pitch Low Control is not supported. (CH:%d VAL:%d)", p, *body); 
		    break;

		case 0x71:	/* Filter EG Depth */
		    ctl->cmsg(CMSG_INFO, VERB_NOISY, "Filter EG Depth is not supported. (CH:%d VAL:%d)", p, *body); 
		    break;

		case 0x72:	/* EQ BASS */
			SETMIDIEVENT(evm[num_events], 0,ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0,ME_NRPN_LSB, p, 0x30, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0,ME_DATA_ENTRY_MSB, p, *body, SYSEX_TAG);
			num_events += 3;
			break;

		case 0x73:	/* EQ TREBLE */
			SETMIDIEVENT(evm[num_events], 0,ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0,ME_NRPN_LSB, p, 0x31, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0,ME_DATA_ENTRY_MSB, p, *body, SYSEX_TAG);
			num_events += 3;
			break;

		case 0x76:	/* EQ BASS frequency */
			SETMIDIEVENT(evm[num_events], 0,ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0,ME_NRPN_LSB, p, 0x34, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0,ME_DATA_ENTRY_MSB, p, *body, SYSEX_TAG);
			num_events += 3;
			break;

		case 0x77:	/* EQ TREBLE frequency */
			SETMIDIEVENT(evm[num_events], 0,ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0,ME_NRPN_LSB, p, 0x35, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0,ME_DATA_ENTRY_MSB, p, *body, SYSEX_TAG);
			num_events += 3;
			break;

		default:
		    ctl->cmsg(CMSG_INFO,VERB_NOISY,"Unsupported XG Bulk Dump SysEx. (ADDR:%02X %02X %02X VAL:%02X)",addhigh,addlow,ent,*body);
		    continue;
		    break;
	    }
	}
    }

    /* XG Drum Setup */
    else if(len >= 10 &&
       val[0] == 0x43 && /* Yamaha ID */
       val[2] == 0x4C && /* XG Model ID */
       ((val[1] <  0x10 && (val[5] & 0xF0) == 0x30) ||	/* Bulk Dump*/
        (val[1] >= 0x10 && (val[3] & 0xF0) == 0x30)))	/* Parameter Change */
    {
	uint8 addhigh, addmid, addlow;		/* Addresses */
	uint8 *body;				/* SysEx body */
	uint8 dp, note;				/* Channel part number [0..15] */
	int ent;				/* Entry # of sub-event */
	uint8 *body_end;			/* End of SysEx body */

	if (val[1] < 0x10)	/* Bulk Dump */
	{
	    addhigh = val[5];
	    addmid = val[6];
	    addlow = val[7];
	    body = val + 8;
	    body_end = val + len - 3;
	}
	else			/* Parameter Change */
	{
	    addhigh = val[3];
	    addmid = val[4];
	    addlow = val[5];
	    body = val + 6;
	    body_end = val + len - 2;
	}

	dp = drum_setup_xg[(addhigh & 0x0F) + 1];
	note = addmid;

	/* set the SYSEX_XG_MSB info */
	SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_MSB, dp, addhigh, addmid);
	num_events++;

	for (ent = addlow; body <= body_end; body++, ent++) {
	    switch(ent) {
		case 0x00:	/* Pitch Coarse */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, dp, 0x18, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, dp, note, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, dp, *body, SYSEX_TAG);
			num_events += 3;
			break;
		case 0x01:	/* Pitch Fine */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, dp, 0x19, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, dp, note, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, dp, *body, SYSEX_TAG);
			num_events += 3;
			break;
		case 0x02:	/* Level */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, dp, 0x1A, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, dp, note, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, dp, *body, SYSEX_TAG);
			num_events += 3;
			break;
		case 0x03:	/* Alternate Group */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Alternate Group is not supported. (CH:%d NOTE:%d VAL:%d)", dp, note, *body);
			break;
		case 0x04:	/* Pan */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, dp, 0x1C, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, dp, note, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, dp, *body, SYSEX_TAG);
			num_events += 3;
			break;
		case 0x05:	/* Reverb Send */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, dp, 0x1D, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, dp, note, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, dp, *body, SYSEX_TAG);
			num_events += 3;
			break;
		case 0x06:	/* Chorus Send */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, dp, 0x1E, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, dp, note, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, dp, *body, SYSEX_TAG);
			num_events += 3;
			break;
		case 0x07:	/* Variation Send */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, dp, 0x1F, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, dp, note, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, dp, *body, SYSEX_TAG);
			num_events += 3;
			break;
		case 0x08:	/* Key Assign */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Key Assign is not supported. (CH:%d NOTE:%d VAL:%d)", dp, note, *body);
			break;
		case 0x09:	/* Rcv Note Off */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_MSB, dp, note, 0);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_SYSEX_LSB, dp, *body, 0x46);
			num_events += 2;
			break;
		case 0x0A:	/* Rcv Note On */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_MSB, dp, note, 0);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_SYSEX_LSB, dp, *body, 0x47);
			num_events += 2;
			break;
		case 0x0B:	/* Filter Cutoff Frequency */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, dp, 0x14, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, dp, note, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, dp, *body, SYSEX_TAG);
			num_events += 3;
			break;
		case 0x0C:	/* Filter Resonance */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, dp, 0x15, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, dp, note, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, dp, *body, SYSEX_TAG);
			num_events += 3;
			break;
		case 0x0D:	/* EG Attack */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, dp, 0x16, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, dp, note, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, dp, *body, SYSEX_TAG);
			num_events += 3;
			break;
		case 0x0E:	/* EG Decay1 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, dp, *body, ent);
			num_events++;
			break;
		case 0x0F:	/* EG Decay2 */
			SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_XG_LSB, dp, *body, ent);
			num_events++;
			break;
		case 0x20:	/* EQ BASS */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, dp, 0x30, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, dp, note, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, dp, *body, SYSEX_TAG);
			num_events += 3;
			break;
		case 0x21:	/* EQ TREBLE */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, dp, 0x31, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, dp, note, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, dp, *body, SYSEX_TAG);
			num_events += 3;
			break;
		case 0x24:	/* EQ BASS frequency */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, dp, 0x34, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, dp, note, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, dp, *body, SYSEX_TAG);
			num_events += 3;
			break;
		case 0x25:	/* EQ TREBLE frequency */
			SETMIDIEVENT(evm[num_events], 0, ME_NRPN_MSB, dp, 0x35, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 1], 0, ME_NRPN_LSB, dp, note, SYSEX_TAG);
			SETMIDIEVENT(evm[num_events + 2], 0, ME_DATA_ENTRY_MSB, dp, *body, SYSEX_TAG);
			num_events += 3;
			break;
		case 0x50:	/* High Pass Filter Cutoff Frequency */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "High Pass Filter Cutoff Frequency is not supported. (CH:%d NOTE:%d VAL:%d)", dp, note, *body);
			break;
		case 0x60:	/* Velocity Pitch Sense */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Velocity Pitch Sense is not supported. (CH:%d NOTE:%d VAL:%d)", dp, note, *body);
			break;
		case 0x61:	/* Velocity LPF Cutoff Sense */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Velocity LPF Cutoff Sense is not supported. (CH:%d NOTE:%d VAL:%d)", dp, note, *body);
			break;
		default:
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Unsupported XG Bulk Dump SysEx. (ADDR:%02X %02X %02X VAL:%02X)",addhigh,addmid,ent,*body);
			break;
	    }
	}
    }

    /* parsing GS System Exclusive Message...
     *
     * val[4] == Parameter Address(High)
     * val[5] == Parameter Address(Middle)
     * val[6] == Parameter Address(Low)
     * val[7]... == Data...
     * val[last] == Checksum(== 128 - (sum of addresses&data bytes % 128)) 
     */
    else if(len >= 9 &&
       val[0] == 0x41 && /* Roland ID */
       val[1] == 0x10 && /* Device ID */
       val[2] == 0x42 && /* GS Model ID */
       val[3] == 0x12) /* Data Set Command */
    {
		uint8 p, dp, udn, gslen, port = 0;
		int i, addr, addr_h, addr_m, addr_l, checksum;
		p = block_to_part(val[5], midi_port_number);

		/* calculate checksum */
		checksum = 0;
		for(gslen = 9; gslen < len; gslen++)
			if(val[gslen] == 0xF7)
				break;
		for(i=4;i<gslen-1;i++) {
			checksum += val[i];
		}
		if(((128 - (checksum & 0x7F)) & 0x7F) != val[gslen-1]) {
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"GS SysEx: Checksum Error.");
			return num_events;
		}

		/* drum channel */
		dp = rhythm_part[(val[5] & 0xF0) >> 4];

		/* calculate user drumset number */
		udn = (val[5] & 0xF0) >> 4;

		addr_h = val[4];
		addr_m = val[5];
		addr_l = val[6];
		if(addr_h == 0x50) {	/* for double module mode */
			port = 1;
			p = block_to_part(val[5], port);
			addr_h = 0x40;
		} else if(addr_h == 0x51) {
			port = 1;
			p = block_to_part(val[5], port);
			addr_h = 0x41;
		}
		addr = (((int32)addr_h)<<16 | ((int32)addr_m)<<8 | (int32)addr_l);

		switch(addr_h) {
		case 0x40:
			if((addr & 0xFFF000) == 0x401000) {
				switch(addr & 0xFF) {
				case 0x00:	/* Tone Number */
					SETMIDIEVENT(evm[0], 0, ME_TONE_BANK_MSB, p,val[7], SYSEX_TAG);
					SETMIDIEVENT(evm[1], 0, ME_PROGRAM, p, val[8], SYSEX_TAG);
					num_events += 2;
					break;
				case 0x02:	/* Rx. Channel */
					if (val[7] == 0x10) {
						SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB,
								block_to_part(val[5],
								midi_port_number ^ port), 0x80, 0x45);
					} else {
						SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB,
								block_to_part(val[5],
								midi_port_number ^ port),
								MERGE_CHANNEL_PORT2(val[7],
								midi_port_number ^ port), 0x45);
					}
					num_events++;
					break;
				case 0x03:	/* Rx. Pitch Bend */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x48);
					num_events++;
					break;
				case 0x04:	/* Rx. Channel Pressure */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x49);
					num_events++;
					break;
				case 0x05:	/* Rx. Program Change */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x4A);
					num_events++;
					break;
				case 0x06:	/* Rx. Control Change */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x4B);
					num_events++;
					break;
				case 0x07:	/* Rx. Poly Pressure */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x4C);
					num_events++;
					break;
				case 0x08:	/* Rx. Note Message */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x4D);
					num_events++;
					break;
				case 0x09:	/* Rx. RPN */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x4E);
					num_events++;
					break;
				case 0x0A:	/* Rx. NRPN */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x4F);
					num_events++;
					break;
				case 0x0B:	/* Rx. Modulation */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x50);
					num_events++;
					break;
				case 0x0C:	/* Rx. Volume */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x51);
					num_events++;
					break;
				case 0x0D:	/* Rx. Panpot */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x52);
					num_events++;
					break;
				case 0x0E:	/* Rx. Expression */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x53);
					num_events++;
					break;
				case 0x0F:	/* Rx. Hold1 */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x54);
					num_events++;
					break;
				case 0x10:	/* Rx. Portamento */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x55);
					num_events++;
					break;
				case 0x11:	/* Rx. Sostenuto */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x56);
					num_events++;
					break;
				case 0x12:	/* Rx. Soft */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x57);
					num_events++;
					break;
				case 0x13:	/* MONO/POLY Mode */
					if(val[7] == 0) {SETMIDIEVENT(evm[0], 0, ME_MONO, p, val[7], SYSEX_TAG);}
					else {SETMIDIEVENT(evm[0], 0, ME_POLY, p, val[7], SYSEX_TAG);}
					num_events++;
					break;
				case 0x14:	/* Assign Mode */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x24);
					num_events++;
					break;
				case 0x15:	/* Use for Rhythm Part */
					if(val[7]) {
						rhythm_part[val[7] - 1] = p;
					}
					break;
				case 0x16:	/* Pitch Key Shift (dummy. see parse_sysex_event()) */
					break;
				case 0x17:	/* Pitch Offset Fine */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x26);
					num_events++;
					break;
				case 0x19:	/* Part Level */
					SETMIDIEVENT(evm[0], 0, ME_MAINVOLUME, p, val[7], SYSEX_TAG);
					num_events++;
					break;
				case 0x1A:	/* Velocity Sense Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x21);
					num_events++;
					break;
				case 0x1B:	/* Velocity Sense Offset */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x22);
					num_events++;
					break;
				case 0x1C:	/* Part Panpot */
					if (val[7] == 0) {
						SETMIDIEVENT(evm[0], 0, ME_RANDOM_PAN, p, 0, SYSEX_TAG);
					} else {
						SETMIDIEVENT(evm[0], 0, ME_PAN, p, val[7], SYSEX_TAG);
					}
					num_events++;
					break;
				case 0x1D:	/* Keyboard Range Low */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x42);
					num_events++;
					break;
				case 0x1E:	/* Keyboard Range High */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x43);
					num_events++;
					break;
				case 0x1F:	/* CC1 Controller Number */
					ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC1 Controller Number is not supported. (CH:%d VAL:%d)", p, val[7]);
					break;
				case 0x20:	/* CC2 Controller Number */
					ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC2 Controller Number is not supported. (CH:%d VAL:%d)", p, val[7]);
					break;
				case 0x21:	/* Chorus Send Level */
					SETMIDIEVENT(evm[0], 0, ME_CHORUS_EFFECT, p, val[7], SYSEX_TAG);
					num_events++;
					break;
				case 0x22:	/* Reverb Send Level */
					SETMIDIEVENT(evm[0], 0, ME_REVERB_EFFECT, p, val[7], SYSEX_TAG);
					num_events++;
					break;
				case 0x23:	/* Rx. Bank Select */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x58);
					num_events++;
					break;
				case 0x24:	/* Rx. Bank Select LSB */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x59);
					num_events++;
					break;
				case 0x2C:	/* Delay Send Level */
					SETMIDIEVENT(evm[0], 0, ME_CELESTE_EFFECT, p, val[7], SYSEX_TAG);
					num_events++;
					break;
				case 0x2A:	/* Pitch Fine Tune */
					SETMIDIEVENT(evm[0], 0, ME_NRPN_MSB, p, 0x00, SYSEX_TAG);
					SETMIDIEVENT(evm[1], 0, ME_NRPN_LSB, p, 0x01, SYSEX_TAG);
					SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, p, val[7], SYSEX_TAG);
					SETMIDIEVENT(evm[3], 0, ME_DATA_ENTRY_LSB, p, val[8], SYSEX_TAG);
					num_events += 4;
					break;
				case 0x30:	/* TONE MODIFY1: Vibrato Rate */
					SETMIDIEVENT(evm[0], 0, ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
					SETMIDIEVENT(evm[1], 0, ME_NRPN_LSB, p, 0x08, SYSEX_TAG);
					SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, p, val[7], SYSEX_TAG);
					num_events += 3;
					break;
				case 0x31:	/* TONE MODIFY2: Vibrato Depth */
					SETMIDIEVENT(evm[0], 0, ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
					SETMIDIEVENT(evm[1], 0, ME_NRPN_LSB, p, 0x09, SYSEX_TAG);
					SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, p, val[7], SYSEX_TAG);
					num_events += 3;
					break;
				case 0x32:	/* TONE MODIFY3: TVF Cutoff Freq */
					SETMIDIEVENT(evm[0], 0, ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
					SETMIDIEVENT(evm[1], 0, ME_NRPN_LSB, p, 0x20, SYSEX_TAG);
					SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, p, val[7], SYSEX_TAG);
					num_events += 3;
					break;
				case 0x33:	/* TONE MODIFY4: TVF Resonance */
					SETMIDIEVENT(evm[0], 0, ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
					SETMIDIEVENT(evm[1], 0, ME_NRPN_LSB, p, 0x21, SYSEX_TAG);
					SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, p, val[7], SYSEX_TAG);
					num_events += 3;
					break;
				case 0x34:	/* TONE MODIFY5: TVF&TVA Env.attack */
					SETMIDIEVENT(evm[0], 0, ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
					SETMIDIEVENT(evm[1], 0, ME_NRPN_LSB, p, 0x63, SYSEX_TAG);
					SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, p, val[7], SYSEX_TAG);
					num_events += 3;
					break;
				case 0x35:	/* TONE MODIFY6: TVF&TVA Env.decay */
					SETMIDIEVENT(evm[0], 0, ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
					SETMIDIEVENT(evm[1], 0, ME_NRPN_LSB, p, 0x64, SYSEX_TAG);
					SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, p, val[7], SYSEX_TAG);
					num_events += 3;
					break;
				case 0x36:	/* TONE MODIFY7: TVF&TVA Env.release */
					SETMIDIEVENT(evm[0], 0, ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
					SETMIDIEVENT(evm[1], 0, ME_NRPN_LSB, p, 0x66, SYSEX_TAG);
					SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, p, val[7], SYSEX_TAG);
					num_events += 3;
					break;
				case 0x37:	/* TONE MODIFY8: Vibrato Delay */
					SETMIDIEVENT(evm[0], 0, ME_NRPN_MSB, p, 0x01, SYSEX_TAG);
					SETMIDIEVENT(evm[1], 0, ME_NRPN_LSB, p, 0x0A, SYSEX_TAG);
					SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, p, val[7], SYSEX_TAG);
					num_events += 3;
					break;
				case 0x40:	/* Scale Tuning */
					for (i = 0; i < 12; i++) {
						SETMIDIEVENT(evm[i],
								0, ME_SCALE_TUNING, p, i, val[i + 7] - 64);
						ctl->cmsg(CMSG_INFO, VERB_NOISY,
								"Scale Tuning %s (CH:%d %d cent)",
								note_name[i], p, val[i + 7] - 64);
					}
					num_events += 12;
					break;
				default:
					ctl->cmsg(CMSG_INFO,VERB_NOISY,"Unsupported GS SysEx. (ADDR:%02X %02X %02X VAL:%02X %02X)",addr_h,addr_m,addr_l,val[7],val[8]);
					break;
				}
			} else if((addr & 0xFFF000) == 0x402000) {
				switch(addr & 0xFF) {
				case 0x00:	/* MOD Pitch Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x16);
					num_events++;
					break;
				case 0x01:	/* MOD TVF Cutoff Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x17);
					num_events++;
					break;
				case 0x02:	/* MOD Amplitude Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x18);
					num_events++;
					break;
				case 0x03:	/* MOD LFO1 Rate Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x19);
					num_events++;
					break;
				case 0x04:	/* MOD LFO1 Pitch Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x1A);
					num_events++;
					break;
				case 0x05:	/* MOD LFO1 TVF Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x1B);
					num_events++;
					break;
				case 0x06:	/* MOD LFO1 TVA Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x1C);
					num_events++;
					break;
				case 0x07:	/* MOD LFO2 Rate Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x1D);
					num_events++;
					break;
				case 0x08:	/* MOD LFO2 Pitch Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x1E);
					num_events++;
					break;
				case 0x09:	/* MOD LFO2 TVF Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x1F);
					num_events++;
					break;
				case 0x0A:	/* MOD LFO2 TVA Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x20);
					num_events++;
					break;
				case 0x10:	/* !!!FIXME!!! Bend Pitch Control */
					SETMIDIEVENT(evm[0], 0, ME_RPN_MSB, p, 0, SYSEX_TAG);
					SETMIDIEVENT(evm[1], 0, ME_RPN_LSB, p, 0, SYSEX_TAG);
					SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, p, (val[7] - 0x40) & 0x7F, SYSEX_TAG);
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x21);
					num_events += 4;
					break;
				case 0x11:	/* Bend TVF Cutoff Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x22);
					num_events++;
					break;
				case 0x12:	/* Bend Amplitude Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x23);
					num_events++;
					break;
				case 0x13:	/* Bend LFO1 Rate Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x24);
					num_events++;
					break;
				case 0x14:	/* Bend LFO1 Pitch Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x25);
					num_events++;
					break;
				case 0x15:	/* Bend LFO1 TVF Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x26);
					num_events++;
					break;
				case 0x16:	/* Bend LFO1 TVA Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x27);
					num_events++;
					break;
				case 0x17:	/* Bend LFO2 Rate Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x28);
					num_events++;
					break;
				case 0x18:	/* Bend LFO2 Pitch Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x29);
					num_events++;
					break;
				case 0x19:	/* Bend LFO2 TVF Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x2A);
					num_events++;
					break;
				case 0x1A:	/* Bend LFO2 TVA Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x2B);
					num_events++;
					break;
				case 0x20:	/* CAf Pitch Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x00);
					num_events++;
					break;
				case 0x21:	/* CAf TVF Cutoff Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x01);
					num_events++;
					break;
				case 0x22:	/* CAf Amplitude Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x02);
					num_events++;
					break;
				case 0x23:	/* CAf LFO1 Rate Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x03);
					num_events++;
					break;
				case 0x24:	/* CAf LFO1 Pitch Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x04);
					num_events++;
					break;
				case 0x25:	/* CAf LFO1 TVF Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x05);
					num_events++;
					break;
				case 0x26:	/* CAf LFO1 TVA Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x06);
					num_events++;
					break;
				case 0x27:	/* CAf LFO2 Rate Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x07);
					num_events++;
					break;
				case 0x28:	/* CAf LFO2 Pitch Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x08);
					num_events++;
					break;
				case 0x29:	/* CAf LFO2 TVF Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x09);
					num_events++;
					break;
				case 0x2A:	/* CAf LFO2 TVA Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x0A);
					num_events++;
					break;
				case 0x30:	/* PAf Pitch Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x0B);
					num_events++;
					break;
				case 0x31:	/* PAf TVF Cutoff Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x0C);
					num_events++;
					break;
				case 0x32:	/* PAf Amplitude Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x0D);
					num_events++;
					break;
				case 0x33:	/* PAf LFO1 Rate Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x0E);
					num_events++;
					break;
				case 0x34:	/* PAf LFO1 Pitch Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x0F);
					num_events++;
					break;
				case 0x35:	/* PAf LFO1 TVF Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x10);
					num_events++;
					break;
				case 0x36:	/* PAf LFO1 TVA Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x11);
					num_events++;
					break;
				case 0x37:	/* PAf LFO2 Rate Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x12);
					num_events++;
					break;
				case 0x38:	/* PAf LFO2 Pitch Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x13);
					num_events++;
					break;
				case 0x39:	/* PAf LFO2 TVF Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x14);
					num_events++;
					break;
				case 0x3A:	/* PAf LFO2 TVA Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x15);
					num_events++;
					break;
				case 0x40:	/* CC1 Pitch Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x2C);
					num_events++;
					break;
				case 0x41:	/* CC1 TVF Cutoff Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x2D);
					num_events++;
					break;
				case 0x42:	/* CC1 Amplitude Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x2E);
					num_events++;
					break;
				case 0x43:	/* CC1 LFO1 Rate Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x2F);
					num_events++;
					break;
				case 0x44:	/* CC1 LFO1 Pitch Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x30);
					num_events++;
					break;
				case 0x45:	/* CC1 LFO1 TVF Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x31);
					num_events++;
					break;
				case 0x46:	/* CC1 LFO1 TVA Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x32);
					num_events++;
					break;
				case 0x47:	/* CC1 LFO2 Rate Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x33);
					num_events++;
					break;
				case 0x48:	/* CC1 LFO2 Pitch Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x34);
					num_events++;
					break;
				case 0x49:	/* CC1 LFO2 TVF Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x35);
					num_events++;
					break;
				case 0x4A:	/* CC1 LFO2 TVA Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x36);
					num_events++;
					break;
				case 0x50:	/* CC2 Pitch Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x37);
					num_events++;
					break;
				case 0x51:	/* CC2 TVF Cutoff Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x38);
					num_events++;
					break;
				case 0x52:	/* CC2 Amplitude Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x39);
					num_events++;
					break;
				case 0x53:	/* CC2 LFO1 Rate Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x3A);
					num_events++;
					break;
				case 0x54:	/* CC2 LFO1 Pitch Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x3B);
					num_events++;
					break;
				case 0x55:	/* CC2 LFO1 TVF Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x3C);
					num_events++;
					break;
				case 0x56:	/* CC2 LFO1 TVA Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x3D);
					num_events++;
					break;
				case 0x57:	/* CC2 LFO2 Rate Control */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x3E);
					num_events++;
					break;
				case 0x58:	/* CC2 LFO2 Pitch Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x3F);
					num_events++;
					break;
				case 0x59:	/* CC2 LFO2 TVF Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x40);
					num_events++;
					break;
				case 0x5A:	/* CC2 LFO2 TVA Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_LSB, p, val[7], 0x41);
					num_events++;
					break;
				default:
					ctl->cmsg(CMSG_INFO,VERB_NOISY,"Unsupported GS SysEx. (ADDR:%02X %02X %02X VAL:%02X %02X)",addr_h,addr_m,addr_l,val[7],val[8]);
					break;
				}
			} else if((addr & 0xFFFF00) == 0x400100) {
				switch(addr & 0xFF) {
				case 0x30:	/* Reverb Macro */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x05);
					num_events++;
					break;
				case 0x31:	/* Reverb Character */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x06);
					num_events++;
					break;
				case 0x32:	/* Reverb Pre-LPF */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x07);
					num_events++;
					break;
				case 0x33:	/* Reverb Level */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x08);
					num_events++;
					break;
				case 0x34:	/* Reverb Time */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x09);
					num_events++;
					break;
				case 0x35:	/* Reverb Delay Feedback */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x0A);
					num_events++;
					break;
				case 0x36:	/* Unknown Reverb Parameter */
					break;
				case 0x37:	/* Reverb Predelay Time */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x0C);
					num_events++;
					break;
				case 0x38:	/* Chorus Macro */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x0D);
					num_events++;
					break;
				case 0x39:	/* Chorus Pre-LPF */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x0E);
					num_events++;
					break;
				case 0x3A:	/* Chorus Level */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x0F);
					num_events++;
					break;
				case 0x3B:	/* Chorus Feedback */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x10);
					num_events++;
					break;
				case 0x3C:	/* Chorus Delay */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x11);
					num_events++;
					break;
				case 0x3D:	/* Chorus Rate */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x12);
					num_events++;
					break;
				case 0x3E:	/* Chorus Depth */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x13);
					num_events++;
					break;
				case 0x3F:	/* Chorus Send Level to Reverb */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x14);
					num_events++;
					break;
				case 0x40:	/* Chorus Send Level to Delay */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x15);
					num_events++;
					break;
				case 0x50:	/* Delay Macro */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x16);
					num_events++;
					break;
				case 0x51:	/* Delay Pre-LPF */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x17);
					num_events++;
					break;
				case 0x52:	/* Delay Time Center */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x18);
					num_events++;
					break;
				case 0x53:	/* Delay Time Ratio Left */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x19);
					num_events++;
					break;
				case 0x54:	/* Delay Time Ratio Right */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x1A);
					num_events++;
					break;
				case 0x55:	/* Delay Level Center */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x1B);
					num_events++;
					break;
				case 0x56:	/* Delay Level Left */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x1C);
					num_events++;
					break;
				case 0x57:	/* Delay Level Right */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x1D);
					num_events++;
					break;
				case 0x58:	/* Delay Level */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x1E);
					num_events++;
					break;
				case 0x59:	/* Delay Feedback */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x1F);
					num_events++;
					break;
				case 0x5A:	/* Delay Send Level to Reverb */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x20);
					num_events++;
					break;
				default:
					ctl->cmsg(CMSG_INFO,VERB_NOISY,"Unsupported GS SysEx. (ADDR:%02X %02X %02X VAL:%02X %02X)",addr_h,addr_m,addr_l,val[7],val[8]);
					break;
				}
			} else if((addr & 0xFFFF00) == 0x400200) {
				switch(addr & 0xFF) {	/* EQ Parameter */
				case 0x00:	/* EQ LOW FREQ */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x01);
					num_events++;
					break;
				case 0x01:	/* EQ LOW GAIN */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x02);
					num_events++;
					break;
				case 0x02:	/* EQ HIGH FREQ */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x03);
					num_events++;
					break;
				case 0x03:	/* EQ HIGH GAIN */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x04);
					num_events++;
					break;
				default:
					ctl->cmsg(CMSG_INFO,VERB_NOISY,"Unsupported GS SysEx. (ADDR:%02X %02X %02X VAL:%02X %02X)",addr_h,addr_m,addr_l,val[7],val[8]);
					break;
				}
			} else if((addr & 0xFFFF00) == 0x400300) {
				switch(addr & 0xFF) {	/* Insertion Effect Parameter */
				case 0x00:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x27);
					SETMIDIEVENT(evm[1], 0, ME_SYSEX_GS_LSB, p, val[8], 0x28);
					num_events += 2;
					break;
				case 0x03:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x29);
					num_events++;
					break;
				case 0x04:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x2A);
					num_events++;
					break;
				case 0x05:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x2B);
					num_events++;
					break;
				case 0x06:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x2C);
					num_events++;
					break;
				case 0x07:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x2D);
					num_events++;
					break;
				case 0x08:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x2E);
					num_events++;
					break;
				case 0x09:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x2F);
					num_events++;
					break;
				case 0x0A:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x30);
					num_events++;
					break;
				case 0x0B:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x31);
					num_events++;
					break;
				case 0x0C:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x32);
					num_events++;
					break;
				case 0x0D:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x33);
					num_events++;
					break;
				case 0x0E:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x34);
					num_events++;
					break;
				case 0x0F:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x35);
					num_events++;
					break;
				case 0x10:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x36);
					num_events++;
					break;
				case 0x11:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x37);
					num_events++;
					break;
				case 0x12:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x38);
					num_events++;
					break;
				case 0x13:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x39);
					num_events++;
					break;
				case 0x14:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x3A);
					num_events++;
					break;
				case 0x15:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x3B);
					num_events++;
					break;
				case 0x16:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x3C);
					num_events++;
					break;
				case 0x17:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x3D);
					num_events++;
					break;
				case 0x18:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x3E);
					num_events++;
					break;
				case 0x19:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x3F);
					num_events++;
					break;
				case 0x1B:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x40);
					num_events++;
					break;
				case 0x1C:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x41);
					num_events++;
					break;
				case 0x1D:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x42);
					num_events++;
					break;
				case 0x1E:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x43);
					num_events++;
					break;
				case 0x1F:
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x44);
					num_events++;
					break;
				default:
					ctl->cmsg(CMSG_INFO,VERB_NOISY,"Unsupported GS SysEx. (ADDR:%02X %02X %02X VAL:%02X %02X)",addr_h,addr_m,addr_l,val[7],val[8]);
					break;
				}
			} else if((addr & 0xFFF000) == 0x404000) {
				switch(addr & 0xFF) {
				case 0x00:	/* TONE MAP NUMBER */
					SETMIDIEVENT(evm[0], 0, ME_TONE_BANK_LSB, p, val[7], SYSEX_TAG);
					num_events++;
					break;
				case 0x01:	/* TONE MAP-0 NUMBER */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x25);
					num_events++;
					break;
				case 0x20:	/* EQ ON/OFF */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x00);
					num_events++;
					break;
				case 0x22:	/* EFX ON/OFF */
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB, p, val[7], 0x23);
					num_events++;
					break;
				default:
					ctl->cmsg(CMSG_INFO,VERB_NOISY,"Unsupported GS SysEx. (ADDR:%02X %02X %02X VAL:%02X %02X)",addr_h,addr_m,addr_l,val[7],val[8]);
					break;
				}
			}
			break;
		case 0x41:
			switch(addr & 0xF00) {
			case 0x100:	/* Play Note Number */
				SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_MSB, dp, val[6], 0);
				SETMIDIEVENT(evm[1], 0, ME_SYSEX_GS_LSB, dp, val[7], 0x47);
				num_events += 2;
				break;
			case 0x200:
				SETMIDIEVENT(evm[0], 0, ME_NRPN_MSB, dp, 0x1A, SYSEX_TAG);
				SETMIDIEVENT(evm[1], 0, ME_NRPN_LSB, dp, val[6], SYSEX_TAG);
				SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, dp, val[7], SYSEX_TAG);
				num_events += 3;
				break;
			case 0x400:
				SETMIDIEVENT(evm[0], 0,ME_NRPN_MSB, dp, 0x1C, SYSEX_TAG);
				SETMIDIEVENT(evm[1], 0,ME_NRPN_LSB, dp, val[6], SYSEX_TAG);
				SETMIDIEVENT(evm[2], 0,ME_DATA_ENTRY_MSB, dp, val[7], SYSEX_TAG);
				num_events += 3;
				break;
			case 0x500:
				SETMIDIEVENT(evm[0], 0, ME_NRPN_MSB, dp, 0x1D, SYSEX_TAG);
				SETMIDIEVENT(evm[1], 0, ME_NRPN_LSB, dp, val[6], SYSEX_TAG);
				SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, dp, val[7], SYSEX_TAG);
				num_events += 3;
				break;
			case 0x600:
				SETMIDIEVENT(evm[0], 0, ME_NRPN_MSB, dp, 0x1E, SYSEX_TAG);
				SETMIDIEVENT(evm[1], 0, ME_NRPN_LSB, dp, val[6], SYSEX_TAG);
				SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, dp, val[7], SYSEX_TAG);
				num_events += 3;
				break;
			case 0x700:	/* Rx. Note Off */
				SETMIDIEVENT(evm[0], 0, ME_SYSEX_MSB, dp, val[6], 0);
				SETMIDIEVENT(evm[1], 0, ME_SYSEX_LSB, dp, val[7], 0x46);
				num_events += 2;
				break;
			case 0x800:	/* Rx. Note On */
				SETMIDIEVENT(evm[0], 0, ME_SYSEX_MSB, dp, val[6], 0);
				SETMIDIEVENT(evm[1], 0, ME_SYSEX_LSB, dp, val[7], 0x47);
				num_events += 2;
				break;
			case 0x900:
				SETMIDIEVENT(evm[0], 0, ME_NRPN_MSB, dp, 0x1F, SYSEX_TAG);
				SETMIDIEVENT(evm[1], 0, ME_NRPN_LSB, dp, val[6], SYSEX_TAG);
				SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, dp, val[7], SYSEX_TAG);
				num_events += 3;
				break;
			default:
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"Unsupported GS SysEx. (ADDR:%02X %02X %02X VAL:%02X %02X)",addr_h,addr_m,addr_l,val[7],val[8]);
				break;
			}
			break;
#if 0
		case 0x20:	/* User Instrument */
			switch(addr & 0xF00) {
				case 0x000:	/* Source Map */
					get_userinst(64 + udn, val[6])->source_map = val[7];
					break;
				case 0x100:	/* Source Bank */
					get_userinst(64 + udn, val[6])->source_bank = val[7];
					break;
#if !defined(TIMIDITY_TOOLS)
				case 0x200:	/* Source Prog */
					get_userinst(64 + udn, val[6])->source_prog = val[7];
					break;
#endif
				default:
					ctl->cmsg(CMSG_INFO,VERB_NOISY,"Unsupported GS SysEx. (ADDR:%02X %02X %02X VAL:%02X %02X)",addr_h,addr_m,addr_l,val[7],val[8]);
					break;
			}
			break;
#endif
		case 0x21:	/* User Drumset */
			switch(addr & 0xF00) {
				case 0x100:	/* Play Note */
					get_userdrum(64 + udn, val[6])->play_note = val[7];
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_MSB, dp, val[6], 0);
					SETMIDIEVENT(evm[1], 0, ME_SYSEX_GS_LSB, dp, val[7], 0x47);
					num_events += 2;
					break;
				case 0x200:	/* Level */
					get_userdrum(64 + udn, val[6])->level = val[7];
					SETMIDIEVENT(evm[0], 0, ME_NRPN_MSB, dp, 0x1A, SYSEX_TAG);
					SETMIDIEVENT(evm[1], 0, ME_NRPN_LSB, dp, val[6], SYSEX_TAG);
					SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, dp, val[7], SYSEX_TAG);
					num_events += 3;
					break;
				case 0x300:	/* Assign Group */
					get_userdrum(64 + udn, val[6])->assign_group = val[7];
					if(val[7] != 0) {recompute_userdrum_altassign(udn + 64, val[7]);}
					break;
				case 0x400:	/* Panpot */
					get_userdrum(64 + udn, val[6])->pan = val[7];
					SETMIDIEVENT(evm[0], 0, ME_NRPN_MSB, dp, 0x1C, SYSEX_TAG);
					SETMIDIEVENT(evm[1], 0, ME_NRPN_LSB, dp, val[6], SYSEX_TAG);
					SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, dp, val[7], SYSEX_TAG);
					num_events += 3;
					break;
				case 0x500:	/* Reverb Send Level */
					get_userdrum(64 + udn, val[6])->reverb_send_level = val[7];
					SETMIDIEVENT(evm[0], 0, ME_NRPN_MSB, dp, 0x1D, SYSEX_TAG);
					SETMIDIEVENT(evm[1], 0, ME_NRPN_LSB, dp, val[6], SYSEX_TAG);
					SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, dp, val[7], SYSEX_TAG);
					num_events += 3;
					break;
				case 0x600:	/* Chorus Send Level */
					get_userdrum(64 + udn, val[6])->chorus_send_level = val[7];
					SETMIDIEVENT(evm[0], 0, ME_NRPN_MSB, dp, 0x1E, SYSEX_TAG);
					SETMIDIEVENT(evm[1], 0, ME_NRPN_LSB, dp, val[6], SYSEX_TAG);
					SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, dp, val[7], SYSEX_TAG);
					num_events += 3;
					break;
				case 0x700:	/* Rx. Note Off */
					get_userdrum(64 + udn, val[6])->rx_note_off = val[7];
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_MSB, dp, val[6], 0);
					SETMIDIEVENT(evm[1], 0, ME_SYSEX_LSB, dp, val[7], 0x46);
					num_events += 2; 
					break;
				case 0x800:	/* Rx. Note On */
					get_userdrum(64 + udn, val[6])->rx_note_on = val[7];
					SETMIDIEVENT(evm[0], 0, ME_SYSEX_MSB, dp, val[6], 0);
					SETMIDIEVENT(evm[1], 0, ME_SYSEX_LSB, dp, val[7], 0x47);
					num_events += 2; 
					break;
				case 0x900:	/* Delay Send Level */
					get_userdrum(64 + udn, val[6])->delay_send_level = val[7];
					SETMIDIEVENT(evm[0], 0, ME_NRPN_MSB, dp, 0x1F, SYSEX_TAG);
					SETMIDIEVENT(evm[1], 0, ME_NRPN_LSB, dp, val[6], SYSEX_TAG);
					SETMIDIEVENT(evm[2], 0, ME_DATA_ENTRY_MSB, dp, val[7], SYSEX_TAG);
					num_events += 3;
					break;
				case 0xA00:	/* Source Map */
					get_userdrum(64 + udn, val[6])->source_map = val[7];
					break;
				case 0xB00:	/* Source Prog */
					get_userdrum(64 + udn, val[6])->source_prog = val[7];
					break;
#if !defined(TIMIDITY_TOOLS)
				case 0xC00:	/* Source Note */
					get_userdrum(64 + udn, val[6])->source_note = val[7];
					break;
#endif
				default:
					ctl->cmsg(CMSG_INFO,VERB_NOISY,"Unsupported GS SysEx. (ADDR:%02X %02X %02X VAL:%02X %02X)",addr_h,addr_m,addr_l,val[7],val[8]);
					break;
			}
			break;
		case 0x00:	/* System */
			switch (addr & 0xfff0) {
			case 0x0100:	/* Channel Msg Rx Port (A) */
				SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB,
						block_to_part(addr & 0xf, 0), val[7], 0x46);
				num_events++;
				break;
			case 0x0110:	/* Channel Msg Rx Port (B) */
				SETMIDIEVENT(evm[0], 0, ME_SYSEX_GS_LSB,
						block_to_part(addr & 0xf, 1), val[7], 0x46);
				num_events++;
				break;
			default:
			/*	ctl->cmsg(CMSG_INFO,VERB_NOISY, "Unsupported GS SysEx. "
						"(ADDR:%02X %02X %02X VAL:%02X %02X)",
						addr_h, addr_m, addr_l, val[7], val[8]);*/
				break;
			}
			break;
		}
    }

	/* Non-RealTime / RealTime Universal SysEx messages
	 * 0 0x7e(Non-RealTime) / 0x7f(RealTime)
	 * 1 SysEx device ID.  Could be from 0x00 to 0x7f.
	 *   0x7f means disregard device.
	 * 2 Sub ID
	 * ...
	 * E 0xf7
	 */
	else if (len > 4 && val[0] >= 0x7e)
		switch (val[2]) {
		case 0x01:	/* Sample Dump header */
		case 0x02:	/* Sample Dump packet */
		case 0x03:	/* Dump Request */
		case 0x04:	/* Device Control */
			if (val[3] == 0x05)	{	/* Global Parameter Control */
				if (val[7] == 0x01 && val[8] == 0x01) {	/* Reverb */
					for (i = 9; i < len && val[i] != 0xf7; i+= 2) {
						switch(val[i]) {
						case 0x00:	/* Reverb Type */
							SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, 0, val[i + 1], 0x60);
							num_events++;
							break;
						case 0x01:	/* Reverb Time */
							SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_GS_LSB, 0, val[i + 1], 0x09);
							num_events++;
							break;
						}
					}
				} else if (val[7] == 0x01 && val[8] == 0x02) {	/* Chorus */
					for (i = 9; i < len && val[i] != 0xf7; i+= 2) {
						switch(val[i]) {
						case 0x00:	/* Chorus Type */
							SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, 0, val[i + 1], 0x61);
							num_events++;
							break;
						case 0x01:	/* Modulation Rate */
							SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_GS_LSB, 0, val[i + 1], 0x12);
							num_events++;
							break;
						case 0x02:	/* Modulation Depth */
							SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_GS_LSB, 0, val[i + 1], 0x13);
							num_events++;
							break;
						case 0x03:	/* Feedback */
							SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_GS_LSB, 0, val[i + 1], 0x10);
							num_events++;
							break;
						case 0x04:	/* Send To Reverb */
							SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_GS_LSB, 0, val[i + 1], 0x14);
							num_events++;
							break;
						}
					}
				}
			}
			break;
		case 0x05:	/* Sample Dump extensions */
		case 0x06:	/* Inquiry Message */
		case 0x07:	/* File Dump */
			break;
		case 0x08:	/* MIDI Tuning Standard */
			switch (val[3]) {
			case 0x01:
				SETMIDIEVENT(evm[0], 0, ME_BULK_TUNING_DUMP, 0, val[4], 0);
				for (i = 0; i < 128; i++) {
					SETMIDIEVENT(evm[i * 2 + 1], 0, ME_BULK_TUNING_DUMP,
							1, i, val[i * 3 + 21]);
					SETMIDIEVENT(evm[i * 2 + 2], 0, ME_BULK_TUNING_DUMP,
							2, val[i * 3 + 22], val[i * 3 + 23]);
				}
				num_events += 257;
				break;
			case 0x02:
				SETMIDIEVENT(evm[0], 0, ME_SINGLE_NOTE_TUNING,
						0, val[4], 0);
				for (i = 0; i < val[5]; i++) {
					SETMIDIEVENT(evm[i * 2 + 1], 0, ME_SINGLE_NOTE_TUNING,
							1, val[i * 4 + 6], val[i * 4 + 7]);
					SETMIDIEVENT(evm[i * 2 + 2], 0, ME_SINGLE_NOTE_TUNING,
							2, val[i * 4 + 8], val[i * 4 + 9]);
				}
				num_events += val[5] * 2 + 1;
				break;
			case 0x0b:
				channel_tt = ((val[4] & 0x03) << 14 | val[5] << 7 | val[6])
						<< ((val[4] >> 2) * 16);
				if (val[1] == 0x7f) {
					SETMIDIEVENT(evm[0], 0, ME_MASTER_TEMPER_TYPE,
							0, val[7], (val[0] == 0x7f));
					num_events++;
				} else {
					for (i = j = 0; i < MAX_CHANNELS; i++)
						if (channel_tt & 1 << i) {
							SETMIDIEVENT(evm[j], 0, ME_TEMPER_TYPE,
									MERGE_CHANNEL_PORT(i),
									val[7], (val[0] == 0x7f));
							j++;
						}
					num_events += j;
				}
				break;
			case 0x0c:
				SETMIDIEVENT(evm[0], 0, ME_USER_TEMPER_ENTRY,
						0, val[4], val[21]);
				for (i = 0; i < val[21]; i++) {
					SETMIDIEVENT(evm[i * 5 + 1], 0, ME_USER_TEMPER_ENTRY,
							1, val[i * 10 + 22], val[i * 10 + 23]);
					SETMIDIEVENT(evm[i * 5 + 2], 0, ME_USER_TEMPER_ENTRY,
							2, val[i * 10 + 24], val[i * 10 + 25]);
					SETMIDIEVENT(evm[i * 5 + 3], 0, ME_USER_TEMPER_ENTRY,
							3, val[i * 10 + 26], val[i * 10 + 27]);
					SETMIDIEVENT(evm[i * 5 + 4], 0, ME_USER_TEMPER_ENTRY,
							4, val[i * 10 + 28], val[i * 10 + 29]);
					SETMIDIEVENT(evm[i * 5 + 5], 0, ME_USER_TEMPER_ENTRY,
							5, val[i * 10 + 30], val[i * 10 + 31]);
				}
				num_events += val[21] * 5 + 1;
				break;
			}
			break;
		case 0x09:	/* General MIDI Message */
			switch(val[3]) {
			case 0x01:	/* Channel Pressure */
				for (i = 5; i < len && val[i] != 0xf7; i+= 2) {
					switch(val[i]) {
					case 0x00:	/* Pitch Control */
						SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, val[4], val[i + 1], 0x00);
						num_events++;
						break;
					case 0x01:	/* Filter Cutoff Control */
						SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, val[4], val[i + 1], 0x01);
						num_events++;
						break;
					case 0x02:	/* Amplitude Control */
						SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, val[4], val[i + 1], 0x02);
						num_events++;
						break;
					case 0x03:	/* LFO Pitch Depth */
						SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, val[4], val[i + 1], 0x04);
						num_events++;
						break;
					case 0x04:	/* LFO Filter Depth */
						SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, val[4], val[i + 1], 0x05);
						num_events++;
						break;
					case 0x05:	/* LFO Amplitude Depth */
						SETMIDIEVENT(evm[num_events], 0, ME_SYSEX_LSB, val[4], val[i + 1], 0x06);
						num_events++;
						break;
					}
				}
				break;
			}
			break;
		case 0x7b:	/* End of File */
		case 0x7c:	/* Handshaking Message: Wait */
		case 0x7d:	/* Handshaking Message: Cancel */
		case 0x7e:	/* Handshaking Message: NAK */
		case 0x7f:	/* Handshaking Message: ACK */
			break;
		}

    return(num_events);
}

int parse_sysex_event(uint8 *val, int32 len, MidiEvent *ev)
{
	uint16 vol;
	
    if(current_file_info->mid == 0 || current_file_info->mid >= 0x7e)
	current_file_info->mid = val[0];

    if(len >= 10 &&
       val[0] == 0x41 && /* Roland ID */
       val[1] == 0x10 && /* Device ID */
       val[2] == 0x42 && /* GS Model ID */
       val[3] == 0x12)   /* Data Set Command */
    {
	/* Roland GS-Based Synthesizers.
	 * val[4..6] is address, val[7..len-2] is body.
	 *
	 * GS     Channel part number
	 * 0      10
	 * 1-9    1-9
	 * 10-15  11-16
	 */

	int32 addr,checksum,i;		/* SysEx address */
	uint8 *body;		/* SysEx body */
	uint8 p,gslen;		/* Channel part number [0..15] */

	/* check Checksum */
	checksum = 0;
	for(gslen = 9; gslen < len; gslen++)
		if(val[gslen] == 0xF7)
			break;
	for(i=4;i<gslen-1;i++) {
		checksum += val[i];
	}
	if(((128 - (checksum & 0x7F)) & 0x7F) != val[gslen-1]) {
		return 0;
	}

	addr = (((int32)val[4])<<16 |
		((int32)val[5])<<8 |
		(int32)val[6]);
	body = val + 7;
	p = (uint8)((addr >> 8) & 0xF);
	if(p == 0)
	    p = 9;
	else if(p <= 9)
	    p--;
	p = MERGE_CHANNEL_PORT(p);

	if(val[4] == 0x50) {	/* for double module mode */
		p += 16;
		addr = (((int32)0x40)<<16 |
			((int32)val[5])<<8 |
			(int32)val[6]);
	} else {	/* single module mode */
		addr = (((int32)val[4])<<16 |
			((int32)val[5])<<8 |
			(int32)val[6]);
	}

	if((addr & 0xFFF0FF) == 0x401015) /* Rhythm Parts */
	{
#ifdef GS_DRUMPART
/* GS drum part check from Masaaki Koyanagi's patch (GS_Drum_Part_Check()) */
/* Modified by Masanao Izumo */
	    SETMIDIEVENT(*ev, 0, ME_DRUMPART, p, *body, SYSEX_TAG);
	    return 1;
#else
	    return 0;
#endif /* GS_DRUMPART */
	}

	if((addr & 0xFFF0FF) == 0x401016) /* Key Shift */
	{
	    SETMIDIEVENT(*ev, 0, ME_KEYSHIFT, p, *body, SYSEX_TAG);
	    return 1;
	}

	if(addr == 0x400004) /* Master Volume */
	{
	    vol = gs_convert_master_vol(*body);
	    SETMIDIEVENT(*ev, 0, ME_MASTER_VOLUME,
			 0, vol & 0xFF, (vol >> 8) & 0xFF);
	    return 1;
	}

	if((addr & 0xFFF0FF) == 0x401019) /* Volume on/off */
	{
#if 0
	    SETMIDIEVENT(*ev, 0, ME_VOLUME_ONOFF, p, *body >= 64, SYSEX_TAG);
#endif
	    return 0;
	}

	if((addr & 0xFFF0FF) == 0x401002) /* Receive channel on/off */
	{
#if 0
	    SETMIDIEVENT(*ev, 0, ME_RECEIVE_CHANNEL, (uint8)p, *body >= 64, SYSEX_TAG);
#endif
	    return 0;
	}

	if(0x402000 <= addr && addr <= 0x402F5A) /* Controller Routing */
	    return 0;

	if((addr & 0xFFF0FF) == 0x401040) /* Alternate Scale Tunings */
	    return 0;

	if((addr & 0xFFFFF0) == 0x400130) /* Changing Effects */
	{
		struct chorus_text_gs_t *chorus_text = &(chorus_status_gs.text);
	    switch(addr & 0xF)
	    {
	      case 0x8: /* macro */
		memcpy(chorus_text->macro, body, 3);
		break;
	      case 0x9: /* PRE-LPF */
		memcpy(chorus_text->pre_lpf, body, 3);
		break;
	      case 0xa: /* level */
		memcpy(chorus_text->level, body, 3);
		break;
	      case 0xb: /* feed back */
		memcpy(chorus_text->feed_back, body, 3);
		break;
	      case 0xc: /* delay */
		memcpy(chorus_text->delay, body, 3);
		break;
	      case 0xd: /* rate */
		memcpy(chorus_text->rate, body, 3);
		break;
	      case 0xe: /* depth */
		memcpy(chorus_text->depth, body, 3);
		break;
	      case 0xf: /* send level */
		memcpy(chorus_text->send_level, body, 3);
		break;
		  default: break;
	    }

	    check_chorus_text_start();
	    return 0;
	}

	if((addr & 0xFFF0FF) == 0x401003) /* Rx Pitch-Bend */
	    return 0;

	if(addr == 0x400110) /* Voice Reserve */
	{
	    if(len >= 25)
		memcpy(chorus_status_gs.text.voice_reserve, body, 18);
	    check_chorus_text_start();
	    return 0;
	}

	if(addr == 0x40007F ||	/* GS Reset */
	   addr == 0x00007F)	/* SC-88 Single Module */
	{
	    SETMIDIEVENT(*ev, 0, ME_RESET, 0, GS_SYSTEM_MODE, SYSEX_TAG);
	    return 1;
	}
	return 0;
    }

    if(len > 9 &&
       val[0] == 0x41 && /* Roland ID */
       val[1] == 0x10 && /* Device ID */
       val[2] == 0x45 && 
       val[3] == 0x12 && 
       val[4] == 0x10 && 
       val[5] == 0x00 && 
       val[6] == 0x00)
    {
	/* Text Insert for SC */
	uint8 save;

	len -= 2;
	save = val[len];
	val[len] = '\0';
	if(readmidi_make_string_event(ME_INSERT_TEXT, (char *)val + 7, ev, 1))
	{
	    val[len] = save;
	    return 1;
	}
	val[len] = save;
	return 0;
    }

    if(len > 9 &&                     /* GS lcd event. by T.Nogami*/
       val[0] == 0x41 && /* Roland ID */
       val[1] == 0x10 && /* Device ID */
       val[2] == 0x45 && 
       val[3] == 0x12 && 
       val[4] == 0x10 && 
       val[5] == 0x01 && 
       val[6] == 0x00)
    {
	/* Text Insert for SC */
	uint8 save;

	len -= 2;
	save = val[len];
	val[len] = '\0';
	if(readmidi_make_lcd_event(ME_GSLCD, (uint8 *)val + 7, ev))
	{
	    val[len] = save;
	    return 1;
	}
	val[len] = save;
	return 0;
    }
    
    if(len >= 8 &&
       val[0] == 0x43 &&
       val[1] == 0x10 &&
       val[2] == 0x4C &&
       val[3] == 0x00 &&
       val[4] == 0x00 &&
       val[5] == 0x7E)
    {
	/* XG SYSTEM ON */
	SETMIDIEVENT(*ev, 0, ME_RESET, 0, XG_SYSTEM_MODE, SYSEX_TAG);
	return 1;
    }

	/* Non-RealTime / RealTime Universal SysEx messages
	 * 0 0x7e(Non-RealTime) / 0x7f(RealTime)
	 * 1 SysEx device ID.  Could be from 0x00 to 0x7f.
	 *   0x7f means disregard device.
	 * 2 Sub ID
	 * ...
	 * E 0xf7
	 */
	if (len > 4 && val[0] >= 0x7e)
		switch (val[2]) {
		case 0x01:	/* Sample Dump header */
		case 0x02:	/* Sample Dump packet */
		case 0x03:	/* Dump Request */
			break;
		case 0x04:	/* MIDI Time Code Setup/Device Control */
			switch (val[3]) {
			case 0x01:	/* Master Volume */
				vol = gm_convert_master_vol(val[4], val[5]);
				if (val[1] == 0x7f) {
					SETMIDIEVENT(*ev, 0, ME_MASTER_VOLUME, 0,
							vol & 0xff, vol >> 8 & 0xff);
				} else {
					SETMIDIEVENT(*ev, 0, ME_MAINVOLUME,
							MERGE_CHANNEL_PORT(val[1]),
							vol >> 8 & 0xff, 0);
				}
				return 1;
			}
			break;
		case 0x05:	/* Sample Dump extensions */
		case 0x06:	/* Inquiry Message */
		case 0x07:	/* File Dump */
			break;
		case 0x08:	/* MIDI Tuning Standard */
			switch (val[3]) {
			case 0x0a:
				SETMIDIEVENT(*ev, 0, ME_TEMPER_KEYSIG, 0,
						val[4] - 0x40 + val[5] * 16, (val[0] == 0x7f));
				return 1;
			}
			break;
		case 0x09:	/* General MIDI Message */
			/* GM System Enable/Disable */
			if(val[3] == 1) {
				ctl->cmsg(CMSG_INFO, VERB_DEBUG, "SysEx: GM System On");
				SETMIDIEVENT(*ev, 0, ME_RESET, 0, GM_SYSTEM_MODE, 0);
			} else if(val[3] == 3) {
				ctl->cmsg(CMSG_INFO, VERB_DEBUG, "SysEx: GM2 System On");
				SETMIDIEVENT(*ev, 0, ME_RESET, 0, GM2_SYSTEM_MODE, 0);
			} else {
				ctl->cmsg(CMSG_INFO, VERB_DEBUG, "SysEx: GM System Off");
				SETMIDIEVENT(*ev, 0, ME_RESET, 0, DEFAULT_SYSTEM_MODE, 0);
			}
			return 1;
		case 0x7b:	/* End of File */
		case 0x7c:	/* Handshaking Message: Wait */
		case 0x7d:	/* Handshaking Message: Cancel */
		case 0x7e:	/* Handshaking Message: NAK */
		case 0x7f:	/* Handshaking Message: ACK */
			break;
		}

    return 0;
}

static int read_sysex_event(int32 at, int me, int32 len,
			    struct timidity_file *tf)
{
    uint8 *val;
    MidiEvent ev, evm[260]; /* maximum number of XG bulk dump events */
    int ne, i;

    if(len == 0)
	return 0;
    if(me != 0xF0)
    {
	skip(tf, len);
	return 0;
    }

    val = (uint8 *)new_segment(&tmpbuffer, len);
    if(tf_read(val, 1, len, tf) != len)
    {
	reuse_mblock(&tmpbuffer);
	return -1;
    }
    if(parse_sysex_event(val, len, &ev))
    {
	ev.time = at;
	readmidi_add_event(&ev);
    }
    if ((ne = parse_sysex_event_multi(val, len, evm)))
    {
	for (i = 0; i < ne; i++) {
	    evm[i].time = at;
	    readmidi_add_event(&evm[i]);
	}
    }
    
    reuse_mblock(&tmpbuffer);

    return 0;
}

static char *fix_string(char *s)
{
    int i, j, w;
    char c;

    if(s == NULL)
	return NULL;
    while(*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
	s++;

    /* s =~ tr/ \t\r\n/ /s; */
    w = 0;
    for(i = j = 0; (c = s[i]) != '\0'; i++)
    {
	if(c == '\t' || c == '\r' || c == '\n')
	    c = ' ';
	if(w)
	    w = (c == ' ');
	if(!w)
	{
	    s[j++] = c;
	    w = (c == ' ');
	}
    }

    /* s =~ s/ $//; */
    if(j > 0 && s[j - 1] == ' ')
	j--;

    s[j] = '\0';
    return s;
}

static void smf_time_signature(int32 at, struct timidity_file *tf, int len)
{
    int n, d, c, b;

    /* Time Signature (nn dd cc bb)
     * [0]: numerator
     * [1]: denominator
     * [2]: number of MIDI clocks in a metronome click
     * [3]: number of notated 32nd-notes in a MIDI
     *      quarter-note (24 MIDI Clocks).
     */

    if(len != 4)
    {
	ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Invalid time signature");
	skip(tf, len);
	return;
    }

    n = tf_getc(tf);
    d = (1<<tf_getc(tf));
    c = tf_getc(tf);
    b = tf_getc(tf);

    if(n == 0 || d == 0)
    {
	ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Invalid time signature");
	return;
    }

    MIDIEVENT(at, ME_TIMESIG, 0, n, d);
    MIDIEVENT(at, ME_TIMESIG, 1, c, b);
    ctl->cmsg(CMSG_INFO, VERB_NOISY,
	      "Time signature: %d/%d %d clock %d q.n.", n, d, c, b);
    if(current_file_info->time_sig_n == -1)
    {
	current_file_info->time_sig_n = n;
	current_file_info->time_sig_d = d;
	current_file_info->time_sig_c = c;
	current_file_info->time_sig_b = b;
    }
}

static void smf_key_signature(int32 at, struct timidity_file *tf, int len)
{
	int8 sf, mi;
	/* Key Signature (sf mi)
	 * sf = -7:  7 flats
	 * sf = -1:  1 flat
	 * sf = 0:   key of C
	 * sf = 1:   1 sharp
	 * sf = 7:   7 sharps
	 * mi = 0:  major key
	 * mi = 1:  minor key
	 */
	
	if (len != 2) {
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Invalid key signature");
		skip(tf, len);
		return;
	}
	sf = tf_getc(tf);
	mi = tf_getc(tf);
	if (sf < -7 || sf > 7) {
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Invalid key signature");
		return;
	}
	if (mi != 0 && mi != 1) {
		ctl->cmsg(CMSG_WARNING, VERB_VERBOSE, "Invalid key signature");
		return;
	}
	MIDIEVENT(at, ME_KEYSIG, 0, sf, mi);
	ctl->cmsg(CMSG_INFO, VERB_NOISY,
			"Key signature: %d %s %s", abs(sf),
			(sf < 0) ? "flat(s)" : "sharp(s)", (mi) ? "minor" : "major");
}

/* Used for WRD reader */
int dump_current_timesig(MidiEvent *codes, int maxlen)
{
    int i, n;
    MidiEventList *e;

    if(maxlen <= 0 || evlist == NULL)
	return 0;
    n = 0;
    for(i = 0, e = evlist; i < event_count; i++, e = e->next)
	if(e->event.type == ME_TIMESIG && e->event.channel == 0)
	{
	    if(n == 0 && e->event.time > 0)
	    {
		/* 4/4 is default */
		SETMIDIEVENT(codes[0], 0, ME_TIMESIG, 0, 4, 4);
		n++;
		if(maxlen == 1)
		    return 1;
	    }

	    if(n > 0)
	    {
		if(e->event.a == codes[n - 1].a &&
		   e->event.b == codes[n - 1].b)
		    continue; /* Unchanged */
		if(e->event.time == codes[n - 1].time)
		    n--; /* overwrite previous code */
	    }
	    codes[n++] = e->event;
	    if(n == maxlen)
		return n;
	}
    return n;
}

/* Read a SMF track */
static int read_smf_track(struct timidity_file *tf, int trackno, int rewindp)
{
    int32 len, next_pos, pos;
    char tmp[4];
    int lastchan, laststatus;
    int me, type, a, b, c;
    int i;
    int32 smf_at_time;

    smf_at_time = readmidi_set_track(trackno, rewindp);

    /* Check the formalities */
    if((tf_read(tmp, 1, 4, tf) != 4) || (tf_read(&len, 4, 1, tf) != 1))
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "%s: Can't read track header.", current_filename);
	return -1;
    }
    len = BE_LONG(len);
    next_pos = tf_tell(tf) + len;
    if(strncmp(tmp, "MTrk", 4))
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "%s: Corrupt MIDI file.", current_filename);
	return -2;
    }

    lastchan = laststatus = 0;

    for(;;)
    {
	if(readmidi_error_flag)
	    return -1;
	if((len = getvl(tf)) < 0)
	    return -1;
	smf_at_time += len;
	errno = 0;
	if((i = tf_getc(tf)) == EOF)
	{
	    if(errno)
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: read_midi_event: %s",
			  current_filename, strerror(errno));
	    else
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "Warning: %s: Too shorten midi file.",
			  current_filename);
	    return -1;
	}

	me = (uint8)i;
	if(me == 0xF0 || me == 0xF7) /* SysEx event */
	{
	    if((len = getvl(tf)) < 0)
		return -1;
	    if((i = read_sysex_event(smf_at_time, me, len, tf)) != 0)
		return i;
	}
	else if(me == 0xFF) /* Meta event */
	{
	    type = tf_getc(tf);
	    if((len = getvl(tf)) < 0)
		return -1;
	    if(type > 0 && type < 16)
	    {
		static char *label[] =
		{
		    "Text event: ", "Text: ", "Copyright: ", "Track name: ",
		    "Instrument: ", "Lyric: ", "Marker: ", "Cue point: "
		};

		if(type == 5 || /* Lyric */
		   (type == 1 && (opt_trace_text_meta_event ||
				  karaoke_format == 2 ||
				  chorus_status_gs.text.status == CHORUS_ST_OK)) ||
		   (type == 6 &&  (current_file_info->format == 0 ||
				   (current_file_info->format == 1 &&
				    current_read_track == 0))))
		{
		    char *str, *text;
		    MidiEvent ev;

		    str = (char *)new_segment(&tmpbuffer, len + 3);
		    if(type != 6)
		    {
			i = tf_read(str, 1, len, tf);
			str[len] = '\0';
		    }
		    else
		    {
			i = tf_read(str + 1, 1, len, tf);
			str[0] = MARKER_START_CHAR;
			str[len + 1] = MARKER_END_CHAR;
			str[len + 2] = '\0';
		    }

		    if(i != len)
		    {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				  "Warning: %s: Too shorten midi file.",
				  current_filename);
			reuse_mblock(&tmpbuffer);
			return -1;
		    }

		    if((text = readmidi_make_string_event(1, str, &ev, 1))
		       == NULL)
		    {
			reuse_mblock(&tmpbuffer);
			continue;
		    }
		    ev.time = smf_at_time;

		    if(type == 6)
		    {
			if(strlen(fix_string(text + 1)) == 2)
			{
			    reuse_mblock(&tmpbuffer);
			    continue; /* Empty Marker */
			}
		    }

		    switch(type)
		    {
		      case 1:
			if(karaoke_format == 2)
			{
			    *text = ME_KARAOKE_LYRIC;
			    if(karaoke_title_flag == 0 &&
			       strncmp(str, "@T", 2) == 0)
				current_file_info->karaoke_title =
				    add_karaoke_title(current_file_info->
						      karaoke_title, str + 2);
			    ev.type = ME_KARAOKE_LYRIC;
			    readmidi_add_event(&ev);
			    continue;
			}
			if(chorus_status_gs.text.status == CHORUS_ST_OK)
			{
			    *text = ME_CHORUS_TEXT;
			    ev.type = ME_CHORUS_TEXT;
			    readmidi_add_event(&ev);
			    continue;
			}
			*text = ME_TEXT;
			ev.type = ME_TEXT;
			readmidi_add_event(&ev);
			continue;
		      case 5:
			*text = ME_LYRIC;
			ev.type = ME_LYRIC;
			readmidi_add_event(&ev);
			continue;
		      case 6:
			*text = ME_MARKER;
			ev.type = ME_MARKER;
			readmidi_add_event(&ev);
			continue;
		    }
		}

		if(type == 3 && /* Sequence or Track Name */
		   (current_file_info->format == 0 ||
		    (current_file_info->format == 1 &&
		     current_read_track == 0)))
		{
		  if(current_file_info->seq_name == NULL) {
		    char *name = dumpstring(3, len, "Sequence: ", 1, tf);
		    current_file_info->seq_name = safe_strdup(fix_string(name));
		    free(name);
		  }
		    else
			dumpstring(3, len, "Sequence: ", 0, tf);
		}
		else if(type == 1 &&
			current_file_info->first_text == NULL &&
			(current_file_info->format == 0 ||
			 (current_file_info->format == 1 &&
			  current_read_track == 0))) {
		  char *name = dumpstring(1, len, "Text: ", 1, tf);
		  current_file_info->first_text = safe_strdup(fix_string(name));
		  free(name);
		}
		else
		    dumpstring(type, len, label[(type>7) ? 0 : type], 0, tf);
	    }
	    else
	    {
		switch(type)
		{
		  case 0x00:
		    if(len == 2)
		    {
			a = tf_getc(tf);
			b = tf_getc(tf);
			ctl->cmsg(CMSG_INFO, VERB_DEBUG,
				  "(Sequence Number %02x %02x)", a, b);
		    }
		    else
			ctl->cmsg(CMSG_INFO, VERB_DEBUG,
				  "(Sequence Number len=%d)", len);
		    break;

		  case 0x2F: /* End of Track */
		    pos = tf_tell(tf);
		    if(pos < next_pos)
			tf_seek(tf, next_pos - pos, SEEK_CUR);
		    return 0;

		  case 0x51: /* Tempo */
		    a = tf_getc(tf);
		    b = tf_getc(tf);
		    c = tf_getc(tf);
		    MIDIEVENT(smf_at_time, ME_TEMPO, c, a, b);
		    break;

		  case 0x54:
		    /* SMPTE Offset (hr mn se fr ff)
		     * hr: hours&type
		     *     0     1     2     3    4    5    6    7   bits
		     *     0  |<--type -->|<---- hours [0..23]---->|
		     * type: 00: 24 frames/second
		     *       01: 25 frames/second
		     *       10: 30 frames/second (drop frame)
		     *       11: 30 frames/second (non-drop frame)
		     * mn: minis [0..59]
		     * se: seconds [0..59]
		     * fr: frames [0..29]
		     * ff: fractional frames [0..99]
		     */
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			      "(SMPTE Offset meta event)");
		    skip(tf, len);
		    break;

		  case 0x58: /* Time Signature */
		    smf_time_signature(smf_at_time, tf, len);
		    break;

		  case 0x59: /* Key Signature */
		    smf_key_signature(smf_at_time, tf, len);
		    break;

		  case 0x7f: /* Sequencer-Specific Meta-Event */
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			      "(Sequencer-Specific meta event, length %ld)",
			      len);
		    skip(tf, len);
		    break;

		  case 0x20: /* MIDI channel prefix (SMF v1.0) */
		    if(len == 1)
		    {
			int midi_channel_prefix = tf_getc(tf);
			ctl->cmsg(CMSG_INFO, VERB_DEBUG,
				  "(MIDI channel prefix %d)",
				  midi_channel_prefix);
		    }
		    else
			skip(tf, len);
		    break;

		  case 0x21: /* MIDI port number */
		    if(len == 1)
		    {
			if((midi_port_number = tf_getc(tf))
			   == EOF)
			{
			    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				      "Warning: %s: Too shorten midi file.",
				      current_filename);
			    return -1;
			}
			midi_port_number &= 0xF;
			ctl->cmsg(CMSG_INFO, VERB_DEBUG,
				  "(MIDI port number %d)", midi_port_number);
		    }
		    else
			skip(tf, len);
		    break;

		  default:
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			      "(Meta event type 0x%02x, length %ld)",
			      type, len);
		    skip(tf, len);
		    break;
		}
	    }
	}
	else /* MIDI event */
	{
	    a = me;
	    if(a & 0x80) /* status byte */
	    {
		lastchan = MERGE_CHANNEL_PORT(a & 0x0F);
		laststatus = (a >> 4) & 0x07;
		if(laststatus != 7)
		    a = tf_getc(tf) & 0x7F;
	    }
	    switch(laststatus)
	    {
	      case 0: /* Note off */
		b = tf_getc(tf) & 0x7F;
		MIDIEVENT(smf_at_time, ME_NOTEOFF, lastchan, a,b);
		break;

	      case 1: /* Note on */
		b = tf_getc(tf) & 0x7F;
		if(b)
		{
		    MIDIEVENT(smf_at_time, ME_NOTEON, lastchan, a,b);
		}
		else /* b == 0 means Note Off */
		{
		    MIDIEVENT(smf_at_time, ME_NOTEOFF, lastchan, a, 0);
		}
		break;

	      case 2: /* Key Pressure */
		b = tf_getc(tf) & 0x7F;
		MIDIEVENT(smf_at_time, ME_KEYPRESSURE, lastchan, a, b);
		break;

	      case 3: /* Control change */
		b = tf_getc(tf);
		readmidi_add_ctl_event(smf_at_time, lastchan, a, b);
		break;

	      case 4: /* Program change */
		MIDIEVENT(smf_at_time, ME_PROGRAM, lastchan, a, 0);
		break;

	      case 5: /* Channel pressure */
		MIDIEVENT(smf_at_time, ME_CHANNEL_PRESSURE, lastchan, a, 0);
		break;

	      case 6: /* Pitch wheel */
		b = tf_getc(tf) & 0x7F;
		MIDIEVENT(smf_at_time, ME_PITCHWHEEL, lastchan, a, b);
		break;

	      default: /* case 7: */
		/* Ignore this event */
		switch(lastchan & 0xF)
		{
		  case 2: /* Sys Com Song Position Pntr */
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			      "(Sys Com Song Position Pntr)");
		    tf_getc(tf);
		    tf_getc(tf);
		    break;

		  case 3: /* Sys Com Song Select(Song #) */
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			      "(Sys Com Song Select(Song #))");
		    tf_getc(tf);
		    break;

		  case 6: /* Sys Com tune request */
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			      "(Sys Com tune request)");
		    break;
		  case 8: /* Sys real time timing clock */
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			      "(Sys real time timing clock)");
		    break;
		  case 10: /* Sys real time start */
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			      "(Sys real time start)");
		    break;
		  case 11: /* Sys real time continue */
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			      "(Sys real time continue)");
		    break;
		  case 12: /* Sys real time stop */
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			      "(Sys real time stop)");
		    break;
		  case 14: /* Sys real time active sensing */
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			      "(Sys real time active sensing)");
		    break;
#if 0
		  case 15: /* Meta */
		  case 0: /* SysEx */
		  case 7: /* SysEx */
#endif
		  default: /* 1, 4, 5, 9, 13 */
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "*** Can't happen: status 0x%02X channel 0x%02X",
			      laststatus, lastchan & 0xF);
		    break;
		}
		}
	}
    }
    /*NOTREACHED*/
}

/* Free the linked event list from memory. */
static void free_midi_list(void)
{
    if(evlist != NULL)
    {
	reuse_mblock(&mempool);
	evlist = NULL;
    }
}

static void move_channels(int *chidx)
{
	int i, ch, maxch, newch;
	MidiEventList *e;
	
	for (i = 0; i < 256; i++)
		chidx[i] = -1;
	/* check channels */
	for (i = maxch = 0, e = evlist; i < event_count; i++, e = e->next)
		if (! GLOBAL_CHANNEL_EVENT_TYPE(e->event.type)) {
			if ((ch = e->event.channel) < REDUCE_CHANNELS)
				chidx[ch] = ch;
			if (maxch < ch)
				maxch = ch;
		}
	if (maxch >= REDUCE_CHANNELS)
		/* Move channel if enable */
		for (i = maxch = 0, e = evlist; i < event_count; i++, e = e->next)
			if (! GLOBAL_CHANNEL_EVENT_TYPE(e->event.type)) {
				if (chidx[ch = e->event.channel] != -1)
					ch = e->event.channel = chidx[ch];
				else {	/* -1 */
					newch = ch % REDUCE_CHANNELS;
					while (newch < ch && newch < MAX_CHANNELS) {
						if (chidx[newch] == -1) {
							ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
									"channel %d => %d", ch, newch);
							ch = e->event.channel = chidx[ch] = newch;
							break;
						}
						newch += REDUCE_CHANNELS;
					}
					if (chidx[ch] == -1) {
						if (ch < MAX_CHANNELS)
							chidx[ch] = ch;
						else {
							newch = ch % MAX_CHANNELS;
							ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
									"channel %d => %d (mixed)", ch, newch);
							ch = e->event.channel = chidx[ch] = newch;
						}
					}
				}
				if (maxch < ch)
					maxch = ch;
			}
	for (i = 0, e = evlist; i < event_count; i++, e = e->next)
		if (e->event.type == ME_SYSEX_GS_LSB) {
			if (e->event.b == 0x45 || e->event.b == 0x46)
				if (maxch < e->event.channel)
					maxch = e->event.channel;
		} else if (e->event.type == ME_SYSEX_XG_LSB) {
			if (e->event.b == 0x99)
				if (maxch < e->event.channel)
					maxch = e->event.channel;
		}
	current_file_info->max_channel = maxch;
}

void change_system_mode(int mode)
{
    int mid;

    if(opt_system_mid)
    {
	mid = opt_system_mid;
	mode = -1; /* Always use opt_system_mid */
    }
    else
	mid = current_file_info->mid;
    pan_table = sc_pan_table;
    switch(mode)
    {
      case GM_SYSTEM_MODE:
	if(play_system_mode == DEFAULT_SYSTEM_MODE)
	{
	    play_system_mode = GM_SYSTEM_MODE;
	    vol_table = def_vol_table;
	}
	break;
      case GM2_SYSTEM_MODE:
	play_system_mode = GM2_SYSTEM_MODE;
	vol_table = def_vol_table;
	pan_table = gm2_pan_table;
	break;
      case GS_SYSTEM_MODE:
	play_system_mode = GS_SYSTEM_MODE;
	vol_table = gs_vol_table;
	break;
      case XG_SYSTEM_MODE:
    if (play_system_mode != XG_SYSTEM_MODE) {init_all_effect_xg();}
	play_system_mode = XG_SYSTEM_MODE;
	vol_table = xg_vol_table;
	break;
      default:
	/* --module option */
	if (is_gs_module()) {
		play_system_mode = GS_SYSTEM_MODE;
		break;
	} else if (is_xg_module()) {
		if (play_system_mode != XG_SYSTEM_MODE) {init_all_effect_xg();}
		play_system_mode = XG_SYSTEM_MODE;
		break;
	}
	switch(mid)
	{
	  case 0x41:
	    play_system_mode = GS_SYSTEM_MODE;
	    vol_table = gs_vol_table;
	    break;
	  case 0x43:
		if (play_system_mode != XG_SYSTEM_MODE) {init_all_effect_xg();}
	    play_system_mode = XG_SYSTEM_MODE;
	    vol_table = xg_vol_table;
	    break;
	  case 0x7e:
	    play_system_mode = GM_SYSTEM_MODE;
	    vol_table = def_vol_table;
	    break;
	  default:
	    play_system_mode = DEFAULT_SYSTEM_MODE;
		vol_table = def_vol_table;
	    break;
	}
	break;
    }
}

int get_default_mapID(int ch)
{
    if(play_system_mode == XG_SYSTEM_MODE)
	return ISDRUMCHANNEL(ch) ? XG_DRUM_MAP : XG_NORMAL_MAP;
    return INST_NO_MAP;
}

/* Allocate an array of MidiEvents and fill it from the linked list of
   events, marking used instruments for loading. Convert event times to
   samples: handle tempo changes. Strip unnecessary events from the list.
   Free the linked list. */
static MidiEvent *groom_list(int32 divisions, int32 *eventsp, int32 *samplesp)
{
    MidiEvent *groomed_list, *lp;
    MidiEventList *meep;
    int32 i, j, our_event_count, tempo, skip_this_event;
    int32 sample_cum, samples_to_do, at, st, dt, counting_time;
    int ch, gch;
    uint8 current_set[MAX_CHANNELS],
	warn_tonebank[128 + MAP_BANK_COUNT], warn_drumset[128 + MAP_BANK_COUNT];
    int8 bank_lsb[MAX_CHANNELS], bank_msb[MAX_CHANNELS], mapID[MAX_CHANNELS];
    int current_program[MAX_CHANNELS];
    int wrd_args[WRD_MAXPARAM];
    int wrd_argc;
    int chidx[256];
    int newbank, newprog;

    move_channels(chidx);

    COPY_CHANNELMASK(drumchannels, current_file_info->drumchannels);
    COPY_CHANNELMASK(drumchannel_mask, current_file_info->drumchannel_mask);

    /* Move drumchannels */
    for(ch = REDUCE_CHANNELS; ch < MAX_CHANNELS; ch++)
    {
	i = chidx[ch];
	if(i != -1 && i != ch && !IS_SET_CHANNELMASK(drumchannel_mask, i))
	{
	    if(IS_SET_CHANNELMASK(drumchannels, ch))
		SET_CHANNELMASK(drumchannels, i);
	    else
		UNSET_CHANNELMASK(drumchannels, i);
	}
    }

    memset(warn_tonebank, 0, sizeof(warn_tonebank));
    if (special_tonebank >= 0)
	newbank = special_tonebank;
    else
	newbank = default_tonebank;
    for(j = 0; j < MAX_CHANNELS; j++)
    {
	if(ISDRUMCHANNEL(j))
	    current_set[j] = 0;
	else
	{
	    if (tonebank[newbank] == NULL)
	    {
		if (warn_tonebank[newbank] == 0)
		{
		    ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
			      "Tone bank %d is undefined", newbank);
		    warn_tonebank[newbank] = 1;
		}
		newbank = 0;
	    }
	    current_set[j] = newbank;
	}
	bank_lsb[j] = bank_msb[j] = 0;
	if(play_system_mode == XG_SYSTEM_MODE && j % 16 == 9)
	    bank_msb[j] = 127; /* Use MSB=127 for XG */
	current_program[j] = default_program[j];
    }

    memset(warn_drumset, 0, sizeof(warn_drumset));
    tempo = 500000;
    compute_sample_increment(tempo, divisions);

    /* This may allocate a bit more than we need */
    groomed_list = lp =
	(MidiEvent *)safe_malloc(sizeof(MidiEvent) * (event_count + 1));
    meep = evlist;

    our_event_count = 0;
    st = at = sample_cum = 0;
    counting_time = 2; /* We strip any silence before the first NOTE ON. */
    wrd_argc = 0;
    change_system_mode(DEFAULT_SYSTEM_MODE);

    for(j = 0; j < MAX_CHANNELS; j++)
	mapID[j] = get_default_mapID(j);

    for(i = 0; i < event_count; i++)
    {
	skip_this_event = 0;
	ch = meep->event.channel;
	gch = GLOBAL_CHANNEL_EVENT_TYPE(meep->event.type);
	if(!gch && ch >= MAX_CHANNELS) /* For safety */
	    meep->event.channel = ch = ch % MAX_CHANNELS;

	if(!gch && IS_SET_CHANNELMASK(quietchannels, ch))
	    skip_this_event = 1;
	else switch(meep->event.type)
	{
	  case ME_NONE:
	    skip_this_event = 1;
	    break;
	  case ME_RESET:
	    change_system_mode(meep->event.a);
	    ctl->cmsg(CMSG_INFO, VERB_NOISY, "MIDI reset at %d sec",
		      (int)((double)st / play_mode->rate + 0.5));
	    for(j = 0; j < MAX_CHANNELS; j++)
	    {
		if(play_system_mode == XG_SYSTEM_MODE && j % 16 == 9)
		    mapID[j] = XG_DRUM_MAP;
		else
		    mapID[j] = get_default_mapID(j);
		if(ISDRUMCHANNEL(j))
		    current_set[j] = 0;
		else
		{
		    if(special_tonebank >= 0)
			current_set[j] = special_tonebank;
		    else
			current_set[j] = default_tonebank;
		    if(tonebank[current_set[j]] == NULL)
			current_set[j] = 0;
		}
		bank_lsb[j] = bank_msb[j] = 0;
		if(play_system_mode == XG_SYSTEM_MODE && j % 16 == 9)
		    bank_msb[j] = 127; /* Use MSB=127 for XG */
		current_program[j] = default_program[j];
	    }
	    break;

	  case ME_PROGRAM:
	    if(ISDRUMCHANNEL(ch))
		newbank = current_program[ch];
	    else
		newbank = current_set[ch];
	    newprog = meep->event.a;
	    switch(play_system_mode)
	    {
	      case GS_SYSTEM_MODE:
		switch(bank_lsb[ch])
		{
		  case 0:	/* No change */
		    break;
		  case 1:
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			      "(GS ch=%d SC-55 MAP)", ch);
		    mapID[ch] = (!ISDRUMCHANNEL(ch) ? SC_55_TONE_MAP
				 : SC_55_DRUM_MAP);
		    break;
		  case 2:
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			      "(GS ch=%d SC-88 MAP)", ch);
		    mapID[ch] = (!ISDRUMCHANNEL(ch) ? SC_88_TONE_MAP
				 : SC_88_DRUM_MAP);
		    break;
		  case 3:
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			      "(GS ch=%d SC-88Pro MAP)", ch);
		    mapID[ch] = (!ISDRUMCHANNEL(ch) ? SC_88PRO_TONE_MAP
				 : SC_88PRO_DRUM_MAP);
		    break;
		  case 4:
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			      "(GS ch=%d SC-8820/SC-8850 MAP)", ch);
		    mapID[ch] = (!ISDRUMCHANNEL(ch) ? SC_8850_TONE_MAP
				 : SC_8850_DRUM_MAP);
		    break;
		  default:
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			      "(GS: ch=%d Strange bank LSB %d)",
			      ch, bank_lsb[ch]);
		    break;
		}
		newbank = bank_msb[ch];
		break;

	      case XG_SYSTEM_MODE: /* XG */
		switch(bank_msb[ch])
		{
		  case 0: /* Normal */
		    if(ch == 9  && bank_lsb[ch] == 127 && mapID[ch] == XG_DRUM_MAP) {
		      /* FIXME: Why this part is drum?  Is this correct? */
		      ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
				"Warning: XG bank 0/127 is found. It may be not correctly played.");
		      ;
		    } else {
		      ctl->cmsg(CMSG_INFO, VERB_DEBUG, "(XG ch=%d Normal voice)",
				ch);
		      midi_drumpart_change(ch, 0);
		      mapID[ch] = XG_NORMAL_MAP;
		    }
		    break;
		  case 64: /* SFX voice */
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "(XG ch=%d SFX voice)",
			      ch);
		    midi_drumpart_change(ch, 0);
		    mapID[ch] = XG_SFX64_MAP;
		    break;
		  case 126: /* SFX kit */
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG, "(XG ch=%d SFX kit)", ch);
		    midi_drumpart_change(ch, 1);
		    mapID[ch] = XG_SFX126_MAP;
		    break;
		  case 127: /* Drum kit */
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			      "(XG ch=%d Drum kit)", ch);
		    midi_drumpart_change(ch, 1);
		    mapID[ch] = XG_DRUM_MAP;
		    break;
		  default:
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG,
			      "(XG: ch=%d Strange bank MSB %d)",
			      ch, bank_msb[ch]);
		    break;
		}
		newbank = bank_lsb[ch];
		break;

	      case GM2_SYSTEM_MODE:
		ctl->cmsg(CMSG_INFO, VERB_DEBUG, "(GM2 ch=%d)", ch);
		mapID[ch] = (!ISDRUMCHANNEL(ch) ? GM2_TONE_MAP : GM2_DRUM_MAP);
		newbank = bank_lsb[ch];
		break;

	      default:
		newbank = bank_msb[ch];
		break;
	    }

	    if(ISDRUMCHANNEL(ch))
		current_set[ch] = newprog;
	    else
	    {
		if(special_tonebank >= 0)
		    newbank = special_tonebank;
		if(current_program[ch] == SPECIAL_PROGRAM)
		    skip_this_event = 1;
		current_set[ch] = newbank;
	    }
	    current_program[ch] = newprog;
	    break;

	  case ME_NOTEON:
	    if(counting_time)
		counting_time = 1;
	    if(ISDRUMCHANNEL(ch))
	    {
		newbank = current_set[ch];
		newprog = meep->event.a;
		instrument_map(mapID[ch], &newbank, &newprog);

		if(!drumset[newbank]) /* Is this a defined drumset? */
		{
		    if(warn_drumset[newbank] == 0)
		    {
			ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
				  "Drum set %d is undefined", newbank);
			warn_drumset[newbank] = 1;
		    }
		    newbank = 0;
		}

		/* Mark this instrument to be loaded */
		if(!(drumset[newbank]->tone[newprog].instrument))
		    drumset[newbank]->tone[newprog].instrument =
			MAGIC_LOAD_INSTRUMENT;
	    }
	    else
	    {
		if(current_program[ch] == SPECIAL_PROGRAM)
		    break;
		newbank = current_set[ch];
		newprog = current_program[ch];
		instrument_map(mapID[ch], &newbank, &newprog);
		if(tonebank[newbank] == NULL)
		{
		    if(warn_tonebank[newbank] == 0)
		    {
			ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
				  "Tone bank %d is undefined", newbank);
			warn_tonebank[newbank] = 1;
		    }
		    newbank = 0;
		}

		/* Mark this instrument to be loaded */
		if(!(tonebank[newbank]->tone[newprog].instrument))
		    tonebank[newbank]->tone[newprog].instrument =
			MAGIC_LOAD_INSTRUMENT;
	    }
	    break;

	  case ME_TONE_BANK_MSB:
	    bank_msb[ch] = meep->event.a;
	    break;

	  case ME_TONE_BANK_LSB:
	    bank_lsb[ch] = meep->event.a;
	    break;

	  case ME_CHORUS_TEXT:
	  case ME_LYRIC:
	  case ME_MARKER:
	  case ME_INSERT_TEXT:
	  case ME_TEXT:
	  case ME_KARAOKE_LYRIC:
	    if((meep->event.a | meep->event.b) == 0)
		skip_this_event = 1;
	    else if(counting_time && ctl->trace_playing)
		counting_time = 1;
	    break;

	  case ME_GSLCD:
	    if (counting_time && ctl->trace_playing)
		counting_time = 1;
	    skip_this_event = !ctl->trace_playing;
	    break;

	  case ME_DRUMPART:
	    midi_drumpart_change(ch, meep->event.a);
	    break;

	  case ME_WRD:
	    if(readmidi_wrd_mode == WRD_TRACE_MIMPI)
	    {
		wrd_args[wrd_argc++] = meep->event.a | 256 * meep->event.b;
		if(ch != WRD_ARG)
		{
		    if(ch == WRD_MAG) {
			wrdt->apply(WRD_MAGPRELOAD, wrd_argc, wrd_args);
		    }
		    else if(ch == WRD_PLOAD)
			wrdt->apply(WRD_PHOPRELOAD, wrd_argc, wrd_args);
		    else if(ch == WRD_PATH)
			wrdt->apply(WRD_PATH, wrd_argc, wrd_args);
		    wrd_argc = 0;
		}
	    }
	    if(counting_time == 2 && readmidi_wrd_mode != WRD_TRACE_NOTHING)
		counting_time = 1;
	    break;

	  case ME_SHERRY:
	    if(counting_time == 2)
		counting_time = 1;
	    break;

	  case ME_NOTE_STEP:
	    if(counting_time == 2)
		skip_this_event = 1;
	    break;
        }

	/* Recompute time in samples*/
	if((dt = meep->event.time - at) && !counting_time)
	{
	    samples_to_do = sample_increment * dt;
	    sample_cum += sample_correction * dt;
	    if(sample_cum & 0xFFFF0000)
	    {
		samples_to_do += ((sample_cum >> 16) & 0xFFFF);
		sample_cum &= 0x0000FFFF;
	    }
	    st += samples_to_do;
	    if(st < 0)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "Overflow the sample counter");
		free(groomed_list);
		return NULL;
	    }
	}
	else if(counting_time == 1)
	    counting_time = 0;

	if(meep->event.type == ME_TEMPO)
	{
	    tempo = ch + meep->event.b * 256 + meep->event.a * 65536;
	    compute_sample_increment(tempo, divisions);
	}

	if(!skip_this_event)
	{
	    /* Add the event to the list */
	    *lp = meep->event;
	    lp->time = st;
	    lp++;
	    our_event_count++;
	}
	at = meep->event.time;
	meep = meep->next;
    }
    /* Add an End-of-Track event */
    lp->time = st;
    lp->type = ME_EOT;
    our_event_count++;
    free_midi_list();
    
    *eventsp = our_event_count;
    *samplesp = st;
    return groomed_list;
}

static int read_smf_file(struct timidity_file *tf)
{
    int32 len, divisions;
    int16 format, tracks, divisions_tmp;
    int i;

    if(current_file_info->file_type == IS_OTHER_FILE)
	current_file_info->file_type = IS_SMF_FILE;

    if(current_file_info->karaoke_title == NULL)
	karaoke_title_flag = 0;
    else
	karaoke_title_flag = 1;

    errno = 0;
    if(tf_read(&len, 4, 1, tf) != 1)
    {
	if(errno)
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s", current_filename,
		      strerror(errno));
	else
	    ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
		      "%s: Not a MIDI file!", current_filename);
	return 1;
    }
    len = BE_LONG(len);

    tf_read(&format, 2, 1, tf);
    tf_read(&tracks, 2, 1, tf);
    tf_read(&divisions_tmp, 2, 1, tf);
    format = BE_SHORT(format);
    tracks = BE_SHORT(tracks);
    divisions_tmp = BE_SHORT(divisions_tmp);

    if(divisions_tmp < 0)
    {
	/* SMPTE time -- totally untested. Got a MIDI file that uses this? */
	divisions=
	    (int32)(-(divisions_tmp / 256)) * (int32)(divisions_tmp & 0xFF);
    }
    else
	divisions = (int32)divisions_tmp;

    if(len > 6)
    {
	ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
		  "%s: MIDI file header size %ld bytes",
		  current_filename, len);
	skip(tf, len - 6); /* skip the excess */
    }
    if(format < 0 || format > 2)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "%s: Unknown MIDI file format %d", current_filename, format);
	return 1;
    }
    ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "Format: %d  Tracks: %d  Divisions: %d",
	      format, tracks, divisions);

    current_file_info->format = format;
    current_file_info->tracks = tracks;
    current_file_info->divisions = divisions;
    if(tf->url->url_tell != NULL)
	current_file_info->hdrsiz = (int16)tf_tell(tf);
    else
	current_file_info->hdrsiz = -1;

    switch(format)
    {
      case 0:
	if(read_smf_track(tf, 0, 1))
	{
	    if(ignore_midi_error)
		break;
	    return 1;
	}
	break;

      case 1:
	for(i = 0; i < tracks; i++)
	{
	    if(read_smf_track(tf, i, 1))
	    {
		if(ignore_midi_error)
		    break;
		return 1;
	    }
	}
	break;

      case 2: /* We simply play the tracks sequentially */
	for(i = 0; i < tracks; i++)
	{
	    if(read_smf_track(tf, i, 0))
	    {
		if(ignore_midi_error)
		    break;
		return 1;
	    }
	}
	break;
    }
    return 0;
}

void readmidi_read_init(void)
{
    int i;

	/* initialize effect status */
	for (i = 0; i < MAX_CHANNELS; i++)
		init_channel_layer(i);
	free_effect_buffers();
	init_reverb_status_gs();
	init_delay_status_gs();
	init_chorus_status_gs();
	init_eq_status_gs();
	init_insertion_effect_gs();
	init_multi_eq_xg();
	if (play_system_mode == XG_SYSTEM_MODE) {init_all_effect_xg();}
	init_userdrum();
	init_userinst();
	rhythm_part[0] = rhythm_part[1] = 9;
	for(i = 0; i < 6; i++) {drum_setup_xg[i] = 9;}

    /* Put a do-nothing event first in the list for easier processing */
    evlist = current_midi_point = alloc_midi_event();
    evlist->event.time = 0;
    evlist->event.type = ME_NONE;
    evlist->event.channel = 0;
    evlist->event.a = 0;
    evlist->event.b = 0;
    evlist->prev = NULL;
    evlist->next = NULL;
    readmidi_error_flag = 0;
    event_count = 1;

    if(string_event_table != NULL)
    {
	free(string_event_table[0]);
	free(string_event_table);
	string_event_table = NULL;
	string_event_table_size = 0;
    }
    init_string_table(&string_event_strtab);
    karaoke_format = 0;

    for(i = 0; i < 256; i++)
	default_channel_program[i] = -1;
    readmidi_wrd_mode = WRD_TRACE_NOTHING;
}

static void insert_note_steps(void)
{
	MidiEventList *e;
	int32 i, n, at, lasttime, meas, beat;
	uint8 num = 0, denom = 1, a, b;
	
	e = evlist;
	for (i = n = 0; i < event_count - 1 && n < 256 - 1; i++, e = e->next)
		if (e->event.type == ME_TIMESIG && e->event.channel == 0) {
			if (n == 0 && e->event.time > 0) {	/* 4/4 is default */
				SETMIDIEVENT(timesig[n], 0, ME_TIMESIG, 0, 4, 4);
				n++;
			}
			if (n > 0 && e->event.a == timesig[n - 1].a
					&& e->event.b == timesig[n - 1].b)
				continue;	/* unchanged */
			if (n > 0 && e->event.time == timesig[n - 1].time)
				n--;	/* overwrite previous timesig */
			timesig[n++] = e->event;
		}
	if (n == 0) {
		SETMIDIEVENT(timesig[n], 0, ME_TIMESIG, 0, 4, 4);
		n++;
	}
	timesig[n] = timesig[n - 1];
	timesig[n].time = 0x7fffffff;	/* stopper */
	lasttime = e->event.time;
	readmidi_set_track(0, 1);
	at = n = meas = beat = 0;
	while (at < lasttime && ! readmidi_error_flag) {
		if (at >= timesig[n].time) {
			if (beat != 0)
				meas++, beat = 0;
			num = timesig[n].a, denom = timesig[n].b, n++;
		}
		a = (meas + 1) & 0xff;
		b = (((meas + 1) >> 8) & 0x0f) + ((beat + 1) << 4);
		MIDIEVENT(at, ME_NOTE_STEP, 0, a, b);
		if (++beat == num)
			meas++, beat = 0;
		at += current_file_info->divisions * 4 / denom;
	}
}

MidiEvent *read_midi_file(struct timidity_file *tf, int32 *count, int32 *sp,
			  char *fn)
{
    char magic[4];
    MidiEvent *ev;
    int err, macbin_check, i;

    macbin_check = 1;
    current_file_info = get_midi_file_info(current_filename, 1);
    COPY_CHANNELMASK(drumchannels, current_file_info->drumchannels);
    COPY_CHANNELMASK(drumchannel_mask, current_file_info->drumchannel_mask);

    errno = 0;

/*
    if((mtype = get_module_type(fn)) > 0)
    {
	readmidi_read_init();
	if(!IS_URL_SEEK_SAFE(tf->url))
	    tf->url = url_cache_open(tf->url, 1);
	err = load_module_file(tf, mtype);
	if(!err)
	{
	    current_file_info->format = 0;
	    memset(&drumchannels, 0, sizeof(drumchannels));
	    goto grooming;
	}
	free_midi_list();

	if(err == 2)
	    return NULL;
	url_rewind(tf->url);
	url_cache_disable(tf->url);
    }
*/
#if MAX_CHANNELS > 16
    for(i = 16; i < MAX_CHANNELS; i++)
    {
	if(!IS_SET_CHANNELMASK(drumchannel_mask, i))
	{
	    if(IS_SET_CHANNELMASK(drumchannels, i & 0xF))
		SET_CHANNELMASK(drumchannels, i);
	    else
		UNSET_CHANNELMASK(drumchannels, i);
	}
    }
#endif

    if(opt_default_mid &&
       (current_file_info->mid == 0 || current_file_info->mid >= 0x7e))
	current_file_info->mid = opt_default_mid;

  retry_read:
    if(tf_read(magic, 1, 4, tf) != 4)
    {
	if(errno)
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s", current_filename,
		      strerror(errno));
	else
	    ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
		      "%s: Not a MIDI file!", current_filename);
	return NULL;
    }

    if(memcmp(magic, "MThd", 4) == 0)
    {
	readmidi_read_init();
	err = read_smf_file(tf);
    }
/*    else if(memcmp(magic, "RCM-", 4) == 0 || memcmp(magic, "COME", 4) == 0)
    {
	readmidi_read_init();
	err = read_rcp_file(tf, magic, fn);
    }
*/  else if (strncmp(magic, "RIFF", 4) == 0) {
       if (tf_read(magic, 1, 4, tf) == 4 &&
           tf_read(magic, 1, 4, tf) == 4 &&
           strncmp(magic, "RMID", 4) == 0 &&
           tf_read(magic, 1, 4, tf) == 4 &&
           strncmp(magic, "data", 4) == 0 &&
           tf_read(magic, 1, 4, tf) == 4) {
           goto retry_read;
       } else {
           err = 1;
           ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
                     "%s: Not a MIDI file!", current_filename);
       }
    }
/*    else if(memcmp(magic, "melo", 4) == 0)
    {
	readmidi_read_init();
	err = read_mfi_file(tf);
    }
*/    else
    {
	if(macbin_check && magic[0] == 0)
	{
	    /* Mac Binary */
	    macbin_check = 0;
	    skip(tf, 128 - 4);
	    goto retry_read;
	}
	else if(memcmp(magic, "RIFF", 4) == 0)
	{
	    /* RIFF MIDI file */
	    skip(tf, 20 - 4);
	    goto retry_read;
	}
	err = 1;
	ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
		  "%s: Not a MIDI file!", current_filename);
    }

    if(err)
    {
	free_midi_list();
	if(string_event_strtab.nstring > 0)
	    delete_string_table(&string_event_strtab);
	return NULL;
    }

    /* Read WRD file */
    if(!(play_mode->flag&PF_CAN_TRACE))
    {
	if(wrdt->start != NULL)
	    wrdt->start(WRD_TRACE_NOTHING);
	readmidi_wrd_mode = WRD_TRACE_NOTHING;
    }
/*    else if(wrdt->id != '-' && wrdt->opened)
    {
	readmidi_wrd_mode = import_wrd_file(fn);
	if(wrdt->start != NULL)
	    if(wrdt->start(readmidi_wrd_mode) == -1)
	    {
		// strip all WRD events
		MidiEventList *e;
		int32 i;
		for(i = 0, e = evlist; i < event_count; i++, e = e->next)
		    if (e->event.type == ME_WRD || e->event.type == ME_SHERRY)
			e->event.type = ME_NONE;
	    }
    }
*/    else
	readmidi_wrd_mode = WRD_TRACE_NOTHING;

    /* make lyric table */
    if(string_event_strtab.nstring > 0)
    {
	string_event_table_size = string_event_strtab.nstring;
	string_event_table = make_string_array(&string_event_strtab);
	if(string_event_table == NULL)
	{
	    delete_string_table(&string_event_strtab);
	    string_event_table_size = 0;
	}
    }

  grooming:
    insert_note_steps();
    ev = groom_list(current_file_info->divisions, count, sp);
    if(ev == NULL)
    {
	free_midi_list();
	if(string_event_strtab.nstring > 0)
	    delete_string_table(&string_event_strtab);
	return NULL;
    }
    current_file_info->samples = *sp;
    if(current_file_info->first_text == NULL)
	current_file_info->first_text = safe_strdup("");
    current_file_info->readflag = 1;
    return ev;
}


struct midi_file_info *new_midi_file_info(const char *filename)
{
    struct midi_file_info *p;
    p = (struct midi_file_info *)safe_malloc(sizeof(struct midi_file_info));

    /* Initialize default members */
    memset(p, 0, sizeof(struct midi_file_info));
    p->hdrsiz = -1;
    p->format = -1;
    p->tracks = -1;
    p->divisions = -1;
    p->time_sig_n = p->time_sig_d = -1;
    p->samples = -1;
    p->max_channel = -1;
    p->file_type = IS_OTHER_FILE;
    if(filename != NULL)
	p->filename = safe_strdup(filename);
    COPY_CHANNELMASK(p->drumchannels, default_drumchannels);
    COPY_CHANNELMASK(p->drumchannel_mask, default_drumchannel_mask);

    /* Append to midi_file_info */
    p->next = midi_file_info;
    midi_file_info = p;

    return p;
}

void free_all_midi_file_info(void)
{
  struct midi_file_info *info, *next;

  info = midi_file_info;
  while (info) {
    next = info->next;
    free(info->filename);
    if (info->seq_name)
      free(info->seq_name);
    if (info->karaoke_title != NULL && info->karaoke_title == info->first_text)
      free(info->karaoke_title);
    else {
      if (info->karaoke_title)
	free(info->karaoke_title);
      if (info->first_text)
	free(info->first_text);
      if (info->midi_data)
	free(info->midi_data);
      if (info->pcm_filename)
	free(info->pcm_filename); /* Note: this memory is freed in playmidi.c*/
    }
    free(info);
    info = next;
  }
  midi_file_info = NULL;
  current_file_info = NULL;
}

struct midi_file_info *get_midi_file_info(char *filename, int newp)
{
    struct midi_file_info *p;

    filename = url_expand_home_dir(filename);
    /* Linear search */
    for(p = midi_file_info; p; p = p->next)
	if(!strcmp(filename, p->filename))
	    return p;
    if(newp)
	return new_midi_file_info(filename);
    return NULL;
}

struct timidity_file *open_midi_file(char *fn,
				     int decompress, int noise_mode)
{
    struct midi_file_info *infop;
    struct timidity_file *tf;
#if defined(SMFCONV) && defined(__W32__)
    extern int opt_rcpcv_dll;
#endif

    infop = get_midi_file_info(fn, 0);
    if(infop == NULL || infop->midi_data == NULL)
	tf = open_file(fn, decompress, noise_mode);
    else
    {
	tf = open_with_mem(infop->midi_data, infop->midi_data_size,
			   noise_mode);
/*	if(infop->compressed)
	{
	    if((tf->url = url_inflate_open(tf->url, infop->midi_data_size, 1))
	       == NULL)
	    {
		close_file(tf);
		return NULL;
	    }
	}*/
    }

#if defined(SMFCONV) && defined(__W32__)
    /* smf convert */
    if(tf != NULL && opt_rcpcv_dll)
    {
	if(smfconv_w32(tf, fn))
	{
	    close_file(tf);
	    return NULL;
	}
    }
#endif

    return tf;
}

#ifndef NO_MIDI_CACHE
static long deflate_url_reader(char *buf, long size, void *user_val)
{
    return url_nread((URL)user_val, buf, size);
}

/*
 * URL data into deflated buffer.
 */
static void url_make_file_data(URL url, struct midi_file_info *infop)
{
    char buff[BUFSIZ];
    MemBuffer b;
    long n;
    DeflateHandler compressor;

    init_memb(&b);

    /* url => b */
    if((compressor = open_deflate_handler(deflate_url_reader, url,
					  ARC_DEFLATE_LEVEL)) == NULL)
	return;
    while((n = zip_deflate(compressor, buff, sizeof(buff))) > 0)
	push_memb(&b, buff, n);
    close_deflate_handler(compressor);
    infop->compressed = 1;

    /* b => mem */
    infop->midi_data_size = b.total_size;
    rewind_memb(&b);
    infop->midi_data = (void *)safe_malloc(infop->midi_data_size);
    read_memb(&b, infop->midi_data, infop->midi_data_size);
    delete_memb(&b);
}

static int check_need_cache(URL url, char *filename)
{
    int t1, t2;
    t1 = url_check_type(filename);
    t2 = url->type;
    return (t1 == URL_http_t || t1 == URL_ftp_t || t1 == URL_news_t)
	 && t2 != URL_arc_t;
}
#else
/*ARGSUSED*/
static void url_make_file_data(URL url, struct midi_file_info *infop)
{
}
/*ARGSUSED*/
static int check_need_cache(URL url, char *filename)
{
    return 0;
}
#endif /* NO_MIDI_CACHE */

int check_midi_file(char *filename)
{
    struct midi_file_info *p;
    struct timidity_file *tf;
    char tmp[4];
    int32 len;
    int16 format;
    int check_cache;

    if(filename == NULL)
    {
	if(current_file_info == NULL)
	    return -1;
	filename = current_file_info->filename;
    }

    p = get_midi_file_info(filename, 0);
    if(p != NULL)
	return p->format;
    p = get_midi_file_info(filename, 1);
/*
    if(get_module_type(filename) > 0)
    {
	p->format = 0;
	return 0;
    }
*/
    tf = open_file(filename, 1, OF_SILENT);
    if(tf == NULL)
	return -1;

/*
    check_cache = check_need_cache(tf->url, filename);
    if(check_cache)
    {
	if(!IS_URL_SEEK_SAFE(tf->url))
	{
	    if((tf->url = url_cache_open(tf->url, 1)) == NULL)
	    {
		close_file(tf);
		return -1;
	    }
	}
    }
*/
    /* Parse MIDI header */
    if(tf_read(tmp, 1, 4, tf) != 4)
    {
	close_file(tf);
	return -1;
    }

    if(tmp[0] == 0)
    {
	skip(tf, 128 - 4);
	if(tf_read(tmp, 1, 4, tf) != 4)
	{
	    close_file(tf);
	    return -1;
	}
    }

    if(strncmp(tmp, "RCM-", 4) == 0 ||
       strncmp(tmp, "COME", 4) == 0 ||
       strncmp(tmp, "RIFF", 4) == 0 ||
       strncmp(tmp, "melo", 4) == 0 ||
       strncmp(tmp, "M1", 2) == 0)
    {
	p->format = format = 1;
	goto end_of_header;
    }

    if(strncmp(tmp, "MThd", 4) != 0)
    {
	close_file(tf);
	return -1;
    }

    if(tf_read(&len, 4, 1, tf) != 1)
    {
	close_file(tf);
	return -1;
    }
    len = BE_LONG(len);

    tf_read(&format, 2, 1, tf);
    format = BE_SHORT(format);
    if(format < 0 || format > 2)
    {
	close_file(tf);
	return -1;
    }
    skip(tf, len - 2);

    p->format = format;
    p->hdrsiz = (int16)tf_tell(tf);

  end_of_header:
/*    if(check_cache)
    {
	url_rewind(tf->url);
	url_cache_disable(tf->url);
	url_make_file_data(tf->url, p);
    }
*/
    close_file(tf);
    return format;
}

static char *get_midi_title1(struct midi_file_info *p)
{
    char *s;

    if(p->format != 0 && p->format != 1)
	return NULL;

    if((s = p->seq_name) == NULL)
	if((s = p->karaoke_title) == NULL)
	    s = p->first_text;
    if(s != NULL)
    {
	int all_space, i;

	all_space = 1;
	for(i = 0; s[i]; i++)
	    if(s[i] != ' ')
	    {
		all_space = 0;
		break;
	    }
	if(all_space)
	    s = NULL;
    }
    return s;
}

char *get_midi_title(char *filename)
{
    struct midi_file_info *p;
    struct timidity_file *tf;
    char tmp[4];
    int32 len;
    int16 format, tracks, trk;
    int laststatus, check_cache;

    if(filename == NULL)
    {
	if(current_file_info == NULL)
	    return NULL;
	filename = current_file_info->filename;
    }

    p = get_midi_file_info(filename, 0);
    if(p == NULL)
	p = get_midi_file_info(filename, 1);
    else 
    {
	if(p->seq_name != NULL || p->first_text != NULL || p->format < 0)
	    return get_midi_title1(p);
    }

    tf = open_file(filename, 1, OF_SILENT);
    if(tf == NULL)
	return NULL;

/* MOD stuff
    mtype = get_module_type(filename);
    check_cache = check_need_cache(tf->url, filename);
    if(check_cache || mtype > 0)
    {
	if(!IS_URL_SEEK_SAFE(tf->url))
	{
	    if((tf->url = url_cache_open(tf->url, 1)) == NULL)
	    {
		close_file(tf);
		return NULL;
	    }
	}
    }

    if(mtype > 0)
    {
	char *title, *str;

	title = get_module_title(tf, mtype);
	if(title == NULL)
	{
	    // No title
	    p->seq_name = NULL;
	    p->format = 0;
	    goto end_of_parse;
	}

	len = (int32)strlen(title);
	len = SAFE_CONVERT_LENGTH(len);
	str = (char *)new_segment(&tmpbuffer, len);
	code_convert(title, str, len, NULL, NULL);
	p->seq_name = (char *)safe_strdup(str);
	reuse_mblock(&tmpbuffer);
	p->format = 0;
	free (title);
	goto end_of_parse;
    }
*/

    /* Parse MIDI header */
    if(tf_read(tmp, 1, 4, tf) != 4)
    {
	close_file(tf);
	return NULL;
    }

    if(tmp[0] == 0)
    {
	skip(tf, 128 - 4);
	if(tf_read(tmp, 1, 4, tf) != 4)
	{
	    close_file(tf);
	    return NULL;
	}
    }

    if(memcmp(tmp, "RCM-", 4) == 0 || memcmp(tmp, "COME", 4) == 0)
    {
	int i;
	char local[0x40 + 1];
	char *str;

	p->format = 1;
	skip(tf, 0x20 - 4);
	tf_read(local, 1, 0x40, tf);
	local[0x40]='\0';

	for(i = 0x40 - 1; i >= 0; i--)
	{
	    if(local[i] == 0x20)
		local[i] = '\0';
	    else if(local[i] != '\0')
		break;
	}

	i = SAFE_CONVERT_LENGTH(i + 1);
	str = (char *)new_segment(&tmpbuffer, i);
	code_convert(local, str, i, NULL, NULL);
	p->seq_name = (char *)safe_strdup(str);
	reuse_mblock(&tmpbuffer);
	p->format = 1;
	goto end_of_parse;
    }
/*    if(memcmp(tmp, "melo", 4) == 0)
    {
	int i;
	char *master, *converted;
	
	master = get_mfi_file_title(tf);
	if (master != NULL)
	{
	    i = SAFE_CONVERT_LENGTH(strlen(master) + 1);
	    converted = (char *)new_segment(&tmpbuffer, i);
	    code_convert(master, converted, i, NULL, NULL);
	    p->seq_name = (char *)safe_strdup(converted);
	    reuse_mblock(&tmpbuffer);
	}
	else
	{
	    p->seq_name = (char *)safe_malloc(1);
	    p->seq_name[0] = '\0';
	}
	p->format = 0;
	goto end_of_parse;
    }
*/
    if(strncmp(tmp, "M1", 2) == 0)
    {
	/* I don't know MPC file format */
	p->format = 1;
	goto end_of_parse;
    }

	  if(strncmp(tmp, "RIFF", 4) == 0)
	  {
	/* RIFF MIDI file */
	skip(tf, 20 - 4);
  if(tf_read(tmp, 1, 4, tf) != 4)
    {
	close_file(tf);
	return NULL;
    }
	  }

    if(strncmp(tmp, "MThd", 4) != 0)
    {
	close_file(tf);
	return NULL;
    }

    if(tf_read(&len, 4, 1, tf) != 1)
    {
	close_file(tf);
	return NULL;
    }

    len = BE_LONG(len);

    tf_read(&format, 2, 1, tf);
    tf_read(&tracks, 2, 1, tf);
    format = BE_SHORT(format);
    tracks = BE_SHORT(tracks);
    p->format = format;
    p->tracks = tracks;
    if(format < 0 || format > 2)
    {
	p->format = -1;
	close_file(tf);
	return NULL;
    }

    skip(tf, len - 4);
    p->hdrsiz = (int16)tf_tell(tf);

    if(format == 2)
	goto end_of_parse;

    if(tracks >= 3)
    {
	tracks = 3;
	karaoke_format = 0;
    }
    else
    {
	tracks = 1;
	karaoke_format = -1;
    }

    for(trk = 0; trk < tracks; trk++)
    {
	int32 next_pos, pos;

	if(trk >= 1 && karaoke_format == -1)
	    break;

	if((tf_read(tmp,1,4,tf) != 4) || (tf_read(&len,4,1,tf) != 1))
	    break;

	if(memcmp(tmp, "MTrk", 4))
	    break;

	next_pos = tf_tell(tf) + len;
	laststatus = -1;
	for(;;)
	{
	    int i, me, type;

	    /* skip Variable-length quantity */
	    do
	    {
		if((i = tf_getc(tf)) == EOF)
		    goto end_of_parse;
	    } while (i & 0x80);

	    if((me = tf_getc(tf)) == EOF)
		goto end_of_parse;

	    if(me == 0xF0 || me == 0xF7) /* SysEx */
	    {
		if((len = getvl(tf)) < 0)
		    goto end_of_parse;
		if((p->mid == 0 || p->mid >= 0x7e) && len > 0 && me == 0xF0)
		{
		    p->mid = tf_getc(tf);
		    len--;
		}
		skip(tf, len);
	    }
	    else if(me == 0xFF) /* Meta */
	    {
		type = tf_getc(tf);
		if((len = getvl(tf)) < 0)
		    goto end_of_parse;
		if((type == 1 || type == 3) && len > 0 &&
		   (trk == 0 || karaoke_format != -1))
		{
		    char *si, *so;
		    int s_maxlen = SAFE_CONVERT_LENGTH(len);

		    si = (char *)new_segment(&tmpbuffer, len + 1);
		    so = (char *)new_segment(&tmpbuffer, s_maxlen);

		    if(len != tf_read(si, 1, len, tf))
		    {
			reuse_mblock(&tmpbuffer);
			goto end_of_parse;
		    }

		    si[len]='\0';
		    code_convert(si, so, s_maxlen, NULL, NULL);
		    if(trk == 0 && type == 3)
		    {
		      if(p->seq_name == NULL) {
			char *name = safe_strdup(so);
			p->seq_name = safe_strdup(fix_string(name));
			free(name);
		      }
		      reuse_mblock(&tmpbuffer);
		      if(karaoke_format == -1)
			goto end_of_parse;
		    }
		    if(p->first_text == NULL) {
		      char *name;
		      name = safe_strdup(so);
		      p->first_text = safe_strdup(fix_string(name));
		      free(name);
		    }
		    if(karaoke_format != -1)
		    {
			if(trk == 1 && strncmp(si, "@KMIDI", 6) == 0)
			    karaoke_format = 1;
			else if(karaoke_format == 1 && trk == 2)
			    karaoke_format = 2;
		    }
		    if(type == 1 && karaoke_format == 2)
		    {
			if(strncmp(si, "@T", 2) == 0)
			    p->karaoke_title =
				add_karaoke_title(p->karaoke_title, si + 2);
			else if(si[0] == '\\')
			    goto end_of_parse;
		    }
		    reuse_mblock(&tmpbuffer);
		}
		else if(type == 0x2F)
		{
		    pos = tf_tell(tf);
		    if(pos < next_pos)
			tf_seek(tf, next_pos - pos, SEEK_CUR);
		    break; /* End of track */
		}
		else
		    skip(tf, len);
	    }
	    else /* MIDI event */
	    {
		/* skip MIDI event */
		karaoke_format = -1;
		if(trk != 0)
		    goto end_of_parse;

		if(me & 0x80) /* status byte */
		{
		    laststatus = (me >> 4) & 0x07;
		    if(laststatus != 7)
			tf_getc(tf);
		}

		switch(laststatus)
		{
		  case 0: case 1: case 2: case 3: case 6:
		    tf_getc(tf);
		    break;
		  case 7:
		    if(!(me & 0x80))
			break;
		    switch(me & 0x0F)
		    {
		      case 2:
			tf_getc(tf);
			tf_getc(tf);
			break;
		      case 3:
			tf_getc(tf);
			break;
		    }
		    break;
		}
	    }
	}
    }

  end_of_parse:
/*    if(check_cache)
    {
	url_rewind(tf->url);
	url_cache_disable(tf->url);
	url_make_file_data(tf->url, p);
    }
*/    close_file(tf);
    if(p->first_text == NULL)
	p->first_text = safe_strdup("");
    return get_midi_title1(p);
}

int midi_file_save_as(char *in_name, char *out_name)
{
    struct timidity_file *tf;
    FILE* ofp;
    char buff[BUFSIZ];
    long n;

    if(in_name == NULL)
    {
	if(current_file_info == NULL)
	    return 0;
	in_name = current_file_info->filename;
    }
    out_name = (char *)url_expand_home_dir(out_name);

    ctl->cmsg(CMSG_INFO, VERB_NORMAL, "Save as %s...", out_name);

    errno = 0;
    if((tf = open_midi_file(in_name, 1, 0)) == NULL)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "%s: %s", out_name,
		  errno ? strerror(errno) : "Can't save file");
	return -1;
    }

    errno = 0;
    if((ofp = fopen(out_name, "wb")) == NULL)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "%s: %s", out_name,
		  errno ? strerror(errno) : "Can't save file");
	close_file(tf);
	return -1;
    }

    while((n = tf_read(buff, 1, sizeof(buff), tf)) > 0)
	fwrite(buff, 1, n, ofp);
    ctl->cmsg(CMSG_INFO, VERB_NORMAL, "Save as %s...Done", out_name);

    fclose(ofp);
    close_file(tf);
    return 0;
}

char *event2string(int id)
{
    if(id == 0)
	return "";
#ifdef ABORT_AT_FATAL
    if(id >= string_event_table_size)
	abort();
#endif /* ABORT_AT_FATAL */
    if(string_event_table == NULL || id < 0 || id >= string_event_table_size)
	return NULL;
    return string_event_table[id];
}

/*! initialize Delay Effect (GS) */
void init_delay_status_gs(void)
{
	struct delay_status_gs_t *p = &delay_status_gs;
	p->type = 0;
	p->level = 0x40;
	p->level_center = 0x7F;
	p->level_left = 0;
	p->level_right = 0;
	p->time_c = 0x61;
	p->time_l = 0x01;
	p->time_r = 0x01;
	p->feedback = 0x50;
	p->pre_lpf = 0;
	recompute_delay_status_gs();
}

/*! recompute Delay Effect (GS) */
void recompute_delay_status_gs(void)
{
	struct delay_status_gs_t *p = &delay_status_gs;
	p->time_center = delay_time_center_table[p->time_c > 0x73 ? 0x73 : p->time_c];
	p->time_ratio_left = (double)p->time_l / 24;
	p->time_ratio_right = (double)p->time_r / 24;
	p->sample_c = p->time_center * play_mode->rate / 1000.0f;
	p->sample_l = p->sample_c * p->time_ratio_left;
	p->sample_r = p->sample_c * p->time_ratio_right;
	p->level_ratio_c = (double)p->level * (double)p->level_center / (127.0f * 127.0f);
	p->level_ratio_l = (double)p->level * (double)p->level_left / (127.0f * 127.0f);
	p->level_ratio_r = (double)p->level * (double)p->level_right / (127.0f * 127.0f);
	p->feedback_ratio = (double)(p->feedback - 64) * (0.763f * 2.0f / 100.0f);
	p->send_reverb_ratio = (double)p->send_reverb * (0.787f / 100.0f);

	if(p->level_left != 0 || (p->level_right != 0 && p->type == 0)) {
		p->type = 1;	/* it needs 3-tap delay effect. */
	}

	if(p->pre_lpf) {
		p->lpf.a = 2.0 * ((double)(7 - p->pre_lpf) / 7.0f * 16000.0f + 200.0f) / play_mode->rate;
		init_filter_lowpass1(&(p->lpf));
	}
}

/*! Delay Macro (GS) */
void set_delay_macro_gs(int macro)
{
	struct delay_status_gs_t *p = &delay_status_gs;
	if(macro >= 4) {p->type = 2;}	/* cross delay */
	macro *= 10;
	p->time_center = delay_time_center_table[delay_macro_presets[macro + 1]];
	p->time_ratio_left = (double)delay_macro_presets[macro + 2] / 24;
	p->time_ratio_right = (double)delay_macro_presets[macro + 3] / 24;
	p->level_center = delay_macro_presets[macro + 4];
	p->level_left = delay_macro_presets[macro + 5];
	p->level_right = delay_macro_presets[macro + 6];
	p->level = delay_macro_presets[macro + 7];
	p->feedback = delay_macro_presets[macro + 8];
}

/*! initialize Reverb Effect (GS) */
void init_reverb_status_gs(void)
{
	struct reverb_status_gs_t *p = &reverb_status_gs;
	p->character = 0x04;
	p->pre_lpf = 0;
	p->level = 0x40;
	p->time = 0x40;
	p->delay_feedback = 0;
	p->pre_delay_time = 0;
	recompute_reverb_status_gs();
	init_reverb();
}

/*! recompute Reverb Effect (GS) */
void recompute_reverb_status_gs(void)
{
	struct reverb_status_gs_t *p = &reverb_status_gs;

	if(p->pre_lpf) {
		p->lpf.a = 2.0 * ((double)(7 - p->pre_lpf) / 7.0f * 16000.0f + 200.0f) / play_mode->rate;
		init_filter_lowpass1(&(p->lpf));
	}
}

/*! Reverb Type (GM2) */
void set_reverb_macro_gm2(int macro)
{
	struct reverb_status_gs_t *p = &reverb_status_gs;
	int type = macro;
	if (macro == 8) {macro = 5;}
	macro *= 6;
	p->character = reverb_macro_presets[macro];
	p->pre_lpf = reverb_macro_presets[macro + 1];
	p->level = reverb_macro_presets[macro + 2];
	p->time = reverb_macro_presets[macro + 3];
	p->delay_feedback = reverb_macro_presets[macro + 4];
	p->pre_delay_time = reverb_macro_presets[macro + 5];

	switch(type) {	/* override GS macro's parameter */
	case 0:	/* Small Room */
		p->time = 44;
		break;
	case 1:	/* Medium Room */
	case 8:	/* Plate */
		p->time = 50;
		break;
	case 2:	/* Large Room */
		p->time = 56;
		break;
	case 3:	/* Medium Hall */
	case 4:	/* Large Hall */
		p->time = 64;
		break;
	}
}

/*! Reverb Macro (GS) */
void set_reverb_macro_gs(int macro)
{
	struct reverb_status_gs_t *p = &reverb_status_gs;
	macro *= 6;
	p->character = reverb_macro_presets[macro];
	p->pre_lpf = reverb_macro_presets[macro + 1];
	p->level = reverb_macro_presets[macro + 2];
	p->time = reverb_macro_presets[macro + 3];
	p->delay_feedback = reverb_macro_presets[macro + 4];
	p->pre_delay_time = reverb_macro_presets[macro + 5];
}

/*! initialize Chorus Effect (GS) */
void init_chorus_status_gs(void)
{
	struct chorus_status_gs_t *p = &chorus_status_gs;
	p->macro = 0;
	p->pre_lpf = 0;
	p->level = 0x40;
	p->feedback = 0x08;
	p->delay = 0x50;
	p->rate = 0x03;
	p->depth = 0x13;
	p->send_reverb = 0;
	p->send_delay = 0;
	recompute_chorus_status_gs();
}

/*! recompute Chorus Effect (GS) */
void recompute_chorus_status_gs()
{
	struct chorus_status_gs_t *p = &chorus_status_gs;

	if(p->pre_lpf) {
		p->lpf.a = 2.0 * ((double)(7 - p->pre_lpf) / 7.0f * 16000.0f + 200.0f) / play_mode->rate;
		init_filter_lowpass1(&(p->lpf));
	}
}

/*! Chorus Macro (GS), Chorus Type (GM2) */
void set_chorus_macro_gs(int macro)
{
	struct chorus_status_gs_t *p = &chorus_status_gs;
	macro *= 8;
	p->pre_lpf = chorus_macro_presets[macro];
	p->level = chorus_macro_presets[macro + 1];
	p->feedback = chorus_macro_presets[macro + 2];
	p->delay = chorus_macro_presets[macro + 3];
	p->rate = chorus_macro_presets[macro + 4];
	p->depth = chorus_macro_presets[macro + 5];
	p->send_reverb = chorus_macro_presets[macro + 6];
	p->send_delay = chorus_macro_presets[macro + 7];
}

/*! initialize EQ (GS) */
void init_eq_status_gs(void)
{
	struct eq_status_gs_t *p = &eq_status_gs;
	p->low_freq = 0;
	p->low_gain = 0x40;
	p->high_freq = 0;
	p->high_gain = 0x40;
	recompute_eq_status_gs();
}

/*! recompute EQ (GS) */
void recompute_eq_status_gs(void)
{
	double freq, dbGain;
	struct eq_status_gs_t *p = &eq_status_gs;

	/* Lowpass Shelving Filter */
	if(p->low_freq == 0) {freq = 200;}
	else {freq = 400;}
	dbGain = p->low_gain - 0x40;
	if(freq < play_mode->rate / 2) {
		p->lsf.q = 0;
		p->lsf.freq = freq;
		p->lsf.gain = dbGain;
		calc_filter_shelving_low(&(p->lsf));
	}

	/* Highpass Shelving Filter */
	if(p->high_freq == 0) {freq = 3000;}
	else {freq = 6000;}
	dbGain = p->high_gain - 0x40;
	if(freq < play_mode->rate / 2) {
		p->hsf.q = 0;
		p->hsf.freq = freq;
		p->hsf.gain = dbGain;
		calc_filter_shelving_high(&(p->hsf));
	}
}

/*! initialize Multi EQ (XG) */
void init_multi_eq_xg(void)
{
	multi_eq_xg.valid = 0;
	set_multi_eq_type_xg(0);
	recompute_multi_eq_xg();
}

/*! set Multi EQ type (XG) */
void set_multi_eq_type_xg(int type)
{
	struct multi_eq_xg_t *p = &multi_eq_xg;
	type *= 20;
	p->gain1 = multi_eq_block_table_xg[type];
	p->freq1 = multi_eq_block_table_xg[type + 1];
	p->q1 = multi_eq_block_table_xg[type + 2];
	p->shape1 = multi_eq_block_table_xg[type + 3];
	p->gain2 = multi_eq_block_table_xg[type + 4];
	p->freq2 = multi_eq_block_table_xg[type + 5];
	p->q2 = multi_eq_block_table_xg[type + 6];
	p->gain3 = multi_eq_block_table_xg[type + 8];
	p->freq3 = multi_eq_block_table_xg[type + 9];
	p->q3 = multi_eq_block_table_xg[type + 10];
	p->gain4 = multi_eq_block_table_xg[type + 12];
	p->freq4 = multi_eq_block_table_xg[type + 13];
	p->q4 = multi_eq_block_table_xg[type + 14];
	p->gain5 = multi_eq_block_table_xg[type + 16];
	p->freq5 = multi_eq_block_table_xg[type + 17];
	p->q5 = multi_eq_block_table_xg[type + 18];
	p->shape5 = multi_eq_block_table_xg[type + 19];
}

/*! recompute Multi EQ (XG) */
void recompute_multi_eq_xg(void)
{
	struct multi_eq_xg_t *p = &multi_eq_xg;

	if(p->freq1 != 0 && p->freq1 < 60 && p->gain1 != 0x40) {
		p->valid1 = 1;
		if(p->shape1) {	/* peaking */
			p->eq1p.q = (double)p->q1 / 10.0;
			p->eq1p.freq = eq_freq_table_xg[p->freq1];
			p->eq1p.gain = p->gain1 - 0x40;
			calc_filter_peaking(&(p->eq1p));
		} else {	/* shelving */
			p->eq1s.q = (double)p->q1 / 10.0;
			p->eq1s.freq = eq_freq_table_xg[p->freq1];
			p->eq1s.gain = p->gain1 - 0x40;
			calc_filter_shelving_low(&(p->eq1s));
		}
	} else {p->valid1 = 0;}
	if(p->freq2 != 0 && p->freq2 < 60 && p->gain2 != 0x40) {
		p->valid2 = 1;
		p->eq2p.q = (double)p->q2 / 10.0;
		p->eq2p.freq = eq_freq_table_xg[p->freq2];
		p->eq2p.gain = p->gain2 - 0x40;
		calc_filter_peaking(&(p->eq2p));
	} else {p->valid2 = 0;}
	if(p->freq3 != 0 && p->freq3 < 60 && p->gain3 != 0x40) {
		p->valid3 = 1;
		p->eq3p.q = (double)p->q3 / 10.0;
		p->eq4p.freq = eq_freq_table_xg[p->freq3];
		p->eq4p.gain = p->gain3 - 0x40;
		calc_filter_peaking(&(p->eq3p));
	} else {p->valid3 = 0;}
	if(p->freq4 != 0 && p->freq4 < 60 && p->gain4 != 0x40) {
		p->valid4 = 1;
		p->eq4p.q = (double)p->q4 / 10.0;
		p->eq4p.freq = eq_freq_table_xg[p->freq4];
		p->eq4p.gain = p->gain4 - 0x40;
		calc_filter_peaking(&(p->eq4p));
	} else {p->valid4 = 0;}
	if(p->freq5 != 0 && p->freq5 < 60 && p->gain5 != 0x40) {
		p->valid5 = 1;
		if(p->shape5) {	/* peaking */
			p->eq5p.q = (double)p->q5 / 10.0;
			p->eq5p.freq = eq_freq_table_xg[p->freq5];
			p->eq5p.gain = p->gain5 - 0x40;
			calc_filter_peaking(&(p->eq5p));
		} else {	/* shelving */
			p->eq5s.q = (double)p->q5 / 10.0;
			p->eq5s.freq = eq_freq_table_xg[p->freq5];
			p->eq5s.gain = p->gain5 - 0x40;
			calc_filter_shelving_high(&(p->eq5s));
		}
	} else {p->valid5 = 0;}
	p->valid = p->valid1 || p->valid2 || p->valid3 || p->valid4 || p->valid5;
}

/*! convert GS user drumset assign groups to internal "alternate assign". */
void recompute_userdrum_altassign(int bank, int group)
{
	int number = 0;
	char *params[131], param[10];
	ToneBank *bk;
	UserDrumset *p;
	
	for(p = userdrum_first; p != NULL; p = p->next) {
		if(p->assign_group == group) {
			sprintf(param, "%d", p->prog);
			params[number] = safe_strdup(param);
			number++;
		}
	}
	params[number] = NULL;

	alloc_instrument_bank(1, bank);
	bk = drumset[bank];
	bk->alt = add_altassign_string(bk->alt, params, number);
}

/*! initialize GS user drumset. */
void init_userdrum()
{
	int i;
	AlternateAssign *alt;

	free_userdrum();

	for(i=0;i<2;i++) {	/* allocate alternative assign */
		alt = (AlternateAssign *)safe_malloc(sizeof(AlternateAssign));
		memset(alt, 0, sizeof(AlternateAssign));
		alloc_instrument_bank(1, 64 + i);
		drumset[64 + i]->alt = alt;
	}
}

/*! recompute GS user drumset. */
void recompute_userdrum(int bank, int prog)
{
	UserDrumset *p;

	p = get_userdrum(bank, prog);

	free_tone_bank_element(&drumset[bank]->tone[prog]);
	if(drumset[p->source_prog]) {
		if(drumset[p->source_prog]->tone[p->source_note].name) {
			copy_tone_bank_element(&drumset[bank]->tone[prog], &drumset[p->source_prog]->tone[p->source_note]);
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"User Drumset (%d %d -> %d %d)", p->source_prog, p->source_note, bank, prog);
		} else if(drumset[0]->tone[p->source_note].name) {
			copy_tone_bank_element(&drumset[bank]->tone[prog], &drumset[0]->tone[p->source_note]);
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"User Drumset (%d %d -> %d %d)", 0, p->source_note, bank, prog);
		}
	}
}

/*! get pointer to requested GS user drumset.
   if it's not found, allocate a new item first. */
UserDrumset *get_userdrum(int bank, int prog)
{
	UserDrumset *p;

	for(p = userdrum_first; p != NULL; p = p->next) {
		if(p->bank == bank && p->prog == prog) {return p;}
	}

	p = (UserDrumset *)safe_malloc(sizeof(UserDrumset));
	memset(p, 0, sizeof(UserDrumset));
	p->next = NULL;
	if(userdrum_first == NULL) {
		userdrum_first = p;
		userdrum_last = p;
	} else {
		userdrum_last->next = p;
		userdrum_last = p;
	}
	p->bank = bank;
	p->prog = prog;

	return p;
}

/*! free GS user drumset. */
void free_userdrum()
{
	UserDrumset *p, *next;

	for(p = userdrum_first; p != NULL; p = next){
		next = p->next;
		free(p);
    }
	userdrum_first = userdrum_last = NULL;
}

/*! initialize GS user instrument. */
void init_userinst()
{
	free_userinst();
}

/*! recompute GS user instrument. */
void recompute_userinst(int bank, int prog)
{
	UserInstrument *p;

	p = get_userinst(bank, prog);

	free_tone_bank_element(&tonebank[bank]->tone[prog]);
	if(tonebank[p->source_bank]) {
		if(tonebank[p->source_bank]->tone[p->source_prog].name) {
			copy_tone_bank_element(&tonebank[bank]->tone[prog], &tonebank[p->source_bank]->tone[p->source_prog]);
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"User Instrument (%d %d -> %d %d)", p->source_bank, p->source_prog, bank, prog);
		} else if(tonebank[0]->tone[p->source_prog].name) {
			copy_tone_bank_element(&tonebank[bank]->tone[prog], &tonebank[0]->tone[p->source_prog]);
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"User Instrument (%d %d -> %d %d)", 0, p->source_prog, bank, prog);
		}
	}
}

/*! get pointer to requested GS user instrument.
   if it's not found, allocate a new item first. */
UserInstrument *get_userinst(int bank, int prog)
{
	UserInstrument *p;

	for(p = userinst_first; p != NULL; p = p->next) {
		if(p->bank == bank && p->prog == prog) {return p;}
	}

	p = (UserInstrument *)safe_malloc(sizeof(UserInstrument));
	memset(p, 0, sizeof(UserInstrument));
	p->next = NULL;
	if(userinst_first == NULL) {
		userinst_first = p;
		userinst_last = p;
	} else {
		userinst_last->next = p;
		userinst_last = p;
	}
	p->bank = bank;
	p->prog = prog;

	return p;
}

/*! free GS user instrument. */
void free_userinst()
{
	UserInstrument *p, *next;

	for(p = userinst_first; p != NULL; p = next){
		next = p->next;
		free(p);
    }
	userinst_first = userinst_last = NULL;
}

static void set_effect_param_xg(struct effect_xg_t *st, int type_msb, int type_lsb)
{
	int i, j;
	for (i = 0; effect_parameter_xg[i].type_msb != -1
		&& effect_parameter_xg[i].type_lsb != -1; i++) {
		if (type_msb == effect_parameter_xg[i].type_msb
			&& type_lsb == effect_parameter_xg[i].type_lsb) {
			for (j = 0; j < 16; j++) {
				st->param_lsb[j] = effect_parameter_xg[i].param_lsb[j];
			}
			for (j = 0; j < 10; j++) {
				st->param_msb[j] = effect_parameter_xg[i].param_msb[j];
			}
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "XG EFX: %s", effect_parameter_xg[i].name);
			return;
		}
	}
	if (type_msb != 0) {
		for (i = 0; effect_parameter_xg[i].type_msb != -1
			&& effect_parameter_xg[i].type_lsb != -1; i++) {
			if (type_lsb == effect_parameter_xg[i].type_lsb) {
				for (j = 0; j < 16; j++) {
					st->param_lsb[j] = effect_parameter_xg[i].param_lsb[j];
				}
				for (j = 0; j < 10; j++) {
					st->param_msb[j] = effect_parameter_xg[i].param_msb[j];
				}
				ctl->cmsg(CMSG_INFO, VERB_NOISY, "XG EFX: %s", effect_parameter_xg[i].name);
				return;
			}
		}
	}
}

/*! recompute XG effect parameters. */
void recompute_effect_xg(struct effect_xg_t *st)
{
	EffectList *efc = st->ef;

	if (efc == NULL) {return;}
	while (efc != NULL && efc->info != NULL)
	{
		(*efc->engine->conv_xg)(st, efc);
		(*efc->engine->do_effect)(NULL, MAGIC_INIT_EFFECT_INFO, efc);
		efc = efc->next_ef;
	}
}

void realloc_effect_xg(struct effect_xg_t *st)
{
	int type_msb = st->type_msb, type_lsb = st->type_lsb;

	free_effect_list(st->ef);
	st->ef = NULL;
	st->use_msb = 0;

	switch(type_msb) {
	case 0x05:
		st->use_msb = 1;
		st->ef = push_effect(st->ef, EFFECT_DELAY_LCR);
		st->ef = push_effect(st->ef, EFFECT_DELAY_EQ2);
		break;
	case 0x06:
		st->use_msb = 1;
		st->ef = push_effect(st->ef, EFFECT_DELAY_LR);
		st->ef = push_effect(st->ef, EFFECT_DELAY_EQ2);
		break;
	case 0x07:
		st->use_msb = 1;
		st->ef = push_effect(st->ef, EFFECT_ECHO);
		st->ef = push_effect(st->ef, EFFECT_DELAY_EQ2);
		break;
	case 0x08:
		st->use_msb = 1;
		st->ef = push_effect(st->ef, EFFECT_CROSS_DELAY);
		st->ef = push_effect(st->ef, EFFECT_DELAY_EQ2);
		break;
	case 0x41:
	case 0x42:
		st->ef = push_effect(st->ef, EFFECT_CHORUS);
		st->ef = push_effect(st->ef, EFFECT_CHORUS_EQ3);
		break;
	case 0x43:
		st->ef = push_effect(st->ef, EFFECT_FLANGER);
		st->ef = push_effect(st->ef, EFFECT_CHORUS_EQ3);
		break;
	case 0x44:
		st->ef = push_effect(st->ef, EFFECT_SYMPHONIC);
		st->ef = push_effect(st->ef, EFFECT_CHORUS_EQ3);
		break;
	case 0x49:
		st->ef = push_effect(st->ef, EFFECT_STEREO_DISTORTION);
		st->ef = push_effect(st->ef, EFFECT_OD_EQ3);
		break;
	case 0x4A:
		st->ef = push_effect(st->ef, EFFECT_STEREO_OVERDRIVE);
		st->ef = push_effect(st->ef, EFFECT_OD_EQ3);
		break;
	case 0x4B:
		st->ef = push_effect(st->ef, EFFECT_STEREO_AMP_SIMULATOR);
		break;
	case 0x4C:
		st->ef = push_effect(st->ef, EFFECT_EQ3);
		break;
	case 0x4D:
		st->ef = push_effect(st->ef, EFFECT_EQ2);
		break;
	case 0x4E:
		if (type_lsb == 0x01 || type_lsb == 0x02) {
			st->ef = push_effect(st->ef, EFFECT_XG_AUTO_WAH);
			st->ef = push_effect(st->ef, EFFECT_XG_AUTO_WAH_EQ2);
			st->ef = push_effect(st->ef, EFFECT_XG_AUTO_WAH_OD);
			st->ef = push_effect(st->ef, EFFECT_XG_AUTO_WAH_OD_EQ3);
		} else {
			st->ef = push_effect(st->ef, EFFECT_XG_AUTO_WAH);
			st->ef = push_effect(st->ef, EFFECT_XG_AUTO_WAH_EQ2);
		}
		break;
	case 0x5E:
		st->ef = push_effect(st->ef, EFFECT_LOFI);
		break;
	default:	/* Not Supported */
		type_msb = type_lsb = 0;
		break;
	}
	set_effect_param_xg(st, type_msb, type_lsb);
	recompute_effect_xg(st);
}

static void init_effect_xg(struct effect_xg_t *st)
{
	int i;

	free_effect_list(st->ef);
	st->ef = NULL;

	st->use_msb = 0;
	st->type_msb = st->type_lsb	= st->connection =
		st->send_reverb = st->send_chorus = 0;
	st->part = 0x7f;
	st->ret = st->pan = st->mw_depth = st->bend_depth =	st->cat_depth =
		st->ac1_depth = st->ac2_depth = st->cbc1_depth = st->cbc2_depth = 0x40;
	for (i = 0; i < 16; i++) {st->param_lsb[i] = 0;}
	for (i = 0; i < 10; i++) {st->param_msb[i] = 0;}
}

/*! initialize XG effect parameters */
static void init_all_effect_xg(void)
{
	int i;
 	init_effect_xg(&reverb_status_xg);
	reverb_status_xg.type_msb = 0x01;
	reverb_status_xg.connection = XG_CONN_SYSTEM_REVERB;
	realloc_effect_xg(&reverb_status_xg);
	init_effect_xg(&chorus_status_xg);
	chorus_status_xg.type_msb = 0x41;
	chorus_status_xg.connection = XG_CONN_SYSTEM_CHORUS;
	realloc_effect_xg(&chorus_status_xg);
	for (i = 0; i < XG_VARIATION_EFFECT_NUM; i++) {
		init_effect_xg(&variation_effect_xg[i]);
		variation_effect_xg[i].type_msb = 0x05;
		realloc_effect_xg(&variation_effect_xg[i]);
	}
	for (i = 0; i < XG_INSERTION_EFFECT_NUM; i++) {
		init_effect_xg(&insertion_effect_xg[i]);
		insertion_effect_xg[i].type_msb = 0x49;
		realloc_effect_xg(&insertion_effect_xg[i]);
	}
	init_ch_effect_xg();
}

/*! initialize GS insertion effect parameters */
void init_insertion_effect_gs(void)
{
	int i;
	struct insertion_effect_gs_t *st = &insertion_effect_gs;

	free_effect_list(st->ef);
	st->ef = NULL;

	for(i = 0; i < 20; i++) {st->parameter[i] = 0;}

	st->type = 0;
	st->type_lsb = 0;
	st->type_msb = 0;
	st->send_reverb = 0x28;
	st->send_chorus = 0;
	st->send_delay = 0;
	st->control_source1 = 0;
	st->control_depth1 = 0x40;
	st->control_source2 = 0;
	st->control_depth2 = 0x40;
	st->send_eq_switch = 0x01;
}

static void set_effect_param_gs(struct insertion_effect_gs_t *st, int msb, int lsb)
{
	int i, j;
	for (i = 0; effect_parameter_gs[i].type_msb != -1
		&& effect_parameter_gs[i].type_lsb != -1; i++) {
		if (msb == effect_parameter_gs[i].type_msb
			&& lsb == effect_parameter_gs[i].type_lsb) {
			for (j = 0; j < 20; j++) {
				st->parameter[j] = effect_parameter_gs[i].param[j];
			}
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "GS EFX: %s", effect_parameter_gs[i].name);
			break;
		}
	}
}

/*! recompute GS insertion effect parameters. */
void recompute_insertion_effect_gs(void)
{
	struct insertion_effect_gs_t *st = &insertion_effect_gs;
	EffectList *efc = st->ef;

	if (st->ef == NULL) {return;}
	while(efc != NULL && efc->info != NULL)
	{
		(*efc->engine->conv_gs)(st, efc);
		(*efc->engine->do_effect)(NULL, MAGIC_INIT_EFFECT_INFO, efc);
		efc = efc->next_ef;
	}
}

/*! re-allocate GS insertion effect parameters. */
void realloc_insertion_effect_gs(void)
{
	struct insertion_effect_gs_t *st = &insertion_effect_gs;
	int type_msb = st->type_msb, type_lsb = st->type_lsb;

	free_effect_list(st->ef);
	st->ef = NULL;

	switch(type_msb) {
	case 0x01:
		switch(type_lsb) {
		case 0x00: /* Stereo-EQ */
			st->ef = push_effect(st->ef, EFFECT_STEREO_EQ);
			break;
		case 0x10: /* Overdrive */
			st->ef = push_effect(st->ef, EFFECT_EQ2);
			st->ef = push_effect(st->ef, EFFECT_OVERDRIVE1);
			break;
		case 0x11: /* Distortion */
			st->ef = push_effect(st->ef, EFFECT_EQ2);
			st->ef = push_effect(st->ef, EFFECT_DISTORTION1);
			break;
		case 0x40: /* Hexa Chorus */
			st->ef = push_effect(st->ef, EFFECT_EQ2);
			st->ef = push_effect(st->ef, EFFECT_HEXA_CHORUS);
			break;
		case 0x72: /* Lo-Fi 1 */
			st->ef = push_effect(st->ef, EFFECT_EQ2);
			st->ef = push_effect(st->ef, EFFECT_LOFI1);
			break;
		case 0x73: /* Lo-Fi 2 */
			st->ef = push_effect(st->ef, EFFECT_EQ2);
			st->ef = push_effect(st->ef, EFFECT_LOFI2);
			break;
		default: break;
		}
		break;
	case 0x11:
		switch(type_lsb) {
		case 0x03: /* OD1 / OD2 */
			st->ef = push_effect(st->ef, EFFECT_OD1OD2);
			break;
		default: break;
		}
		break;
	default: break;
	}

	set_effect_param_gs(st, type_msb, type_lsb);

	recompute_insertion_effect_gs();
}

/*! initialize channel layers. */
void init_channel_layer(int ch)
{
	if (ch >= MAX_CHANNELS)
		return;
	CLEAR_CHANNELMASK(channel[ch].channel_layer);
	SET_CHANNELMASK(channel[ch].channel_layer, ch);
	channel[ch].port_select = ch >> 4;
}

/*! add a new layer. */
void add_channel_layer(int to_ch, int from_ch)
{
	if (to_ch >= MAX_CHANNELS || from_ch >= MAX_CHANNELS)
		return;
	/* add a channel layer */
	UNSET_CHANNELMASK(channel[to_ch].channel_layer, to_ch);
	SET_CHANNELMASK(channel[to_ch].channel_layer, from_ch);
	ctl->cmsg(CMSG_INFO, VERB_NOISY,
			"Channel Layer (CH:%d -> CH:%d)", from_ch, to_ch);
}

/*! remove all layers for this channel. */
void remove_channel_layer(int ch)
{
	int i, offset;
	
	if (ch >= MAX_CHANNELS)
		return;
	/* remove channel layers */
	offset = ch & ~0xf;
	for (i = offset; i < offset + REDUCE_CHANNELS; i++)
		UNSET_CHANNELMASK(channel[i].channel_layer, ch);
	SET_CHANNELMASK(channel[ch].channel_layer, ch);
}


void free_midi_file_data()
{
    if( string_event_table != NULL )
    {
		free(string_event_table[0]);
		free(string_event_table);
	}
}
