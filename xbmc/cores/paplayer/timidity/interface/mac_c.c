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
	
    mac_c.c
    Macintosh control mode
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <Drag.h>
#include <Icons.h>
#include <Threads.h>
#include <TextUtils.h>

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"
#include "mfnode.h"
#include "miditrace.h"
#ifdef SUPPORT_SOUNDSPEC
#include "soundspec.h"
#endif /* SUPPORT_SOUNDSPEC */

#include "mac_main.h"
#include "mac_util.h"
#include "mac_c.h"
#ifdef MAC_USE_OMS
#include "mac_oms.h"
#endif

// *******************************

static void ctl_refresh(void);
static void ctl_total_time(int tt);
static void ctl_master_volume(int mv);
static void ctl_file_name(char *name);
static void ctl_current_time(int ct, int v);
static void ctl_note(int status, int ch, int note, int vel);
static void ctl_volume(int channel, int val);
static void ctl_expression(int channel, int val);
static void ctl_panning(int channel, int val);
static void ctl_sustain(int channel, int val);
static void ctl_pitch_bend(int channel, int val);
static void ctl_reset(void);
static int ctl_open(int using_stdin, int using_stdout);
static void ctl_pass_playing_list(int number_of_files, char *list_of_files[]);
static void ctl_close(void);
static int ctl_read(int32 *valp);
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void ctl_event(CtlEvent *e);

/**********************************/
/* export the interface functions */

#define ctl mac_control_mode

ControlMode ctl= 
{
  "mac interface", 'm',
  1,1,0,
  0,
  ctl_open,
  ctl_close,
  ctl_pass_playing_list,
  ctl_read,
  cmsg,
  ctl_event
};

// ***********************************************
Boolean	gCursorIsWatch, gHasDragMgr;
#define FILESTR_LEN 40
#define TIMESTR_LEN 10
char	fileStr[FILESTR_LEN]="", timeStr[TIMESTR_LEN]="";
Rect	rFileName={10,10, 26,140},
		rTime={10,150, 26,200},
		rLogo={38,5,94,98},
		rVol={28,220,92,239},
		
		rPlay={40,110,62,142},
		rStop={40,145,62,177},
		rPause={40,180,62,212},
		rPrevious={70,110,92,142},
		rForward={70,145,92,177},
		rEject={70,180,92,212},
		rLoop={8,210,28,240};

PicHandle	logo, logoDown;
//static RgnHandle	receiveRgn;
CIconHandle	button[6], iconPlay, iconPause, iconVol, iconTab, iconNotLoop,iconLoop;
static TEHandle	gLogTE=0,gDocTE=0;

/****************************************/
static Boolean	recieveHiliting;

pascal OSErr DragTrackingProc(
	DragTrackingMessage		theMessage,
	WindowPtr				/*window*/,
	void*					/*theRefCon*/,
	DragReference			/*drag*/)
{	
	switch (theMessage)
	{
	case kDragTrackingEnterWindow:
		SetPort(mac_PlayerWindow.ref);
		//ShowDragHilite(drag, receiveRgn, true);
		DrawPicture(logoDown, &rLogo);
		recieveHiliting=true;
		break;
	case kDragTrackingInWindow:				
		break;
	case kDragTrackingLeaveWindow:
		if( recieveHiliting )
		{
			SetPort(mac_PlayerWindow.ref);
			//HideDragHilite(drag);
			DrawPicture(logo, &rLogo);
			recieveHiliting=false;
		}
		break;
	}
	return noErr;
}

pascal OSErr DragReceiveFunc(
	WindowPtr		/*window*/,
	void*			/*theRefCon*/,
	DragReference	drag)
{
	long			i;
	char*			data;
	unsigned short	numFlavors;
	Size			dataSize;
	ItemReference	theItem;
	unsigned short	itemCount;
	FlavorType		flavor;

	if( recieveHiliting )
	{		
		int	oldListEnd=mac_n_files;
		CountDragItems(drag, &itemCount);

		for( i=1; i<=itemCount; i++)
		{
			GetDragItemReferenceNumber(drag, i, &theItem);
			
			if( noErr!=CountDragItemFlavors(drag, theItem, &numFlavors))
						continue;
			if( noErr!=GetFlavorType(drag, theItem, 1, &flavor))
						continue;
			if( noErr!=GetFlavorDataSize(drag, theItem, flavor, &dataSize))
						continue;
			data= (char*)malloc(dataSize);
			if( data==0 ) continue;

			if( noErr!=GetFlavorData(drag, theItem, flavor,
						data, &dataSize, 0))
						continue;
			if(flavor== flavorTypeHFS)
				AddHFS2PlayList((HFSFlavor*)data);
			free(data);
		}
		if( gShuffle ) ShuffleList( oldListEnd, mac_n_files);
	}
	
	return noErr;
}

// *****************************************************
#pragma mark -

int open_window(MacWindow* macwin, short resID)
{
	macwin->ref=GetNewCWindow(resID, 0, (WindowRef)-1);
	if( macwin->ref==0) mac_ErrorExit("\pCannot open Window!");
	SetWRefCon(macwin->ref, (long)macwin);
	return 0;
}

void position_window(MacWindow* macwin)
{
	Point		p;

	p.h=macwin->X; p.v=macwin->Y;
	if( PtInRect(p, &qd.screenBits.bounds) ){
		MoveWindow(macwin->ref, macwin->X, macwin->Y, true);
	}
	SizeWindow(macwin->ref, macwin->width, macwin->hight, 0);

	if( macwin->show ) ShowWindow(macwin->ref);
}

static void setsize_window(MacWindow* macwin)
{
	SizeWindow(macwin->ref, macwin->width, macwin->hight, 0);
}

void goaway_default(MacWindow* macwin)
{
	HideWindow(macwin->ref); macwin->show=false;
}

void close_default(MacWindow* macwin)
{
	Point	p;
	Rect	r;
	
	//Get Log Window position
	SetPortWindowPort(macwin->ref);
	p.v=p.h=0;
	LocalToGlobal(&p);
	macwin->X=p.h, macwin->Y=p.v;
	r= GetWindowPort(macwin->ref)->portRect;
	macwin->width= r.right-r.left;
	macwin->hight= r.bottom-r.top;
}

// *****************************************************
#pragma mark -
#pragma mark =================Player Window

static int open_PlayerWin();
static void click_PlayerWin(Point local, short modifiers);
static void update_PlayerWin();
static void goaway_PlayerWin(MacWindow*);
static int	message_PlayerWin(int message, long param);

#define win mac_PlayerWindow
MacWindow win={
	0,	//WindowRef
	open_PlayerWin,
	click_PlayerWin,
	update_PlayerWin,
	goaway_PlayerWin,
	close_default,
	message_PlayerWin,
	1, 10,50
};

void mac_setVolume(short amplitude)
{
	SndCommand	theCmd;

	if( amplitude<0 ) amplitude=0;
	if( amplitude>255 ) amplitude=255;
	theCmd.cmd=ampCmd;
	theCmd.param1=amplitude;
	SndDoImmediate(gSndCannel, &theCmd);
}

void DoVolume()
{
	short		lastAmp=mac_amplitude;
	//SndCommand	theCmd;
	Point		p;
	
	SetPortWindowPort(win.ref);
	GetMouse(&p);
	/*GlobalToLocal(&p);*/
	if( PtInRect(p, &rVol) && FrontWindow()==win.ref )
	{
		
		mac_amplitude=255*((double)rVol.bottom-10-p.v)/(rVol.bottom-rVol.top-20);
		if( mac_amplitude<0 ) mac_amplitude=0;
		if( mac_amplitude>255 ) mac_amplitude=255;
		
		if( lastAmp!=mac_amplitude )
		{
			//theCmd.cmd=ampCmd;
			//theCmd.param1=mac_amplitude;
			//SndDoImmediate(gSndCannel, &theCmd);
			mac_setVolume(mac_amplitude);
			InvalRect(&rVol);
		}
	}
}		

void DrawButton()
{
	SetPortWindowPort(win.ref);
	PlotCIcon(&rPlay, skin_state==PLAYING || skin_state==PAUSE? iconPlay:button[0] );
	PlotCIcon(&rStop, button[1]);
	PlotCIcon(&rPause, skin_state==PAUSE? iconPause:button[2] );
	PlotCIcon(&rPrevious, button[3]);
	PlotCIcon(&rForward, button[4]);
	PlotCIcon(&rEject, button[5]);
}

static void DrawFileStr()
{
	SetPortWindowPort(win.ref);
	TETextBox(fileStr, strlen(fileStr), &rFileName, teCenter);
}

static void DrawTimeStr()
{
	SetPortWindowPort(win.ref);
	TETextBox(timeStr, strlen(timeStr), &rTime, teCenter);
}

static int open_PlayerWin()
		/*success-> return 0;*/
{
	int	i;
	OSErr	err;
	RGBColor	back={0,0,0},
				fore={65535,65535,65535};
	
	open_window( &win, kPlayerWinID);
	position_window(&win);
	
	SetPortWindowPort(win.ref);
	RGBForeColor(&fore);
	RGBBackColor(&back);
	logo= GetPicture(128);
	logoDown= GetPicture(132);
	
	for(i=0; i<6; i++)
		button[i]= GetCIcon(i+200);
	iconPlay=GetCIcon(210);
	iconPause=GetCIcon(211);
	iconVol=GetCIcon(206);
	iconTab=GetCIcon(207);
	iconNotLoop=GetCIcon(208);
	iconLoop=GetCIcon(209);
	
	if(gHasDragMgr)
	{
		//receiveRgn=NewRgn();
		//if( receiveRgn )
		{
			//GetWindowContentRgn(win.ref, receiveRgn);
			err=InstallTrackingHandler(NewDragTrackingHandlerProc(DragTrackingProc),
								(WindowPtr)win.ref, 0);
			if(err) ExitToShell();
			
			err=InstallReceiveHandler(NewDragReceiveHandlerProc(DragReceiveFunc),
							(WindowPtr)win.ref, 0);
			if(err) ExitToShell();
		}
	}
	return 0;
}

static void click_PlayerWin(Point p, short /*modifiers*/)
{
		if( PtInRect(p, &rStop)){ mac_rc=RC_QUIT; ctl_current_time(0,0); }
	else	if( PtInRect(p, &rPlay)){ mac_rc=RC_CONTINUE; }
	else	if( PtInRect(p, &rPause)){ mac_rc=RC_TOGGLE_PAUSE; }
	else	if( PtInRect(p, &rPrevious)){ mac_rc=RC_PREVIOUS;  }
	else	if( PtInRect(p, &rForward)){ mac_rc=RC_NEXT; }
	else	if( PtInRect(p, &rEject)){
				if( skin_state==PLAYING ) mac_rc=RC_QUIT;
				skin_state=WAITING; mac_n_files=nPlaying=0; fileStr[0]=timeStr[0]=0;
				init_ListWin();
				update_PlayerWin();
			}
	else	if( PtInRect(p, &rLoop)){ skin_f_repeat=!skin_f_repeat; SetPortWindowPort(win.ref);
									InvalRect(&rLoop); return; } /* don't call mac_HandleControl();*/
	else	if( PtInRect(p, &rVol))	DoVolume();
	else	return; /* no button click*/
	
	/* if button clicked */
	mac_HandleControl();
}

static void update_PlayerWin()
{
	short	y=((double)mac_amplitude/255)*(rVol.bottom-rVol.top-20);
	Rect	rFrame,rTab;
	
	SetPortWindowPort(win.ref);
	DrawButton();
	skin_f_repeat? PlotCIcon(&rLoop, iconLoop):PlotCIcon(&rLoop, iconNotLoop);
	PlotCIcon(&rVol, iconVol);
	SetRect(&rTab, rVol.left+2, rVol.top+49-y, rVol.right-2, rVol.top+59-y);
	PlotCIcon(&rTab, iconTab);
	
	rFrame=rFileName; InsetRect(&rFrame, -2, -2);	DrawPicture(GetPicture(129), &rFrame);
	rFrame=rTime; InsetRect(&rFrame, -2, -2);	DrawPicture(GetPicture(130), &rFrame);
	
	DrawPicture(logo, &rLogo);
	
	DrawTimeStr();
	DrawFileStr();
}

static void goaway_PlayerWin(MacWindow *macwin)
{
	Do_Quit();
}

static int	message_PlayerWin(int /*message*/, long /*param*/)
{
	return -1;
}

#undef win


// *****************************************************
#pragma mark -
#pragma mark ==================Log Window

static int open_LogWin();
static void click_LogWin(Point local, short modifiers);
static void update_LogWin();
static int	message_LogWin(int message, long param);

#define win mac_LogWindow
MacWindow win={
	0,	//WindowRef
	open_LogWin,
	click_LogWin,
	update_LogWin,
	goaway_default,
	close_default,
	message_LogWin,
	1, 270,60, 300,200
};

static void	PrintLogStr(char str[])
{
	int len=strlen(str);
	
	if( !mac_LogWindow.show ) return;
	
	if( (**gLogTE).teLength > 10000)	/* clear first half*/
	{
		TEFeatureFlag(teFAutoScroll, teBitClear, gLogTE);
		TESetSelect(0, 5000, gLogTE);
		TEDelete(gLogTE);
	}
	
	TESetSelect((**gLogTE).teLength, (**gLogTE).teLength, gLogTE); /* move to end*/
	if( len>=1 && str[len-1]=='\n' ){
		str[len-1]=0; len--;	//convert dos return code to mac's
	}
	TEInsert(str, len, gLogTE);
	TESelView(gLogTE);
	TEFeatureFlag(teFAutoScroll, teBitSet, gLogTE);
}

static int open_LogWin()
		/*success-> return 0;*/
{
	open_window(&win, kLogWinID);
	position_window(&win);
	setsize_window(&win);
	
	SetPortWindowPort(win.ref);
	TextSize(10);
	gLogTE=TENew(&win.ref->portRect, &win.ref->portRect);
	if( gLogTE==0 ) mac_ErrorExit("\ppCannot open LogWindow!");
	
	TEFeatureFlag(teFAutoScroll, teBitSet, gLogTE);
	TEActivate(gLogTE);
	return 0;
}

static void click_LogWin(Point local, short /*modifiers*/)
{
	TEClick(local, 0, gLogTE);
}

static void update_LogWin()
{
	SetPortWindowPort(win.ref);
	TEUpdate(&win.ref->portRect, gLogTE);
}

static int	message_LogWin(int message, long /*param*/)
{
	Rect rect;
	
	switch(message){
	case MW_GROW:
		rect=win.ref->portRect;
		//rect.right-=15; rect.bottom-=15;
		rect = win.ref->portRect;
		rect.right  -= 15;
		//rect.bottom -= 15;
		(**gLogTE).viewRect= (**gLogTE).destRect= rect;
		(**gLogTE).destRect.right = (**gLogTE).destRect.right - 14;
		TECalText(gLogTE);
		TESetSelect((**gLogTE).teLength, (**gLogTE).teLength, gLogTE);
		return 0;
	}

	return -1;  //not supported
}

#undef win
// *****************************************************
#pragma mark -
#pragma mark ==================List Window

ListHandle	gPlaylist;

static int open_ListWin();
static void click_ListWin(Point local, short modifiers);
static void update_ListWin();
static int message_ListWin(int message, long param);

enum{
	MW_LIST_SELECT=	0x00010001,
	MW_DOC_SET=		0x00020001
};

#define win mac_ListWindow
MacWindow win={
	0,	//WindowRef
	open_ListWin,
	click_ListWin,
	update_ListWin,
	goaway_default,
	close_default,
	message_ListWin,
	1, 100,100, 300,200
};

void change_ListRow( short row, const MidiFile* file)
{
	Point	aCell;
	char	buf[256]="",*p;
	
	if( file && file->mfn && file->mfn->title )
	{
		strncpy(buf, file->mfn->title, sizeof(buf) - 1);
		buf[sizeof(buf)-1] = '\0';
	}
	
	p= strrchr(file->filename, PATH_SEP);
	if( p ){
		size_t len = strlen(buf);
		char* q = buf[len];		 /* last */
		snprintf(buf, 256-len-1, "	(%s)", p+1);
		buf[sizeof(buf)-1] = '\0';
	}

	SetPt(&aCell, 0, row);
	LSetCell( buf, strlen(buf), aCell, gPlaylist);
}

void add_ListWin(MidiFile * file)
{
	short	rowNum=1;
	//Str255	fullPath;
	
	//GetFullPath(&file->spec, fullPath);
	file->mfn=make_new_MFnode_entry(file->filename);
	rowNum = (**gPlaylist).dataBounds.bottom;	
	rowNum = LAddRow(1, rowNum, gPlaylist);
	change_ListRow(  rowNum, file);
}

void init_ListWin()
{
	short	rowNum= (**gPlaylist).dataBounds.bottom;

	SetPortWindowPort(win.ref);
	LDelRow(0, 0, gPlaylist);
}

static int open_ListWin()
		/*success-> return 0;*/
{
	Rect		listRect, dataBounds;
	Point		cSize;

	open_window(&win, kListWinID);
	position_window(&win);
	setsize_window(&win);
	
	SetPortWindowPort(win.ref);
	TextSize(10);
	listRect=win.ref->portRect;	listRect.right-=14; listRect.bottom-=14;
	dataBounds.top=dataBounds.left=0; dataBounds.right=1; dataBounds.bottom=0;
	cSize.h=1024; cSize.v=14;
	gPlaylist = LNew(&listRect, &dataBounds, cSize, 0, win.ref, 
						1, 1, 0, 1);
	return 0;
}

static void click_ListWin(Point local, short modifiers)
{
	Boolean	doubleclick;
	Cell	cell;
	
	SetPortWindowPort(win.ref);
	doubleclick=LClick(local, modifiers, gPlaylist);
	if(doubleclick){
		mac_rc=RC_LOAD_FILE;
		cell = LLastClick(gPlaylist);
		nPlaying=cell.v;
		mac_HandleControl();
	}
}

static void update_ListWin()
{
	SetPortWindowPort(win.ref);
	LUpdate( win.ref->visRgn, gPlaylist);
	DrawGrowIcon(win.ref);
}

static int message_ListWin(int message, long param)
{
	Cell	cell;
	Rect	rect;
	
	switch(message){
	case MW_GROW:
		rect=win.ref->portRect;
		LSize(rect.right-14, rect.bottom-14, gPlaylist);
		return 0;

	case MW_LIST_SELECT:
		LDeselectAll(gPlaylist);
		cell.h=0; cell.v=param;
		LSetSelect(true, cell, gPlaylist);
		LAutoScroll(gPlaylist);
		return 0;
	}

	return -1; //not supported
}

#undef win
// *****************************************************
#pragma mark -
#pragma mark ==================Doc Window

static int open_DocWin();
static void click_DocWin(Point local, short modifiers);
static void update_DocWin();
static void close_DocWin();
static int	message_DocWin(int message, long param);

#define win mac_DocWindow
MacWindow win={
	0,	//WindowRef
	open_DocWin,
	click_DocWin,
	update_DocWin,
	goaway_default,
	close_default,
	message_DocWin,
	0, 150,150, 300,200
};

static void	PrintDocStr(char str[])
{
	if( (**gDocTE).teLength > 10000)	/* clear first half*/
	{
		TESetSelect(0, 5000, gDocTE);
		TEDelete(gDocTE);
	}
	
	TESetSelect((**gDocTE).teLength, (**gDocTE).teLength, gDocTE); /* move to end*/
	TEInsert(str, strlen(str), gDocTE);
	TESelView(gDocTE);
}

static int open_DocWin()
		/*success-> return 0;*/
{
	Rect	r;

	open_window(&win, kDocWinID);
	position_window(&win);
	setsize_window(&win);
	
	SetPortWindowPort(win.ref);
	TextSize(10);
	r=win.ref->portRect; r.right-=16;
	gDocTE=TENew(&r, &r);
	if( gDocTE==0 ) mac_ErrorExit("\pCannot open DocWindow!");
	
	TEFeatureFlag(teFAutoScroll, teBitSet, gDocTE);
	TEActivate(gDocTE);
	return 0;
}

static void click_DocWin(Point local, short /*modifiers*/)
{
	TEClick(local, 0, gDocTE);
}

static void update_DocWin()
{
	SetPortWindowPort(win.ref);
	DrawGrowIcon(win.ref);
	TEUpdate(&win.ref->portRect, gDocTE);
}

static int	message_DocWin(int message, long param)
{	
	switch(message){
	case MW_GROW:
		{
			Rect rect;
			rect=win.ref->portRect;
			//rect.right-=15; rect.bottom-=15;
			rect = win.ref->portRect;
			rect.right  -= 16;
			//rect.bottom -= 15;
			(**gDocTE).viewRect= (**gDocTE).destRect= rect;
			//(**gDocTE).destRect.right = (**gDocTE).destRect.right - 14;
			TECalText(gDocTE);
			TESetSelect((**gDocTE).teLength, (**gDocTE).teLength, gDocTE);
			return 0;
		}
	case MW_DOC_SET:
		{
			const char	*midiname=(char*)param;
			char		*p, docname[256];

			//midiname= (char*)param;
		    strcpy(docname, midiname, 256);
	    	if((p = strrchr(docname, '.')) == NULL){
				return 0;
		    }
            else if(p - docname >= 256 - 4) {
                /* cannot strcpy: that cause buffer overrun */
            }
		    if('A' <= p[1] && p[1] <= 'Z')	strcpy(p + 1, "DOC");
	    						else		strcpy(p + 1, "doc");

	    	TEReadFile( docname, gDocTE );
			break;
		}
	}

	return -1;  //not supported
}

#undef win
// *****************************************************
#pragma mark -
#pragma mark ================== spec Window

static int open_SpecWin();
static void click_SpecWin(Point local, short modifiers);
static void update_SpecWin();
static void goaway_SpecWin(MacWindow*);
static int	message_SpecWin(int message, long param);

#define win mac_SpecWindow
MacWindow win={
	0,	//WindowRef
	open_SpecWin,
	click_SpecWin,
	update_SpecWin,
	goaway_SpecWin,
	close_default,
	message_SpecWin,
	0, 100,100
};

static int open_SpecWin()
		/*success-> return 0;*/
{
#ifdef SUPPORT_SOUNDSPEC
	open_window(&win, kSpecWinID);
	position_window(&win);
	
	//always 
	//if( win.show ){ //Preference on
		open_soundspec();
	   	soundspec_update_wave(NULL, 0);
	//}
#endif /* SUPPORT_SOUNDSPEC */
	return 0;
}

static void click_SpecWin(Point /*local*/, short /*modifiers*/)
{
}

static void update_SpecWin()
{
	SetPortWindowPort(win.ref);
}

static void goaway_SpecWin(MacWindow *macwin)
{
	goaway_default(macwin);
	//close_soundspec();
}

static int	message_SpecWin(int message, long /*param*/)
{
	Rect rect;
	
	switch(message){
	case MW_GROW:
		rect=win.ref->portRect;
		//rect.right-=15; rect.bottom-=15;
		rect = win.ref->portRect;
		rect.right  -= 15;
		//rect.bottom -= 15;
		return 0;
	
	}

	return -1;  //not supported
}

#undef win
// *****************************************************
#pragma mark -
static MacWindow* WindowList[]={
	&mac_PlayerWindow,
	&mac_LogWindow,
	&mac_ListWindow,
	&mac_WrdWindow,
	&mac_DocWindow,
	&mac_SpecWindow,
	&mac_TraceWindow,
	&mac_SkinWindow,
	0
};

void DoUpdate(WindowRef win_ref)
{
	MacWindow* macwin;
	
	macwin = (MacWindow*)GetWRefCon(win_ref);
	if( macwin ) macwin->update();
}

static void mac_AdjustMenus(short modifiers)
{
	MenuHandle	filenemu=GetMenu(mFile),
				synthemu=GetMenu(mSynth);
	
	
	//CheckItem(filenemu, iLogWindow  & 0xFFFF, gShowMsg);
	//CheckItem(filenemu, iListWindow & 0xFFFF, gShowList);
	//CheckItem(filenemu, iWrdWindow  & 0xFFFF, gShowWrd);
	if( modifiers & optionKey ){
		//EnableItem(filenemu, iSkinWindow);
		EnableItem(synthemu, iQuickTime);
		EnableItem(synthemu, iOMS);
	}
}

void HandleMouseDown(EventRecord *event)
{
	short	part;
	WindowRef	window;
	MacWindow   *macwin;
	Rect		growRect={100,100,32000,32000};
	long		size;
	
	part=FindWindow(event->where, &window);
	macwin = (MacWindow*)GetWRefCon(window);
	
	switch(part)
	{
	case inMenuBar:
		mac_AdjustMenus(event->modifiers);
		mac_HandleMenuSelect(MenuSelect(event->where), event->modifiers);
		HiliteMenu(0);
		break;
	case inContent:
		SetPortWindowPort(window);
		SelectWindow(window);
		GlobalToLocal(&event->where);
		if(macwin) macwin->click(event->where, event->modifiers);
		break;
	case inDrag:
		DragWindow(window, event->where, &qd.screenBits.bounds);
		break;
	case inGrow:
		SetPortWindowPort(window);
		size=GrowWindow(window, event->where, &growRect);
		if( size )
		{
			SizeWindow(window, size&0x0000FFFF, size>>16, 0);
			EraseRect(&GetWindowPort(window)->portRect);
			InvalRect(&GetWindowPort(window)->portRect);
			if( macwin ) macwin->message(MW_GROW, size);
		}
		break;
	case inGoAway:
		if( TrackGoAway(window, event->where) ){
			if( macwin ){
				macwin->goaway(macwin);
			}
		}
		break;
	//case inZoomIn:
	//case inZoomOut:
	//	break;
	}
}

// *****************************************************
#pragma mark -
#pragma mark ===============control function 


static int ctl_open(int /*using_stdin*/, int /*using_stdout*/)
		/*success-> return 0;*/
{
	int i, err;
	
	for(i=0; WindowList[i]; i++){
		err=WindowList[i]->open();
		if(err) break;
	}
	if( !err ) ctl.opened=1;
	return err;
}

static void ctl_close(void)
{
	int i;
	for( i=0; WindowList[i]; i++ ){
		WindowList[i]->close(WindowList[i]);
	}
	
	ctl.opened=0;
	return;
}


static void ctl_pass_playing_list(int init_number_of_files,
				  char * /*init_list_of_files*/ [])
{
	EventRecord	event;

	if( init_number_of_files!=0 ){
		cmsg(CMSG_FATAL, VERB_NORMAL,
		  "ctl_pass_playing_list: Sorry. Fatal error.");
	}
	
#ifdef MAC_USE_OMS
	mac_oms_setup();
#endif
	
	{
		FSSpec	spec;
		OSErr	err;
		err=FSMakeFSSpec(0, 0, MAC_STARTUP_FOLDER_NAME, &spec);
		if( err==noErr ){ mac_add_fsspec( &spec ); }
	}
	
	gQuit=false;
	while(!gQuit)
	{
		WaitNextEvent(everyEvent, &event, 1,0);
		mac_HandleEvent(&event);
	}	
	Do_Quit();
}

static Boolean UserWantsControl()
{
	/* Mouse down¡¤or key down¡¥*/
	KeyMap km;

	GetKeys(km);
	km[1] &= ~0x02; /* exclude caps lock */
	return   Button() || km[0]|| km[1] || km[2] || km[3] || skin_state==PAUSE;
}

static int ctl_read(int32* /*valp*/)
{
	int			ret;
	
	//if( gCursorIsWatch ){ gCursorIsWatch=false; InitCursor(); }
	if( gQuit ) Do_Quit();	/* Quit Apple event occured */
	if( mac_rc ){ret=mac_rc; mac_rc=0; return ret;}
	if( !gBusy || UserWantsControl()){
		YieldToAnyThread();
	}
	return RC_NONE;
}

static void ctl_lyric(int lyricid)
{
	char *lyric;

	lyric = event2string(lyricid);
	if(lyric == NULL) return;
	ctl.cmsg(CMSG_TEXT, VERB_VERBOSE, "%s", lyric + 1);
}

static void ctl_gslcd(int lyricid)
{
	char *lyric;

	lyric = event2string(lyricid);
	if(lyric == NULL) return;
	
	if( lyric[0] == ME_GSLCD ){
		int i,j, data, mask;
		char tmp[3]= "00";
		
		lyric++;
		for( j=0; j<4; j++ ){
		for( i=0; i<16; i++ ){
			tmp[0]= lyric[0]; tmp[1]= lyric[1]; lyric+=2;
			sscanf(tmp, "%X", &data);
			mask=0x10;
			ctl_note((data&mask)? VOICE_ON:-1, i, 40+j*10, 127);
			ctl_note((data&mask)? VOICE_ON:-1, i, 41+j*10, 127); mask>>=1;
			ctl_note((data&mask)? VOICE_ON:-1, i, 42+j*10, 127);
			ctl_note((data&mask)? VOICE_ON:-1, i, 43+j*10, 127); mask>>=1;
			ctl_note((data&mask)? VOICE_ON:-1, i, 44+j*10, 127);
			ctl_note((data&mask)? VOICE_ON:-1, i, 45+j*10, 127); mask>>=1;
			ctl_note((data&mask)? VOICE_ON:-1, i, 46+j*10, 127);
			ctl_note((data&mask)? VOICE_ON:-1, i, 47+j*10, 127); mask>>=1;
			ctl_note((data&mask)? VOICE_ON:-1, i, 48+j*10, 127);
			ctl_note((data&mask)? VOICE_ON:-1, i, 49+j*10, 127); mask>>=1;
		}
		}
		return;
	}
	//else normal text
	ctl.cmsg(CMSG_TEXT, VERB_VERBOSE, "%s", lyric + 1);
}

static int cmsg(int type, int verbosity_level, char * fmt, ...)
{
#define BUFSIZE 1024
  char	buf[BUFSIZE];
  va_list ap;

  if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
      ctl.verbosity<verbosity_level){
		return 0;
	}
  va_start(ap, fmt);
    {
    	vsnprintf(buf, BUFSIZE, fmt, ap);
		if(mac_LogWindow.ref ){
			PrintLogStr(buf);
			PrintLogStr("\r");
		}
		if( !mac_LogWindow.ref || type==CMSG_FATAL){
      		StopAlertMessage(c2pstr(buf));  /* no Window or Fatal ERR*/
    	}
	}
  	va_end(ap);
  	return 0;
}

static void ctl_refresh(void)
{}

static void ctl_total_time(int tt)
{
	mac_trc_update_time( -1, tt/play_mode->rate);
	mac_SkinWindow.message(MW_SKIN_TOTAL_TIME, tt/play_mode->rate);
}

static void ctl_master_volume(int /*mv*/) {}

static void ctl_file_name(char *name)
{
	int	i;
	char	*s;
	
	for( i=strlen(name); i>=0; i--)
		if( name[i]==PATH_SEP ) {
			s=&name[i]+1; //remove pathname
			break;
		}
	snprintf(fileStr, FILESTR_LEN, "%d. %s", nPlaying+1, s);
	
	if (ctl.verbosity>=0 || ctl.trace_playing){
		DrawFileStr();
	}
	mac_ListWindow.message(MW_LIST_SELECT, nPlaying);
	mac_DocWindow.message(MW_DOC_SET, (long)name);
	mac_trc_update_all_info();
	mac_SkinWindow.message(MW_SKIN_FILENAME, (long)fileStr);
}

static void ctl_current_time(int current, int /*v*/)
{
	static int	lastSec=-1, lastYieldThread=-1;
	int			mins, secs, realSecs;
	
	if( gStartTick==0 ) gStartTick=TickCount();
						/*select*/
	realSecs=(TickCount()-gStartTick)/60; /*real time*/
	secs=current;

	if( secs!=lastSec && (skin_state==PLAYING || skin_state==PAUSE))
	{
		lastSec=secs;
		mins=secs/60;
		secs-=mins*60;
		snprintf(timeStr, TIMESTR_LEN, "%02d:%02d", mins, secs);
		DrawTimeStr();
		mac_trc_update_time(current,-1);
		mac_SkinWindow.message(MW_SKIN_TIME, lastSec);
	}
	if( realSecs!=lastYieldThread ){
		lastYieldThread= realSecs;
		mac_trc_update_time(current,-1);
		mac_trc_update_voices();
		if( (realSecs % (evil_level*evil_level))==0 )
			YieldToAnyThread();		// YieldToAnyThread every 1sec
	}
}

static void ctl_note(int status, int ch, int note, int vel)
{
	if( ! mac_TraceWindow.show ) return;
    if(note == -1)
    {
	//if(ctl.trace_playing && !midi_trace.flush_flag)
	//    push_midi_trace0(update_indicator);
	return;
    }
    //push_midi_note(v_ctl_note, note);
    v_ctl_note( status, ch, note, vel);
}

static void ctl_program(int ch, int val, char *comm){mac_ctl_program(ch,val,comm);}

static void ctl_volume(int /*channel*/, int /*val*/) {}

static void ctl_expression(int /*channel*/, int /*val*/) {}

static void ctl_panning(int /*channel*/, int /*val*/) {}

static void ctl_sustain(int /*channel*/, int /*val*/) {}

static void ctl_pitch_bend(int /*channel*/, int /*val*/) {}

static void ctl_reset(void)
{
	mac_ctl_reset_trc();
}

static void ctl_event(CtlEvent *e)
{
    switch(e->type)
    {
      case CTLE_NOW_LOADING:
	ctl_file_name((char *)e->v1);
	break;
      case CTLE_LOADING_DONE:
	break;
      case CTLE_PLAY_START:
	ctl_total_time((int)e->v1);
	break;
      case CTLE_PLAY_END:
	break;
      case CTLE_TEMPO:
	break;
      case CTLE_METRONOME:
	break;
      case CTLE_CURRENT_TIME:
	ctl_current_time((int)e->v1, (int)e->v2);
	break;
      case CTLE_NOTE:
	ctl_note((int)e->v1, (int)e->v2, (int)e->v3, (int)e->v4);
	break;
      case CTLE_MASTER_VOLUME:
	ctl_master_volume((int)e->v1);
	break;
      case CTLE_PROGRAM:
	ctl_program((int)e->v1, (int)e->v2, (char *)e->v3);
	break;
      case CTLE_VOLUME:
	ctl_volume((int)e->v1, (int)e->v2);
	break;
      case CTLE_EXPRESSION:
	ctl_expression((int)e->v1, (int)e->v2);
	break;
      case CTLE_PANNING:
	ctl_panning((int)e->v1, (int)e->v2);
	break;
      case CTLE_SUSTAIN:
	ctl_sustain((int)e->v1, (int)e->v2);
	break;
      case CTLE_PITCH_BEND:
	ctl_pitch_bend((int)e->v1, (int)e->v2);
	break;
      case CTLE_MOD_WHEEL:
	ctl_pitch_bend((int)e->v1, e->v2 ? -1 : 0x2000);
	break;
      case CTLE_CHORUS_EFFECT:
	break;
      case CTLE_REVERB_EFFECT:
	break;
      case CTLE_LYRIC:
	ctl_lyric((int)e->v1);
	break;
      case CTLE_GSLCD:
	ctl_gslcd((int)e->v1);
	break;
      case CTLE_REFRESH:
	ctl_refresh();
	break;
      case CTLE_RESET:
	ctl_reset();
	break;
#ifdef SUPPORT_SOUNDSPEC
      case CTLE_SPEANA:
        ctl_speana_data((double *)e->v1, (int)e->v2);
      break;
#endif /* SUPPORT_SOUNDSPEC */
    }
}

