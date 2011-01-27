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

    rtsyn_common.c
        Copyright (c) 2003  Keishi Suenaga <s_keishi@mutt.freemail.ne.jp>

    I referenced following sources.
        alsaseq_c.c - ALSA sequencer server interface
            Copyright (c) 2000  Takashi Iwai <tiwai@suse.de>
        readmidi.c
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "interface.h"

#include <stdio.h>

#include <stdarg.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#endif
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <math.h>
#include <signal.h>

#include "server_defs.h"

#ifdef __W32__
#include <windows.h>
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

#include "rtsyn.h"


//int seq_quit;

int rtsyn_system_mode=DEFAULT_SYSTEM_MODE;

static int32 event_time_offset;
static double starttime;
double rtsyn_reachtime;
static int time_advance;
static int set_time_first=2;

//acitive sensing
static int active_sensing_flag=0;
static double active_sensing_time=0;

//timer interrupt
#ifdef USE_WINSYN_TIMER_I
rtsyn_mutex_t timerMUTEX;

#ifdef __W32__
MMRESULT timerID;
#else 
pthread_t timer_thread;
int thread_on_f=0;
#endif

#endif

#define EX_RESET_NO 7
static char sysex_resets[EX_RESET_NO][11]={
		'\xf0','\x7e','\x7f','\x09','\x00','\xf7','\x00','\x00','\x00','\x00','\x00',
		'\xf0','\x7e','\x7f','\x09','\x01','\xf7','\x00','\x00','\x00','\x00','\x00',
		'\xf0','\x7e','\x7f','\x09','\x03','\xf7','\x00','\x00','\x00','\x00','\x00',
		'\xf0','\x41','\x10','\x42','\x12','\x40','\x00','\x7f','\x00','\x41','\xf7',
		'\xf0','\x41','\x10','\x42','\x12','\x00','\x00','\x7f','\x00','\x01','\xf7',
		'\xf0','\x41','\x10','\x42','\x12','\x00','\x00','\x7f','\x01','\x00','\xf7',
		'\xf0','\x43','\x10','\x4c','\x00','\x00','\x7E','\x00','\xf7','\x00','\x00' };
/*
#define EX_RESET_NO 9
static char sysex_resets[EX_RESET_NO][11]={
	'\xf0','\x7e','\x7f','\x09','\x00','\xf7','\x00','\x00','\x00','\x00','\x00', //gm off
	'\xf0','\x7e','\x7f','\x09','\x01','\xf7','\x00','\x00','\x00','\x00','\x00', //gm1
	'\xf0','\x7e','\x7f','\x09','\x02','\xf7','\x00','\x00','\x00','\x00','\x00', //gm off
	'\xf0','\x7e','\x7f','\x09','\x03','\xf7','\x00','\x00','\x00','\x00','\x00', //gm2
	'\xf0','\x41','\x10','\x42','\x12','\x40','\x00','\x7f','\x00','\x41','\xf7', //GS
	'\xf0','\x41','\x10','\x42','\x12','\x40','\x00','\x7f','\x7f','\x41','\xf7', //GS off
	'\xf0','\x41','\x10','\x42','\x12','\x00','\x00','\x7f','\x00','\x01','\xf7', //88
	'\xf0','\x41','\x10','\x42','\x12','\x00','\x00','\x7f','\x01','\x00','\xf7', //88
	'\xf0','\x43','\x10','\x4c','\x00','\x00','\x7E','\x00','\xf7','\x00','\x00'  //XG on
	};
*/

static void seq_set_time(MidiEvent *ev);



void rtsyn_gm_reset(){
	MidiEvent ev;

	rtsyn_server_reset();
	ev.type=ME_RESET;
	ev.a=GM_SYSTEM_MODE;
	rtsyn_play_event(&ev);

}


void rtsyn_gs_reset(){
	MidiEvent ev;

	rtsyn_server_reset();
	ev.type=ME_RESET;
	ev.a=GS_SYSTEM_MODE;
	rtsyn_play_event(&ev);
}


void rtsyn_xg_reset(){
	MidiEvent ev;

	rtsyn_server_reset();
	ev.type=ME_RESET;
	ev.a=XG_SYSTEM_MODE;
	ev.time=0;
	rtsyn_play_event(&ev);
}


void rtsyn_normal_reset(){
	MidiEvent ev;

	rtsyn_server_reset();
	ev.type=ME_RESET;
	ev.a=rtsyn_system_mode;
	rtsyn_play_event(&ev);
}
void rtsyn_gm_modeset(){
	MidiEvent ev;

	rtsyn_server_reset();
	rtsyn_system_mode=GM_SYSTEM_MODE;
	ev.type=ME_RESET;
	ev.a=GM_SYSTEM_MODE;
	rtsyn_play_event(&ev);
	change_system_mode(rtsyn_system_mode);
}


void rtsyn_gs_modeset(){
	MidiEvent ev;

	rtsyn_server_reset();
	rtsyn_system_mode=GS_SYSTEM_MODE;
	ev.type=ME_RESET;
	ev.a=GS_SYSTEM_MODE;
	rtsyn_play_event(&ev);
	change_system_mode(rtsyn_system_mode);
}


void rtsyn_xg_modeset(){
	MidiEvent ev;

	rtsyn_server_reset();
	rtsyn_system_mode=XG_SYSTEM_MODE;
	ev.type=ME_RESET;
	ev.a=XG_SYSTEM_MODE;
	rtsyn_play_event(&ev);
	change_system_mode(rtsyn_system_mode);
}


void rtsyn_normal_modeset(){
	MidiEvent ev;

	rtsyn_server_reset();
	rtsyn_system_mode=DEFAULT_SYSTEM_MODE;
	ev.type=ME_RESET;
	ev.a=GS_SYSTEM_MODE;
	rtsyn_play_event(&ev);
	change_system_mode(rtsyn_system_mode);
}



#ifdef USE_WINSYN_TIMER_I
#ifdef __W32__
VOID CALLBACK timercalc(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dummy1, DWORD dummy2){
	MidiEvent ev;
	double time_div, currenttime;
	
	rtsyn_mutex_lock(timerMUTEX);
	currenttime=get_current_calender_time();
	time_div=currenttime-starttime;
	if( time_div > 1.0/TICKTIME_HZ*2.0){
		time_div= 1.0/TICKTIME_HZ-3/(play_mode->rate);
	}
	ev.time= ((double)current_sample
			+ (play_mode->rate)*time_div+0.5);
	starttime=currenttime;
	ev.type = ME_NONE;
	play_event(&ev);
//	compute_data(tmdy_struct,(tmdy_struct->output->play_mode->rate)*time_div);
	aq_fill_nonblocking();
	rtsyn_reachtime=currenttime +  (double)(1.0f/TICKTIME_HZ);
	rtsyn_mutex_unlock(timerMUTEX);
	return;
}
#else
void *timercalc(void *arg){
 	MidiEvent ev;
	unsigned int slt;
	double reachtime,delay;
	delay=(double)(1.0/TICKTIME_HZ);
	while(thread_on_f==1){		
		rtsyn_mutex_lock(timerMUTEX);
		reachtime=get_current_calender_time()+delay;
		ev.type = ME_NONE;
		seq_set_time(&ev);
		play_event(&ev);
		aq_fill_nonblocking();
		rtsyn_mutex_unlock(timerMUTEX);
		do{
			sleep(0);
		}while(get_current_calender_time()<reachtime);
	}
	return NULL;
}
#endif
#endif
void rtsyn_init(void){
	int i,j;
		/* set constants */
	opt_realtime_playing = 1; /* Enable loading patch while playing */
	allocate_cache_size = 0; /* Don't use pre-calclated samples */
	auto_reduce_polyphony = 0;
	current_keysig = (opt_init_keysig == 8) ? 0 : opt_init_keysig;
	note_key_offset = key_adjust;
	time_advance=play_mode->rate/TICKTIME_HZ*2;
	if (!(play_mode->encoding & PE_MONO))
		time_advance >>= 1;
	if (play_mode->encoding & PE_24BIT)
		time_advance /= 3;
	else if (play_mode->encoding & PE_16BIT)
		time_advance >>= 1;

	i = current_keysig + ((current_keysig < 8) ? 7 : -9), j = 0;
	while (i != 7)
		i += (i < 7) ? 5 : -7, j++;
	j += note_key_offset, j -= floor(j / 12.0) * 12;
	current_freq_table = j;
	
#ifdef USE_WINSYN_TIMER_I

	rtsyn_mutex_init(timerMUTEX);
#ifdef __W32__
	timeBeginPeriod(1);
	{
		DWORD data = 0;
		UINT delay;
		delay=(1000/TICKTIME_HZ);
		 
		
		timerID = timeSetEvent( delay, 0, timercalc, data,
			TIME_PERIODIC | TIME_CALLBACK_FUNCTION );
        if( !timerID ){
        	ctl->cmsg(  CMSG_ERROR, VERB_NORMAL,"Fail to setup Timer Interrupt (winsyn) \n");
        }
	}
#else
	thread_on_f=1;
	if(0!=pthread_create(&timer_thread,NULL,timercalc,NULL)){
        	ctl->cmsg(  CMSG_ERROR, VERB_NORMAL,"Fail to setup Timer Interrupt (winsyn) \n");
	}
#endif

#endif
	rtsyn_server_reset();
}

void rtsyn_close(void){
#ifdef USE_WINSYN_TIMER_I
#ifdef __W32__
	timeKillEvent( timerID );
	timeEndPeriod(1);
#else
	thread_on_f=0;
	pthread_join(timer_thread, NULL);
#endif
	rtsyn_mutex_destroy(timerMUTEX);
#endif 
}

void rtsyn_play_event(MidiEvent *ev)
{
  int gch;
  int32 cet;
#ifdef USE_WINSYN_TIMER_I
	rtsyn_mutex_lock(timerMUTEX);
#endif 

	gch = GLOBAL_CHANNEL_EVENT_TYPE(ev->type);
	if(gch || !IS_SET_CHANNELMASK(quietchannels, ev->channel) ){
//    if ( !seq_quit ) {
			ev->time=0;
			play_event(ev);
//		}
	}
#ifdef USE_WINSYN_TIMER_I
	rtsyn_mutex_unlock(timerMUTEX);
#endif 

}

void rtsyn_reset(void){
		rtsyn_stop_playing();
#ifdef USE_WINSYN_TIMER_I
	        rtsyn_mutex_lock(timerMUTEX);
#endif

		free_instruments(0);        //also in rtsyn_server_reset
		free_global_mblock();
#ifdef USE_WINSYN_TIMER_I
	        rtsyn_mutex_unlock(timerMUTEX);
#endif

		rtsyn_server_reset();
//		printf("system reseted\n");
}

void rtsyn_server_reset(void){
#ifdef USE_WINSYN_TIMER_I
	rtsyn_mutex_lock(timerMUTEX);
#endif 
	play_mode->close_output();	// PM_REQ_PLAY_START wlll called in playmidi_stream_init()
	play_mode->open_output();	// but w32_a.c does not have it.
	readmidi_read_init();
	playmidi_stream_init();
	starttime=get_current_calender_time();
	reduce_voice_threshold = 0; // * Disable auto reduction voice *
	auto_reduce_polyphony = 0;
	event_time_offset = 0;
#ifdef USE_WINSYN_TIMER_I
	rtsyn_mutex_unlock(timerMUTEX);
#endif 
}

void rtsyn_stop_playing(void)
{
	if(upper_voices) {
		MidiEvent ev;
		ev.type = ME_EOT;
		ev.a = 0;
		ev.b = 0;
		rtsyn_play_event(&ev);
#ifdef USE_WINSYN_TIMER_I
	        rtsyn_mutex_lock(timerMUTEX);
#endif
		aq_flush(1);
#ifdef USE_WINSYN_TIMER_I
		        rtsyn_mutex_unlock(timerMUTEX);
#endif
	}
}
extern int32 current_sample;
extern FLOAT_T midi_time_ratio;
extern int volatile stream_max_compute;


static void seq_set_time(MidiEvent *ev)
{
	double currenttime, time_div;
	
	currenttime=get_current_calender_time();
	time_div=currenttime-starttime;
	ev->time=((double) current_sample
			+ (play_mode->rate)*time_div+0.5);
	starttime=currenttime;
	
	rtsyn_reachtime=currenttime +  (double)(1.0f/TICKTIME_HZ);
}

#if 0
static void seq_set_time(MidiEvent *ev)
{
	double past_time,btime;
	static int shift=0;

	if(set_time_first==2){
		starttime=get_current_calender_time()-(double)current_sample/(double)play_mode->rate;
	}
	past_time = (int32)((get_current_calender_time() - starttime)*play_mode->rate);	
//	printf("%f,%f\n",(double)past_time,(  (double)current_sample-(double)past_time )  );
	if (set_time_first==1){
		shift=(double)past_time-(double)current_sample;
///		printf("%d\n",shift);
	}
	if (set_time_first>0) set_time_first--;
	event_time_offset=play_mode->rate/TICKTIME_HZ;
	ev->time = past_time;
	if(set_time_first==0 && (past_time-current_sample>stream_max_compute*play_mode->rate/1000)){ 
		starttime=get_current_calender_time()-(double)(current_sample+shift)/(double)play_mode->rate;
		ev->time=current_sample+shift;
	}
	ev->time += (int32)event_time_offset;

	
	rtsyn_reachtime=get_current_calender_time()+  (double)(1.0f/TICKTIME_HZ);
	
#if 0
	btime = (double)((ev->time-current_sample/midi_time_ratio)/play_mode->rate);
	btime *= 1.01; /* to be sure */
	aq_set_soft_queue(btime, 0.0);
#endif
}
#endif

void rtsyn_play_calculate(){
	MidiEvent ev;

#ifndef USE_WINSYN_TIMER_I	
	ev.type = ME_NONE;
	seq_set_time(&ev);
	play_event(&ev);
	aq_fill_nonblocking();
#endif
	
	if(active_sensing_flag==~0 && (get_current_calender_time() > active_sensing_time+0.5)){
//normaly acitive sensing expiering time is 330ms(>300ms) but this loop is heavy
		play_mode->close_output();
		play_mode->open_output();
		ctl->cmsg(  CMSG_ERROR, VERB_NORMAL,"Active Sensing Expired\n");
		active_sensing_flag=0;
	}
}
	
int rtsyn_play_one_data (int port, int32 dwParam1){
	MidiEvent ev;

	ev.type = ME_NONE;
	ev.channel = dwParam1 & 0x0000000f;
	ev.channel = ev.channel+port*16;
	ev.a = (dwParam1 >> 8) & 0xff;
	ev.b = (dwParam1 >> 16) & 0xff;
	switch ((int) (dwParam1 & 0x000000f0)) {
	case 0x80:
		ev.type = ME_NOTEOFF;
//		rtsyn_play_event(&ev);
		break;
	case 0x90:
		ev.type = (ev.b) ? ME_NOTEON : ME_NOTEOFF;
//		rtsyn_play_event(&ev);
		break;
	case 0xa0:
		ev.type = ME_KEYPRESSURE;
//		rtsyn_play_event(&ev);
		break;
	case 0xb0:
		if (! convert_midi_control_change(ev.channel, ev.a, ev.b, &ev))
		ev.type = ME_NONE;
		break;
	case 0xc0:
		ev.type = ME_PROGRAM;
//		rtsyn_play_event(&ev);
		break;
	case 0xd0:
		ev.type = ME_CHANNEL_PRESSURE;
//		rtsyn_play_event(&ev);
		break;
	case 0xe0:
		ev.type = ME_PITCHWHEEL;
//		rtsyn_play_event(&ev);
		break;
	case 0xf0:
#ifdef IA_PORTMIDISYN
		if ( (dwParam1 & 0x000000ff) == 0xf0) {
			//SysEx
			return 1;
		}
#endif
		if ((dwParam1 & 0x000000ff) == 0xf2) {
			ev.type = ME_PROGRAM;
//			rtsyn_play_event(&ev);
		}
#if 0
		if ((dwParam1 & 0x000000ff) == 0xf1)
			//MIDI Time Code Qtr. Frame (not need)
			printf("MIDI Time Code Qtr\n");
		if ((dwParam1 & 0x000000ff) == 0xf3)
			//Song Select(Song #) (not need)
		if ((dwParam1 & 0x000000ff) == 0xf6)
			//Tune request (not need)
			printf("Tune request\n");
		if ((dwParam1 & 0x000000ff) == 0xf8)
			//Timing Clock (not need)
			printf("Timing Clock\n");
		if ((dwParam1&0x000000ff)==0xfa)
			//Start
		if ((dwParam1 & 0x000000ff) == 0xfb)
			//Continue
		if ((dwParam1 & 0x000000ff) == 0xfc) {
			//Stop
			printf("Stop\n");
		}
#endif
		if ((dwParam1 & 0x000000ff) == 0xfe) {
			//Active Sensing
//			printf("Active Sensing\n");
			active_sensing_flag = ~0;
			active_sensing_time = get_current_calender_time();
		}
		if ((dwParam1 & 0x000000ff) == 0xff) {
			//System Reset
			printf("System Reset\n");
		}
		break;
	default:
//		printf("Unsup/ed event %d\n", aevp->type);
		break;
	}
	if (ev.type != ME_NONE) {
		rtsyn_play_event(&ev);
	}
	return 0;
}


void rtsyn_play_one_sysex (char *sysexbuffer, int exlen ){
	int i,j,chk,ne;
	MidiEvent ev;
	MidiEvent evm[260];

	if(sysexbuffer[exlen-1] == '\xf7'){            // I don't konw why this need
		for(i=0;i<EX_RESET_NO;i++){
			chk=0;
			for(j=0;(j<exlen)&&(j<11);j++){
				if(chk==0 && sysex_resets[i][j]!=sysexbuffer[j]){
					chk=~0;
				}
			}
			if(chk==0){
				 rtsyn_server_reset();
			}
		}
/*
		printf("SyeEx length=%x bytes \n", exlen);
		for(i=0;i<exlen;i++){
			printf("%x ",sysexbuffer[i]);
		}
		printf("\n");
*/
		if(parse_sysex_event(sysexbuffer+1,exlen-1,&ev)){
			if(ev.type==ME_RESET && rtsyn_system_mode!=DEFAULT_SYSTEM_MODE){
				ev.a=rtsyn_system_mode;
				change_system_mode(rtsyn_system_mode);
				rtsyn_play_event(&ev);
			}else{
				rtsyn_play_event(&ev);
			}
		}
		if(ne=parse_sysex_event_multi(sysexbuffer+1,exlen-1, evm)){
			for (i = 0; i < ne; i++){
					rtsyn_play_event(&evm[i]);
			}
		}
	}
}
