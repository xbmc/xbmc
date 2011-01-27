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


    rtsyn_portmidi.c
        Copyright (c) 2003 Keishi Suenaga <s_keishi@mutt.freemail.ne.jp>

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

#include <portmidi.h>
#include <porttime.h>

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

int rtsyn_portnumber=1;
unsigned int portID[MAX_PORT];
char rtsyn_portlist[32][80];
int rtsyn_nportlist;

#define MAX_EXBUF 20
#define BUFF_SIZE 512


PmError pmerr;
static unsigned int InNum;
struct midistream_t{
	PortMidiStream* stream;
};
static struct midistream_t  midistream[MAX_PORT];
//static PmDeviceID portID[MAX_PORT];
#define PMBUFF_SIZE 8192
#define EXBUFF_SIZE 512
static PmEvent pmbuffer[PMBUFF_SIZE];
static char    sysexbuffer[EXBUFF_SIZE];

void rtsyn_get_port_list(){
	int i,j;
	PmDeviceInfo *deviceinfo;
	
	pmerr=Pm_Initialize();
	if( pmerr != pmNoError ) goto pmerror;

	InNum = Pm_CountDevices();
	j=0;
	for (i=1;i <=InNum && i<=32;i++){
		deviceinfo=(PmDeviceInfo *)Pm_GetDeviceInfo(i-1);
		if(TRUE==deviceinfo->input){
			sprintf(rtsyn_portlist[j],"%d:%s",i,deviceinfo->name);
			j++;
		}
	}
	rtsyn_nportlist=j;
	Pm_Terminate();
	
	return;
pmerror:
		Pm_Terminate();
	ctl->cmsg(  CMSG_ERROR, VERB_NORMAL, "PortMIDI error: %s\n", Pm_GetErrorText( pmerr ) );
	return;
}

int rtsyn_synth_start(){
	int i;
	unsigned int port;
	MidiEvent ev;

	rtsyn_reset();
	rtsyn_system_mode=DEFAULT_SYSTEM_MODE;
	change_system_mode(rtsyn_system_mode);
	ev.type=ME_RESET;
	ev.a=GS_SYSTEM_MODE; //GM is mor better ???
	rtsyn_play_event(&ev);
	
	port=0;
	pmerr=Pm_Initialize();
	if( pmerr != pmNoError ) goto pmerror;
	for(port=0;port<rtsyn_portnumber;port++){
				PortMidiStream* stream;
				void* timeinfo;

				pmerr=Pm_OpenInput( &stream,
				portID[port],
				NULL,
				(PMBUFF_SIZE),
				NULL,
				Pt_Time,
				NULL);
		midistream[port].stream=stream;
		if( pmerr != pmNoError ) goto pmerror;
		pmerr=Pm_SetFilter(midistream[port].stream,PM_FILT_CLOCK);
		if( pmerr != pmNoError ) goto pmerror;
	}

	return ~0;

pmerror:
		Pm_Terminate();
	ctl->cmsg(  CMSG_ERROR, VERB_NORMAL, "PortMIDI error: %s\n", Pm_GetErrorText( pmerr ) );
	return 0;

}


void rtsyn_synth_stop(){

	rtsyn_stop_playing();
//	play_mode->close_output();
	rtsyn_midiports_close();
	
	return;
pmerror:
		Pm_Terminate();
	ctl->cmsg(  CMSG_ERROR, VERB_NORMAL, "PortMIDI error: %s\n", Pm_GetErrorText( pmerr ) );
	return ;
}
void rtsyn_midiports_close(void){
	unsigned int port;

	for(port=0;port<rtsyn_portnumber;port++){
		pmerr=Pm_Abort(midistream[port].stream);
//		if( pmerr != pmNoError ) goto pmerror;
	}
	Pm_Terminate();
}

int rtsyn_play_some_data (void){
	PmMessage pmmsg;
	int played;	
	int j,port,exlen,data,shift;
	long pmlength,pmbpoint;
	
	played=0;
	do{
		sleep(0);
		for(port=0;port<rtsyn_portnumber;port++){
			pmerr=Pm_Read(midistream[port].stream, pmbuffer, PMBUFF_SIZE);
			if(pmerr<0) goto pmerror;
			pmlength=pmerr;
			pmbpoint=0;
			while(pmbpoint<pmlength){
				played=~0;
				pmmsg=pmbuffer[pmbpoint].message;
				pmbpoint++;
				if( 1==rtsyn_play_one_data (port, pmmsg) ){	

					j=0;
					sysexbuffer[j++] = 0xf0;
					for (shift = 8,data=0; shift < 32 && (data != 0x0f7); shift += 8) {
       		         	data= (pmmsg >> shift) & 0x0FF;
						sysexbuffer[j++]=data;
					}
					if(data!=0x0f7){
						if(pmbpoint>=pmlength){
							{
								pmerr=Pm_Read(midistream[port].stream, pmbuffer, PMBUFF_SIZE);
								if(pmerr<0){goto pmerror; }
								sleep(0);
							}while(pmerr==8);
							pmlength=pmerr;
							pmbpoint=0;
						}
						while(j<EXBUFF_SIZE-4){
							for (shift=0,data=0; shift < 32 && (data != 0x0f7); shift += 8) {
                				data= (pmbuffer[pmbpoint].message >> shift) & 0x0FF;
								sysexbuffer[j++]=data;
							}
							pmbpoint++;
							if(data==0x0f7) break;
							if( pmbpoint>=pmlength ){
								{
									pmerr=Pm_Read(midistream[port].stream, pmbuffer, PMBUFF_SIZE);
									if(pmerr<0){goto pmerror;}
									sleep(0);
								}while(pmerr==0);
								pmlength=pmerr;
								pmbpoint=0;
							}
						}
					}
					exlen=j;
					rtsyn_play_one_sysex (sysexbuffer,exlen );
				}

			}
		}
	}while(rtsyn_reachtime>get_current_calender_time());
	return played;
pmerror:
	Pm_Terminate();
	ctl->cmsg(  CMSG_ERROR, VERB_NORMAL, "PortMIDI error: %s\n", Pm_GetErrorText( pmerr ) );
	return 0;
}

