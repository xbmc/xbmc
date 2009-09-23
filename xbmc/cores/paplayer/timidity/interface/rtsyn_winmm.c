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


    rtsyn_winmm.c
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
#include <mmsystem.h>
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

int rtsyn_portnumber=1;
unsigned int portID[MAX_PORT];
char rtsyn_portlist[32][80];
int rtsyn_nportlist;

#define MAX_EXBUF 20
#define BUFF_SIZE 512

UINT InNum;
HMIDIIN  hMidiIn[MAX_PORT];
HMIDIOUT hMidiOut[MAX_PORT];
MIDIHDR *IMidiHdr[MAX_PORT][MAX_EXBUF];

char sIMidiHdr[MAX_PORT][MAX_EXBUF][sizeof(MIDIHDR)];
char sImidiHdr_data[MAX_PORT][MAX_EXBUF][BUFF_SIZE];

struct evbuf_t{
	int status;
	UINT wMsg;
	DWORD	dwInstance;
	DWORD	dwParam1;
	DWORD	dwParam2;
};
#define EVBUFF_SIZE 512
struct evbuf_t evbuf[EVBUFF_SIZE];
UINT  evbwpoint=0;
UINT  evbrpoint=0;
UINT evbsysexpoint;
UINT  mvbuse=0;
enum{B_OK, B_WORK, B_END};

void CALLBACK MidiInProc(HMIDIIN,UINT,DWORD,DWORD,DWORD);

void rtsyn_get_port_list(){
	int i;
	MIDIINCAPS InCaps;
	InNum = midiInGetNumDevs();
	for (i=1;i <=InNum && i<=32;i++){
		midiInGetDevCaps(i-1,(LPMIDIINCAPSA) &InCaps,sizeof(InCaps));
		sprintf(rtsyn_portlist[i-1],"%d:%s",i,(LPSTR)InCaps.szPname);
	}
	rtsyn_nportlist=i-1;
}

int rtsyn_synth_start(){
	int i;
	UINT port;
	MidiEvent ev;
#ifdef __W32__
	DWORD processPriority;
	processPriority = GetPriorityClass(GetCurrentProcess());
#endif

	rtsyn_reset();
	rtsyn_system_mode=DEFAULT_SYSTEM_MODE;
	change_system_mode(rtsyn_system_mode);
	ev.type=ME_RESET;
	ev.a=GS_SYSTEM_MODE; //GM is mor better ???
	rtsyn_play_event(&ev);
	
	port=0;

	for(port=0;port<rtsyn_portnumber;port++){
		for (i=0;i<MAX_EXBUF;i++){
			IMidiHdr[port][i] = (MIDIHDR *)sIMidiHdr[port][i];
			memset(IMidiHdr[port][i],0,sizeof(MIDIHDR));
			IMidiHdr[port][i]->lpData = sImidiHdr_data[port][i];
			memset((IMidiHdr[port][i]->lpData),0,BUFF_SIZE);
			IMidiHdr[port][i]->dwBufferLength = BUFF_SIZE;
		}
	}
	evbuf[0].status=B_END;
	evbwpoint=0;
	evbrpoint=0;
	mvbuse=0;

	for(port=0;port<rtsyn_portnumber;port++){
		midiInOpen(&hMidiIn[port],portID[port],(DWORD)MidiInProc,(DWORD)port,CALLBACK_FUNCTION);
		for (i=0;i<MAX_EXBUF;i++){
			midiInUnprepareHeader(hMidiIn[port],IMidiHdr[port][i],sizeof(MIDIHDR));
			midiInPrepareHeader(hMidiIn[port],IMidiHdr[port][i],sizeof(MIDIHDR));
			midiInAddBuffer(hMidiIn[port],IMidiHdr[port][i],sizeof(MIDIHDR));
		}
	}

#ifdef __W32__
	// HACK:midiInOpen()でリセットされてしまうため、再設定
	SetPriorityClass(GetCurrentProcess(), processPriority);
#endif

	for(port=0;port<rtsyn_portnumber;port++){
		if(MMSYSERR_NOERROR !=midiInStart(hMidiIn[port])){
			int i;
			for(i=0;i<port;i++){
				midiInStop(hMidiIn[i]);
				midiInReset(hMidiIn[i]);
				midiInClose(hMidiIn[i]);
			}
			goto winmmerror;
		}
	}

	return ~0;

winmmerror:
	ctl->cmsg(  CMSG_ERROR, VERB_NORMAL, "midiInStarterror\n" );
	return 0;
}

void rtsyn_synth_stop(){
	rtsyn_stop_playing();
	//	play_mode->close_output();
	rtsyn_midiports_close();

	return;
}
void rtsyn_midiports_close(void){
	UINT port;
	
	for(port=0;port<rtsyn_portnumber;port++){
		if( MMSYSERR_NOERROR!=midiInStop(hMidiIn[port]) )
			ctl->cmsg(  CMSG_ERROR, VERB_NORMAL,"MIDI Stop Error\n");
		if( MMSYSERR_NOERROR!=midiInReset(hMidiIn[port]) ) 
			ctl->cmsg(  CMSG_ERROR, VERB_NORMAL,"MIDI Rest Error\n");
		if( MMSYSERR_NOERROR!=midiInClose(hMidiIn[port]) ) 
			ctl->cmsg(  CMSG_ERROR, VERB_NORMAL,"MIDI Close Error\n");
	}
}

int rtsyn_buf_check(void){
	if( (evbuf[evbrpoint].status==B_OK)&&(evbrpoint!=evbwpoint) ){
		return ~0;
	}else{
		return 0;
	}
}

int rtsyn_play_some_data(void){
	UINT wMsg;
	DWORD	dwInstance;
	DWORD	dwParam1;
	DWORD	dwParam2;
	MidiEvent ev;
	MidiEvent evm[260];
	int port;
	UINT evbpoint;
	MIDIHDR *IIMidiHdr;
	int exlen;
	char *sysexbuffer;
	int ne,i,j,chk,played;
	
	played=0;
#ifndef USE_WINSYN_TIMER_I
	do{
		Sleep(1);
#endif
		if( !(evbuf[evbrpoint].status==B_OK)&&(evbrpoint!=evbwpoint) ) played=~0;
		while( (evbuf[evbrpoint].status==B_OK)&&(evbrpoint!=evbwpoint) ){

			evbpoint=evbrpoint;
			if (++evbrpoint >= EVBUFF_SIZE)
					evbrpoint -= EVBUFF_SIZE;

			wMsg=evbuf[evbpoint].wMsg;
			dwInstance=evbuf[evbpoint].dwInstance;
			dwParam1=evbuf[evbpoint].dwParam1;
			dwParam2=evbuf[evbpoint].dwParam2;

			port=(UINT)dwInstance;
			switch (wMsg) {
			case MIM_DATA:
				rtsyn_play_one_data (port, dwParam1);
				break;
			case MIM_LONGDATA:
				IIMidiHdr = (MIDIHDR *) dwParam1;
				exlen=(int)IIMidiHdr->dwBytesRecorded;
				sysexbuffer=IIMidiHdr->lpData;
				rtsyn_play_one_sysex (sysexbuffer,exlen );
				if (MMSYSERR_NOERROR != midiInUnprepareHeader(
						hMidiIn[port], IIMidiHdr, sizeof(MIDIHDR)))
					ctl->cmsg(  CMSG_ERROR, VERB_NORMAL,"error1\n");
				if (MMSYSERR_NOERROR != midiInPrepareHeader(
						hMidiIn[port], IIMidiHdr, sizeof(MIDIHDR)))
					ctl->cmsg(  CMSG_ERROR, VERB_NORMAL,"error5\n");
				if (MMSYSERR_NOERROR != midiInAddBuffer(
						hMidiIn[port], IIMidiHdr, sizeof(MIDIHDR)))
					ctl->cmsg(  CMSG_ERROR, VERB_NORMAL,"error6\n");
				break;
			}
		}	
#ifndef USE_WINSYN_TIMER_I
	}while(rtsyn_reachtime>get_current_calender_time());
#endif 
	return played;
}

void CALLBACK MidiInProc(HMIDIIN hMidiInL, UINT wMsg, DWORD dwInstance,
		DWORD dwParam1, DWORD dwParam2)
{
	UINT evbpoint;
	UINT port;
	
	port=(UINT)dwInstance;
	switch (wMsg) {
	case MIM_DATA:
	case MIM_LONGDATA:
		evbpoint = evbwpoint;
		if (++evbwpoint >= EVBUFF_SIZE)
			evbwpoint -= EVBUFF_SIZE;
		evbuf[evbwpoint].status = B_END;
		evbuf[evbpoint].status = B_WORK;
		evbuf[evbpoint].wMsg = wMsg;
		evbuf[evbpoint].dwInstance = dwInstance;
		evbuf[evbpoint].dwParam1 = dwParam1;
		evbuf[evbpoint].dwParam2 = dwParam2;
		evbuf[evbpoint].status = B_OK;
		break;
	case MIM_OPEN:
//		ctl->cmsg(  CMSG_ERROR, VERB_NORMAL,"MIM_OPEN\n");
		break;
	case MIM_CLOSE:
//		ctl->cmsg(  CMSG_ERROR, VERB_NORMAL,"MIM_CLOSE\n");
		break;
	case MIM_LONGERROR:
		ctl->cmsg(  CMSG_ERROR, VERB_NORMAL,"MIM_LONGERROR\n");
		break;
	case MIM_ERROR:
		ctl->cmsg(  CMSG_ERROR, VERB_NORMAL,"MIM_ERROR\n");
		break;
	case MIM_MOREDATA:
		ctl->cmsg(  CMSG_ERROR, VERB_NORMAL,"MIM_MOREDATA\n");
		break;
	}
}
