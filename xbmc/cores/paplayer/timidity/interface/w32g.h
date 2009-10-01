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

#ifndef ___W32G_H_
#define ___W32G_H_

#include <process.h>
#ifdef RC_NONE
#undef RC_NONE
#endif
#include <windows.h>
#ifdef RC_NONE
#undef RC_NONE
#endif
#define RC_NONE	0

#define LANGUAGE_JAPANESE				0x0001
#define LANGUAGE_ENGLISH				0x0002
extern int PlayerLanguage;
extern int PlayerMode;


#ifndef MAXPATH
#define MAXPATH 256
#endif /* MAXPATH */


typedef struct argc_argv_t_ {
	int argc;
	char **argv;
} argc_argv_t;

#if defined(_MSC_VER) || defined(__WATCOMC__)
//typedef void (__cdecl *MSVC_BEGINTHREAD_START_ADDRESS)(void *);
typedef LPTHREAD_START_ROUTINE MSVC_BEGINTHREAD_START_ADDRESS;
#elif defined(_BORLANDC_)
// typedef _USERENTRY (*BCC_BEGINTHREAD_START_ADDRESS)(void *);
typedef LPTHREAD_START_ROUTINE BCC_BEGINTHREAD_START_ADDRESS;
#endif

// beginthread for C RUNTIME LIBRARY

// HANDLE crt_beginthread(LPTHREAD_START_ROUTINE start_address, DWORD stack_size, LPVOID arglist);
#if defined(_MSC_VER) || defined(__WATCOMC__)
#define crt_beginthread(start_address,stack_size,arglist) \
(HANDLE)_beginthread((MSVC_BEGINTHREAD_START_ADDRESS)start_address,(unsigned)stack_size,(void *)arglist)
#elif defined(_BORLANDC_)
#define crt_beginthread(start_address,stack_size,arglist) \
(HANDLE)_beginthread((BCC_BEGINTHREAD_START_ADDRESS)start_address,(unsigned)stack_size,(void *)arglist)
#else
#define crt_beginthread(start_address,stack_size,arglist) \
(HANDLE)CreateThread(NULL,(DWORD)stack_size,(LPTHREAD_START_ROUTINE)start_address,(LPVOID)arglist,0,&dwTmp)
#endif

// (HANDLE)crt_beginthreadex(LPSECURITY_ATTRIBUTES security, DWORD stack_size, LPTHREAD_START_ROUTINE start_address, LPVOID arglist, DWORD initflag, LPDWORD thrdaddr );
#if defined(_MSC_VER) || defined(__WATCOMC__)
#define crt_beginthreadex(security,stack_size,start_address,arglist,initflag,thrdaddr ) \
(HANDLE)_beginthreadex((void *)security,(unsigned)stack_size,(MSVC_BEGINTHREAD_START_ADDRESS)start_address,(void *)arglist,(unsigned)initflag,(unsigned *)thrdaddr)
#elif defined(_BORLANDC_)
#define crt_beginthreadex(security,stack_size,start_address,arglist,initflag,thrdaddr ) \
(HANDLE)_beginthreadNT((BCC_BEGINTHREAD_START_ADDRESS)start_address,(unsigned)stack_size,(void *)arglist,(void *)security_attrib,(unsigned long)create_flags,(unsigned long *)thread_id)
#else
#define crt_beginthreadex(security,stack_size,start_address,arglist,initflag,thrdaddr ) \
(HANDLE)CreateThread((LPSECURITY_ATTRIBUTES)security,(DWORD)stack_size,(LPTHREAD_START_ROUTINE)start_address,(LPVOID)arglist,(DWORD)initflag,(LPDWORD)thrdaddr)
#endif

#if defined(_MSC_VER) || defined(__WATCOMC__)
#define crt_endthread() _endthread()
#elif defined(_BORLANDC_)
#define crt_endthread() _endthread()
#else
#define crt_endthread() ExitThread(0);
#endif

#define RANGE(x,min,max) (((x)<(min))?((x)=(min)):(((x)>(max))?((x)=(max)):(x)))

#define RC_EXT_BASE 1000
enum {
    RC_EXT_DROP = RC_EXT_BASE,
    RC_EXT_LOAD_FILE,
    RC_EXT_LOAD_PLAYLIST,
    RC_EXT_SAVE_PLAYLIST,
    RC_EXT_MODE_CHANGE,
    RC_EXT_APPLY_SETTING,
    RC_EXT_DELETE_PLAYLIST,
    RC_EXT_UPDATE_PLAYLIST,
    RC_EXT_UNIQ_PLAYLIST,
    RC_EXT_REFINE_PLAYLIST,
    RC_EXT_JUMP_FILE,
    RC_EXT_ROTATE_PLAYLIST,
    RC_EXT_CLEAR_PLAYLIST,
    RC_EXT_OPEN_DOC,
    RC_EXT_RESTART_GUI,
	RC_EXT_LOAD_FILES_AND_PLAY
};

#define MAX_W32G_MIDI_CHANNELS	32


// Toolbar Macros
#define IDM_STOP		2501
#define IDM_PAUSE		2502
#define IDM_PREV		2503
#define IDM_FOREWARD	2504
#define IDM_PLAY		2505
#define IDM_BACKWARD	2506
#define IDM_NEXT		2507

#define IDM_CONSOLE		2511
#define IDM_LIST		2512
#define IDM_TRACER	 	2513
#define IDM_DOC			2514
#define IDM_WRD			2515
#define IDM_SOUNDSPEC	2516


#define FLAG_NOTE_OFF	1
#define FLAG_NOTE_ON	2

#define FLAG_BANK	0x0001
#define FLAG_PROG	0x0002
#define FLAG_PAN	0x0004
#define FLAG_SUST	0x0008

#define FLAG_NOTE_OFF	1
#define FLAG_NOTE_ON	2

#define FLAG_BANK	0x0001
#define FLAG_PROG	0x0002
#define FLAG_PAN	0x0004
#define FLAG_SUST	0x0008

typedef struct {
	int reset_panel;
	int wait_reset;
	int multi_part;

	char v_flags[MAX_W32G_MIDI_CHANNELS];
	int16 cnote[MAX_W32G_MIDI_CHANNELS];
	int16 cvel[MAX_W32G_MIDI_CHANNELS];
	int16 ctotal[MAX_W32G_MIDI_CHANNELS];
	char c_flags[MAX_W32G_MIDI_CHANNELS];
	Channel channel[MAX_W32G_MIDI_CHANNELS];

	int32 total_time;
	int total_time_h;
	int total_time_m;
	int total_time_s;
	int total_time_ss;
	int32 cur_time;
	int cur_time_h;
	int cur_time_m;
	int cur_time_s;
	int cur_time_ss;
	int cur_voices;
	int voices;
	int upper_voices;
	char filename[MAXPATH + 64];
	char titlename[MAXPATH + 64];
	int filename_setflag;
	int titlename_setflag;
	int32 master_volume;
	int32 master_volume_max;
	int meas;
	int beat;
	char keysig[7];
	int key_offset;
	int tempo;
	int tempo_ratio;
	int invalid_flag;

	int32 xnote[MAX_W32G_MIDI_CHANNELS][4];
	int aq_ratio;

	int changed;
	char dummy[1024];

	int8 GSLCD[16][16];
	double gslcd_last_display_time;
	int8 gslcd_displayed_flag;
} PanelInfo;
extern PanelInfo *Panel;

#define PANELRESET_TIME			0x0001
#define PANELRESET_CHANNEL		0x0002
#define PANELRESET_MIDIINFO	0x0004
#define PANELRESET_MISC			0x0008
#define PANELRESET_EFFECT		0x0010


#define CANVAS_MODE_GSLCD		0x0001
#define CANVAS_MODE_MAP16		0x0002
#define CANVAS_MODE_MAP32		0x0003
#define CANVAS_MODE_KBD_A		0x0004
#define CANVAS_MODE_KBD_B		0x0005
#define CANVAS_MODE_SLEEP		0x0006

#if 0
#define TMCCC_BLACK	RGB(0x00,0x00,0x00)
#define TMCCC_WHITE	RGB(0xff,0xff,0xff)
#define TMCCC_RED	RGB(0xff,0x00,0x00)

#define TMCCC_FORE	TMCCC_BLACK // Aliased
#define TMCCC_BACK 	RGB(0x00, 0xf0, 0x00)
#define TMCCC_LOW	RGB(0x80, 0xd0, 0x00)
#define TMCCC_MIDDLE	RGB(0xb0, 0xb0, 0x00)
#define TMCCC_HIGH	RGB(0xe0, 0x00, 0x00)

enum {
    TMCC_BLACK, // Aliased FORE
    TMCC_WHITE,
    TMCC_RED,
    TMCC_BACK,
    TMCC_LOW,
    TMCC_MIDDLE,
    TMCC_HIGH,
    TMCC_FORE_HALF,
    TMCC_LOW_HALF,
    TMCC_MIDDLE_HALF,
    TMCC_HIGH_HALF,
    TMCC_FORE_WEAKHALF,
    TMCC_SIZE
};
#define TMCC_FORE TMCC_BLACK // Aliased

typedef struct _TmColors {
    COLORREF color;
    HPEN pen;
    HBRUSH brush;
} TmColors;
#endif

/* w32g_i.c */
extern int w32g_open(void);
extern void w32g_close(void);
extern void w32g_send_rc(int rc, int32 value);
extern int w32g_get_rc(int32 *value, int wait_if_empty);
extern void w32g_lock(void);
extern void w32g_unlock(void);
extern void MainWndScrollbarProgressUpdate(int sec);
extern void PutsConsoleWnd(char *str);
extern void w32g_ctle_play_start(int sec);
extern void SettingWndApply(void);
extern int w32g_lock_open_file;
extern void w32g_i_init();
extern void CanvasChange(int mode);
extern HINSTANCE hInst;
extern void w32g_show_console();
extern void MPanelStartLoad(char *filename);


/* w32g_utl.c */

/* w32g_playlist.c */
extern int w32g_add_playlist(int nfiles, char **files, int expand_flag,
			     int autouniq, int autorefine);
extern char *w32g_get_playlist(int idx);
extern int w32g_next_playlist(int skip_invalid_file);
extern int w32g_prev_playlist(int skip_invalid_file);
extern int w32g_random_playlist(int skip_invalid_file);
extern int w32g_shuffle_playlist_reset(int preserve);
extern int w32g_shuffle_playlist_next(int skip_invalid_file);
extern void w32g_first_playlist(int skip_invalid_file);
extern int w32g_isempty_playlist(void);
extern char *w32g_curr_playlist(void);
extern void w32g_update_playlist(void);
extern void w32g_get_playlist_index(int *selected, int *nfiles, int *cursel);
extern int w32g_goto_playlist(int num, int skip_invalid_file);
extern int w32g_delete_playlist(int pos);
extern int w32g_nvalid_playlist(void);
extern int w32g_ismidi_playlist(int n);
extern void w32g_setcur_playlist(void);
extern int w32g_refine_playlist(int *is_selected_removed);
extern int w32g_uniq_playlist(int *is_selected_removed);
extern void w32g_clear_playlist(void);
extern void w32g_rotate_playlist(int dest);

#if 0
/* w32g_panel.c */
extern void w32g_init_panel(HWND hwnd);
extern void TmPanelStartToLoad(char *filename);
extern void TmPanelStartToPlay(int total_sec);
extern void TmPanelSetVoices(int v);
//extern void TmPanelInit(HWND hwnd);
extern void TmPanelRefresh(void);
extern void TmPanelSetTime(int sec);
extern void TmPanelSetMasterVol(int v);
extern void TmPanelUpdateList(void);

/* w32g_canvas.c */
extern void w32g_init_canvas(HWND hwnd);
extern void TmCanvasRefresh(void);
extern void TmCanvasReset(void);
extern void TmCanvasNote(int status, int ch, int note, int vel);
extern int TmCanvasChange(void);
extern void TmCanvasUpdateInterval();
extern int TmCanvasMode;
#endif

/* w32g_c.c */
extern volatile int w32g_play_active;
extern int w32g_current_volume[/* MAX_CHANNELS */];
extern int w32g_current_expression[/* MAX_CHANNELS */];
extern volatile int w32g_restart_gui_flag;


void PrefSettingApplyReally(void);



// flags
extern int InitMinimizeFlag;
extern int DebugWndStartFlag;
extern int ConsoleWndStartFlag;
extern int ListWndStartFlag;
extern int TracerWndStartFlag;
extern int DocWndStartFlag;
extern int WrdWndStartFlag;
extern int DebugWndFlag;
extern int ConsoleWndFlag;
extern int ListWndFlag;
extern int TracerWndFlag;
extern int DocWndFlag;
extern int WrdWndFlag;
extern int SoundSpecWndFlag;

extern int SubWindowMax;

extern char *IniFile;
extern char *ConfigFile;
extern char *PlaylistFile;
extern char *PlaylistHistoryFile;
extern char *MidiFileOpenDir;
extern char *ConfigFileOpenDir;
extern char *PlaylistFileOpenDir;

extern int PlayerThreadPriority;
extern int GUIThreadPriority;

extern int WrdGraphicFlag;
extern int TraceGraphicFlag;
extern int DocMaxSize;
extern char *DocFileExt;

extern int w32g_has_ini_file;
extern char *w32g_output_dir;
extern int w32g_auto_output_mode;

// HWND
extern HWND hMainWnd;
extern HWND hDebugWnd;
extern HWND hConsoleWnd;
extern HWND hTracerWnd;
extern HWND hDocWnd;
extern HWND hListWnd;
extern HWND hWrdWnd;
extern HWND hSoundSpecWnd;
extern HWND hDebugEditWnd;
extern HWND hDocEditWnd;

// gdi_lock
extern int gdi_lock(void);
extern int gdi_unlock(void);
#define GDI_SAFETY(command) (gdi_lock(),(command),gdi_unlock);

#define W32G_RANDOM_IS_SHUFFLE

#ifndef BELOW_NORMAL_PRIORITY_CLASS	/* VC6.0 doesn't support them. */
#define BELOW_NORMAL_PRIORITY_CLASS 0x4000
#define ABOVE_NORMAL_PRIORITY_CLASS 0x8000
#endif /* BELOW_NORMAL_PRIORITY_CLASS */

#endif
