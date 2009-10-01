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
	
    mac_c.h
    header for Macintosh control mode
*/

#ifndef MAC_C_H
#define MAC_C_H

void position_window(MacWindow* macwin);
int open_window(MacWindow* macwin, short resID);
void goaway_default(MacWindow* macwin);
void close_default(MacWindow* macwin);

void mac_ctl_reset_trc();
void mac_ctl_program(int ch, int val, void *comm);
void v_ctl_note(int status, int ch, int note, int vel);
void mac_trc_update_time( int cur_sec, int tot_sec );
void mac_trc_update_voices();
void mac_trc_update_all_info();
void mac_setVolume(short amplitude);
void ctl_speana_data(double *val, int size);

pascal OSErr DragTrackingProc(
	DragTrackingMessage theMessage, WindowPtr window,
	 void*	/*theRefCon*/,	DragReference /*drag*/);
pascal OSErr DragReceiveFunc(
	WindowPtr /*window*/, void* /*theRefCon*/,
	DragReference	drag);

enum{
	MW_SKIN_LOAD_BMP,
	MW_SKIN_TOTAL_TIME,
	MW_SKIN_TIME,
	MW_SKIN_FILENAME
};

#define SKIN_ACTION_PREV() {mac_rc=RC_PREVIOUS;mac_HandleControl();}
#define SKIN_ACTION_PLAY() {mac_rc=RC_CONTINUE;mac_HandleControl();}
#define SKIN_ACTION_PAUSE() {mac_rc=RC_TOGGLE_PAUSE;mac_HandleControl();}
#define SKIN_ACTION_STOP() {mac_rc=RC_QUIT;mac_HandleControl();}
#define SKIN_ACTION_NEXT() {mac_rc=RC_NEXT;mac_HandleControl();}
#define SKIN_ACTION_EJECT()
#define SKIN_ACTION_EXIT()

#endif //MAC_C_H
