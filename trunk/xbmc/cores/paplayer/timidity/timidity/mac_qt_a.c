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

	Macintosh interface for TiMidity
	by T.Nogami	<t-nogami@happy.email.ne.jp>
	   K.KINOSHITA
		
    mac_qt_a.c
    Macintosh QuickTime audio driver
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

#include <Threads.h>
#include <QuickTimeComponents.h>

#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"
#include "controls.h"
#include "miditrace.h"
#include "wrd.h"

#include "mac_main.h"

static int open_output(void);
static void close_output(void);
static int output_data(char *buf, int32 count);
static int acntl(int request, void *arg);

static void play_event_prescan(void *);
static int wait_event_time(void *);
static void qt_play_event(void *);

static unsigned long	start_tic;
static NoteAllocator	gNoteAllocator;

#define dmp mac_quicktime_play_mode

PlayMode dmp =
{
	DEFAULT_RATE, PE_16BIT|PE_SIGNED, PF_MIDI_EVENT|PF_CAN_TRACE,
    -1,
    {0,0,0,0,0},
    "QuickTime MIDI mode", 'q',
    "-",
    open_output,
    close_output,
	output_data,
    acntl
};

static uint16 xg_sfx_voice[] = { // bank msb 64, lsb 0
	(1<<7)+120, // 0: cutting noise
	(1<<7)+120, // 1: cutting noise 2
	(1<<7)+120, // 2: distotion cutting noise
	(2<<7)+120, // 3: string slap
	(0<<7)+120, // 4: bass slide -> guitar fret noise
	5, // 5: pick scrape -> (no tone)
	6,7,8,9,10,11,12,13,14,15,
	(1<<7)+121, // 16: flute key click
	17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
	(1<<7)+122, // 32: rain
	(2<<7)+122, // 33: thunder
	(3<<7)+122, // 34: wind
	(4<<7)+122, // 35: stream
	(5<<7)+122, // 36: bubble
	37, // 37: feed -> (no tone)
	38,39,40,41,42,43,44,45,46,47,
	(1<<7)+123, // 48: dog
	(2<<7)+123, // 49: horse
	(3<<7)+123, // 50: bird 2
	51, // 51: kitty -> (no tone)
	52, // 52: growl -> (no tone)
	53, // 53: haunted -> (no tone)
	54, // 54: ghost -> (no tone)
	55, // 55: maou -> (no tone)
	56,57,58,59,60,61,62,63,
	(0<<7)+124, // 64: telephone dial
	(2<<7)+124, // 65: door squeek?
	(3<<7)+124, // 66: door slam
	(4<<7)+124, // 67: scratch
	(4<<7)+124, // 68: scratch 2
	(5<<7)+124, // 69: wind chime
	(1<<7)+124, // 70: telephone 2
	71,72,73,74,75,76,77,78,79,
	(1<<7)+125, // 80: car engine
	(2<<7)+125, // 81: car stop
	(3<<7)+125, // 82: car pass
	(4<<7)+125, // 83: car crash
	(5<<7)+125, // 84: siren
	(6<<7)+125, // 85: train
	(7<<7)+125, // 86: jetplane
	(8<<7)+125, // 87: starship
	(9<<7)+125, // 88: burst
	89, // 89: coaster
	90, // 90: submarine
	91,92,93,94,95,
	(1<<7)+126, // 96: laughing
	(2<<7)+126, // 97: scream
	(3<<7)+126, // 98: punch
	(4<<7)+126, // 99: heart
	(5<<7)+126, // 100: footstep
	(0<<7)+126, // 101: applause 2
	102,103,104,105,106,107,108,109,110,111,
	(1<<7)+127, // 112: machine gun
	(2<<7)+127, // 113: laser gun
	(3<<7)+127, // 114: explosion
	115 // 115: fire work
};

static uint16 sc88_drum_kit[] = {
	0, // 0: Standard 1 -> Standard
	0, // 1: Standard 2 -> Standard
	2,3,4,5,6,7,
	8, // 8: Room -> Room
	9,10,11,12,13,14,15,
	16, // 16: Power -> Power
	17,18,19,20,21,22,23,
	24, // 24: Electronic -> Electronic
	25, // 25: TR-808 -> TR-808
	24, // 26: Dance -> Electronic
	27,28,29,30,31,
	32, // 32: Jazz -> Jazz
	33,34,35,36,37,38,39,
	40, // 40: Brush -> Brush
	41,42,43,44,45,46,47,
	48, // 48: Orchestra -> Orchestra
	49, // 49: Ethnic ->
	50, // Kick & Snare ->
	51,52,53,54,55,
	56, // 56: SFX -> SFX
	57 // 57: Rhythm FX ->
};
static uint16 sc88pro_drum_kit[] = {
	0, // 0: Standard 1 -> Standard
	0, // 1: Standard 2 -> Standard
	0, // 2: Standard 3 -> Standard
	3,4,5,6,7,
	8, // 8: Room -> Room
	25, // 9: Hip-Hop -> TR-808
	25, // 10: Jungle -> TR-808
	25, // 11: Techno -> TR-808
	12,13,14,15,
	16, // 16: Power -> Power
	17,18,19,20,21,22,23,
	24, // 24: Electronic -> Electronic
	25, // 25: TR-808 -> TR-808
	24, // 26: Dance -> Electronic
	0, // 27: CR-78 -> Standard
	25, // 28: TR-606 -> TR-808
	25, // 29: TR-707 -> TR-808
	25, // 30: TR-909 -> TR-808
	31,
	32, // 32: Jazz -> Jazz
	33,34,35,36,37,38,39,
	40, // 40: Brush -> Brush
	41,42,43,44,45,46,47,
	48, // 48: Orchestra -> Orchestra
	49, // 49: Ethnic ->
	50, // 50: Kick & Snare ->
	51,
	52, // 52: Asia ->
	53, // 53: Cymbal & Claps ->
	54,55,
	56, // 56: SFX -> SFX
	57, // 57: Rhythm FX ->
	58 // 58: Rhythm FX 2 ->
};

#define	XG_DRUM_VOICE_FIRST_NOTE	13
#define	XG_DRUM_VOICE_LAST_NOTE		34
static uint8 xg_drum_voice[][2] = { // bank msb 127, lsb 0, prog 0
	{0,86}, // 13: surdo mute
	{0,87}, // 14: surdo open
	{0,27}, // 15: hi q
	{0,28}, // 16: whip slap
	{0,29}, // 17: scratch push
	{0,30}, // 18: scratch pull
	{0,26}, // 19: finger snap (no tone)
	{56,51}, // 20: click noise
	{0,33}, // 21: metronome click
	{0,34}, // 22: metronome bell
	{0,32}, // 23: seq click l
	{0,32}, // 24: seq click h
	{40,38}, // 25: brush tap
	{40,40}, // 26: brush swirl l
	{40,39}, // 27: brush slap
	{40,40}, // 28: brush swirl h
	{0,25}, // 29: snare roll
	{0,85}, // 30: castanet
	{0,38}, // 31: snare l
	{0,31}, // 32: sticks
	{0,36}, // 33: bass drum l
	{0,38} // 34: open rim shot
};

#define	MAX_NOTE_CHANNELS	64

static NoteChannel	note_channel[MAX_NOTE_CHANNELS];
static uint16 bend_sense[MAX_CHANNELS], master_tune[MAX_CHANNELS];
static uint8 rpn_addr[MAX_CHANNELS], nrpn_lsb[MAX_CHANNELS], nrpn_msb[MAX_CHANNELS];
static long instrument_number[MAX_CHANNELS];
static char instrument_name[MAX_CHANNELS][32];
static Boolean drum_part[MAX_CHANNELS], rpn_flag, prescan = 0;
static Boolean			qt5orlater;

static void init_variable(void)
{
	start_tic = TickCount();
}

static int open_output(void)
{
	int	i;
	long response;

	qt5orlater = Gestalt(gestaltQuickTimeVersion, &response) == noErr && response >= 0x05000000;
	// open the note allocator component
	gNoteAllocator = OpenDefaultComponent(kNoteAllocatorComponentType, 0);
	if(gNoteAllocator == NULL){
		ctl->cmsg(CMSG_INFO,VERB_VERBOSE,"open_output fail:");
		close_output();
		return -1;
	}else{
		ctl->cmsg(CMSG_INFO,VERB_VERBOSE,"open_output success:");
	}
	dmp.fd = 1; //normaly opened flag
	for(i = 0; i < MAX_CHANNELS; i++)
		bend_sense[i] = 0x100;
	return 0;
}

static int output_data(char *buf, int32 count)
{
 	// Never called
 	return 0;
}

static long current_tick(void)
{
	return TickCount() - start_tic;
}

static int32 current_samples(void)
{
    return (current_tick()*play_mode->rate + 30)/60;
}

static void close_output(void)
{
	if(dmp.fd == -1) return;
	if(gNoteAllocator != NULL)
		CloseComponent(gNoteAllocator);
	dmp.fd = -1;	//disabled
}

static void qt_ctl_note_event(int channel, int note, int velocity)
{
	CtlEvent ce;
	ce.type = CTLE_NOTE;
	ce.v1 = (velocity ? VOICE_ON : VOICE_FREE);
	ce.v2 = channel;
	ce.v3 = note;
	ce.v4 = velocity;
	if(ctl->trace_playing && !midi_trace.flush_flag)
		push_midi_trace_ce(ctl->event, &ce);
	else
		ctl->event(&ce);
}

static void ctl_timestamp(void)
{
	long secs;
	CtlEvent ce;
	static int last_secs = -1;

	secs = current_tick() / 60;
	if(secs == last_secs)
		return;
	ce.type = CTLE_CURRENT_TIME;
	ce.v1 = last_secs = secs;
	if(ctl->trace_playing && !midi_trace.flush_flag)
		push_midi_trace_ce(ctl->event, &ce);
	else
		ctl->event(&ce);
}

static void ctl_prog_event(int channel, int program)
{
	CtlEvent ce;
	ce.type = CTLE_PROGRAM;
	ce.v1 = channel;
	ce.v2 = program;
	ce.v3 = (long)instrument_name[channel];
	if(ctl->trace_playing && !midi_trace.flush_flag)
		push_midi_trace_ce(ctl->event, &ce);
	else
		ctl->event(&ce);
}

static int xg_instrument_number(int ch, int program)
{
	if(channel[ch].bank_msb == 64)
		return kFirstGSInstrument + xg_sfx_voice[program];
	switch(channel[ch].bank_lsb){
	case 8:
		switch(program){
		case 40: return kFirstGSInstrument + (8<<7) + 40;	// Slow Violin
		case 80: return kFirstGSInstrument + (1<<7) + 80;	// Square
		case 81: return kFirstGSInstrument + (1<<7) + 81;	// Saw
		case 102: return kFirstGSInstrument + (2<<7) + 102;	// Echo Pan
		}
		break;
	case 14:
		switch(program){
		case 102: return kFirstGSInstrument + (2<<7) + 102;	// Echo Pan
		}
		break;
	case 16:
		switch(program){
		case 24: return kFirstGSInstrument + (32<<7) + 24;	// Nylon Gt.2
		case 52: return kFirstGSInstrument + (32<<7) + 52;	// Choir Aahs 2
		}
		break;
	case 18:
		switch(program){
		case 0: return kFirstGSInstrument + (16<<7) + 0;	// Piano 1d
		case 57: return kFirstGSInstrument + (1<<7) + 57;	// Trombone 2
		}
		break;
	case 25:
		switch(program){
		case 6: return kFirstGSInstrument + (24<<7) + 6;	// Coupled hps.
		case 24: return kFirstGSInstrument + (16<<7) + 24;	// Nylon Gt.3
		}
		break;
	case 27:
		switch(program){
		case 62: return kFirstGSInstrument + (8<<7) + 62;	// Synth Brass 3
		}
		break;
	case 32:
		switch(program){
		case 2: return kFirstGSInstrument + (8<<7) + 4;		// Detuned EP1
		case 16: return kFirstGSInstrument + (8<<7) + 16;	// Detuned Or.1
		case 17: return kFirstGSInstrument + (8<<7) + 17;	// Detuned Or.2
		case 19: return kFirstGSInstrument + (16<<7) + 19;	// Church Or.2
		case 21: return kFirstGSInstrument + (8<<7) + 21;	// Accordion It.
		case 27: return kFirstGSInstrument + (8<<7) + 27;	// Chorus Gt.
		case 60: return kFirstGSInstrument + (1<<7) + 60;	// French horn 2
		}
		break;
	case 33:
		switch(program){
		case 16: return kFirstGSInstrument + (16<<7) + 16;	// 60's Organ 1
		}
		break;
	case 34:
		switch(program){
		case 16: return kFirstGSInstrument + (16<<7) + 16;	// 60's Organ 1
		}
		break;
	case 35:
		switch(program){
		case 6: return kFirstGSInstrument + (8<<7) + 6;		// Coupled hps.
		case 19: return kFirstGSInstrument + (8<<7) + 19;	// Church Or.2
		case 25: return kFirstGSInstrument + (8<<7) + 25;	// 12-str Gt.
		case 50: return kFirstGSInstrument + (8<<7) + 50;	// Syn.String 3
		case 104: return kFirstGSInstrument + (1<<7) + 104;	// Sitar 2
		}
		break;
	case 37:
		switch(program){
		case 16: return kFirstGSInstrument + (16<<7) + 16;	// 60's Organ 1
		}
		break;
	case 40:
		switch(program){
		case 28: return kFirstGSInstrument + (8<<7) + 28;	// Funk Gt.
		case 30: return kFirstGSInstrument + (8<<7) + 30;	// DistortionGt
		case 38: return kFirstGSInstrument + (1<<7) + 38;	// SynthBass101
		case 48: return kFirstGSInstrument + (8<<7) + 48;	// Orchestra
		case 61: return kFirstGSInstrument + (8<<7) + 61;	// Brass 2
		case 63: return kFirstGSInstrument + (8<<7) + 63;	// Synth Brass4
		}
		break;
	case 41:
		switch(program){
		case 30: return kFirstGSInstrument + (8<<7) + 30;	// Feedback Gt.
		case 48: return kFirstGSInstrument + (8<<7) + 48;	// Orchestra
		case 81: return kFirstGSInstrument + (8<<7) + 81;	// Doctor Solo
		}
		break;
	case 43:
		switch(program){
		case 28: return kFirstGSInstrument + (16<<7) + 28;	// Funk Gt.2
		}
		break;
	case 64:
		switch(program){
		case 4: return kFirstGSInstrument + (24<<7) + 4;	// 60's E.Piano
		case 62: return kFirstGSInstrument + (16<<7) + 62;	// AnalogBrass1
		case 63: return kFirstGSInstrument + (16<<7) + 63;	// AnalogBrass2
		case 98: return kFirstGSInstrument + (1<<7) + 98;	// Syn Mallet
		case 102: return kFirstGSInstrument + (1<<7) + 102;	// Echo Bell
		case 117: return kFirstGSInstrument + (8<<7) + 117;	// Melo.Tom 2
		case 118: return kFirstGSInstrument + (8<<7) + 118;	// 808 Tom
		}
		break;
	case 65:
		switch(program){
		case 31: return kFirstGSInstrument + (8<<7) + 31;	// Gt.Feedback
		case 80: return kFirstGSInstrument + (8<<7) + 80;	// Sine Wave
		case 117: return kFirstGSInstrument + (8<<7) + 117;	// Melo.Tom 2
		case 118: return kFirstGSInstrument + (9<<7) + 118;	// Elec Prec
		}
		break;
	case 66:
		switch(program){
		case 38: return kFirstGSInstrument + (16<<7) + 39;	// Rubber Bass
		case 80: return kFirstGSInstrument + (8<<7) + 80;	// Sine Wave
		case 117: return kFirstGSInstrument + (8<<7) + 117;	// Melo.Tom 2
		}
		break;
	case 96:
		switch(program){
		case 14: return kFirstGSInstrument + (8<<7) + 14;	// Church Bell
		case 24: return kFirstGSInstrument + (8<<7) + 24;	// Ukulele
		case 25: return kFirstGSInstrument + (16<<7) + 25;	// Mandolin
		case 107: return kFirstGSInstrument + (8<<7) + 107;	// Taisho Koto
		case 115: return kFirstGSInstrument + (8<<7) + 115;	// Castanets
		}
		break;
	case 97:
		switch(program){
		case 14: return kFirstGSInstrument + (9<<7) + 14;	// Carillon
		}
		break;
	}
	return kFirstGMInstrument + program;
}

static void set_instrument(MidiEvent *ev)
{
	long instrumentNumber;
	int ch = ev->channel;

	channel[ch].program = ev->a;
	if(drum_part[ch]){
		if(play_system_mode == GS_SYSTEM_MODE && channel[ch].bank_lsb == 2)				// SC-88 Map
			instrumentNumber = kFirstDrumkit + sc88_drum_kit[ev->a] + 1;
		else if(play_system_mode == GS_SYSTEM_MODE && channel[ch].bank_lsb == 3)		// SC-88Pro Map
			instrumentNumber = kFirstDrumkit + sc88pro_drum_kit[ev->a] + 1;
		else
			instrumentNumber = kFirstDrumkit + ev->a + 1;
	}
	else {
		if(play_system_mode == GS_SYSTEM_MODE)
			instrumentNumber = kFirstGSInstrument + (channel[ch].bank_msb<<7) + ev->a;
		else if(play_system_mode == XG_SYSTEM_MODE)
			instrumentNumber = xg_instrument_number(ch, ev->a);
		else
			instrumentNumber = kFirstGMInstrument + ev->a;
	}
	if(instrument_number[ch] != instrumentNumber){
		NoteRequest	nr;
		long index,  part;
		OSType synthType;
		Str31 name;
		SynthesizerConnections connections;
		MusicComponent mc;
                char tmp[256];  /* enough */

		instrument_number[ch] = instrumentNumber;
		if(note_channel[ch] != NULL)
			NADisposeNoteChannel(gNoteAllocator, note_channel[ch]);
		nr.info.flags = 0;
		if (qt5orlater && ch <= 16)
			nr.info.midiChannelAssignment = kNoteRequestSpecifyMIDIChannel | ch;
		else
			nr.info.midiChannelAssignment = 0;	// see note QuickTimeMusic.h.
		*(short *)(&nr.info.polyphony) = EndianS16_NtoB(8);			// 8 voices poliphonic
		*(Fixed *)(&nr.info.typicalPolyphony) = EndianU32_NtoB(0x00010000);
		NAStuffToneDescription(gNoteAllocator, instrumentNumber, &nr.tone);
		NANewNoteChannel(gNoteAllocator, &nr, &note_channel[ch]);
		NAGetNoteChannelInfo(gNoteAllocator, note_channel[ch], &index, &part);
		NAGetRegisteredMusicDevice(gNoteAllocator, index, &synthType, name, &connections, &mc);
		MusicGetPartName(mc, part, name);
		p2cstrcpy(tmp, name);
                strncpy(instrument_name[ch], tmp, sizeof(instrument_name[ch]));
	}
	ctl_prog_event(ch, ev->a);
}

static void play_event_prescan(void *p)
{
	MidiEvent *ev = (MidiEvent *)p;
	int ch;

	for(ch = 0; ch < MAX_CHANNELS; ch++){
		if(note_channel[ch] != NULL){
			NADisposeNoteChannel(gNoteAllocator, note_channel[ch]);
			note_channel[ch] = NULL;
		}
		instrument_number[ch] = -1;
		instrument_name[ch][0] = '\0';
		drum_part[ch] = false;
		channel[ch].bank_lsb = 0;
		channel[ch].bank_msb = 0;
	}
	drum_part[9] = true;
		for(;; ev++){
			ch = ev->channel;
		if(ev->type == ME_NOTEON && note_channel[ch] == NULL){
			//MidiEvent *ev;

			ev->channel = ch;
			ev->a = 0;
			set_instrument(ev);
		}
		else if(ev->type == ME_PROGRAM){
			set_instrument(ev);
		}
		else if(ev->type == ME_TONE_BANK_LSB && ev->a != channel[ch].bank_lsb){
			channel[ch].bank_lsb = ev->a;
		}
		else if(ev->type == ME_TONE_BANK_MSB && ev->a != channel[ch].bank_msb){
			channel[ch].bank_msb = ev->a;
			if(play_system_mode == XG_SYSTEM_MODE && (ev->a == 126 || ev->a == 127) && !drum_part[ch])
				drum_part[ch] = true;
		}
		else if(ev->type == ME_DRUMPART && !drum_part[ch]){
			//MidiEvent *ev;

			ev->channel = ch;
			ev->a = channel[ch].program;
			set_instrument(ev);
			drum_part[ch] = true;
			}
			else if(ev->type == ME_RESET){
				play_system_mode = ev->a;
			}
			else if(ev->type == ME_EOT){
				prescan = 1;
				for(ch = 0; ch < MAX_CHANNELS; ch++){
				channel[ch].bank_lsb = 0;
				channel[ch].bank_msb = 0;
			}
			init_variable();
			break;
		}
	}
}

static int wait_event_time(void *p)
{
	MidiEvent *ev = (MidiEvent *)p;
	int rc, ch;

	for(;;){
		if(ev->time - (current_tick()*play_mode->rate+30)/60 < 0)
			break;
		trace_loop();
		YieldToAnyThread();
		rc = check_apply_control();
		if(RC_IS_SKIP_FILE(rc)){
			prescan = 0;
			for(ch = 0; ch < MAX_CHANNELS; ch++){
				if(note_channel[ch] != NULL){
					NADisposeNoteChannel(gNoteAllocator, note_channel[ch]);
					note_channel[ch] = NULL;
				}
				channel[ch].bank_lsb = 0;
				channel[ch].bank_msb = 0;
			}
			trace_flush();
			return rc;
		}
	}
	return RC_NONE;
}

static void qt_play_event(void *p)
{
	MidiEvent *ev = (MidiEvent *)p;
	int ch, i;

	ch = ev->channel;
	switch(ev->type)
	{
	case ME_NOTEON:
		if(play_system_mode == XG_SYSTEM_MODE && channel[ch].bank_msb == 127
			&& ev->a >= XG_DRUM_VOICE_FIRST_NOTE && ev->a <= XG_DRUM_VOICE_LAST_NOTE){
			int a = ev->a - XG_DRUM_VOICE_FIRST_NOTE;
			a = xg_drum_voice[a][1];
			NAPlayNote(gNoteAllocator, note_channel[ch], a, ev->b);
			qt_ctl_note_event(ch, ev->a, ev->b);
		}
		else if(play_system_mode == XG_SYSTEM_MODE && channel[ch].bank_msb == 126){
			NAPlayNote(gNoteAllocator, note_channel[ch], ev->a, ev->b);
			qt_ctl_note_event(ch, ev->a, ev->b);
		}
		else {
			NAPlayNote(gNoteAllocator, note_channel[ch], ev->a, ev->b);
			qt_ctl_note_event(ch, ev->a, ev->b);
		}
		break;
	case ME_NOTEOFF:
		if(play_system_mode == XG_SYSTEM_MODE && channel[ch].bank_msb == 127
			&& ev->a >= XG_DRUM_VOICE_FIRST_NOTE && ev->a <= XG_DRUM_VOICE_LAST_NOTE){
			int a = ev->a - XG_DRUM_VOICE_FIRST_NOTE;
			a = xg_drum_voice[a][1];
			NAPlayNote(gNoteAllocator, note_channel[ch], a, 0);
			qt_ctl_note_event(ch, ev->a, 0);
		}
		else if(play_system_mode == XG_SYSTEM_MODE && channel[ch].bank_msb == 126){
			NAPlayNote(gNoteAllocator, note_channel[ch], ev->a, 0);
			qt_ctl_note_event(ch, ev->a, 0);
		}
		else {
			NAPlayNote(gNoteAllocator, note_channel[ch], ev->a, 0);
			qt_ctl_note_event(ch, ev->a, 0);
		}
		break;
	case ME_PROGRAM:
		set_instrument(ev);
		break;
	/* MIDI Events */
	case ME_KEYPRESSURE:
	case ME_CHANNEL_PRESSURE:
		NASetController(gNoteAllocator, note_channel[ch], kControllerAfterTouch, ev->a<<8);
		break;
	case ME_PITCHWHEEL:
		// kControllerPitchBend 256/halfnote
		i = (ev->b<<7) + ev->a - 0x2000;
		i *= (bend_sense[ch]<<1);
		i += (i > 0 ? (1<<12) : -(1<<12));
		i >>= 13;
		NASetController(gNoteAllocator, note_channel[ch], kControllerPitchBend, i);
		break;
	/* Controls */
	case ME_TONE_BANK_LSB:
		channel[ch].bank_lsb = ev->a;
		break;
	case ME_TONE_BANK_MSB:
		channel[ch].bank_msb = ev->a;
		if(play_system_mode == XG_SYSTEM_MODE && (ev->a == 126 || ev->a == 127) && !drum_part[ch])
			drum_part[ch] = true;
		break;
	case ME_MODULATION_WHEEL:
		NASetController(gNoteAllocator, note_channel[ch], kControllerModulationWheel, ev->a);
		break;
	case ME_BREATH:
		NASetController(gNoteAllocator, note_channel[ch], kControllerBreath, ev->a<<8);
		break;
	case ME_FOOT:
		NASetController(gNoteAllocator, note_channel[ch], kControllerFoot, ev->a<<8);
		break;
	case ME_BALANCE:
		NASetController(gNoteAllocator, note_channel[ch], kControllerBalance, ev->a<<8);
		break;
	case ME_MAINVOLUME:
		NASetController(gNoteAllocator, note_channel[ch], kControllerVolume, ev->a<<8);
		break;
	case ME_PAN:
		// kControllerPan 256-512
		NASetController(gNoteAllocator, note_channel[ch], kControllerPan, (ev->a<<1) + 256);
		ctl_mode_event(CTLE_PANNING, 1, ch, ev->a);
		break;
	case ME_EXPRESSION:
		NASetController(gNoteAllocator, note_channel[ch], kControllerExpression, ev->a<<8);
		ctl_mode_event(CTLE_EXPRESSION, 1, ch, ev->a);
		break;
	case ME_SUSTAIN:
		// kControllerSustain on/off only
		NASetController(gNoteAllocator, note_channel[ch], kControllerSustain, ev->a<<8);
		ctl_mode_event(CTLE_SUSTAIN, 1, ch, ev->a);
		break;
	case ME_PORTAMENTO_TIME_MSB:
	    channel[ch].portamento_time_msb = ev->a;
		NASetController(gNoteAllocator, note_channel[ch], kControllerPortamentoTime,
			(channel[ch].portamento_time_msb + (channel[ch].portamento_time_lsb<<7))<<1);
		break;
	case ME_PORTAMENTO_TIME_LSB:
	    channel[ch].portamento_time_lsb = ev->a;
		NASetController(gNoteAllocator, note_channel[ch], kControllerPortamentoTime,
			(channel[ch].portamento_time_msb + (channel[ch].portamento_time_lsb<<7))<<1);
		break;
	case ME_PORTAMENTO:
		NASetController(gNoteAllocator, note_channel[ch], kControllerPortamento, ev->a<<8);
		break;
	case ME_DATA_ENTRY_MSB:
		if(rpn_flag){
			if(rpn_addr[ch] == 0)			// pitchbend sensitivity
				bend_sense[ch] = (ev->a<<7) + ev->b;
			else if(rpn_addr[ch] == 1){		// master tuning (fine)
				master_tune[ch] |= ev->a;
				NASetController(gNoteAllocator, note_channel[ch], kControllerMasterTune, master_tune[ch]);
			}
			else if(rpn_addr[ch] == 2){		// master tuning (coarse)
				master_tune[ch] |= (ev->a<<7);
				NASetController(gNoteAllocator, note_channel[ch], kControllerMasterTune, master_tune[ch]);
			}
		}
		else {
		}
		break;
	case ME_REVERB_EFFECT:
		NASetController(gNoteAllocator, note_channel[ch], kControllerReverb, ev->a<<8);
	    channel[ch].reverb_level = ev->a;
	    ctl_mode_event(CTLE_REVERB_EFFECT, 1, ch, ev->a);
		break;
	case ME_TREMOLO_EFFECT:
		NASetController(gNoteAllocator, note_channel[ch], kControllerTremolo, ev->a<<8);
		break;
	case ME_CHORUS_EFFECT:
		NASetController(gNoteAllocator, note_channel[ch], kControllerChorus, ev->a<<8);
	    channel[ch].chorus_level = ev->a;
	    ctl_mode_event(CTLE_CHORUS_EFFECT, 1, ch, ev->a);
		break;
	case ME_CELESTE_EFFECT:
		NASetController(gNoteAllocator, note_channel[ch], kControllerCeleste, ev->a<<8);
		break;
	case ME_PHASER_EFFECT:
		NASetController(gNoteAllocator, note_channel[ch], kControllerPhaser, ev->a<<8);
		break;
	case ME_RPN_INC:
		rpn_flag = 1;
		rpn_addr[ch]++;
		break;
	case ME_RPN_DEC:
		rpn_flag = 1;
		rpn_addr[ch]--;
		break;
	case ME_NRPN_LSB:
		rpn_flag = 0;
		nrpn_lsb[ch] = ev->a;
		break;
	case ME_NRPN_MSB:
		rpn_flag = 0;
		nrpn_msb[ch] = ev->a;
		break;
	case ME_RPN_LSB:
		rpn_flag = 1;
		rpn_addr[ch] = ev->a;
		break;
	case ME_RPN_MSB:
		break;
	case ME_ALL_SOUNDS_OFF:
		for(i = 0; i < 128; i++)
			NAPlayNote(gNoteAllocator, note_channel[ch], i, 0);
		break;
	case ME_RESET_CONTROLLERS:
		NAResetNoteChannel(gNoteAllocator, note_channel[ch]);
		ctl_mode_event(CTLE_VOLUME, 1, ch, 0);
		ctl_mode_event(CTLE_EXPRESSION, 1, ch, 127);
		ctl_mode_event(CTLE_SUSTAIN, 1, ch, 0);
		ctl_mode_event(CTLE_MOD_WHEEL, 1, ch, 0);
		ctl_mode_event(CTLE_PITCH_BEND, 1, ch, 0x2000);
		ctl_prog_event(ch, channel[ch].program);
		ctl_mode_event(CTLE_CHORUS_EFFECT, 1, ch, 0);
		ctl_mode_event(CTLE_REVERB_EFFECT, 1, ch, 0);
		break;
	case ME_ALL_NOTES_OFF:
		for(i = 0; i < 128; i++)
			NAPlayNote(gNoteAllocator, note_channel[ch], i, 0);
		break;
	case ME_MONO:
		break;
	case ME_POLY:
		break;
	case ME_SOSTENUTO:
		NASetController(gNoteAllocator, note_channel[ch], kControllerSostenuto, ev->a);
		break;
	case ME_SOFT_PEDAL:
		NASetController(gNoteAllocator, note_channel[ch], kControllerSoftPedal, ev->a);
		break;
	/* TiMidity Extensionals */
	case ME_RANDOM_PAN:
		break;
	case ME_SET_PATCH:
		break;
	case ME_TEMPO:
		break;
	case ME_CHORUS_TEXT:
	case ME_LYRIC:
	case ME_MARKER:
	case ME_INSERT_TEXT:
	case ME_TEXT:
	case ME_KARAOKE_LYRIC:
		i = ev->a | ((int)ev->b << 8);
		ctl_mode_event(CTLE_LYRIC, 1, i, 0);
		break;
	case ME_MASTER_VOLUME:
		NASetController(gNoteAllocator, NULL, kControllerMasterVolume, ev->a + (ev->b<<7));
		break;
	case ME_PATCH_OFFS:
		break;
	case ME_RESET:
		play_system_mode = ev->a;
		break;
	case ME_WRD:
		push_midi_trace2(wrd_midi_event, ch, ev->a | (ev->b<<8));
		break;
	case ME_DRUMPART:
		if(!drum_part[ch]){
			//MidiEvent *ev;

			ev->channel = ch;
			ev->a = channel[ch].program;
			set_instrument(ev);
			drum_part[ch] = true;
		}
		break;
	case ME_KEYSHIFT:
		NASetController(gNoteAllocator, NULL, kControllerMasterTune, ev->a);
		break;
	case ME_NOTE_STEP:
		break;
	case ME_EOT:
		prescan = 0;
		break;
	}
	if(ev->type != ME_EOT)
		ctl_timestamp();
}

static int acntl(int request, void *arg)
{
 	int rc, ch;

    switch(request)
    {
      case PM_REQ_MIDI:
		if(!prescan)
			play_event_prescan(arg);
		rc = wait_event_time(arg);
		if(RC_IS_SKIP_FILE(rc))
			return rc;
		qt_play_event(arg);
		return RC_NONE;
	case PM_REQ_INST_NAME:
		ch = (int)*(char **)arg;
		*(char **)arg = instrument_name[ch];
		return 0;
      case PM_REQ_GETSAMPLES:
	*(int32 *)arg = current_samples();
	return 0;
	case PM_REQ_PLAY_START:
		init_variable();
      case PM_REQ_DISCARD:
      case PM_REQ_FLUSH:
		for(ch = 0; ch < MAX_CHANNELS; ch++){
			if(note_channel[ch] != NULL){
				NADisposeNoteChannel(gNoteAllocator, note_channel[ch]);
				note_channel[ch] = NULL;
			}
			channel[ch].bank_lsb = 0;
			channel[ch].bank_msb = 0;
		}
		trace_flush();
	return 0;
    }
    return -1;
}
