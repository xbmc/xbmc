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
		
    mac_trace.c
    Macintosh trace window
*/

// includs
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "string.h"
#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"
#include "miditrace.h"
#include "bitset.h"
#include "mfnode.h"
//#include "aq.h"

#include "mac_main.h"
#include "mac_util.h"
#include "mac_c.h"

// macros & const
#define UPPER_MERGIN 40
#define LEFT_MERGIN  130
#define CHANNEL_HIGHT 12
#define CHANNEL_WIDTH 4
#define win mac_TraceWindow
#define COLS 80
const RGBColor	black={0,0,0};

// global
static int selected_channel = -1;
static int current_voices=0;
static Bitset channel_program_flags[MAX_CHANNELS];
static int scr_modified_flag = 1; /* delay flush for trace mode */
static int display_velocity_flag = 0;
static const char note_name_char[12] =
{
    'c', 'C', 'd', 'D', 'e', 'f', 'F', 'g', 'G', 'a', 'A', 'b'
};

// static function
static int open_TraceWin();
static void click_TraceWin(Point local, short modifiers);
static void update_TraceWin();
static void goaway_TraceWin();
static void close_TraceWin();
static int	message_TraceWin(int message, long param);
// ****************************************************
static void init_trace_window_chan(int /*ch*/)
{	
}

static void DrawInstrumentName(int ch, char *comm)
{
	char		buf[80];
	RGBColor	black={0,0,0};
	Rect		box;

	SetPortWindowPort(win.ref);
	RGBForeColor(&black);
	
		//channel number
	MoveTo(2, UPPER_MERGIN+CHANNEL_HIGHT*(ch+1)-1);
	snprintf(buf, 80,"%2d", ch+1);
	DrawText(buf, 0, strlen(buf));
		
		//InstrumentName
	box.top=UPPER_MERGIN+CHANNEL_HIGHT*ch;
	box.left=20;
	box.bottom=box.top+12;
	box.right=LEFT_MERGIN-10;
	if( !comm || !*comm )
		EraseRect(&box);
	else
		TETextBox(comm, strlen(comm), &box, teFlushDefault);
}

static void mac_ctl_refresh_trc()
{
	int		i;
	Rect	r;
	RGBColor	black={0,0,0},
				darkGray={0x2000,0x2000,0x2000};

	if( !win.show ) return;
	SetPortWindowPort(win.ref);
	for( i=0; i<16; i++ ){
		DrawInstrumentName(i, instr_comment[i].comm);
	}
	
#define MAX_NOTE_NUM 120
	r.top=		UPPER_MERGIN;
	r.left=		LEFT_MERGIN;
	r.bottom=	r.top+CHANNEL_HIGHT*16;
	r.right=	r.left+CHANNEL_WIDTH*MAX_NOTE_NUM;
	RGBForeColor(&darkGray);
	PaintRect(&r);
	
		//draw separater line
	RGBForeColor(&black);
	for(i=1; i<16; i++){	//horizontal
		MoveTo(LEFT_MERGIN, UPPER_MERGIN+CHANNEL_HIGHT*i-1);
		Line(CHANNEL_WIDTH*MAX_NOTE_NUM-1, 0);
	}
	for(i=12; i<MAX_NOTE_NUM; i+=12){	//vertical
		MoveTo(LEFT_MERGIN+CHANNEL_WIDTH*i-1, UPPER_MERGIN);
		Line(0, CHANNEL_HIGHT*16-1);
	}
}

void mac_ctl_reset_trc()
{
	int i;
	
    for(i = 0; i < MAX_CHANNELS; i++)
    {
		instr_comment[i].last_note_on = 0.0;
		instr_comment[i].comm =""; // channel_instrum_name(i);
		clear_bitset(channel_program_flags + i, 0, 128);
    }
	SetPortWindowPort(win.ref);
	TextMode(srcCopy);
	TextSize(9);
	mac_ctl_refresh_trc();
	current_voices=0;
}

void mac_ctl_program(int ch, int val, void *comm)
{
    int pr;

    if(ch >= 16)
	return;

    if(!ctl->trace_playing)
	return;

    if(IS_CURRENT_MOD_FILE)
		pr = val;
    else
		pr = val + progbase;

    //if(ctl_ncurs_mode == NCURS_MODE_TRACE)
    {
		if(ch == selected_channel)
			init_trace_window_chan(ch);
		else
		{
			//wmove(dftwin, NOTE_LINE+ch, COLS-21);
			if(ISDRUMCHANNEL(ch))
			{
				//wattron(dftwin, A_BOLD);
				//wprintw(dftwin, " %03d", pr);
				//wattroff(dftwin, A_BOLD);
		    }
		    else
			//wprintw(dftwin, " %03d", pr);
			;
		}
    }

    //if(comm != NULL)
	indicator_set_prog(ch, val, (char *)comm);
    //scr_modified_flag = 1;

	instr_comment[ch].last_note_on = 0.0;
	instr_comment[ch].comm = channel_instrum_name(ch);
	DrawInstrumentName(ch, instr_comment[ch].comm);
}
// ****************************************************
#pragma mark -
static void update_channel(int /*ch*/)
{
	
}

static void update_filename()
{
	char	buf[256]="";

	if( !mac_TraceWindow.show ) return;
	SetPortWindowPort(win.ref);
	
	if( mac_n_files>0 && nPlaying<=mac_n_files && fileList[nPlaying].mfn &&
										fileList[nPlaying].mfn->file )
		snprintf(buf, 256,"File: %s", fileList[nPlaying].mfn->file);
	RGBForeColor(&black);
	MoveTo(2,12); DrawText(buf, 0, strlen(buf));
}

static void update_title()
{
	char	buf[256]="";

	if( !mac_TraceWindow.show ) return;
	SetPortWindowPort(win.ref);
	if( mac_n_files>0 && nPlaying<=mac_n_files && fileList[nPlaying].mfn &&
										fileList[nPlaying].mfn->title )
		snprintf(buf, 256, "Title: %s", fileList[nPlaying].mfn->title);
	RGBForeColor(&black);
	MoveTo(2,24); DrawText(buf, 0, strlen(buf));
}

void mac_trc_update_time( int cur_sec, int tot_sec )
{
	static int	save_tot_sec=0, save_cur_sec;
	//int rate;
	char		buf[80];
	
	if( cur_sec!=-1 ) save_cur_sec=tot_sec;
	if( tot_sec!=-1 ) save_tot_sec=tot_sec;
	if( cur_sec==-1 ) cur_sec=0;
	if( cur_sec > save_tot_sec ) cur_sec=save_tot_sec;
	
	if( !win.show ) return;
	//rate = (int)(aq_filled_ratio() * 100 + 0.5);

	SetPortWindowPort(win.ref);
	snprintf(buf, 80," %3d:%02d /%3d:%02d   " /*"buffering=%3d %% "*/ "buffer %d/256  ",
		cur_sec/60, cur_sec%60, save_tot_sec/60,save_tot_sec%60,
		/*rate,*/ mac_buf_using_num );
	RGBForeColor(&black);
	MoveTo(400,12); DrawText(buf, 0, strlen(buf));
}

void mac_trc_update_voices()
{
	char	buf[20];
	static	int	prev=-1;

	if( !mac_TraceWindow.show ) return;
	SetPortWindowPort(win.ref);
	
	snprintf(buf, 20, "Voice %3d/%3d   ", current_voices, voices);
	RGBForeColor(&black);
	MoveTo(450,24); DrawText(buf, 0, strlen(buf));
}

void mac_trc_update_all_info()
{
	Rect r;
	
	SetPortWindowPort(win.ref);
	SetRect(&r, 0,0, win.ref->portRect.right,UPPER_MERGIN);
	EraseRect(&r);
	
	mac_trc_update_voices();
	update_filename();
	update_title();
	mac_trc_update_time(-1,-1);
}
// *****************************************************
#pragma mark -
MacWindow win={
	0,	//WindowRef
	open_TraceWin,
	click_TraceWin,
	update_TraceWin,
	goaway_default,
	close_default,
	message_TraceWin,
	0, 120,120
};

static int open_TraceWin()
		/*success-> return 0;*/
{
	int		i;

	open_window(&win, kTraceWinID);
	position_window(&win);
	
	mac_ctl_reset_trc();
	for(i = 0; i < 16; i++)
	    init_bitset(channel_program_flags + i, 128);

	return 0;
}

static void click_TraceWin(Point /*local*/, short /*modifiers*/)
{
}

static void update_TraceWin()
{
	SetPortWindowPort(win.ref);
	mac_ctl_refresh_trc();
	mac_trc_update_all_info();
}

static int	message_TraceWin(int /*message*/, long /*param*/)
{
	//Rect rect;
	
	//switch(message){	
	//}

	return -1;  //not supported
}

const RGBColor	vel_color[10]={
					{0x0000, 0x0000, 0xFFFF},	//0:blue
					{0x0000, 0xFFFF, 0x0000},	//6;green
					{0x0000, 0x6666, 0xFFFF},	//1;
					{0x0000, 0xFFFF, 0x6666},	//5;
					{0x6666, 0xFFFF, 0x0000},	//7;
					{0x0000, 0x9999, 0xFFFF},	//2;
					{0x0000, 0xFFFF, 0x9999},	//4;
					{0x9999, 0xFFFF, 0x0000},	//8;
					{0x0000, 0xFFFF, 0xFFFF},	//3;
					{0xFFFF, 0xFFFF, 0x0000},	//9;yellow
				};

#define DARKEN2(c) (c.red/=2,c.green/=2,c.blue/=2)
#define DARKEN4(c) (c.red/=4,c.green/=4,c.blue/=4)

static unsigned int UpdateNote(int status, int ch, int note, int vel)
{
	//int	vel;
	Rect r1,r2;
    unsigned int onoff=0 /*, check, prev_check*/;
	const RGBColor	dieColor=	{0x3000,0x3000,0x3000},	//dark gray
					freeColor=	{0x3000,0x3000,0x3000},	//dark gray
			onColor=	{0xffff,0xffff,0}, 	//yellow
					sustainedColor={0x8000,0x8000,0},	//dark yellow
			offColor=	{0x4000,0x4000,0},	//dark yellow
			noColor=	{0x2000,0x2000,0x2000};
	RGBColor	color;
	
	vel=(10 * vel) / 128; /* 0-9 */
	if( vel>9 ) vel=9;
	
	r1.left=r2.left=	LEFT_MERGIN+CHANNEL_WIDTH* note;
	r1.right=r2.right=	r1.left+CHANNEL_WIDTH-1;

	r1.top=		UPPER_MERGIN+CHANNEL_HIGHT* ch;
	r1.bottom= 	r1.top+(9-vel);
	r2.top=		r1.bottom;
	r2.bottom=	r1.top+CHANNEL_HIGHT-1;
	
	SetPortWindowPort(win.ref);
    
    color=vel_color[vel];
    switch(status){
	case VOICE_DIE:	 DARKEN2(color);  onoff = 1;	break;
	case VOICE_FREE: color=freeColor; onoff = 0;	break;
	case VOICE_SUSTAINED:DARKEN2(color); onoff = 1;	break;
	case VOICE_OFF:	DARKEN4(color); onoff = 1;		break;
	case VOICE_ON:			onoff = 1;		break;
	default:	color= noColor; break;
    }
    RGBForeColor(&freeColor);
    PaintRect(&r1);    
    RGBForeColor(&color);
    PaintRect(&r2);
    return onoff;
}


//void v_ctl_note(struct MidiTraceNote v)
void v_ctl_note(int status, int ch, int note, int vel)
{
    int xl, n, c;
    unsigned int onoff=0, prev_onoff, check;
    Bitset *bitset;
	
    if( !mac_TraceWindow.show || ch >= 16)
	return;

    if( /*ctl_ncurs_mode != NCURS_MODE_TRACE ||*/ selected_channel == ch)
		return;

    scr_modified_flag = 1;
    if(display_velocity_flag)
		n = '0' + (10 * vel) / 128;
    else
		n = note_name_char[note % 12];
    c = (COLS - 24) / 12 * 12;
    if(c <= 0)
		c = 1;
    xl=note % c;
    if( note>=MAX_NOTE_NUM ) return;
	
	onoff= UpdateNote( status, ch, note, vel);   //draw at first
    bitset = channel_program_flags + ch;
	get_bitset(bitset, &prev_onoff, note, 1);
    onoff <<= (8 * sizeof(onoff) - 1);
    set_bitset(bitset, &onoff, note, 1);
    check = has_bitset(bitset);
            
	if( prev_onoff && !onoff ) current_voices--;
	if( !prev_onoff && onoff ){
		current_voices++;
		mac_trc_update_voices();
	}
	
}

#undef win
