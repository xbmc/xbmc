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
	
    mac_main.h
*/

#ifndef MAC_MAIN_H
#define MAC_MAIN_H

#include	<Sound.h>
#include	<Drag.h>

#include "mfnode.h"

extern SndChannelPtr	gSndCannel;

extern Boolean	skin_f_repeat, gQuit,gBusy, gCursorIsWatch,
				gHasDragMgr, gShuffle;
extern int		mac_rc;
extern short	mac_amplitude;				
extern long		gStartTick;
extern int		skin_state, mac_n_files, nPlaying;
extern double	gSilentSec;

#define		LISTSIZE	1000
typedef struct _MidiFile{
//	FSSpec	spec;
	char	*filename;
	MFnode	*mfn;
} MidiFile;
extern MidiFile		fileList[];

// **************************************
void	mac_ErrorExit(Str255 msg);

void	mac_HandleEvent(EventRecord*);
void	mac_HandleControl();
void	HandleMouseDown(EventRecord *event);
void	mac_HandleMenuSelect(long select, short modifiers);

OSErr	mac_SetPlayOption();
void	mac_DefaultOption();
OSErr	mac_GetPreference();
OSErr	mac_SetPreference();

void	AddHFS2PlayList(HFSFlavor*);
OSErr	AddFolder2PlayList( short vRefNum, long dirID);
void	DoVolume();
void	DrawButton();
void	DoUpdate(WindowRef);
void	DoQuit();
void	add_ListWin(MidiFile * file);
void	change_ListRow( short row, const MidiFile* file);
void	init_ListWin();
void 	HandleSpecKeydownEvent(long message, short /*modifiers*/);
void	ShuffleList(int start, int end);

void	mac_add_fsspec( FSSpec *spec );
void	read_viscolor(const char * viscolor_file);
/******************************/
enum{
	WAITING,	/* waiting at end of list*/
	PLAYING,
	PAUSE,
	STOP
};
/* ************************ */
/*	resurece */
enum{
	mApple=0x0080,
	iAbout=	0x00800001,
	
	mFile=		0x0081,
	iOpen=		0x00810001,
	iClose=		0x00810002,
	//-                  3,
	iLogWindow=	0x00810004,
	iListWindow=0x00810005,
	iWrdWindow=	0x00810006,
	iDocWindow=	0x00810007,
	iSpecWindow=0x00810008,
	iTraceWindow=0x00810009,
	iSkinWindow=0x0081000A,
	//-                  B,
	//-                  C,	
	iSaveAs=	0x0081000D,
	iPref=		0x0081000E,
	//-                  F,
	iQuit=		0x00810010,

	mPlay=0x0082,
	iPlay=0x00820001,
	iStop=0x00820002,
	iPause=0x00820003,
	//-            4
	iPrev=0x00820005,
	iNext=0x00820006,
	
	mSynth=0x00A0,
	iTiMidity=0x00A00001,
	iQuickTime=0x00A00002,
	iOMS=0x00A00003
	
};

#define	kPlayerWinID	128
#define	kLogWinID		129
#define kListWinID		130
#define	kWrdWinID		131
#define	kDocWinID		132
#define kSpecWinID		133
#define kTraceWinID		134
#define kSkinWinID		135
#define kOmsWinID		136

enum{
	MW_NOMSG=0,
	MW_GROW=1
};

typedef struct MacWindow_ {
	WindowRef	ref;
	int			(*open)();
						//return 0 if no error
	void		(*click)(Point local, short modifiers);
	void		(*update)();
	void		(*goaway)(struct MacWindow_* mwin);
	void		(*close)(struct MacWindow_* mwin);
	int			(*message)(int message, long param);
						//return -1 if message is not supported
	int			show, X, Y, width, hight, opened;
	
}MacWindow;

extern MacWindow 	mac_PlayerWindow,
					mac_LogWindow,
					mac_ListWindow,
					mac_WrdWindow,
					mac_DocWindow,
					mac_SpecWindow,
					mac_TraceWindow,
					mac_SkinWindow;

#define SHOW_WINDOW(mwin) {ShowWindow(mwin.ref);SelectWindow(mwin.ref);mwin.show=true;}

#ifdef __POWERPC__
#define PREF_FILENAME "\pTiMidity++ pref"
#elif __MC68881__
#define PREF_FILENAME "\pTiMidity++68kFPU pref"
#else
#define PREF_FILENAME "\pTiMidity++68k pref"
#endif

extern int evil_level;
#define EVIL_NORMAL 	1
#define EVIL_SUPER		2
#define EVIL_SPECIAL 	3

extern int		do_initial_filling;
extern volatile int	mac_buf_using_num, mac_flushing_flag;

#endif //MAC_MAIN_H
