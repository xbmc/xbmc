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

	w32g_pref.c: Written by Daisuke Aoki <dai@y7.net>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <stddef.h>
#include <windows.h>
#undef RC_NONE

#include <commctrl.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"
#include "controls.h"
#include "tables.h"
#include "miditrace.h"
#include "reverb.h"
#ifdef SUPPORT_SOUNDSPEC
#include "soundspec.h"
#endif /* SUPPORT_SOUNDSPEC */
#include "recache.h"
#include "arc.h"
#include "strtab.h"
#include "wrd.h"
#include "mid.defs"

#include "w32g.h"
#include "w32g_res.h"
#include "w32g_utl.h"
#include "w32g_pref.h"

#ifdef AU_GOGO
/* #include <musenc.h>		/* for gogo */
#include <gogo/gogo.h>		/* for gogo */
#include "gogo_a.h"
#endif

/* TiMidity Win32GUI preference / PropertySheet */

#ifndef IA_W32G_SYN
extern void w32g_restart(void);
#endif
extern void set_gogo_opts_use_commandline_options(char *commandline);

extern void restore_voices(int save_voices);

extern void TracerWndApplyQuietChannel( ChannelBitMask quietchannels_ );

volatile int PrefWndDoing = 0;

static void PrefSettingApply(void);

static volatile int PrefWndSetOK = 0;
static HWND hPrefWnd = NULL;
static BOOL APIENTRY CALLBACK PrefWndDialogProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam);
static BOOL APIENTRY PrefPlayerDialogProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam);
static BOOL APIENTRY PrefTiMidity1DialogProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam);
static BOOL APIENTRY PrefTiMidity2DialogProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam);
static BOOL APIENTRY PrefTiMidity3DialogProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam);
static BOOL APIENTRY PrefTiMidity4DialogProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam);
#ifdef IA_W32G_SYN
static BOOL APIENTRY PrefSyn1DialogProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam);
#endif
static int DlgOpenConfigFile(char *Filename, HWND hwnd);
static int DlgOpenOutputFile(char *Filename, HWND hwnd);

static int vorbisCofigDialog(void);
static int gogoCofigDialog(void);

//#if defined(__CYGWIN32__) || defined(__MINGW32__)
#if 0 /* New version of mingw */
//#define pszTemplate	u1.pszTemplate
//#define pszIcon		u2.pszIcon
#ifndef NONAMELESSUNION
#define NONAMELESSUNION
#endif
#ifndef DUMMYUNIONNAME
#define DUMMYUNIONNAME	u1
#endif
#ifndef DUMMYUNIONNAME2
#define DUMMYUNIONNAME2	u2
#endif
#ifndef DUMMYUNIONNAME3
#define DUMMYUNIONNAME3	u3
#endif
#endif

#ifdef IA_W32G_SYN 
static char **GetMidiINDrivers ( void );
#endif

#define WM_MYSAVE (WM_USER + 100)
#define WM_MYRESTORE (WM_USER + 101)

typedef struct pref_page_t_ {
	int index;
	char *title;
	HWND hwnd;
	UINT control;
	DLGPROC dlgproc;
	int opt;
} pref_page_t;

static pref_page_t pref_pages_ja[] = {
	{ 0, "プレイヤ", (HWND)NULL, IDD_PREF_PLAYER, (DLGPROC) PrefPlayerDialogProc, 0 },
	{ 1, "エフェクト", (HWND)NULL, IDD_PREF_TIMIDITY1, (DLGPROC) PrefTiMidity1DialogProc, 0 },
	{ 2, "その他", (HWND)NULL, IDD_PREF_TIMIDITY2, (DLGPROC) PrefTiMidity2DialogProc, 0 },
	{ 3, "出力", (HWND)NULL, IDD_PREF_TIMIDITY3, (DLGPROC) PrefTiMidity3DialogProc, 0 },
	{ 4, "チャンネル", (HWND)NULL, IDD_PREF_TIMIDITY4, (DLGPROC) PrefTiMidity4DialogProc, 0 },
#ifdef IA_W32G_SYN
	{ 5, "シンセサイザ", (HWND)NULL, IDD_PREF_SYN1, (DLGPROC) PrefSyn1DialogProc, 0 },
#endif
};

static pref_page_t pref_pages_en[] = {
	{ 0, "Player", (HWND)NULL, IDD_PREF_PLAYER_EN, (DLGPROC) PrefPlayerDialogProc, 0 },
	{ 1, "Effect", (HWND)NULL, IDD_PREF_TIMIDITY1_EN, (DLGPROC) PrefTiMidity1DialogProc, 0 },
	{ 2, "Misc", (HWND)NULL, IDD_PREF_TIMIDITY2_EN, (DLGPROC) PrefTiMidity2DialogProc, 0 },
	{ 3, "Output", (HWND)NULL, IDD_PREF_TIMIDITY3_EN, (DLGPROC) PrefTiMidity3DialogProc, 0 },
	{ 4, "Channel", (HWND)NULL, IDD_PREF_TIMIDITY4_EN, (DLGPROC) PrefTiMidity4DialogProc, 0 },
#ifdef IA_W32G_SYN
	{ 5, "Synthesizer", (HWND)NULL, IDD_PREF_SYN1_EN, (DLGPROC) PrefSyn1DialogProc, 0 },
#endif
};

#ifndef IA_W32G_SYN
#define PREF_PAGE_MAX 5
#else
#define PREF_PAGE_MAX 6
#endif

static pref_page_t *pref_pages;
static void PrefWndCreatePage ( HWND hwnd )
{
	int i;
	RECT rc;
	HWND hwnd_tab;

#ifdef IA_W32G_SYN
	GetMidiINDrivers ();
#endif

	switch(PlayerLanguage) {
		case LANGUAGE_JAPANESE:
			pref_pages = pref_pages_ja;
			break;
		default:
		case LANGUAGE_ENGLISH:
			pref_pages = pref_pages_en;
			break;
	}

	hwnd_tab = GetDlgItem ( hwnd, IDC_TAB_MAIN );
	for ( i = 0; i < PREF_PAGE_MAX; i++ ) {
		TC_ITEM tci;
    tci.mask = TCIF_TEXT;
    tci.pszText = pref_pages[i].title;
		tci.cchTextMax = strlen ( pref_pages[i].title );
		SendMessage ( hwnd_tab, TCM_INSERTITEM, (WPARAM)i, (LPARAM)&tci );
	}
	GetClientRect ( hwnd_tab, &rc );
	SendDlgItemMessage ( hwnd, IDC_TAB_MAIN, TCM_ADJUSTRECT, (WPARAM)0, (LPARAM)&rc );
	for ( i = 0; i < PREF_PAGE_MAX; i++ ) {
		pref_pages[i].hwnd = CreateDialog ( hInst, MAKEINTRESOURCE(pref_pages[i].control),
			hwnd, pref_pages[i].dlgproc );
    ShowWindow ( pref_pages[i].hwnd, SW_HIDE );
		MoveWindow ( pref_pages[i].hwnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE );
	}
}

void PrefWndCreate(HWND hwnd)
{
	VOLATILE_TOUCH(PrefWndDoing);
	if(PrefWndDoing)
		return;
	PrefWndDoing = 1;
	PrefWndSetOK = 1;

	switch(PlayerLanguage) {
		case LANGUAGE_JAPANESE:
			DialogBox ( hInst, MAKEINTRESOURCE(IDD_DIALOG_PREF), hwnd, PrefWndDialogProc );
			break;
		default:
		case LANGUAGE_ENGLISH:
			DialogBox ( hInst, MAKEINTRESOURCE(IDD_DIALOG_PREF_EN), hwnd, PrefWndDialogProc );
			break;
	}
	hPrefWnd = NULL;
	PrefWndSetOK = 0;
	PrefWndDoing = 0;
	return;
}

static BOOL APIENTRY CALLBACK PrefWndDialogProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam)
{
	int i;

	switch (uMess){
	case WM_INITDIALOG:
	{
		hPrefWnd = hwnd;
		PrefWndCreatePage ( hwnd );
		SendDlgItemMessage ( hwnd, IDC_TAB_MAIN, TCM_SETCURSEL, (WPARAM)0, (LPARAM)0 );
		ShowWindow ( pref_pages[0].hwnd, TRUE );
		return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			for ( i = 0; i < PREF_PAGE_MAX; i++ ) {
				SendMessage ( pref_pages[i].hwnd, WM_MYSAVE, (WPARAM)0, (LPARAM)0 );
			}
			PrefSettingApply();
			SetWindowLong(hwnd,	DWL_MSGRESULT, TRUE);
			EndDialog ( hwnd, TRUE );
			return TRUE;
		case IDCANCEL:
			SetWindowLong(hwnd,	DWL_MSGRESULT, FALSE);
			EndDialog ( hwnd, FALSE );
			return TRUE;
		case IDC_BUTTON_APPLY:
			for ( i = 0; i < PREF_PAGE_MAX; i++ ) {
				SendMessage ( pref_pages[i].hwnd, WM_MYSAVE, (WPARAM)0, (LPARAM)0 );
			}
			PrefSettingApply();
#ifndef IA_W32G_SYN
			TracerWndApplyQuietChannel(st_temp->quietchannels);
#endif
			SetWindowLong(hwnd,	DWL_MSGRESULT, TRUE);
			return TRUE;
		}
		break;

	case WM_NOTIFY:
	{
		int idCtrl = (int) wParam; 
    LPNMHDR pnmh = (LPNMHDR) lParam; 
		if ( pnmh->idFrom == IDC_TAB_MAIN ) {
			switch ( pnmh->code ) {
			case TCN_SELCHANGE:
			{
				int nIndex = SendDlgItemMessage ( hwnd, IDC_TAB_MAIN, TCM_GETCURSEL, (WPARAM)0, (LPARAM)0);
				for ( i = 0; i < PREF_PAGE_MAX; i++ ) {
					if ( nIndex == i ) {
						ShowWindow ( pref_pages[i].hwnd, TRUE );
					} else {
						ShowWindow ( pref_pages[i].hwnd, FALSE );
					}
				}
			}
				return TRUE;
			}
		}
		break;
	}

	case WM_SIZE:
	{
		RECT rc;
		HWND hwnd_tab = GetDlgItem ( hwnd, IDC_TAB_MAIN );
		GetClientRect ( hwnd_tab, &rc );
		SendDlgItemMessage ( hwnd, IDC_TAB_MAIN, TCM_ADJUSTRECT, (WPARAM)TRUE, (LPARAM)&rc );
		for ( i = 0; i < PREF_PAGE_MAX; i++ ) {
			MoveWindow ( pref_pages[i].hwnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE );
		}
		return TRUE;
	}

	case WM_CLOSE:
		break;

	default:
	  break;
	}

	return FALSE;
}

//			SetWindowLong(hwnd,	DWL_MSGRESULT, FALSE);

#define DLG_CHECKBUTTON_TO_FLAG(hwnd,ctlid,x)	\
	((SendDlgItemMessage((hwnd),(ctlid),BM_GETCHECK,0,0))?((x)=1):((x)=0))
#define DLG_FLAG_TO_CHECKBUTTON(hwnd,ctlid,x)	\
	((x)?(SendDlgItemMessage((hwnd),(ctlid),BM_SETCHECK,1,0)):\
	(SendDlgItemMessage((hwnd),(ctlid),BM_SETCHECK,0,0)))


extern void TracerWndApplyQuietChannel( ChannelBitMask quietchannels_ );

#ifndef IA_W32G_SYN
/* st_temp, sp_temp を適用する
 * 注意: MainThread からの呼び出し禁止、危険！
 */
void PrefSettingApplyReally(void)
{
	int restart;
	extern int IniFileAutoSave;

	free_instruments(1);
	if(play_mode->fd != -1)
		play_mode->close_output();

	restart = (PlayerLanguage != sp_temp->PlayerLanguage);
//	restart |= (strcmp(sp_temp->ConfigFile,ConfigFile) != 0);
	if(sp_temp->PlayerLanguage == LANGUAGE_JAPANESE)
		strcpy(st_temp->output_text_code, "SJIS");
	else
		strcpy(st_temp->output_text_code, "ASCII");
	ApplySettingPlayer(sp_temp);
	ApplySettingTiMidity(st_temp);
	SaveSettingPlayer(sp_current);
	SaveSettingTiMidity(st_current);
	memcpy(sp_temp, sp_current, sizeof(SETTING_PLAYER));
	memcpy(st_temp, st_current, sizeof(SETTING_TIMIDITY));
	restore_voices(1);
	PrefWndSetOK = 0;
	if(IniFileAutoSave)
		SaveIniFile(sp_current, st_current);
	if(restart &&
	   MessageBox(hListWnd,"Restart TiMidity?", "TiMidity",
				  MB_YESNO)==IDYES)
	{
		w32g_restart();
//		PrefWndDoing = 0;
	}
}
#endif

#ifdef IA_W32G_SYN
extern int w32g_syn_do_before_pref_apply ( void );
extern int w32g_syn_do_after_pref_apply ( void );
#endif

extern int IniFileAutoSave;
static void PrefSettingApply(void)
{
#ifndef IA_W32G_SYN
	 w32g_send_rc(RC_EXT_APPLY_SETTING, 0);
#else
	int before_pref_apply_ok;
	before_pref_apply_ok = ( w32g_syn_do_before_pref_apply () == 0 );
	ApplySettingPlayer(sp_temp);
	ApplySettingTiMidity(st_temp);
	SaveSettingPlayer(sp_current);
	SaveSettingTiMidity(st_current);
	memcpy(sp_temp, sp_current, sizeof(SETTING_PLAYER));
	memcpy(st_temp, st_current, sizeof(SETTING_TIMIDITY));
	if(IniFileAutoSave)
		SaveIniFile(sp_current, st_current);
	if ( before_pref_apply_ok )
		w32g_syn_do_after_pref_apply ();
	PrefWndSetOK = 0;
//	PrefWndDoing = 0;
#endif
}

void reload_cfg(void)
{
	free_instrument_map();
	clean_up_pathlist();
	free_instruments(0);
	tmdy_free_config();
	timidity_start_initialize();
	read_config_file ( sp_temp->ConfigFile, 0 );
	PrefSettingApply();
#ifndef IA_W32G_SYN
	TracerWndApplyQuietChannel(st_temp->quietchannels);
#endif
}

static BOOL APIENTRY
PrefPlayerDialogProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam)
{
	static int initflag = 1; 
	switch (uMess){
	case WM_INITDIALOG:
		SetDlgItemText(hwnd,IDC_EDIT_CONFIG_FILE,TEXT(sp_temp->ConfigFile));
		{
		char buff[64];
		sprintf(buff,"%d",sp_temp->SubWindowMax);
		SetDlgItemText(hwnd,IDC_EDIT_SUBWINDOW_MAX,TEXT(buff));
		}
		switch(sp_temp->PlayerLanguage){
		case LANGUAGE_ENGLISH:
			CheckRadioButton(hwnd,IDC_RADIOBUTTON_JAPANESE,IDC_RADIOBUTTON_ENGLISH,
			IDC_RADIOBUTTON_ENGLISH);
			break;
		default:
		case LANGUAGE_JAPANESE:
			CheckRadioButton(hwnd,IDC_RADIOBUTTON_JAPANESE,IDC_RADIOBUTTON_ENGLISH,
			IDC_RADIOBUTTON_JAPANESE);
			break;
		}
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_AUTOQUIT,
								strchr(st_temp->opt_ctl + 1, 'x'));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_AUTOUNIQ,
								strchr(st_temp->opt_ctl + 1, 'u'));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_AUTOREFINE,
								strchr(st_temp->opt_ctl + 1, 'R'));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_AUTOSTART,
								strchr(st_temp->opt_ctl + 1, 'a'));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_NOT_CONTINUE,
								strchr(st_temp->opt_ctl + 1, 'C'));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_NOT_DRAG_START,
								!strchr(st_temp->opt_ctl + 1, 'd'));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_NOT_LOOPING,
								!strchr(st_temp->opt_ctl + 1, 'l'));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_RANDOM,
								strchr(st_temp->opt_ctl + 1, 'r'));

		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECK_SEACHDIRRECURSIVE,
								sp_temp->SeachDirRecursive);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECK_DOCWNDINDEPENDENT,
								sp_temp->DocWndIndependent);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECK_DOCWNDAUTOPOPUP,
								sp_temp->DocWndAutoPopup);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECK_INIFILE_AUTOSAVE,
								sp_temp->IniFileAutoSave);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECK_AUTOLOAD_PLAYLIST,
								sp_temp->AutoloadPlaylist);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECK_AUTOSAVE_PLAYLIST,
								sp_temp->AutosavePlaylist);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECK_POS_SIZE_SAVE,
								sp_temp->PosSizeSave);
		initflag = 0;
		break;
	case WM_COMMAND:
	switch (LOWORD(wParam)) {
		case IDC_BUTTON_CONFIG_FILE:
			{
				char filename[MAXPATH+1];
				filename[0] = '\0';
				SendDlgItemMessage(hwnd,IDC_EDIT_CONFIG_FILE,WM_GETTEXT,
					(WPARAM)MAX_PATH,(LPARAM)TEXT(filename));
				if(!DlgOpenConfigFile(filename,hwnd))
				if(filename[0]!='\0')
						SetDlgItemText(hwnd,IDC_EDIT_CONFIG_FILE,TEXT(filename));
	   }
			break;
		case IDC_BUTTON_CFG_EDIT:
			ShellExecute(NULL, "open", "notepad.exe", ConfigFile, NULL, SW_SHOWNORMAL);
			break;
/*		case IDC_BUTTON_CFG_DIR:
			ShellExecute(NULL, "open", ConfigFileOpenDir, NULL, NULL, SW_SHOWNORMAL);
			break;*/
		case IDC_BUTTON_CFG_RELOAD:
		{
			int i;
			for (i = 0; i < PREF_PAGE_MAX; i++ ) {
				SendMessage ( pref_pages[i].hwnd, WM_MYSAVE, (WPARAM)0, (LPARAM)0 );
			}
			reload_cfg();
			SetWindowLong(hwnd,	DWL_MSGRESULT, TRUE);
		}
			break;
		case IDC_RADIOBUTTON_JAPANESE:
		case IDC_RADIOBUTTON_ENGLISH:
			break;
		default:
		break;
	  }
		PrefWndSetOK = 1;
		break;

	case WM_MYSAVE:
	{
		if ( initflag ) break;
		SendDlgItemMessage(hwnd,IDC_EDIT_CONFIG_FILE,WM_GETTEXT,
			(WPARAM)MAX_PATH,(LPARAM)TEXT(sp_temp->ConfigFile));
		{
		char buff[64];
		SendDlgItemMessage(hwnd,IDC_EDIT_SUBWINDOW_MAX,WM_GETTEXT,
			(WPARAM)60,(LPARAM)TEXT(buff));
		sp_temp->SubWindowMax = atoi(buff);
		if ( sp_temp->SubWindowMax < 1 )
			sp_temp->SubWindowMax = 1;
		if ( sp_temp->SubWindowMax > 10 )
			sp_temp->SubWindowMax = 10;
		}
		if(SendDlgItemMessage(hwnd,IDC_RADIOBUTTON_ENGLISH,BM_GETCHECK,0,0)){
			sp_temp->PlayerLanguage = LANGUAGE_ENGLISH;
		} else if(SendDlgItemMessage(hwnd,IDC_RADIOBUTTON_JAPANESE,BM_GETCHECK,0,0)){
			sp_temp->PlayerLanguage = LANGUAGE_JAPANESE;
		}
	 {
	 int flag;

		SettingCtlFlag(st_temp, 'x',
							DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_AUTOQUIT,flag));
		SettingCtlFlag(st_temp, 'u',
							DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_AUTOUNIQ,flag));
		SettingCtlFlag(st_temp, 'R',
							DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_AUTOREFINE,flag));
		SettingCtlFlag(st_temp, 'a',
							DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_AUTOSTART,flag));
		SettingCtlFlag(st_temp, 'C',
							DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_NOT_CONTINUE,flag));
		SettingCtlFlag(st_temp, 'd',
							!DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_NOT_DRAG_START,flag));
		SettingCtlFlag(st_temp, 'l',
							!DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_NOT_LOOPING,flag));
		SettingCtlFlag(st_temp, 'r',
							DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_RANDOM,flag));
		}
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECK_SEACHDIRRECURSIVE,
			sp_temp->SeachDirRecursive);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECK_DOCWNDINDEPENDENT,
			sp_temp->DocWndIndependent);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECK_DOCWNDAUTOPOPUP,
			sp_temp->DocWndAutoPopup);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECK_INIFILE_AUTOSAVE,
			sp_temp->IniFileAutoSave);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECK_AUTOLOAD_PLAYLIST,
			sp_temp->AutoloadPlaylist);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECK_AUTOSAVE_PLAYLIST,
			sp_temp->AutosavePlaylist);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECK_POS_SIZE_SAVE,
			sp_temp->PosSizeSave);
		SetWindowLong(hwnd,DWL_MSGRESULT,FALSE);
	}
		break;
	case WM_SIZE:
		return FALSE;
	case WM_CLOSE:
		break;
	default:
	  break;
	}
	return FALSE;
}

// IDC_COMBO_REVERB
#define cb_num_IDC_COMBO_REVERB 5

static char *cb_info_IDC_COMBO_REVERB_en[] = {
	"No Reverb",
	"Standard Reverb",
	"Global Old Reverb",
	"New Reverb",
	"Global New Reverb",
};

static char *cb_info_IDC_COMBO_REVERB_jp[] = {
	"リバーブなし",
	"標準リバーブ",
	"標準リバーブ（グローバル）",
	"新リバーブ",
	"新リバーブ（グローバル）",
};

// IDC_COMBO_CHORUS
#define cb_num_IDC_COMBO_CHORUS 3

static char *cb_info_IDC_COMBO_CHORUS_en[] = {
	"No Chorus",
	"Standard Chorus",
	"Surround Chorus",
};

static char *cb_info_IDC_COMBO_CHORUS_jp[] = {
	"コーラスなし",
	"標準コーラス",
	"サラウンドコーラス",
};

// IDC_COMBO_DELAY
#define cb_num_IDC_COMBO_DELAY 2

static char *cb_info_IDC_COMBO_DELAY_en[] = {
	"No Delay",
	"Standard Delay",
};

static char *cb_info_IDC_COMBO_DELAY_jp[] = {
	"ディレイなし",
	"標準ディレイ",
};

// IDC_COMBO_LPF
#define cb_num_IDC_COMBO_LPF 3

static char *cb_info_IDC_COMBO_LPF_en[] = {
	"No Filter",
	"Lowpass Filter (12dB/oct)",
	"Lowpass Filter (24dB/oct)",
};

static char *cb_info_IDC_COMBO_LPF_jp[] = {
	"フィルタなし",
	"LPF (12dB/oct)",
	"LPF (24dB/oct)",
};

// IDC_COMBO_MODULE
struct _ModuleList {
	int num;
	char *name;
};

static struct _ModuleList cb_info_IDC_COMBO_MODULE[] = {
	MODULE_TIMIDITY_DEFAULT, "TiMidity++ Default",
	MODULE_SC55, "SC-55",
	MODULE_SC88, "SC-88",
	MODULE_SC88PRO, "SC-88Pro",
	MODULE_SC8850, "SC-8850",
	MODULE_MU50, "MU-50",
	MODULE_MU80, "MU-80",
	MODULE_MU90, "MU-90",
	MODULE_MU100, "MU-100",
	MODULE_SBLIVE, "Sound Blaster Live!",
	MODULE_SBAUDIGY, "Sound Blaster Audigy",
	MODULE_TIMIDITY_SPECIAL1, "TiMidity++ Special 1",
	MODULE_TIMIDITY_DEBUG, "TiMidity++ Debug",
};

#define cb_num_IDC_COMBO_MODULE (sizeof(cb_info_IDC_COMBO_MODULE) / sizeof(struct _ModuleList))

static int find_combo_module_item(int val)
{
	int i;
	for (i = 0; i < cb_num_IDC_COMBO_MODULE; i++)
		if (val == cb_info_IDC_COMBO_MODULE[i].num) {return i;}
	return 0;
}

static BOOL APIENTRY
PrefTiMidity1DialogProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam)
{
	static int initflag = 1; 
	int i;
	char **cb_info;
	switch (uMess){
	case WM_INITDIALOG:
		// CHORUS
		if (PlayerLanguage == LANGUAGE_JAPANESE)
			cb_info = cb_info_IDC_COMBO_CHORUS_jp;
		else 
			cb_info = cb_info_IDC_COMBO_CHORUS_en;

		for (i = 0; i < cb_num_IDC_COMBO_CHORUS; i++)
			SendDlgItemMessage(hwnd, IDC_COMBO_CHORUS,
					CB_INSERTSTRING, (WPARAM) -1,
					(LPARAM) cb_info[i]);

		if(GetDlgItemInt(hwnd, IDC_EDIT_CHORUS, NULL, FALSE)==0)
			SetDlgItemInt(hwnd, IDC_EDIT_CHORUS, 1, TRUE);
		if (st_temp->opt_surround_chorus)
			i = 2;
		else
			i = st_temp->opt_chorus_control;
		if (i >= 0) {
			SendDlgItemMessage(hwnd, IDC_COMBO_CHORUS, CB_SETCURSEL,
					(WPARAM) i, (LPARAM) 0);
			SendDlgItemMessage(hwnd, IDC_CHECKBOX_CHORUS_LEVEL, BM_SETCHECK, 0, 0);
		} else {
			SendDlgItemMessage(hwnd, IDC_COMBO_CHORUS, CB_SETCURSEL,
					(WPARAM) ((-i) / 128 + 1), (LPARAM) 0);
			SendDlgItemMessage(hwnd, IDC_CHECKBOX_CHORUS_LEVEL, BM_SETCHECK, 1, 0);
			SetDlgItemInt(hwnd, IDC_EDIT_CHORUS, -i, TRUE);
		}
		SendMessage(hwnd, WM_COMMAND, IDC_CHECKBOX_CHORUS_LEVEL, 0);
		// REVERB
		if (PlayerLanguage == LANGUAGE_JAPANESE)
			cb_info = cb_info_IDC_COMBO_REVERB_jp;
		else 
			cb_info = cb_info_IDC_COMBO_REVERB_en;

		for (i = 0; i < cb_num_IDC_COMBO_REVERB; i++)
			SendDlgItemMessage(hwnd, IDC_COMBO_REVERB,
					CB_INSERTSTRING, (WPARAM) -1,
					(LPARAM) cb_info[i]);

		if(GetDlgItemInt(hwnd, IDC_EDIT_REVERB, NULL, FALSE)==0)
			SetDlgItemInt(hwnd, IDC_EDIT_REVERB, 1, TRUE);
		if (st_temp->opt_reverb_control >= 0) {
			SendDlgItemMessage(hwnd, IDC_COMBO_REVERB, CB_SETCURSEL,
					(WPARAM) st_temp->opt_reverb_control, (LPARAM) 0);
			SendDlgItemMessage(hwnd, IDC_CHECKBOX_REVERB_LEVEL, BM_SETCHECK, 0, 0);
		} else {
			SendDlgItemMessage(hwnd, IDC_COMBO_REVERB, CB_SETCURSEL,
					(WPARAM) ((-st_temp->opt_reverb_control) / 128 + 1), (LPARAM) 0);
			SendDlgItemMessage(hwnd, IDC_CHECKBOX_REVERB_LEVEL, BM_SETCHECK, 1, 0);
			SetDlgItemInt(hwnd, IDC_EDIT_REVERB, (-st_temp->opt_reverb_control) % 128, TRUE);
		}
		SendMessage(hwnd, WM_COMMAND, IDC_CHECKBOX_REVERB_LEVEL, 0);
		// DELAY
		if (PlayerLanguage == LANGUAGE_JAPANESE)
			cb_info = cb_info_IDC_COMBO_DELAY_jp;
		else 
			cb_info = cb_info_IDC_COMBO_DELAY_en;

		for (i = 0; i < cb_num_IDC_COMBO_DELAY; i++)
			SendDlgItemMessage(hwnd, IDC_COMBO_DELAY,
					CB_INSERTSTRING, (WPARAM) -1,
					(LPARAM) cb_info[i]);

		SendDlgItemMessage(hwnd, IDC_COMBO_DELAY, CB_SETCURSEL,
				(WPARAM) st_temp->opt_delay_control, (LPARAM) 0);
		// DEFAULT MODULE
		for (i = 0; i < cb_num_IDC_COMBO_MODULE; i++)
			SendDlgItemMessage(hwnd, IDC_COMBO_MODULE,
					CB_INSERTSTRING, (WPARAM) -1,
					(LPARAM) cb_info_IDC_COMBO_MODULE[i].name);

		SendDlgItemMessage(hwnd, IDC_COMBO_MODULE, CB_SETCURSEL,
				(WPARAM) find_combo_module_item(st_temp->opt_default_module), (LPARAM) 0);
		// LPF
		if (PlayerLanguage == LANGUAGE_JAPANESE)
			cb_info = cb_info_IDC_COMBO_LPF_jp;
		else 
			cb_info = cb_info_IDC_COMBO_LPF_en;

		for (i = 0; i < cb_num_IDC_COMBO_LPF; i++)
			SendDlgItemMessage(hwnd, IDC_COMBO_LPF,
					CB_INSERTSTRING, (WPARAM) -1,
					(LPARAM) cb_info[i]);

		SendDlgItemMessage(hwnd, IDC_COMBO_LPF, CB_SETCURSEL,
				(WPARAM) st_temp->opt_lpf_def, (LPARAM) 0);
		// L&R DELAY
		SetDlgItemInt(hwnd,IDC_EDIT_DELAY,st_temp->effect_lr_delay_msec,TRUE);
		if(st_temp->effect_lr_mode<0){
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_DELAY,BM_SETCHECK,0,0);
		} else {
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_DELAY,BM_SETCHECK,1,0);
			switch(st_temp->effect_lr_mode){
		case 0:
				CheckRadioButton(hwnd,IDC_RADIOBUTTON_DELAY_LEFT,IDC_RADIOBUTTON_DELAY_CENTER,
			IDC_RADIOBUTTON_DELAY_LEFT);
				break;
			case 1:
				CheckRadioButton(hwnd,IDC_RADIOBUTTON_DELAY_LEFT,IDC_RADIOBUTTON_DELAY_CENTER,
			IDC_RADIOBUTTON_DELAY_RIGHT);
				break;
			case 2:
		default:
				CheckRadioButton(hwnd,IDC_RADIOBUTTON_DELAY_LEFT,IDC_RADIOBUTTON_DELAY_CENTER,
			IDC_RADIOBUTTON_DELAY_CENTER);
				break;
		 }
	 }
		SendMessage(hwnd,WM_COMMAND,IDC_CHECKBOX_DELAY,0);
		// NOISESHARPING
	 SetDlgItemInt(hwnd,IDC_EDIT_NOISESHARPING,st_temp->noise_sharp_type,TRUE);
	 // Misc
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_MODWHEEL,st_temp->opt_modulation_wheel);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_PORTAMENTO,st_temp->opt_portamento);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_NRPNVIB,st_temp->opt_nrpn_vibrato);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CHPRESS,st_temp->opt_channel_pressure);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_OVOICE,st_temp->opt_overlap_voice_allow);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_TRACETEXT,st_temp->opt_trace_text_meta_event);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_TVAA,st_temp->opt_tva_attack);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_TVAD,st_temp->opt_tva_decay);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_TVAR,st_temp->opt_tva_release);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_DRUM_EFFECT,st_temp->opt_drum_effect);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_MOD_ENV,st_temp->opt_modulation_envelope);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_PAN_DELAY,st_temp->opt_pan_delay);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_EQ,st_temp->opt_eq_control);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_IEFFECT,st_temp->opt_insertion_effect);
		SetDlgItemInt(hwnd,IDC_EDIT_MODIFY_RELEASE,st_temp->modify_release,TRUE);
		initflag = 0;
		break;
	case WM_COMMAND:
	switch (LOWORD(wParam)) {
	  case IDCLOSE:
		break;
		case IDC_CHECKBOX_CHORUS_LEVEL:
			if(SendDlgItemMessage(hwnd, IDC_CHECKBOX_CHORUS_LEVEL, BM_GETCHECK, 0, 0)){
				EnableWindow(GetDlgItem(hwnd, IDC_EDIT_CHORUS), TRUE);
			} else {
				EnableWindow(GetDlgItem(hwnd, IDC_EDIT_CHORUS), FALSE);
			}
			break;
		case IDC_CHECKBOX_REVERB_LEVEL:
			if(SendDlgItemMessage(hwnd, IDC_CHECKBOX_REVERB_LEVEL, BM_GETCHECK, 0, 0)){
				EnableWindow(GetDlgItem(hwnd, IDC_EDIT_REVERB), TRUE);
			} else {
				EnableWindow(GetDlgItem(hwnd, IDC_EDIT_REVERB), FALSE);
			}
			break;
	 case IDC_CHECKBOX_DELAY:
		case IDC_RADIOBUTTON_DELAY_LEFT:
		case IDC_RADIOBUTTON_DELAY_RIGHT:
		case IDC_RADIOBUTTON_DELAY_CENTER:
			if(!SendDlgItemMessage(hwnd,IDC_RADIOBUTTON_DELAY_LEFT,BM_GETCHECK,0,0))
			if(!SendDlgItemMessage(hwnd,IDC_RADIOBUTTON_DELAY_RIGHT,BM_GETCHECK,0,0))
			if(!SendDlgItemMessage(hwnd,IDC_RADIOBUTTON_DELAY_CENTER,BM_GETCHECK,0,0))
				CheckRadioButton(hwnd,IDC_RADIOBUTTON_DELAY_LEFT,IDC_RADIOBUTTON_DELAY_CENTER,IDC_RADIOBUTTON_DELAY_CENTER);
			if(SendDlgItemMessage(hwnd,IDC_CHECKBOX_DELAY,BM_GETCHECK,0,0)){
				EnableWindow(GetDlgItem(hwnd,IDC_RADIOBUTTON_DELAY_LEFT),TRUE);
				EnableWindow(GetDlgItem(hwnd,IDC_RADIOBUTTON_DELAY_RIGHT),TRUE);
				EnableWindow(GetDlgItem(hwnd,IDC_RADIOBUTTON_DELAY_CENTER),TRUE);
				EnableWindow(GetDlgItem(hwnd,IDC_EDIT_DELAY),TRUE);
		 } else {
				EnableWindow(GetDlgItem(hwnd,IDC_RADIOBUTTON_DELAY_LEFT),FALSE);
				EnableWindow(GetDlgItem(hwnd,IDC_RADIOBUTTON_DELAY_RIGHT),FALSE);
				EnableWindow(GetDlgItem(hwnd,IDC_RADIOBUTTON_DELAY_CENTER),FALSE);
				EnableWindow(GetDlgItem(hwnd,IDC_EDIT_DELAY),FALSE);
		 }
			break;
		default:
		PrefWndSetOK = 1;
		return FALSE;
		break;
	  }
		PrefWndSetOK = 1;
		break;
	case WM_MYSAVE:
	{
		if ( initflag ) break;
		// CHORUS
		st_temp->opt_chorus_control = (int)SendDlgItemMessage(hwnd, IDC_COMBO_CHORUS, CB_GETCURSEL, 0, 0);
		if (st_temp->opt_chorus_control && SendDlgItemMessage(hwnd, IDC_CHECKBOX_CHORUS_LEVEL, BM_GETCHECK, 0, 0)) {
			st_temp->opt_chorus_control = -(int)GetDlgItemInt(hwnd, IDC_EDIT_CHORUS, NULL, TRUE);
		}
		if (st_temp->opt_chorus_control == 2) {
			st_temp->opt_chorus_control = 1;
			st_temp->opt_surround_chorus = 1;
		} else {
			st_temp->opt_surround_chorus = 0;
		}
  		// REVERB
		st_temp->opt_reverb_control = (int)SendDlgItemMessage(hwnd, IDC_COMBO_REVERB, CB_GETCURSEL, 0, 0);
		if(st_temp->opt_reverb_control && SendDlgItemMessage(hwnd, IDC_CHECKBOX_REVERB_LEVEL, BM_GETCHECK, 0, 0)) {
			st_temp->opt_reverb_control = -(int)GetDlgItemInt(hwnd, IDC_EDIT_REVERB, NULL, TRUE)
				- (st_temp->opt_reverb_control - 1) * 128;
		}
		// DELAY
		st_temp->opt_delay_control = (int)SendDlgItemMessage(hwnd, IDC_COMBO_DELAY, CB_GETCURSEL, 0, 0);
		// DEFAULT MODULE
		st_temp->opt_default_module = cb_info_IDC_COMBO_MODULE[(int)SendDlgItemMessage(hwnd, IDC_COMBO_MODULE, CB_GETCURSEL, 0, 0)].num;
		// LPF
		st_temp->opt_lpf_def = (int)SendDlgItemMessage(hwnd, IDC_COMBO_LPF, CB_GETCURSEL, 0, 0);
		// L&R DELAY
		st_temp->effect_lr_delay_msec = GetDlgItemInt(hwnd,IDC_EDIT_DELAY,NULL,FALSE);
		if(SendDlgItemMessage(hwnd,IDC_CHECKBOX_DELAY,BM_GETCHECK,0,0)){
			if(SendDlgItemMessage(hwnd,IDC_RADIOBUTTON_DELAY_LEFT,BM_GETCHECK,0,0)){
				st_temp->effect_lr_mode = 0;
			} else if(SendDlgItemMessage(hwnd,IDC_RADIOBUTTON_DELAY_RIGHT,BM_GETCHECK,0,0)){
				st_temp->effect_lr_mode = 1;
			} else {
				st_temp->effect_lr_mode = 2;
			}
		} else {
			st_temp->effect_lr_mode = -1;
		}
		// NOISESHARPING
	 st_temp->noise_sharp_type = GetDlgItemInt(hwnd,IDC_EDIT_NOISESHARPING,NULL,FALSE);
	 // Misc
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_MODWHEEL,st_temp->opt_modulation_wheel);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_PORTAMENTO,st_temp->opt_portamento);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_NRPNVIB,st_temp->opt_nrpn_vibrato);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_CHPRESS,st_temp->opt_channel_pressure);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_OVOICE,st_temp->opt_overlap_voice_allow);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_TRACETEXT,st_temp->opt_trace_text_meta_event);
	 st_temp->modify_release = GetDlgItemInt(hwnd,IDC_EDIT_MODIFY_RELEASE,NULL,FALSE);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_TVAA,st_temp->opt_tva_attack);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_TVAD,st_temp->opt_tva_decay);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_TVAR,st_temp->opt_tva_release);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_DRUM_EFFECT,st_temp->opt_drum_effect);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_MOD_ENV,st_temp->opt_modulation_envelope);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_PAN_DELAY,st_temp->opt_pan_delay);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_EQ,st_temp->opt_eq_control);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_IEFFECT,st_temp->opt_insertion_effect);
		SetWindowLong(hwnd,DWL_MSGRESULT,FALSE);
	}
		break;
	case WM_SIZE:
		return FALSE;
	case WM_CLOSE:
		break;
	default:
	  break;
	}
	return FALSE;
}

static int char_count(char *s, int c)
{
	 int n = 0;
	 while(*s)
		  n += (*s++ == c);
	 return n;
}

// IDC_COMBO_(INIT|FORCE)_KEYSIG
static char *cb_info_IDC_COMBO_KEYSIG[] = {
	"Cb Maj / Ab Min (b7)",
	"Gb Maj / Eb Min (b6)",
	"Db Maj / Bb Min (b5)",
	"Ab Maj / F  Min (b4)",
	"Eb Maj / C  Min (b3)",
	"Bb Maj / G  Min (b2)",
	"F  Maj / D  Min (b1)",
	"C  Maj / A  Min (0)",
	"G  Maj / E  Min (#1)",
	"D  Maj / B  Min (#2)",
	"A  Maj / F# Min (#3)",
	"E  Maj / C# Min (#4)",
	"B  Maj / G# Min (#5)",
	"F# Maj / D# Min (#6)",
	"C# Maj / A# Min (#7)"
};

static BOOL APIENTRY
PrefTiMidity2DialogProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam)
{
	int i;
	static int initflag = 1; 

	switch (uMess){
	case WM_INITDIALOG:
		SetDlgItemInt(hwnd,IDC_EDIT_VOICES,st_temp->voices,FALSE);
		SetDlgItemInt(hwnd,IDC_EDIT_AMPLIFICATION,st_temp->amplification,FALSE);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_FREE_INST,st_temp->free_instruments_afterwards);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_ANTIALIAS,st_temp->antialiasing_allowed);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_LOADINST_PLAYING,st_temp->opt_realtime_playing);
		SetDlgItemInt(hwnd,IDC_EDIT_CACHE_SIZE,st_temp->allocate_cache_size,FALSE);

		SetDlgItemInt(hwnd,IDC_EDIT_REDUCE_VOICE,st_temp->reduce_voice_threshold,TRUE);
		SendDlgItemMessage(hwnd,IDC_CHECKBOX_REDUCE_VOICE,BM_SETCHECK,st_temp->reduce_voice_threshold,0);
		SetDlgItemInt(hwnd,IDC_EDIT_DEFAULT_TONEBANK,st_temp->default_tonebank,FALSE);
		SetDlgItemInt(hwnd,IDC_EDIT_SPECIAL_TONEBANK,st_temp->special_tonebank,TRUE);
		if(st_temp->special_tonebank<0){
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_SPECIAL_TONEBANK,BM_SETCHECK,0,0);
		} else {
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_SPECIAL_TONEBANK,BM_SETCHECK,1,0);
		}
		SendMessage(hwnd,WM_COMMAND,IDC_CHECKBOX_SPECIAL_TONEBANK,0);
		switch(st_temp->opt_default_mid){
	 case 0x41:
			CheckRadioButton(hwnd,IDC_RADIOBUTTON_GM,IDC_RADIOBUTTON_XG,IDC_RADIOBUTTON_GS);
			break;
	 case 0x43:
			CheckRadioButton(hwnd,IDC_RADIOBUTTON_GM,IDC_RADIOBUTTON_XG,IDC_RADIOBUTTON_XG);
			break;
		default:
	 case 0x7e:
			CheckRadioButton(hwnd,IDC_RADIOBUTTON_GM,IDC_RADIOBUTTON_XG,IDC_RADIOBUTTON_GM);
			break;
		}
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CTL_TRACE_PLAYING,
										strchr(st_temp->opt_ctl + 1, 't'));
		SetDlgItemInt(hwnd,IDC_EDIT_CTL_VEBOSITY,
							char_count(st_temp->opt_ctl + 1, 'v') -
							char_count(st_temp->opt_ctl + 1, 'q') + 1, TRUE);
		SetDlgItemInt(hwnd,IDC_EDIT_CONTROL_RATIO,st_temp->control_ratio,FALSE);
		SetDlgItemInt(hwnd,IDC_EDIT_DRUM_POWER,st_temp->opt_drum_power,FALSE);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_AMP_COMPENSATION,st_temp->opt_amp_compensation);
		DLG_FLAG_TO_CHECKBUTTON(hwnd, IDC_CHECKBOX_PURE_INTONATION,
				st_temp->opt_pure_intonation);
		SendMessage(hwnd, WM_COMMAND, IDC_CHECKBOX_PURE_INTONATION, 0);
		DLG_FLAG_TO_CHECKBUTTON(hwnd, IDC_CHECKBOX_INIT_KEYSIG,
				(st_temp->opt_init_keysig != 8));
		SendMessage(hwnd, WM_COMMAND, IDC_CHECKBOX_PURE_INTONATION, 0);
		for (i = 0; i < 15; i++)
			SendDlgItemMessage(hwnd, IDC_COMBO_INIT_KEYSIG, CB_INSERTSTRING,
					(WPARAM) -1, (LPARAM) cb_info_IDC_COMBO_KEYSIG[i]);
		if (st_temp->opt_init_keysig == 8) {
			SendDlgItemMessage(hwnd, IDC_COMBO_INIT_KEYSIG, CB_SETCURSEL,
					(WPARAM) 7, (LPARAM) 0);
			SendDlgItemMessage(hwnd, IDC_CHECKBOX_INIT_MI, BM_SETCHECK,
					0, 0);
		} else {
			SendDlgItemMessage(hwnd, IDC_COMBO_INIT_KEYSIG, CB_SETCURSEL,
					(WPARAM) st_temp->opt_init_keysig + 7 & 0x0f,
					(LPARAM) 0);
			SendDlgItemMessage(hwnd, IDC_CHECKBOX_INIT_MI, BM_SETCHECK,
					(st_temp->opt_init_keysig + 7 & 0x10) ? 1 : 0, 0);
		}
		SetDlgItemInt(hwnd, IDC_EDIT_KEY_ADJUST, st_temp->key_adjust, TRUE);
		DLG_FLAG_TO_CHECKBUTTON(hwnd, IDC_CHECKBOX_FORCE_KEYSIG,
				(st_temp->opt_force_keysig != 8));
		SendMessage(hwnd, WM_COMMAND, IDC_CHECKBOX_FORCE_KEYSIG, 0);
		for (i = 0; i < 15; i++)
			SendDlgItemMessage(hwnd, IDC_COMBO_FORCE_KEYSIG, CB_INSERTSTRING,
					(WPARAM) -1, (LPARAM) cb_info_IDC_COMBO_KEYSIG[i]);
		if (st_temp->opt_force_keysig == 8)
			SendDlgItemMessage(hwnd, IDC_COMBO_FORCE_KEYSIG, CB_SETCURSEL,
					(WPARAM) 7, (LPARAM) 0);
		else
			SendDlgItemMessage(hwnd, IDC_COMBO_FORCE_KEYSIG, CB_SETCURSEL,
					(WPARAM) st_temp->opt_force_keysig + 7, (LPARAM) 0);
		initflag = 0;
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCLOSE:
			break;
		case IDC_CHECKBOX_SPECIAL_TONEBANK:
			if (SendDlgItemMessage(hwnd, IDC_CHECKBOX_SPECIAL_TONEBANK,
					BM_GETCHECK, 0, 0))
				EnableWindow(GetDlgItem(hwnd, IDC_EDIT_SPECIAL_TONEBANK),
						TRUE);
			else
				EnableWindow(GetDlgItem(hwnd, IDC_EDIT_SPECIAL_TONEBANK),
						FALSE);
			break;
		case IDC_CHECKBOX_PURE_INTONATION:
			if (SendDlgItemMessage(hwnd, IDC_CHECKBOX_PURE_INTONATION,
					BM_GETCHECK, 0, 0)) {
				EnableWindow(GetDlgItem(hwnd,
						IDC_CHECKBOX_INIT_KEYSIG), TRUE);
				if (SendDlgItemMessage(hwnd,
						IDC_CHECKBOX_INIT_KEYSIG, BM_GETCHECK, 0, 0)) {
					EnableWindow(GetDlgItem(hwnd,
							IDC_COMBO_INIT_KEYSIG), TRUE);
					EnableWindow(GetDlgItem(hwnd,
							IDC_CHECKBOX_INIT_MI), TRUE);
				} else {
					EnableWindow(GetDlgItem(hwnd,
							IDC_COMBO_INIT_KEYSIG), FALSE);
					EnableWindow(GetDlgItem(hwnd,
							IDC_CHECKBOX_INIT_MI), FALSE);
				}
			} else {
				EnableWindow(GetDlgItem(hwnd,
						IDC_CHECKBOX_INIT_KEYSIG), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_COMBO_INIT_KEYSIG), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_CHECKBOX_INIT_MI), FALSE);
			}
			break;
		case IDC_CHECKBOX_INIT_KEYSIG:
			if (SendDlgItemMessage(hwnd,
					IDC_CHECKBOX_INIT_KEYSIG, BM_GETCHECK, 0, 0)) {
				EnableWindow(GetDlgItem(hwnd, IDC_COMBO_INIT_KEYSIG), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDC_CHECKBOX_INIT_MI), TRUE);
			} else {
				EnableWindow(GetDlgItem(hwnd, IDC_COMBO_INIT_KEYSIG), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_CHECKBOX_INIT_MI), FALSE);
			}
			break;
		case IDC_COMBO_INIT_KEYSIG:
		case IDC_CHECKBOX_INIT_MI:
			st_temp->opt_init_keysig = SendDlgItemMessage(hwnd,
					IDC_COMBO_INIT_KEYSIG, CB_GETCURSEL,
					(WPARAM) 0, (LPARAM) 0) + ((SendDlgItemMessage(hwnd,
					IDC_CHECKBOX_INIT_MI, BM_GETCHECK,
					0, 0)) ? 16 : 0) - 7;
			break;
		case IDC_CHECKBOX_FORCE_KEYSIG:
			if (SendDlgItemMessage(hwnd,
					IDC_CHECKBOX_FORCE_KEYSIG, BM_GETCHECK, 0, 0))
				EnableWindow(GetDlgItem(hwnd, IDC_COMBO_FORCE_KEYSIG), TRUE);
			else
				EnableWindow(GetDlgItem(hwnd, IDC_COMBO_FORCE_KEYSIG), FALSE);
			break;
		case IDC_COMBO_FORCE_KEYSIG:
			st_temp->opt_force_keysig = SendDlgItemMessage(hwnd,
					IDC_COMBO_FORCE_KEYSIG, CB_GETCURSEL,
					(WPARAM) 0, (LPARAM) 0) - 7;
			break;
		default:
			break;
		}
		PrefWndSetOK = 1;
		break;
	case WM_MYSAVE:
		if ( initflag ) break;
	{
		int i;
		char *p;
		st_temp->voices = GetDlgItemInt(hwnd,IDC_EDIT_VOICES,NULL,FALSE);
		st_temp->amplification = GetDlgItemInt(hwnd,IDC_EDIT_AMPLIFICATION,NULL,FALSE);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_FREE_INST,st_temp->free_instruments_afterwards);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_ANTIALIAS,st_temp->antialiasing_allowed);
		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_LOADINST_PLAYING,st_temp->opt_realtime_playing);
		st_temp->allocate_cache_size = GetDlgItemInt(hwnd,IDC_EDIT_CACHE_SIZE,NULL,FALSE);
		if(SendDlgItemMessage(hwnd,IDC_CHECKBOX_REDUCE_VOICE,BM_GETCHECK,0,0))
		{
			st_temp->reduce_voice_threshold = -1;
			st_temp->auto_reduce_polyphony = 1;
		}
		else
		{
			st_temp->reduce_voice_threshold = 0;
			st_temp->auto_reduce_polyphony = 0;
		}

		st_temp->default_tonebank = GetDlgItemInt(hwnd,IDC_EDIT_DEFAULT_TONEBANK,NULL,FALSE);
		if(SendDlgItemMessage(hwnd,IDC_CHECKBOX_SPECIAL_TONEBANK,BM_GETCHECK,0,0)){
			st_temp->special_tonebank = GetDlgItemInt(hwnd,IDC_EDIT_SPECIAL_TONEBANK,NULL,TRUE);
	 } else {
			st_temp->special_tonebank = -1;
	 }
		if(SendDlgItemMessage(hwnd,IDC_RADIOBUTTON_GS,BM_GETCHECK,0,0)){
		st_temp->opt_default_mid = 0x41;
		} else if(SendDlgItemMessage(hwnd,IDC_RADIOBUTTON_XG,BM_GETCHECK,0,0)){
		st_temp->opt_default_mid = 0x43;
	 } else
		st_temp->opt_default_mid = 0x7e;

		SettingCtlFlag(st_temp, 't',
							DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_CTL_TRACE_PLAYING,i));

		/* remove 'v' and 'q' from st_temp->opt_ctl */
		while(strchr(st_temp->opt_ctl + 1, 'v'))
			 SettingCtlFlag(st_temp, 'v', 0);
		while(strchr(st_temp->opt_ctl + 1, 'q'))
			 SettingCtlFlag(st_temp, 'q', 0);

		/* append 'v' or 'q' */
		p = st_temp->opt_ctl + strlen(st_temp->opt_ctl);
		i = GetDlgItemInt(hwnd,IDC_EDIT_CTL_VEBOSITY,NULL,TRUE);
		while(i > 1) { *p++ = 'v'; i--; }
		while(i < 1) { *p++ = 'q'; i++; }

		st_temp->control_ratio = GetDlgItemInt(hwnd,IDC_EDIT_CONTROL_RATIO,NULL,FALSE);
		st_temp->opt_drum_power = GetDlgItemInt(hwnd,IDC_EDIT_DRUM_POWER,NULL,FALSE);

		DLG_CHECKBUTTON_TO_FLAG(hwnd,IDC_CHECKBOX_AMP_COMPENSATION,st_temp->opt_amp_compensation);

		DLG_CHECKBUTTON_TO_FLAG(hwnd,
				IDC_CHECKBOX_PURE_INTONATION, st_temp->opt_pure_intonation);
		if (SendDlgItemMessage(hwnd, IDC_CHECKBOX_PURE_INTONATION,
				BM_GETCHECK, 0, 0) && SendDlgItemMessage(hwnd,
				IDC_CHECKBOX_INIT_KEYSIG, BM_GETCHECK, 0, 0))
			st_temp->opt_init_keysig = SendDlgItemMessage(hwnd,
					IDC_COMBO_INIT_KEYSIG, CB_GETCURSEL,
					(WPARAM) 0, (LPARAM) 0) + ((SendDlgItemMessage(hwnd,
					IDC_CHECKBOX_INIT_MI, BM_GETCHECK,
					0, 0)) ? 16 : 0) - 7;
		else
			st_temp->opt_init_keysig = 8;
		st_temp->key_adjust = GetDlgItemInt(hwnd,
				IDC_EDIT_KEY_ADJUST, NULL, TRUE);
		if (SendDlgItemMessage(hwnd,
				IDC_CHECKBOX_FORCE_KEYSIG, BM_GETCHECK, 0, 0))
			st_temp->opt_force_keysig = SendDlgItemMessage(hwnd,
					IDC_COMBO_FORCE_KEYSIG, CB_GETCURSEL,
					(WPARAM) 0, (LPARAM) 0) - 7;
		else
			st_temp->opt_force_keysig = 8;

		SetWindowLong(hwnd,DWL_MSGRESULT,FALSE);
	}
		break;
  case WM_SIZE:
		return FALSE;
	case WM_CLOSE:
		break;
	default:
	  break;
	}
	return FALSE;
}

// IDC_COMBO_SAMPLE_RATE
#define cb_num_IDC_COMBO_SAMPLE_RATE 10
static char *cb_info_IDC_COMBO_SAMPLE_RATE[] = {
	"4000",
	"8000",
	"11025",
	"16000",
	"22050",
	"24000",
	"32000",
	"40000",
	"44100",
	"48000",
};

// IDC_COMBO_BANDWIDTH
#define cb_num_IDC_COMBO_BANDWIDTH 3
enum {
	BANDWIDTH_8BIT = 0,
	BANDWIDTH_16BIT = 1,
	BANDWIDTH_24BIT = 2,
};
static char *cb_info_IDC_COMBO_BANDWIDTH_en[] = {
	"8-bit",
	"16-bit",
	"24-bit",
};
static char *cb_info_IDC_COMBO_BANDWIDTH_jp[] = {
	"8ビット",
	"16ビット",
	"24ビット",
};
static char **cb_info_IDC_COMBO_BANDWIDTH;

// IDC_COMBO_OUTPUT_MODE
static char *cb_info_IDC_COMBO_OUTPUT_MODE_jp[]= {
	"以下のファイルに出力",(char *)0,
#if defined(__CYGWIN32__) || defined(__MINGW32__)
	"ファイル名を自動で決定し、ソ\ースと同じフォルダに出力",(char *)1,
#else
	"ファイル名を自動で決定し、ソースと同じフォルダに出力",(char *)1,
#endif

	"ファイル名を自動で決定し、以下のフォルダに出力",(char *)2,
	"ファイル名を自動で決定し、以下のフォルダに出力(フォルダ名付き)",(char *)3,
	NULL
};
static char *cb_info_IDC_COMBO_OUTPUT_MODE_en[]= {
	"next output file",(char *)0,
	"auto filename",(char *)1,
	"auto filename and output in next dir",(char *)2,
	"auto filename and output in next dir (with folder name)",(char *)3,
	NULL
};
static char **cb_info_IDC_COMBO_OUTPUT_MODE;

static BOOL APIENTRY
PrefTiMidity3DialogProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam)
{
	static int initflag = 1;
	switch (uMess){
   case WM_INITDIALOG:
		{
			int i;
			SendDlgItemMessage(hwnd,IDC_COMBO_OUTPUT,CB_RESETCONTENT,(WPARAM)0,(LPARAM)0);
			for(i=0;play_mode_list[i]!=0;i++){
				SendDlgItemMessage(hwnd,IDC_COMBO_OUTPUT,CB_INSERTSTRING,(WPARAM)-1,(LPARAM)play_mode_list[i]->id_name);
			}
			if (PlayerLanguage == LANGUAGE_JAPANESE)
			  cb_info_IDC_COMBO_OUTPUT_MODE = cb_info_IDC_COMBO_OUTPUT_MODE_jp;
			else
			  cb_info_IDC_COMBO_OUTPUT_MODE = cb_info_IDC_COMBO_OUTPUT_MODE_en;
			for(i=0;cb_info_IDC_COMBO_OUTPUT_MODE[i];i+=2){
				SendDlgItemMessage(hwnd,IDC_COMBO_OUTPUT_MODE,CB_INSERTSTRING,(WPARAM)-1,(LPARAM)cb_info_IDC_COMBO_OUTPUT_MODE[i]);
			}
			{
				int cb_num;
				for(cb_num=0;(int)cb_info_IDC_COMBO_OUTPUT_MODE[cb_num];cb_num+=2){
					SendDlgItemMessage(hwnd,IDC_COMBO_OUTPUT_MODE,CB_SETCURSEL,(WPARAM)0,(LPARAM)0);
					if(st_temp->auto_output_mode==(int)cb_info_IDC_COMBO_OUTPUT_MODE[cb_num+1]){
						SendDlgItemMessage(hwnd,IDC_COMBO_OUTPUT_MODE,CB_SETCURSEL,(WPARAM)cb_num/2,(LPARAM)0);
						break;
					}
				}
			}
		}
		{
		char *opt;
		int num = 0;
		int i;
		for(i=0;play_mode_list[i]!=0;i++){
			if(st_temp->opt_playmode[0]==play_mode_list[i]->id_character){
				num = i;
			break;
		}
		}
		SendDlgItemMessage(hwnd,IDC_COMBO_OUTPUT,CB_SETCURSEL,(WPARAM)num,(LPARAM)0);
		if(st_temp->auto_output_mode==0){
		if(st_temp->OutputName[0]=='\0')
			SetDlgItemText(hwnd,IDC_EDIT_OUTPUT_FILE,TEXT("output.wav"));
		else
			SetDlgItemText(hwnd,IDC_EDIT_OUTPUT_FILE,TEXT(st_temp->OutputName));
		} else
			SetDlgItemText(hwnd,IDC_EDIT_OUTPUT_FILE,st_temp->OutputDirName);

		opt = st_temp->opt_playmode + 1;
		if(strchr(opt, 'U')){
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_ULAW,BM_SETCHECK,1,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_ALAW,BM_SETCHECK,0,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_LINEAR,BM_SETCHECK,0,0);
		} else if(strchr(opt, 'A')){
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_ULAW,BM_SETCHECK,0,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_ALAW,BM_SETCHECK,1,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_LINEAR,BM_SETCHECK,0,0);
		} else {
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_ULAW,BM_SETCHECK,0,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_ALAW,BM_SETCHECK,0,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_LINEAR,BM_SETCHECK,1,0);
		}
		// BANDWIDTH
		if (PlayerLanguage == LANGUAGE_JAPANESE)
		  cb_info_IDC_COMBO_BANDWIDTH = cb_info_IDC_COMBO_BANDWIDTH_jp;
		else
		  cb_info_IDC_COMBO_BANDWIDTH = cb_info_IDC_COMBO_BANDWIDTH_en;
		for (i = 0; i < cb_num_IDC_COMBO_BANDWIDTH; i++)
			SendDlgItemMessage(hwnd, IDC_COMBO_BANDWIDTH,
					CB_INSERTSTRING, (WPARAM) -1,
					(LPARAM) cb_info_IDC_COMBO_BANDWIDTH[i]);
		if (strchr(opt, '2')) {	// 24-bit
			SendDlgItemMessage(hwnd, IDC_COMBO_BANDWIDTH, CB_SETCURSEL,
					(WPARAM) BANDWIDTH_24BIT, (LPARAM) 0);
		} else if (strchr(opt, '1')) {	// 16-bit
			SendDlgItemMessage(hwnd, IDC_COMBO_BANDWIDTH, CB_SETCURSEL,
					(WPARAM) BANDWIDTH_16BIT, (LPARAM) 0);
		} else {	// 8-bit
			SendDlgItemMessage(hwnd, IDC_COMBO_BANDWIDTH, CB_SETCURSEL,
					(WPARAM) BANDWIDTH_8BIT, (LPARAM) 0);
		}
		if(strchr(opt, 's')){
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_SIGNED,BM_SETCHECK,1,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_UNSIGNED,BM_SETCHECK,0,0);
		} else {
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_SIGNED,BM_SETCHECK,0,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_UNSIGNED,BM_SETCHECK,1,0);
		}
		if(strchr(opt, 'x')){
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_BYTESWAP,BM_SETCHECK,1,0);
		} else {
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_BYTESWAP,BM_SETCHECK,0,0);
		}
		if(strchr(opt, 'M')){
			SendDlgItemMessage(hwnd,IDC_RADIO_STEREO,BM_SETCHECK,0,0);
			SendDlgItemMessage(hwnd,IDC_RADIO_MONO,BM_SETCHECK,1,0);
		} else {
			SendDlgItemMessage(hwnd,IDC_RADIO_STEREO,BM_SETCHECK,1,0);
			SendDlgItemMessage(hwnd,IDC_RADIO_MONO,BM_SETCHECK,0,0);
		}
		// SAMPLE_RATE
		for (i = 0; i < cb_num_IDC_COMBO_SAMPLE_RATE; i++)
			SendDlgItemMessage(hwnd, IDC_COMBO_SAMPLE_RATE,
					CB_INSERTSTRING, (WPARAM) -1,
					(LPARAM) cb_info_IDC_COMBO_SAMPLE_RATE[i]);
		SetDlgItemInt(hwnd, IDC_COMBO_SAMPLE_RATE, st_temp->output_rate, FALSE);
		}
		initflag = 0;
		break;
	case WM_COMMAND:
	switch (LOWORD(wParam)) {
	  case IDCLOSE:
		break;
		case IDC_BUTTON_OUTPUT_FILE:
			{
				char filename[MAXPATH+1];
				filename[0] = '\0';
				SendDlgItemMessage(hwnd,IDC_EDIT_OUTPUT_FILE,WM_GETTEXT,
					(WPARAM)MAX_PATH,(LPARAM)TEXT(filename));
				if(!DlgOpenOutputFile(filename,hwnd))
				if(filename[0]!='\0')
						SetDlgItemText(hwnd,IDC_EDIT_OUTPUT_FILE,TEXT(filename));
	   }
		break;
		case IDC_BUTTON_OUTPUT_FILE_DEL:
			{
			char filename[MAXPATH+1];
			DWORD res;
			if(st_temp->auto_output_mode>0){
				break;
			}
			GetDlgItemText(hwnd,IDC_EDIT_OUTPUT_FILE,filename,(WPARAM)MAX_PATH);
		 res = GetFileAttributes(filename);
		 if(res!=0xFFFFFFFF && !(res & FILE_ATTRIBUTE_DIRECTORY)){
				if(DeleteFile(filename)!=TRUE){
				char buffer[MAXPATH + 1024];
			   sprintf(buffer,"Can't delete file %s !",filename);
					MessageBox(NULL,buffer,"Error!", MB_OK);
				} else {
				char buffer[MAXPATH + 1024];
			   sprintf(buffer,"Delete file %s !",filename);
					MessageBox(NULL,buffer,"Delete!", MB_OK);
			}
			}
		 }
			break;
		case IDC_CHECKBOX_ULAW:
			if(SendDlgItemMessage(hwnd,IDC_CHECKBOX_ULAW,BM_GETCHECK,0,0)){
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ULAW,BM_SETCHECK,1,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ALAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_LINEAR,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd, IDC_COMBO_BANDWIDTH, CB_SETCURSEL,
					(WPARAM) BANDWIDTH_8BIT, (LPARAM) 0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_SIGNED,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_UNSIGNED,BM_SETCHECK,1,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_BYTESWAP,BM_SETCHECK,0,0);
			 } else {
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ULAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ALAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_LINEAR,BM_SETCHECK,1,0);
			 }
			break;
		case IDC_CHECKBOX_ALAW:
			if(SendDlgItemMessage(hwnd,IDC_CHECKBOX_ALAW,BM_GETCHECK,0,0)){
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ULAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ALAW,BM_SETCHECK,1,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_LINEAR,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd, IDC_COMBO_BANDWIDTH, CB_SETCURSEL,
					(WPARAM) BANDWIDTH_8BIT, (LPARAM) 0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_SIGNED,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_UNSIGNED,BM_SETCHECK,1,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_BYTESWAP,BM_SETCHECK,0,0);
			 } else {
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ULAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ALAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_LINEAR,BM_SETCHECK,1,0);
			 }
			break;
		case IDC_CHECKBOX_LINEAR:
			if(SendDlgItemMessage(hwnd,IDC_CHECKBOX_LINEAR,BM_GETCHECK,0,0)){
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ULAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ALAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_LINEAR,BM_SETCHECK,1,0);
			 } else {
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_LINEAR,BM_SETCHECK,1,0);
			 }
			break;
		case IDC_CHECKBOX_SIGNED:
			if(SendDlgItemMessage(hwnd,IDC_CHECKBOX_SIGNED,BM_GETCHECK,0,0)){
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_SIGNED,BM_SETCHECK,1,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_UNSIGNED,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ULAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ALAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_LINEAR,BM_SETCHECK,1,0);
			 } else {
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_SIGNED,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_UNSIGNED,BM_SETCHECK,1,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ULAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ALAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_LINEAR,BM_SETCHECK,1,0);
			 }
			break;
		case IDC_CHECKBOX_UNSIGNED:
			if(SendDlgItemMessage(hwnd,IDC_CHECKBOX_UNSIGNED,BM_GETCHECK,0,0)){
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_SIGNED,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_UNSIGNED,BM_SETCHECK,1,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ULAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ALAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_LINEAR,BM_SETCHECK,1,0);
			} else {
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_SIGNED,BM_SETCHECK,1,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_UNSIGNED,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ULAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ALAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_LINEAR,BM_SETCHECK,1,0);
			 }
			break;
		case IDC_CHECKBOX_BYTESWAP:
			if(SendDlgItemMessage(hwnd,IDC_CHECKBOX_BYTESWAP,BM_GETCHECK,0,0)){
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_BYTESWAP,BM_SETCHECK,1,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ULAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ALAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_LINEAR,BM_SETCHECK,1,0);
			} else {
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_BYTESWAP,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ULAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_ALAW,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_CHECKBOX_LINEAR,BM_SETCHECK,1,0);
			}
			break;
		case IDC_RADIO_STEREO:
			if(SendDlgItemMessage(hwnd,IDC_RADIO_STEREO,BM_GETCHECK,0,0)){
				SendDlgItemMessage(hwnd,IDC_RADIO_STEREO,BM_SETCHECK,1,0);
				SendDlgItemMessage(hwnd,IDC_RADIO_MONO,BM_SETCHECK,0,0);
			 } else {
				SendDlgItemMessage(hwnd,IDC_RADIO_STEREO,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_RADIO_MONO,BM_SETCHECK,1,0);
			 }
			break;
		case IDC_RADIO_MONO:
			if(SendDlgItemMessage(hwnd,IDC_RADIO_MONO,BM_GETCHECK,0,0)){
				SendDlgItemMessage(hwnd,IDC_RADIO_STEREO,BM_SETCHECK,0,0);
				SendDlgItemMessage(hwnd,IDC_RADIO_MONO,BM_SETCHECK,1,0);
		 } else {
				SendDlgItemMessage(hwnd,IDC_RADIO_STEREO,BM_SETCHECK,1,0);
				SendDlgItemMessage(hwnd,IDC_RADIO_MONO,BM_SETCHECK,0,0);
		 }
			break;
		case IDC_BUTTON_OUTPUT_OPTIONS:
			{
				int num;
				num = SendDlgItemMessage(hwnd,IDC_COMBO_OUTPUT,CB_GETCURSEL,(WPARAM)0,(LPARAM)0);
				if(num>=0){
					st_temp->opt_playmode[0]=play_mode_list[num]->id_character;
				} else {
					st_temp->opt_playmode[0]='d';
				}
#ifdef AU_VORBIS
				if(st_temp->opt_playmode[0]=='v'){
					vorbisConfigDialog();
				}
#endif
#ifdef AU_GOGO
				if(st_temp->opt_playmode[0]=='g'){
					gogoConfigDialog();
				}
#endif
			}
			break;
		case IDC_COMBO_OUTPUT_MODE:
			{
				int cb_num1, cb_num2;
				cb_num1 = SendDlgItemMessage(hwnd,IDC_COMBO_OUTPUT_MODE,CB_GETCURSEL,(WPARAM)0,(LPARAM)0);
				if (PlayerLanguage == LANGUAGE_JAPANESE)
				  cb_info_IDC_COMBO_OUTPUT_MODE = cb_info_IDC_COMBO_OUTPUT_MODE_jp;
				else
				  cb_info_IDC_COMBO_OUTPUT_MODE = cb_info_IDC_COMBO_OUTPUT_MODE_en;
				for(cb_num2=0;(int)cb_info_IDC_COMBO_OUTPUT_MODE[cb_num2];cb_num2+=2){
					if(cb_num1*2==cb_num2){
						st_temp->auto_output_mode = (int)cb_info_IDC_COMBO_OUTPUT_MODE[cb_num2+1];
						break;
					}
				}
				if (PlayerLanguage == LANGUAGE_JAPANESE) {
				  if(st_temp->auto_output_mode>0){
				    SendDlgItemMessage(hwnd,IDC_BUTTON_OUTPUT_FILE,WM_SETTEXT,0,(LPARAM)"出力先");
				    SetDlgItemText(hwnd,IDC_EDIT_OUTPUT_FILE,st_temp->OutputDirName);
				  } else {
				    SendDlgItemMessage(hwnd,IDC_BUTTON_OUTPUT_FILE,WM_SETTEXT,0,(LPARAM)"出力ファイル");
				    SetDlgItemText(hwnd,IDC_EDIT_OUTPUT_FILE,st_temp->OutputName);
				  }
				} else {
				  if(st_temp->auto_output_mode>0){
				    SendDlgItemMessage(hwnd,IDC_BUTTON_OUTPUT_FILE,WM_SETTEXT,0,(LPARAM)"Output File");
				    SetDlgItemText(hwnd,IDC_EDIT_OUTPUT_FILE,st_temp->OutputDirName);
				  } else {
				    SendDlgItemMessage(hwnd,IDC_BUTTON_OUTPUT_FILE,WM_SETTEXT,0,(LPARAM)"Output File");
				    SetDlgItemText(hwnd,IDC_EDIT_OUTPUT_FILE,st_temp->OutputName);
				  }
				}
			}
			break;
		default:
		break;
	  }
		PrefWndSetOK = 1;
		break;
	case WM_MYSAVE:
		if ( initflag ) break;
	{
		int i = 0;
		int num;
		num = SendDlgItemMessage(hwnd,IDC_COMBO_OUTPUT,CB_GETCURSEL,(WPARAM)0,(LPARAM)0);
		if(num>=0){
			st_temp->opt_playmode[i]=play_mode_list[num]->id_character;
		} else {
			st_temp->opt_playmode[i]='d';
		}
		i++;
		if(SendDlgItemMessage(hwnd,IDC_CHECKBOX_ULAW,BM_GETCHECK,0,0))
			st_temp->opt_playmode[i++] = 'U';
		if(SendDlgItemMessage(hwnd,IDC_CHECKBOX_ALAW,BM_GETCHECK,0,0))
			st_temp->opt_playmode[i++] = 'A';
		if(SendDlgItemMessage(hwnd,IDC_CHECKBOX_LINEAR,BM_GETCHECK,0,0))
			st_temp->opt_playmode[i++] = 'l';
		num = SendDlgItemMessage(hwnd, IDC_COMBO_BANDWIDTH, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
		if(num == BANDWIDTH_8BIT)
			st_temp->opt_playmode[i++] = '8';
		else if(num == BANDWIDTH_24BIT)
			st_temp->opt_playmode[i++] = '2';
		else	// 16-bit
			st_temp->opt_playmode[i++] = '1';
		if(SendDlgItemMessage(hwnd,IDC_CHECKBOX_SIGNED,BM_GETCHECK,0,0))
			st_temp->opt_playmode[i++] = 's';
		if(SendDlgItemMessage(hwnd,IDC_CHECKBOX_UNSIGNED,BM_GETCHECK,0,0))
			st_temp->opt_playmode[i++] = 'u';
		if(SendDlgItemMessage(hwnd,IDC_CHECKBOX_BYTESWAP,BM_GETCHECK,0,0))
			st_temp->opt_playmode[i++] = 'x';
		if(SendDlgItemMessage(hwnd,IDC_RADIO_STEREO,BM_GETCHECK,0,0))
			st_temp->opt_playmode[i++] = 'S';
		if(SendDlgItemMessage(hwnd,IDC_RADIO_MONO,BM_GETCHECK,0,0))
			st_temp->opt_playmode[i++] = 'M';
		st_temp->opt_playmode[i] = '\0';
		st_temp->output_rate = GetDlgItemInt(hwnd, IDC_COMBO_SAMPLE_RATE, NULL, FALSE);
 		if(st_temp->auto_output_mode==0)
			GetDlgItemText(hwnd,IDC_EDIT_OUTPUT_FILE,st_temp->OutputName,(WPARAM)sizeof(st_temp->OutputName));
		else
			GetDlgItemText(hwnd,IDC_EDIT_OUTPUT_FILE,st_temp->OutputDirName,(WPARAM)sizeof(st_temp->OutputDirName));

		SetWindowLong(hwnd,DWL_MSGRESULT,FALSE);
	}
		break;

		break;
  case WM_SIZE:
		return FALSE;
	case WM_CLOSE:
		break;
	default:
	  break;
	}
	return FALSE;
}

#define PREF_CHANNEL_MODE_DRUM_CHANNEL		1
#define PREF_CHANNEL_MODE_DRUM_CHANNEL_MASK	2
#define PREF_CHANNEL_MODE_QUIET_CHANNEL		3
static BOOL APIENTRY
PrefTiMidity4DialogProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam)
{
	static int initflag = 1; 
	static int pref_channel_mode;
	switch (uMess){
  case WM_INITDIALOG:
		pref_channel_mode = PREF_CHANNEL_MODE_DRUM_CHANNEL;
		SendMessage(hwnd,WM_MYRESTORE,(WPARAM)0,(LPARAM)0);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_TEMPER_EQUAL,
				st_temp->temper_type_mute & 1 << 0);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_TEMPER_PYTHA,
				st_temp->temper_type_mute & 1 << 1);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_TEMPER_MEANTONE,
				st_temp->temper_type_mute & 1 << 2);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_TEMPER_PUREINT,
				st_temp->temper_type_mute & 1 << 3);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_TEMPER_USER0,
				st_temp->temper_type_mute & 1 << 4);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_TEMPER_USER1,
				st_temp->temper_type_mute & 1 << 5);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_TEMPER_USER2,
				st_temp->temper_type_mute & 1 << 6);
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_TEMPER_USER3,
				st_temp->temper_type_mute & 1 << 7);
		initflag = 0;
		break;
	case WM_MYRESTORE:
	{
		ChannelBitMask channelbitmask;
		switch(pref_channel_mode){
		case PREF_CHANNEL_MODE_DRUM_CHANNEL_MASK:
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_DRUM_CHANNEL,BM_SETCHECK,0,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_DRUM_CHANNEL_MASK,BM_SETCHECK,1,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_QUIET_CHANNEL,BM_SETCHECK,0,0);
			channelbitmask = st_temp->default_drumchannel_mask;
			break;
		case PREF_CHANNEL_MODE_QUIET_CHANNEL:
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_DRUM_CHANNEL,BM_SETCHECK,0,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_DRUM_CHANNEL_MASK,BM_SETCHECK,0,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_QUIET_CHANNEL,BM_SETCHECK,1,0);
			channelbitmask = st_temp->quietchannels;
			break;
		default:
		case PREF_CHANNEL_MODE_DRUM_CHANNEL:
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_DRUM_CHANNEL,BM_SETCHECK,1,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_DRUM_CHANNEL_MASK,BM_SETCHECK,0,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_QUIET_CHANNEL,BM_SETCHECK,0,0);
			channelbitmask = st_temp->default_drumchannels;
			break;
		}
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH01,IS_SET_CHANNELMASK(channelbitmask,0));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH02,IS_SET_CHANNELMASK(channelbitmask,1));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH03,IS_SET_CHANNELMASK(channelbitmask,2));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH04,IS_SET_CHANNELMASK(channelbitmask,3));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH05,IS_SET_CHANNELMASK(channelbitmask,4));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH06,IS_SET_CHANNELMASK(channelbitmask,5));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH07,IS_SET_CHANNELMASK(channelbitmask,6));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH08,IS_SET_CHANNELMASK(channelbitmask,7));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH09,IS_SET_CHANNELMASK(channelbitmask,8));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH10,IS_SET_CHANNELMASK(channelbitmask,9));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH11,IS_SET_CHANNELMASK(channelbitmask,10));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH12,IS_SET_CHANNELMASK(channelbitmask,11));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH13,IS_SET_CHANNELMASK(channelbitmask,12));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH14,IS_SET_CHANNELMASK(channelbitmask,13));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH15,IS_SET_CHANNELMASK(channelbitmask,14));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH16,IS_SET_CHANNELMASK(channelbitmask,15));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH17,IS_SET_CHANNELMASK(channelbitmask,16));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH18,IS_SET_CHANNELMASK(channelbitmask,17));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH19,IS_SET_CHANNELMASK(channelbitmask,18));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH20,IS_SET_CHANNELMASK(channelbitmask,19));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH21,IS_SET_CHANNELMASK(channelbitmask,20));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH22,IS_SET_CHANNELMASK(channelbitmask,21));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH23,IS_SET_CHANNELMASK(channelbitmask,22));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH24,IS_SET_CHANNELMASK(channelbitmask,23));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH25,IS_SET_CHANNELMASK(channelbitmask,24));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH26,IS_SET_CHANNELMASK(channelbitmask,25));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH27,IS_SET_CHANNELMASK(channelbitmask,26));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH28,IS_SET_CHANNELMASK(channelbitmask,27));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH29,IS_SET_CHANNELMASK(channelbitmask,28));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH30,IS_SET_CHANNELMASK(channelbitmask,29));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH31,IS_SET_CHANNELMASK(channelbitmask,30));
		DLG_FLAG_TO_CHECKBUTTON(hwnd,IDC_CHECKBOX_CH32,IS_SET_CHANNELMASK(channelbitmask,31));
	}
		break;
	case WM_COMMAND:
	switch (LOWORD(wParam)) {
	  case IDCLOSE:
		break;
		case IDC_CHECKBOX_DRUM_CHANNEL:
			{
			SendMessage(hwnd,WM_MYSAVE,(WPARAM)0,(LPARAM)0);
			pref_channel_mode = PREF_CHANNEL_MODE_DRUM_CHANNEL;
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_DRUM_CHANNEL,BM_SETCHECK,1,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_DRUM_CHANNEL_MASK,BM_SETCHECK,0,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_QUIET_CHANNEL,BM_SETCHECK,0,0);
			SendMessage(hwnd,WM_MYRESTORE,(WPARAM)0,(LPARAM)0);
			}
		break;
		case IDC_CHECKBOX_DRUM_CHANNEL_MASK:
			{
			SendMessage(hwnd,WM_MYSAVE,(WPARAM)0,(LPARAM)0);
			pref_channel_mode = PREF_CHANNEL_MODE_DRUM_CHANNEL_MASK;
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_DRUM_CHANNEL,BM_SETCHECK,0,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_DRUM_CHANNEL_MASK,BM_SETCHECK,1,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_QUIET_CHANNEL,BM_SETCHECK,0,0);
			SendMessage(hwnd,WM_MYRESTORE,(WPARAM)0,(LPARAM)0);
			}
		break;
		case IDC_CHECKBOX_QUIET_CHANNEL:
			{
			SendMessage(hwnd,WM_MYSAVE,(WPARAM)0,(LPARAM)0);
			pref_channel_mode = PREF_CHANNEL_MODE_QUIET_CHANNEL;
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_DRUM_CHANNEL,BM_SETCHECK,0,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_DRUM_CHANNEL_MASK,BM_SETCHECK,0,0);
			SendDlgItemMessage(hwnd,IDC_CHECKBOX_QUIET_CHANNEL,BM_SETCHECK,1,0);
			SendMessage(hwnd,WM_MYRESTORE,(WPARAM)0,(LPARAM)0);
			}
		break;
		case IDC_BUTTON_REVERSE:
			{
			SendMessage(hwnd,WM_MYSAVE,(WPARAM)0,(LPARAM)0);
			switch(pref_channel_mode){
			case PREF_CHANNEL_MODE_DRUM_CHANNEL_MASK:
				REVERSE_CHANNELMASK(st_temp->default_drumchannel_mask);
				break;
			case PREF_CHANNEL_MODE_QUIET_CHANNEL:
				REVERSE_CHANNELMASK(st_temp->quietchannels);
				break;
			default:
			case PREF_CHANNEL_MODE_DRUM_CHANNEL:
				REVERSE_CHANNELMASK(st_temp->default_drumchannels);
				break;
			}
			SendMessage(hwnd,WM_MYRESTORE,(WPARAM)0,(LPARAM)0);
			}
			break;
		default:
		break;
	  }
		PrefWndSetOK = 1;
		break;
	case WM_MYSAVE:
		if ( initflag ) break;
	{
		ChannelBitMask channelbitmask;
		int tmp;
#define PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,ctlid,channelbitmask,ch,tmp) \
{	if(DLG_CHECKBUTTON_TO_FLAG((hwnd),(ctlid),(tmp))) SET_CHANNELMASK((channelbitmask),(ch)); \
else UNSET_CHANNELMASK((channelbitmask),(ch)); }
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH01,channelbitmask,0,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH02,channelbitmask,1,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH03,channelbitmask,2,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH04,channelbitmask,3,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH05,channelbitmask,4,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH06,channelbitmask,5,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH07,channelbitmask,6,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH08,channelbitmask,7,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH09,channelbitmask,8,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH10,channelbitmask,9,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH11,channelbitmask,10,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH12,channelbitmask,11,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH13,channelbitmask,12,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH14,channelbitmask,13,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH15,channelbitmask,14,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH16,channelbitmask,15,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH17,channelbitmask,16,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH18,channelbitmask,17,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH19,channelbitmask,18,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH20,channelbitmask,19,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH21,channelbitmask,20,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH22,channelbitmask,21,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH23,channelbitmask,22,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH24,channelbitmask,23,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH25,channelbitmask,24,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH26,channelbitmask,25,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH27,channelbitmask,26,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH28,channelbitmask,27,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH29,channelbitmask,28,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH30,channelbitmask,29,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH31,channelbitmask,30,tmp);
		PREF_CHECKBUTTON_SET_CHANNELMASK(hwnd,IDC_CHECKBOX_CH32,channelbitmask,31,tmp);
		switch(pref_channel_mode){
		case PREF_CHANNEL_MODE_DRUM_CHANNEL_MASK:
			st_temp->default_drumchannel_mask = channelbitmask;
			break;
		case PREF_CHANNEL_MODE_QUIET_CHANNEL:
			st_temp->quietchannels = channelbitmask;
			break;
		default:
		case PREF_CHANNEL_MODE_DRUM_CHANNEL:
			st_temp->default_drumchannels = channelbitmask;
			break;
		}
	}
		st_temp->temper_type_mute = 0;
		if (SendDlgItemMessage(hwnd, IDC_CHECKBOX_TEMPER_EQUAL,
				BM_GETCHECK, 0, 0))
			st_temp->temper_type_mute |= 1 << 0;
		if (SendDlgItemMessage(hwnd, IDC_CHECKBOX_TEMPER_PYTHA,
				BM_GETCHECK, 0, 0))
			st_temp->temper_type_mute |= 1 << 1;
		if (SendDlgItemMessage(hwnd, IDC_CHECKBOX_TEMPER_MEANTONE,
				BM_GETCHECK, 0, 0))
			st_temp->temper_type_mute |= 1 << 2;
		if (SendDlgItemMessage(hwnd, IDC_CHECKBOX_TEMPER_PUREINT,
				BM_GETCHECK, 0, 0))
			st_temp->temper_type_mute |= 1 << 3;
		if (SendDlgItemMessage(hwnd, IDC_CHECKBOX_TEMPER_USER0,
				BM_GETCHECK, 0, 0))
			st_temp->temper_type_mute |= 1 << 4;
		if (SendDlgItemMessage(hwnd, IDC_CHECKBOX_TEMPER_USER1,
				BM_GETCHECK, 0, 0))
			st_temp->temper_type_mute |= 1 << 5;
		if (SendDlgItemMessage(hwnd, IDC_CHECKBOX_TEMPER_USER2,
				BM_GETCHECK, 0, 0))
			st_temp->temper_type_mute |= 1 << 6;
		if (SendDlgItemMessage(hwnd, IDC_CHECKBOX_TEMPER_USER3,
				BM_GETCHECK, 0, 0))
			st_temp->temper_type_mute |= 1 << 7;
	SetWindowLong(hwnd,DWL_MSGRESULT,FALSE);
	break;
  case WM_SIZE:
		return FALSE;
	case WM_CLOSE:
		break;
	default:
	  break;
	}
	return FALSE;
}

#ifdef IA_W32G_SYN 
extern int syn_ThreadPriority;
static char **MidiINDrivers = NULL;
// 0 MIDI Mapper -1
// 1 MIDI IN Driver 0
// 2 MIDI IN Driver 1
static char **GetMidiINDrivers ( void )
{
	int i;
	int max = midiInGetNumDevs ();
	if ( MidiINDrivers != NULL ) {
		for ( i = 0; MidiINDrivers[i] != NULL; i ++ ) {
			free ( MidiINDrivers[i] );
		}
		free ( MidiINDrivers );
		MidiINDrivers = NULL;
	}
	MidiINDrivers = ( char ** ) malloc ( sizeof ( char * ) * ( max + 2 ) );
	if ( MidiINDrivers == NULL ) return MidiINDrivers;
	MidiINDrivers[0] = strdup ( "MIDI Mapper" );
	for ( i = 1; i <= max; i ++ ) {
		MIDIINCAPS mic;
		if ( midiInGetDevCaps ( i - 1, &mic, sizeof ( MIDIINCAPS ) ) == 0 ) {
			MidiINDrivers[i] = strdup ( mic.szPname );
			if ( MidiINDrivers[i] == NULL )
				break;
		} else {
			MidiINDrivers[i] = NULL;
			break;
		}
		MidiINDrivers[max+1] = NULL;
	}
	return MidiINDrivers;
}

static BOOL APIENTRY
PrefSyn1DialogProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam)
{
	int i;
	static int initflag = 1; 
	switch (uMess){
  case WM_INITDIALOG:
		for ( i = 0; i <= MAX_PORT; i ++ ) {
			char buff[32];
			sprintf ( buff, "%d", i );
			SendDlgItemMessage(hwnd, IDC_COMBO_PORT_NUM,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) buff );
		}
		if ( MidiINDrivers != NULL ) {
			for ( i = 0; MidiINDrivers[i] != NULL; i ++ ) {
				SendDlgItemMessage(hwnd, IDC_COMBO_IDPORT0,
					CB_INSERTSTRING, (WPARAM) -1, (LPARAM) MidiINDrivers[i] );
				SendDlgItemMessage(hwnd, IDC_COMBO_IDPORT1,
					CB_INSERTSTRING, (WPARAM) -1, (LPARAM) MidiINDrivers[i] );
				SendDlgItemMessage(hwnd, IDC_COMBO_IDPORT2,
					CB_INSERTSTRING, (WPARAM) -1, (LPARAM) MidiINDrivers[i] );
				SendDlgItemMessage(hwnd, IDC_COMBO_IDPORT3,
					CB_INSERTSTRING, (WPARAM) -1, (LPARAM) MidiINDrivers[i] );
			}
		}
		DLG_FLAG_TO_CHECKBUTTON(hwnd, IDC_CHECK_SYN_AUTOSTART, st_temp->syn_AutoStart);
		if (PlayerLanguage == LANGUAGE_JAPANESE) {
			SendDlgItemMessage(hwnd, IDC_COMBO_PROCESS_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "低い" );
			SendDlgItemMessage(hwnd, IDC_COMBO_PROCESS_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "少し低い" );
			SendDlgItemMessage(hwnd, IDC_COMBO_PROCESS_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "普通" );
			SendDlgItemMessage(hwnd, IDC_COMBO_PROCESS_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "少し高い" );
			SendDlgItemMessage(hwnd, IDC_COMBO_PROCESS_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "高い" );
			SendDlgItemMessage(hwnd, IDC_COMBO_PROCESS_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "リアルタイム" );

			SendDlgItemMessage(hwnd, IDC_COMBO_SYN_THREAD_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "低い" );
			SendDlgItemMessage(hwnd, IDC_COMBO_SYN_THREAD_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "少し低い" );
			SendDlgItemMessage(hwnd, IDC_COMBO_SYN_THREAD_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "普通" );
			SendDlgItemMessage(hwnd, IDC_COMBO_SYN_THREAD_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "少し高い" );
			SendDlgItemMessage(hwnd, IDC_COMBO_SYN_THREAD_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "高い" );
			SendDlgItemMessage(hwnd, IDC_COMBO_SYN_THREAD_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "タイムクリティカル" );
		} else {
			SendDlgItemMessage(hwnd, IDC_COMBO_PROCESS_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "Lowest" );
			SendDlgItemMessage(hwnd, IDC_COMBO_PROCESS_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "Below normal" );
			SendDlgItemMessage(hwnd, IDC_COMBO_PROCESS_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "Normal" );
			SendDlgItemMessage(hwnd, IDC_COMBO_PROCESS_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "Above nomal" );
			SendDlgItemMessage(hwnd, IDC_COMBO_PROCESS_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "Highest" );
			SendDlgItemMessage(hwnd, IDC_COMBO_PROCESS_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "Realtime" );

			SendDlgItemMessage(hwnd, IDC_COMBO_SYN_THREAD_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "Lowest" );
			SendDlgItemMessage(hwnd, IDC_COMBO_SYN_THREAD_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "Below normal" );
			SendDlgItemMessage(hwnd, IDC_COMBO_SYN_THREAD_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "Normal" );
			SendDlgItemMessage(hwnd, IDC_COMBO_SYN_THREAD_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "Above nomal" );
			SendDlgItemMessage(hwnd, IDC_COMBO_SYN_THREAD_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "Highest" );
			SendDlgItemMessage(hwnd, IDC_COMBO_SYN_THREAD_PRIORITY,
				CB_INSERTSTRING, (WPARAM) -1, (LPARAM) "Time critical" );
		}
		SendDlgItemMessage(hwnd, IDC_COMBO_PORT_NUM,
			CB_SETCURSEL, (WPARAM) st_temp->SynPortNum, (LPARAM) 0 );
		SendDlgItemMessage(hwnd, IDC_COMBO_IDPORT0,
			CB_SETCURSEL, (WPARAM) st_temp->SynIDPort[0], (LPARAM) 0 );
		SendDlgItemMessage(hwnd, IDC_COMBO_IDPORT1,
			CB_SETCURSEL, (WPARAM) st_temp->SynIDPort[1], (LPARAM) 0 );
		SendDlgItemMessage(hwnd, IDC_COMBO_IDPORT2,
			CB_SETCURSEL, (WPARAM) st_temp->SynIDPort[2], (LPARAM) 0 );
		SendDlgItemMessage(hwnd, IDC_COMBO_IDPORT3,
			CB_SETCURSEL, (WPARAM) st_temp->SynIDPort[3], (LPARAM) 0 );
		{
			int index;

			// Select process priority
			if ( st_temp->processPriority == IDLE_PRIORITY_CLASS )
				index = 0;
			else if ( st_temp->processPriority == BELOW_NORMAL_PRIORITY_CLASS )
				index = 1;
			else if ( st_temp->processPriority == NORMAL_PRIORITY_CLASS )
				index = 2;
			else if ( st_temp->processPriority == ABOVE_NORMAL_PRIORITY_CLASS )
				index = 3;
			else if ( st_temp->processPriority == HIGH_PRIORITY_CLASS )
				index = 4;
			else if ( st_temp->processPriority == REALTIME_PRIORITY_CLASS )
				index = 5;
			SendDlgItemMessage(hwnd, IDC_COMBO_PROCESS_PRIORITY,
				CB_SETCURSEL, (WPARAM) index, (LPARAM) 0 );

			// Select thread priority
			if ( st_temp->syn_ThreadPriority == THREAD_PRIORITY_LOWEST )
				index = 0;
			else if ( st_temp->syn_ThreadPriority == THREAD_PRIORITY_BELOW_NORMAL )
				index = 1;
			else if ( st_temp->syn_ThreadPriority == THREAD_PRIORITY_NORMAL )
				index = 2;
			else if ( st_temp->syn_ThreadPriority == THREAD_PRIORITY_ABOVE_NORMAL )
				index = 3;
			else if ( st_temp->syn_ThreadPriority == THREAD_PRIORITY_HIGHEST )
				index = 4;
			else if ( st_temp->syn_ThreadPriority == THREAD_PRIORITY_TIME_CRITICAL )
				index = 5;
			SendDlgItemMessage(hwnd, IDC_COMBO_SYN_THREAD_PRIORITY,
				CB_SETCURSEL, (WPARAM) index, (LPARAM) 0 );
		}
		SetDlgItemInt(hwnd,IDC_EDIT_SYN_SH_TIME,st_temp->SynShTime,FALSE);
		initflag = 0;
		break;
	case WM_COMMAND:
	switch (LOWORD(wParam)) {
		case IDC_RADIOBUTTON_JAPANESE:
		case IDC_RADIOBUTTON_ENGLISH:
			break;
		default:
		break;
	  }
		PrefWndSetOK = 1;
		break;

	case WM_MYSAVE:
		if ( initflag ) break;
	{
		int res;
		res = SendDlgItemMessage ( hwnd, IDC_COMBO_PORT_NUM, CB_GETCURSEL, 0, 0 );
		if ( res != CB_ERR ) st_temp->SynPortNum = res;
		res = SendDlgItemMessage ( hwnd, IDC_COMBO_IDPORT0, CB_GETCURSEL, 0, 0 );
		if ( res != CB_ERR ) st_temp->SynIDPort[0] = res;
		res = SendDlgItemMessage ( hwnd, IDC_COMBO_IDPORT1, CB_GETCURSEL, 0, 0 );
		if ( res != CB_ERR ) st_temp->SynIDPort[1] = res;
		res = SendDlgItemMessage ( hwnd, IDC_COMBO_IDPORT2, CB_GETCURSEL, 0, 0 );
		if ( res != CB_ERR ) st_temp->SynIDPort[2] = res;
		res = SendDlgItemMessage ( hwnd, IDC_COMBO_IDPORT3, CB_GETCURSEL, 0, 0 );
		if ( res != CB_ERR ) st_temp->SynIDPort[3] = res;
		SetWindowLong(hwnd,DWL_MSGRESULT,FALSE);
		DLG_CHECKBUTTON_TO_FLAG(hwnd, IDC_CHECK_SYN_AUTOSTART, st_temp->syn_AutoStart);
		// Set process priority
		res = SendDlgItemMessage ( hwnd, IDC_COMBO_PROCESS_PRIORITY, CB_GETCURSEL, 0, 0 );
		if ( res != CB_ERR ) {
			switch ( res ) {
			case 0:
				st_temp->processPriority = IDLE_PRIORITY_CLASS;
				break;
			case 1:
				st_temp->processPriority = BELOW_NORMAL_PRIORITY_CLASS;
				break;
			default:
			case 2:
				st_temp->processPriority = NORMAL_PRIORITY_CLASS;
				break;
			case 3:
				st_temp->processPriority = ABOVE_NORMAL_PRIORITY_CLASS;
				break;
			case 4:
				st_temp->processPriority = HIGH_PRIORITY_CLASS;
				break;
			case 5:
				st_temp->processPriority = REALTIME_PRIORITY_CLASS;
				break;
			}
		}

		// Set thread priority
		res = SendDlgItemMessage ( hwnd, IDC_COMBO_SYN_THREAD_PRIORITY, CB_GETCURSEL, 0, 0 );
		if ( res != CB_ERR ) {
			switch ( res ) {
			case 0:
				st_temp->syn_ThreadPriority = THREAD_PRIORITY_LOWEST;
				break;
			case 1:
				st_temp->syn_ThreadPriority = THREAD_PRIORITY_BELOW_NORMAL;
				break;
			default:
			case 2:
				st_temp->syn_ThreadPriority = THREAD_PRIORITY_NORMAL;
				break;
			case 3:
				st_temp->syn_ThreadPriority = THREAD_PRIORITY_ABOVE_NORMAL;
				break;
			case 4:
				st_temp->syn_ThreadPriority = THREAD_PRIORITY_HIGHEST;
				break;
			case 5:
				st_temp->syn_ThreadPriority = THREAD_PRIORITY_TIME_CRITICAL;
				break;
			}
		}

		st_temp->SynShTime = GetDlgItemInt(hwnd,IDC_EDIT_SYN_SH_TIME,NULL,FALSE);
		if ( st_temp->SynShTime < 0 ) st_temp->SynShTime = 0;
	}
		break;
  case WM_SIZE:
		return FALSE;
	case WM_CLOSE:
		break;
	default:
	  break;
	}
	return FALSE;
}
#endif

void ShowPrefWnd ( void )
{
	if ( IsWindow ( (HWND)hPrefWnd ) )
		ShowWindow ( (HWND)hPrefWnd, SW_SHOW );
}
void HidePrefWnd ( void )
{
	if ( IsWindow ( (HWND)hPrefWnd ) )
		ShowWindow ( (HWND)hPrefWnd, SW_HIDE );
}
BOOL IsVisiblePrefWnd ( void )
{
	return IsWindowVisible ( (HWND)hPrefWnd );
}

static int DlgOpenConfigFile(char *Filename, HWND hwnd)
{
	OPENFILENAME ofn;
   char filename[MAXPATH + 256];
   char dir[MAXPATH + 256];
   int res;

   strncpy(dir,ConfigFileOpenDir,MAXPATH);
   dir[MAXPATH-1] = '\0';
   strncpy(filename,Filename,MAXPATH);
   filename[MAXPATH-1] = '\0';

	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.hInstance = hInst ;
	ofn.lpstrFilter =
		"config file\0*.cfg;*.config\0"
		"all files\0*.*\0"
		"\0\0";
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1 ;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = 256;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	if(dir[0] != '\0')
		ofn.lpstrInitialDir	= dir;
	else
		ofn.lpstrInitialDir	= 0;
	ofn.lpstrTitle	= "Open Config File";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER
	| OFN_READONLY | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = 0;
	ofn.lCustData = 0;
	ofn.lpfnHook = 0;
	ofn.lpTemplateName= 0;

	res = GetOpenFileName(&ofn);
   strncpy(ConfigFileOpenDir,dir,MAXPATH);
   ConfigFileOpenDir[MAXPATH-1] = '\0';
   if(res==TRUE){
	strncpy(Filename,filename,MAXPATH);
	Filename[MAXPATH-1] = '\0';
	return 0;
	}
   else {
	Filename[0] = '\0';
	return -1;
	}
}

static int DlgOpenOutputFile(char *Filename, HWND hwnd)
{
	OPENFILENAME ofn;
   char filename[MAXPATH + 256];
   char dir[MAXPATH + 256];
   int res;
	static char OutputFileOpenDir[MAXPATH+256];
   static int initflag = 1;

   if(initflag){
	OutputFileOpenDir[0] = '\0';
	  initflag = 0;
   }
   strncpy(dir,OutputFileOpenDir,MAXPATH);
   dir[MAXPATH-1] = '\0';
   strncpy(filename,Filename,MAXPATH);
   filename[MAXPATH-1] = '\0';

	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.hInstance = hInst ;
	ofn.lpstrFilter =
		"wave file\0*.wav;*.wave;*.aif;*.aiff;*.aifc;*.au;*.snd;*.audio\0"
		"all files\0*.*\0"
		"\0\0";
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1 ;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = 256;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	if(dir[0] != '\0')
		ofn.lpstrInitialDir	= dir;
	else
		ofn.lpstrInitialDir	= 0;
	ofn.lpstrTitle	= "Output File";
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = 0;
	ofn.lCustData = 0;
	ofn.lpfnHook = 0;
	ofn.lpTemplateName= 0;

	res = GetSaveFileName(&ofn);
   strncpy(OutputFileOpenDir,dir,MAXPATH);
   OutputFileOpenDir[MAXPATH-1] = '\0';
   if(res==TRUE){
	strncpy(Filename,filename,MAXPATH);
	Filename[MAXPATH-1] = '\0';
	return 0;
	} else {
	Filename[0] = '\0';
	return -1;
	}
}

volatile int w32g_interactive_id3_tag_set = 0;
int w32g_gogo_id3_tag_dialog(void)
{
	return 0;
}


#ifdef AU_GOGO
///////////////////////////////////////////////////////////////////////
//
// gogo ConfigDialog
//
///////////////////////////////////////////////////////////////////////

// id のコンボボックスの情報の定義
#define CB_INFO_TYPE1_BEGIN(id) static int cb_info_ ## id [] = {
#define CB_INFO_TYPE1_END };
#define CB_INFO_TYPE2_BEGIN(id) static char * cb_info_ ## id [] = {
#define CB_INFO_TYPE2_END };

// cb_info_type1_ＩＤ  cb_info_type2_ＩＤ というふうになる。

// IDC_COMBO_OUTPUT_FORMAT
CB_INFO_TYPE2_BEGIN(IDC_COMBO_OUTPUT_FORMAT)
	"MP3+TAG",(char *)MC_OUTPUT_NORMAL,
	"RIFF/WAVE",(char *)MC_OUTPUT_RIFF_WAVE,
	"RIFF/RMP",(char *)MC_OUTPUT_RIFF_RMP,
	NULL
CB_INFO_TYPE2_END

// IDC_COMBO_MPEG1_AUDIO_BITRATE
CB_INFO_TYPE1_BEGIN(IDC_COMBO_MPEG1_AUDIO_BITRATE)
	32,40,48,56,64,80,96,112,128,160,192,224,256,320,-1
CB_INFO_TYPE1_END

// IDC_COMBO_MPEG2_AUDIO_BITRATE
CB_INFO_TYPE1_BEGIN(IDC_COMBO_MPEG2_AUDIO_BITRATE)
	8,16,24,32,40,48,56,64,80,96,112,128,144,160,-1
CB_INFO_TYPE1_END

// IDC_COMBO_ENCODE_MODE
CB_INFO_TYPE2_BEGIN(IDC_COMBO_ENCODE_MODE)
	"monoral",(char *)MC_MODE_MONO,
	"stereo",(char *)MC_MODE_STEREO,
	"joint stereo",(char *)MC_MODE_JOINT,
	"mid/side stereo",(char *)MC_MODE_MSSTEREO,
	"dual channel",(char *)MC_MODE_DUALCHANNEL,
	NULL
CB_INFO_TYPE2_END

// IDC_COMBO_EMPHASIS_TYPE
CB_INFO_TYPE2_BEGIN(IDC_COMBO_EMPHASIS_TYPE)
	"NONE",(char *)MC_EMP_NONE,
	"50/15ms (normal CD-DA emphasis)",(char *)MC_EMP_5015MS,
	"CCITT",(char *)MC_EMP_CCITT,
	NULL
CB_INFO_TYPE2_END

// IDC_COMBO_VBR_BITRATE_LOW
CB_INFO_TYPE1_BEGIN(IDC_COMBO_VBR_BITRATE_LOW)
	32,40,48,56,64,80,96,112,128,160,192,224,256,320,-1
CB_INFO_TYPE1_END

// IDC_COMBO_VBR_BITRATE_HIGH
CB_INFO_TYPE1_BEGIN(IDC_COMBO_VBR_BITRATE_HIGH)
	32,40,48,56,64,80,96,112,128,160,192,224,256,320,-1
CB_INFO_TYPE1_END

// IDC_COMBO_VBR
CB_INFO_TYPE2_BEGIN(IDC_COMBO_VBR)
	"Quality 0 (320 - 32 kbps)",(char *)0,
	"Quality 1 (256 - 32 kbps)",(char *)1,
	"Quality 2 (256 - 32 kbps)",(char *)2,
	"Quality 3 (256 - 32 kbps)",(char *)3,
	"Quality 4 (256 - 32 kbps)",(char *)4,
	"Quality 5 (224 - 32 kbps)",(char *)5,
	"Quality 6 (192 - 32 kbps)",(char *)6,
	"Quality 7 (160 - 32 kbps)",(char *)7,
	"Quality 8 (128 - 32 kbps)",(char *)8,
	"Quality 9 (128 - 32 kbps)",(char *)9,
	NULL
CB_INFO_TYPE2_END

// id のコンボボックスを選択の設定する。
#define CB_SETCURSEL_TYPE1(id) \
{ \
	int cb_num; \
	for(cb_num=0;(int)cb_info_ ## id [cb_num]>=0;cb_num++){ \
		SendDlgItemMessage(hwnd,id,CB_SETCURSEL,(WPARAM)0,(LPARAM)0); \
		if(gogo_ConfigDialogInfo.opt ## id == (int) cb_info_ ## id [cb_num]){ \
			SendDlgItemMessage(hwnd,id,CB_SETCURSEL,(WPARAM)cb_num,(LPARAM)0); \
			break; \
		} \
	} \
}
#define CB_SETCURSEL_TYPE2(id) \
{ \
	int cb_num; \
	for(cb_num=0;(int)cb_info_ ## id [cb_num];cb_num+=2){ \
		SendDlgItemMessage(hwnd,id,CB_SETCURSEL,(WPARAM)0,(LPARAM)0); \
	    if(gogo_ConfigDialogInfo.opt ## id == (int) cb_info_ ## id [cb_num+1]){ \
			SendDlgItemMessage(hwnd,id,CB_SETCURSEL,(WPARAM)cb_num/2,(LPARAM)0); \
			break; \
		} \
	} \
}
// id のコンボボックスの選択を変数に代入する。
#define CB_GETCURSEL_TYPE1(id) \
{ \
	int cb_num1, cb_num2; \
	cb_num1 = SendDlgItemMessage(hwnd,id,CB_GETCURSEL,(WPARAM)0,(LPARAM)0); \
	for(cb_num2=0;(int)cb_info_ ## id [cb_num2]>=0;cb_num2++) \
		if(cb_num1==cb_num2){ \
			gogo_ConfigDialogInfo.opt ## id = (int)cb_info_ ## id [cb_num2]; \
			break; \
		} \
}
#define CB_GETCURSEL_TYPE2(id) \
{ \
	int cb_num1, cb_num2; \
	cb_num1 = SendDlgItemMessage(hwnd,id,CB_GETCURSEL,(WPARAM)0,(LPARAM)0); \
	for(cb_num2=0;(int)cb_info_ ## id [cb_num2];cb_num2+=2) \
		if(cb_num1*2==cb_num2){ \
			gogo_ConfigDialogInfo.opt ## id = (int)cb_info_ ## id [cb_num2+1]; \
			break; \
		} \
}
// チェックされているか。
#define IS_CHECK(id) SendDlgItemMessage(hwnd,id,BM_GETCHECK,0,0)
// チェックする。
#define CHECK(id) SendDlgItemMessage(hwnd,id,BM_SETCHECK,1,0)
// チェックをはずす。
#define UNCHECK(id) SendDlgItemMessage(hwnd,id,BM_SETCHECK,0,0)
// id のチェックボックスを設定する。
#define CHECKBOX_SET(id) \
	if(gogo_ConfigDialogInfo.opt ## id>0) \
		SendDlgItemMessage(hwnd,id,BM_SETCHECK,1,0); \
	else \
		SendDlgItemMessage(hwnd,id,BM_SETCHECK,0,0); \
// id のチェックボックスを変数に代入する。
#define CHECKBOX_GET(id) \
	if(SendDlgItemMessage(hwnd,id,BM_GETCHECK,0,0)) \
		gogo_ConfigDialogInfo.opt ## id = 1; \
	else \
		gogo_ConfigDialogInfo.opt ## id = 0; \
// id のエディットを設定する。
#define EDIT_SET(id) SendDlgItemMessage(hwnd,id,WM_SETTEXT,0,(LPARAM)gogo_ConfigDialogInfo.opt ## id);
// id のエディットを変数に代入する。
#define EDIT_GET(id,size) SendDlgItemMessage(hwnd,id,WM_GETTEXT,(WPARAM)size,(LPARAM)gogo_ConfigDialogInfo.opt ## id);
#define EDIT_GET_RANGE(id,size,min,max) \
{ \
	char tmpbuf[1024]; \
	int value; \
	SendDlgItemMessage(hwnd,id,WM_GETTEXT,(WPARAM)size,(LPARAM)gogo_ConfigDialogInfo.opt ## id); \
	value = atoi((char *)gogo_ConfigDialogInfo.opt ## id); \
	if(value<min) value = min; \
	if(value>max) value = max; \
	sprintf(tmpbuf,"%d",value); \
	strncpy((char *)gogo_ConfigDialogInfo.opt ## id,tmpbuf,size); \
	(gogo_ConfigDialogInfo.opt ## id)[size] = '\0'; \
}
// コントロールの有効化
#define ENABLE_CONTROL(id) EnableWindow(GetDlgItem(hwnd,id),TRUE);
// コントロールの無効化
#define DISABLE_CONTROL(id) EnableWindow(GetDlgItem(hwnd,id),FALSE);

static void gogoConfigDialogProcControlEnableDisable(HWND hwnd);
static void gogoConfigDialogProcControlApply(HWND hwnd);
static void gogoConfigDialogProcControlReset(HWND hwnd);
static int gogo_ConfigDialogInfoLock();
static int gogo_ConfigDialogInfoUnLock();
static HANDLE hgogoConfigDailog = NULL;
static BOOL APIENTRY gogoConfigDialogProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam)
{
	char buff[1024];
	switch (uMess){
	case WM_INITDIALOG:
	{
		int i;
		// コンボボックスの初期化
		for(i=0;cb_info_IDC_COMBO_OUTPUT_FORMAT[i];i+=2){
			SendDlgItemMessage(hwnd,IDC_COMBO_OUTPUT_FORMAT,CB_INSERTSTRING,(WPARAM)-1,(LPARAM)cb_info_IDC_COMBO_OUTPUT_FORMAT[i]);
		}
		for(i=0;cb_info_IDC_COMBO_MPEG1_AUDIO_BITRATE[i]>=0;i++){
			sprintf(buff,"%d kbit/sec",cb_info_IDC_COMBO_MPEG1_AUDIO_BITRATE[i]);
			SendDlgItemMessage(hwnd,IDC_COMBO_MPEG1_AUDIO_BITRATE,CB_INSERTSTRING,(WPARAM)-1,(LPARAM)buff);
		}
		for(i=0;cb_info_IDC_COMBO_MPEG2_AUDIO_BITRATE[i]>=0;i++){
			sprintf(buff,"%d kbit/sec",cb_info_IDC_COMBO_MPEG2_AUDIO_BITRATE[i]);
			SendDlgItemMessage(hwnd,IDC_COMBO_MPEG2_AUDIO_BITRATE,CB_INSERTSTRING,(WPARAM)-1,(LPARAM)buff);
		}
		for(i=0;cb_info_IDC_COMBO_ENCODE_MODE[i];i+=2){
			SendDlgItemMessage(hwnd,IDC_COMBO_ENCODE_MODE,CB_INSERTSTRING,(WPARAM)-1,(LPARAM)cb_info_IDC_COMBO_ENCODE_MODE[i]);
		}
		for(i=0;cb_info_IDC_COMBO_EMPHASIS_TYPE[i];i+=2){
			SendDlgItemMessage(hwnd,IDC_COMBO_EMPHASIS_TYPE,CB_INSERTSTRING,(WPARAM)-1,(LPARAM)cb_info_IDC_COMBO_EMPHASIS_TYPE[i]);
		}
		for(i=0;cb_info_IDC_COMBO_VBR_BITRATE_LOW[i]>=0;i++){
			sprintf(buff,"%d kbit/sec",cb_info_IDC_COMBO_VBR_BITRATE_LOW[i]);
			SendDlgItemMessage(hwnd,IDC_COMBO_VBR_BITRATE_LOW,CB_INSERTSTRING,(WPARAM)-1,(LPARAM)buff);
		}
		for(i=0;cb_info_IDC_COMBO_VBR_BITRATE_HIGH[i]>=0;i++){
			sprintf(buff,"%d kbit/sec",cb_info_IDC_COMBO_VBR_BITRATE_HIGH[i]);
			SendDlgItemMessage(hwnd,IDC_COMBO_VBR_BITRATE_HIGH,CB_INSERTSTRING,(WPARAM)-1,(LPARAM)buff);
		}
		for(i=0;cb_info_IDC_COMBO_VBR[i];i+=2){
			SendDlgItemMessage(hwnd,IDC_COMBO_VBR,CB_INSERTSTRING,(WPARAM)-1,(LPARAM)cb_info_IDC_COMBO_VBR[i]);
		}
		// 設定
		gogoConfigDialogProcControlReset(hwnd);
	}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCLOSE:
			SendMessage(hwnd,WM_CLOSE,(WPARAM)0,(LPARAM)0);
			break;
		case IDOK:
			gogoConfigDialogProcControlApply(hwnd);
			SendMessage(hwnd,WM_CLOSE,(WPARAM)0,(LPARAM)0);
			break;
		case IDCANCEL:
			SendMessage(hwnd,WM_CLOSE,(WPARAM)0,(LPARAM)0);
			break;
		case IDC_BUTTON_APPLY:
			gogoConfigDialogProcControlApply(hwnd);
			break;
		case IDC_CHECK_DEFAULT:
			gogoConfigDialogProcControlEnableDisable(hwnd);
			break;
		case IDC_CHECK_COMMANDLINE_OPTS:
			gogoConfigDialogProcControlEnableDisable(hwnd);
			break;
		case IDC_EDIT_COMMANDLINE_OPTION:
			break;
		case IDC_CHECK_OUTPUT_FORMAT:
			gogoConfigDialogProcControlEnableDisable(hwnd);
			break;
		case IDC_COMBO_OUTPUT_FORMAT:
			break;
		case IDC_CHECK_MPEG1AUDIOBITRATE:
			if(IS_CHECK(IDC_CHECK_MPEG1AUDIOBITRATE)){
				CHECK(IDC_CHECK_MPEG2AUDIOBITRATE);
				UNCHECK(IDC_CHECK_VBR);
				UNCHECK(IDC_CHECK_VBR_BITRATE);
			} else {
				UNCHECK(IDC_CHECK_MPEG2AUDIOBITRATE);
			}
			gogoConfigDialogProcControlEnableDisable(hwnd);
			break;
		case IDC_COMBO_MPEG1_AUDIO_BITRATE:
			break;
		case IDC_CHECK_MPEG2AUDIOBITRATE:
			if(IS_CHECK(IDC_CHECK_MPEG2AUDIOBITRATE)){
				CHECK(IDC_CHECK_MPEG1AUDIOBITRATE);
				UNCHECK(IDC_CHECK_VBR);
				UNCHECK(IDC_CHECK_VBR_BITRATE);
			} else {
				UNCHECK(IDC_CHECK_MPEG1AUDIOBITRATE);
			}
			gogoConfigDialogProcControlEnableDisable(hwnd);
			break;
		case IDC_COMBO_MPEG2_AUDIO_BITRATE:
			break;
		case IDC_CHECK_ENHANCED_LOW_PASS_FILTER:
			if(IS_CHECK(IDC_CHECK_ENHANCED_LOW_PASS_FILTER)){
				UNCHECK(IDC_CHECK_16KHZ_LOW_PASS_FILTER);
			}
			gogoConfigDialogProcControlEnableDisable(hwnd);
			break;
		case IDC_EDIT_LPF_PARA1:
			break;
		case IDC_EDIT_LPF_PARA2:
			break;
		case IDC_CHECK_ENCODE_MODE:
			gogoConfigDialogProcControlEnableDisable(hwnd);
			break;
		case IDC_COMBO_ENCODE_MODE:
			break;
		case IDC_CHECK_EMPHASIS_TYPE:
			gogoConfigDialogProcControlEnableDisable(hwnd);
			break;
		case IDC_COMBO_EMPHASIS_TYPE:
			break;
		case IDC_CHECK_OUTFREQ:
			gogoConfigDialogProcControlEnableDisable(hwnd);
			break;
		case IDC_EDIT_OUTFREQ:
			break;
		case IDC_CHECK_MSTHRESHOLD:
			gogoConfigDialogProcControlEnableDisable(hwnd);
			break;
		case IDC_EDIT_MSTHRESHOLD_THRESHOLD:
			break;
		case IDC_EDIT_MSTHRESHOLD_MSPOWER:
			break;
		case IDC_CHECK_USE_CPU_OPTS:
			gogoConfigDialogProcControlEnableDisable(hwnd);
			break;
		case IDC_CHECK_CPUMMX:
			break;
		case IDC_CHECK_CPUSSE:
			break;
		case IDC_CHECK_CPU3DNOW:
			break;
		case IDC_CHECK_CPUE3DNOW:
			break;
		case IDC_CHECK_CPUCMOV:
			break;
		case IDC_CHECK_CPUEMMX:
			break;
		case IDC_CHECK_CPUSSE2:
			break;
		case IDC_CHECK_VBR:
			if(IS_CHECK(IDC_CHECK_VBR)){
				UNCHECK(IDC_COMBO_MPEG1_AUDIO_BITRATE);
				UNCHECK(IDC_COMBO_MPEG2_AUDIO_BITRATE);
			}
			gogoConfigDialogProcControlEnableDisable(hwnd);
			break;
		case IDC_COMBO_VBR:
			break;
		case IDC_CHECK_VBR_BITRATE:
			if(IS_CHECK(IDC_CHECK_VBR_BITRATE)){
				CHECK(IDC_CHECK_VBR);
				UNCHECK(IDC_COMBO_MPEG1_AUDIO_BITRATE);
				UNCHECK(IDC_COMBO_MPEG2_AUDIO_BITRATE);
			}
			gogoConfigDialogProcControlEnableDisable(hwnd);
			break;
		case IDC_COMBO_VBR_BITRATE_LOW:
			break;
		case IDC_COMBO_VBR_BITRATE_HIGH:
			break;
		case IDC_CHECK_USEPSY:
			break;
		case IDC_CHECK_VERIFY:
			break;
		case IDC_CHECK_16KHZ_LOW_PASS_FILTER:
			if(IS_CHECK(IDC_CHECK_16KHZ_LOW_PASS_FILTER)){
				UNCHECK(IDC_CHECK_ENHANCED_LOW_PASS_FILTER);
			}
			gogoConfigDialogProcControlEnableDisable(hwnd);
			break;
		default:
			break;
		}
		break;
	case WM_NOTIFY:
		break;
	case WM_SIZE:
		return FALSE;
	case WM_CLOSE:
		gogo_ConfigDialogInfoSaveINI();
//MessageBox(NULL,"CLOSE","CLOSE",MB_OK);
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		hgogoConfigDailog = NULL;
//MessageBox(NULL,"DESTROY","DESTROY",MB_OK);
		break;
	default:
		break;
	}
	return FALSE;
}

// コントロールの有効 / 無効化
static void gogoConfigDialogProcControlEnableDisable(HWND hwnd)
{
	ENABLE_CONTROL(IDC_CHECK_DEFAULT);
	if(IS_CHECK(IDC_CHECK_DEFAULT)){
		DISABLE_CONTROL(IDC_CHECK_COMMANDLINE_OPTS);
		DISABLE_CONTROL(IDC_EDIT_COMMANDLINE_OPTION);
		DISABLE_CONTROL(IDC_CHECK_OUTPUT_FORMAT);
		DISABLE_CONTROL(IDC_COMBO_OUTPUT_FORMAT);
		DISABLE_CONTROL(IDC_CHECK_MPEG1AUDIOBITRATE);
		DISABLE_CONTROL(IDC_COMBO_MPEG1_AUDIO_BITRATE);
		DISABLE_CONTROL(IDC_CHECK_MPEG2AUDIOBITRATE);
		DISABLE_CONTROL(IDC_COMBO_MPEG2_AUDIO_BITRATE);
		DISABLE_CONTROL(IDC_CHECK_ENHANCED_LOW_PASS_FILTER);
		DISABLE_CONTROL(IDC_EDIT_LPF_PARA1);
		DISABLE_CONTROL(IDC_EDIT_LPF_PARA2);
		DISABLE_CONTROL(IDC_CHECK_ENCODE_MODE);
		DISABLE_CONTROL(IDC_COMBO_ENCODE_MODE);
		DISABLE_CONTROL(IDC_CHECK_EMPHASIS_TYPE);
		DISABLE_CONTROL(IDC_COMBO_EMPHASIS_TYPE);
		DISABLE_CONTROL(IDC_CHECK_OUTFREQ);
		DISABLE_CONTROL(IDC_EDIT_OUTFREQ);
		DISABLE_CONTROL(IDC_CHECK_MSTHRESHOLD);
		DISABLE_CONTROL(IDC_EDIT_MSTHRESHOLD_THRESHOLD);
		DISABLE_CONTROL(IDC_EDIT_MSTHRESHOLD_MSPOWER);
		DISABLE_CONTROL(IDC_CHECK_USE_CPU_OPTS);
		DISABLE_CONTROL(IDC_CHECK_CPUMMX);
		DISABLE_CONTROL(IDC_CHECK_CPUSSE);
		DISABLE_CONTROL(IDC_CHECK_CPU3DNOW);
		DISABLE_CONTROL(IDC_CHECK_CPUE3DNOW);
		DISABLE_CONTROL(IDC_CHECK_CPUCMOV);
		DISABLE_CONTROL(IDC_CHECK_CPUEMMX);
		DISABLE_CONTROL(IDC_CHECK_CPUSSE2);
		DISABLE_CONTROL(IDC_CHECK_VBR);
		DISABLE_CONTROL(IDC_COMBO_VBR);
		DISABLE_CONTROL(IDC_CHECK_VBR_BITRATE);
		DISABLE_CONTROL(IDC_COMBO_VBR_BITRATE_LOW);
		DISABLE_CONTROL(IDC_COMBO_VBR_BITRATE_HIGH);
		DISABLE_CONTROL(IDC_CHECK_USEPSY);
		DISABLE_CONTROL(IDC_CHECK_VERIFY);
		DISABLE_CONTROL(IDC_CHECK_16KHZ_LOW_PASS_FILTER);
	} else {
		ENABLE_CONTROL(IDC_CHECK_COMMANDLINE_OPTS);
		if(IS_CHECK(IDC_CHECK_COMMANDLINE_OPTS)){
			ENABLE_CONTROL(IDC_EDIT_COMMANDLINE_OPTION);
			DISABLE_CONTROL(IDC_CHECK_OUTPUT_FORMAT);
			DISABLE_CONTROL(IDC_COMBO_OUTPUT_FORMAT);
			DISABLE_CONTROL(IDC_CHECK_MPEG1AUDIOBITRATE);
			DISABLE_CONTROL(IDC_COMBO_MPEG1_AUDIO_BITRATE);
			DISABLE_CONTROL(IDC_CHECK_MPEG2AUDIOBITRATE);
			DISABLE_CONTROL(IDC_COMBO_MPEG2_AUDIO_BITRATE);
			DISABLE_CONTROL(IDC_CHECK_ENHANCED_LOW_PASS_FILTER);
			DISABLE_CONTROL(IDC_EDIT_LPF_PARA1);
			DISABLE_CONTROL(IDC_EDIT_LPF_PARA2);
			DISABLE_CONTROL(IDC_CHECK_ENCODE_MODE);
			DISABLE_CONTROL(IDC_COMBO_ENCODE_MODE);
			DISABLE_CONTROL(IDC_CHECK_EMPHASIS_TYPE);
			DISABLE_CONTROL(IDC_COMBO_EMPHASIS_TYPE);
			DISABLE_CONTROL(IDC_CHECK_OUTFREQ);
			DISABLE_CONTROL(IDC_EDIT_OUTFREQ);
			DISABLE_CONTROL(IDC_CHECK_MSTHRESHOLD);
			DISABLE_CONTROL(IDC_EDIT_MSTHRESHOLD_THRESHOLD);
			DISABLE_CONTROL(IDC_EDIT_MSTHRESHOLD_MSPOWER);
			DISABLE_CONTROL(IDC_CHECK_USE_CPU_OPTS);
			DISABLE_CONTROL(IDC_CHECK_CPUMMX);
			DISABLE_CONTROL(IDC_CHECK_CPUSSE);
			DISABLE_CONTROL(IDC_CHECK_CPU3DNOW);
			DISABLE_CONTROL(IDC_CHECK_CPUE3DNOW);
			DISABLE_CONTROL(IDC_CHECK_CPUCMOV);
			DISABLE_CONTROL(IDC_CHECK_CPUEMMX);
			DISABLE_CONTROL(IDC_CHECK_CPUSSE2);
			DISABLE_CONTROL(IDC_CHECK_VBR);
			DISABLE_CONTROL(IDC_COMBO_VBR);
			DISABLE_CONTROL(IDC_CHECK_VBR_BITRATE);
			DISABLE_CONTROL(IDC_COMBO_VBR_BITRATE_LOW);
			DISABLE_CONTROL(IDC_COMBO_VBR_BITRATE_HIGH);
			DISABLE_CONTROL(IDC_CHECK_USEPSY);
			DISABLE_CONTROL(IDC_CHECK_VERIFY);
			DISABLE_CONTROL(IDC_CHECK_16KHZ_LOW_PASS_FILTER);
		} else {
			DISABLE_CONTROL(IDC_EDIT_COMMANDLINE_OPTION);
			ENABLE_CONTROL(IDC_CHECK_OUTPUT_FORMAT);
			if(IS_CHECK(IDC_CHECK_OUTPUT_FORMAT)){
				ENABLE_CONTROL(IDC_COMBO_OUTPUT_FORMAT);
			} else {
				DISABLE_CONTROL(IDC_COMBO_OUTPUT_FORMAT);
			}
			ENABLE_CONTROL(IDC_CHECK_16KHZ_LOW_PASS_FILTER);
			ENABLE_CONTROL(IDC_CHECK_ENHANCED_LOW_PASS_FILTER);
			if(IS_CHECK(IDC_CHECK_16KHZ_LOW_PASS_FILTER)){
				UNCHECK(IDC_CHECK_ENHANCED_LOW_PASS_FILTER);
				DISABLE_CONTROL(IDC_EDIT_LPF_PARA1);
				DISABLE_CONTROL(IDC_EDIT_LPF_PARA2);
			} else {
				if(IS_CHECK(IDC_CHECK_ENHANCED_LOW_PASS_FILTER)){
					UNCHECK(IDC_CHECK_16KHZ_LOW_PASS_FILTER);
					ENABLE_CONTROL(IDC_EDIT_LPF_PARA1);
					ENABLE_CONTROL(IDC_EDIT_LPF_PARA2);
				} else {
					DISABLE_CONTROL(IDC_EDIT_LPF_PARA1);
					DISABLE_CONTROL(IDC_EDIT_LPF_PARA2);
				}
			}
			ENABLE_CONTROL(IDC_CHECK_ENCODE_MODE);
			if(IS_CHECK(IDC_CHECK_ENCODE_MODE)){
				ENABLE_CONTROL(IDC_COMBO_ENCODE_MODE);
			} else {
				DISABLE_CONTROL(IDC_COMBO_ENCODE_MODE);
			}
			ENABLE_CONTROL(IDC_CHECK_EMPHASIS_TYPE);
			if(IS_CHECK(IDC_CHECK_EMPHASIS_TYPE)){
				ENABLE_CONTROL(IDC_COMBO_EMPHASIS_TYPE);
			} else {
				DISABLE_CONTROL(IDC_COMBO_EMPHASIS_TYPE);
			}
			ENABLE_CONTROL(IDC_CHECK_OUTFREQ);
			if(IS_CHECK(IDC_CHECK_OUTFREQ)){
				ENABLE_CONTROL(IDC_EDIT_OUTFREQ);
			} else {
				DISABLE_CONTROL(IDC_EDIT_OUTFREQ);
			}
			ENABLE_CONTROL(IDC_CHECK_MSTHRESHOLD);
			if(IS_CHECK(IDC_CHECK_MSTHRESHOLD)){
				ENABLE_CONTROL(IDC_EDIT_MSTHRESHOLD_THRESHOLD);
				ENABLE_CONTROL(IDC_EDIT_MSTHRESHOLD_MSPOWER);
			} else {
				DISABLE_CONTROL(IDC_EDIT_MSTHRESHOLD_THRESHOLD);
				DISABLE_CONTROL(IDC_EDIT_MSTHRESHOLD_MSPOWER);
			}
			ENABLE_CONTROL(IDC_CHECK_USE_CPU_OPTS);
			if(IS_CHECK(IDC_CHECK_USE_CPU_OPTS)){
				ENABLE_CONTROL(IDC_CHECK_CPUMMX);
				ENABLE_CONTROL(IDC_CHECK_CPUSSE);
				ENABLE_CONTROL(IDC_CHECK_CPU3DNOW);
				ENABLE_CONTROL(IDC_CHECK_CPUE3DNOW);
				ENABLE_CONTROL(IDC_CHECK_CPUCMOV);
				ENABLE_CONTROL(IDC_CHECK_CPUEMMX);
				ENABLE_CONTROL(IDC_CHECK_CPUSSE2);
			} else {
				DISABLE_CONTROL(IDC_CHECK_CPUMMX);
				DISABLE_CONTROL(IDC_CHECK_CPUSSE);
				DISABLE_CONTROL(IDC_CHECK_CPU3DNOW);
				DISABLE_CONTROL(IDC_CHECK_CPUE3DNOW);
				DISABLE_CONTROL(IDC_CHECK_CPUCMOV);
				DISABLE_CONTROL(IDC_CHECK_CPUEMMX);
				DISABLE_CONTROL(IDC_CHECK_CPUSSE2);
			}
			ENABLE_CONTROL(IDC_CHECK_VBR);
			ENABLE_CONTROL(IDC_CHECK_MPEG1AUDIOBITRATE);
			ENABLE_CONTROL(IDC_CHECK_MPEG2AUDIOBITRATE);
			if(IS_CHECK(IDC_CHECK_VBR)){
				ENABLE_CONTROL(IDC_COMBO_VBR);
				ENABLE_CONTROL(IDC_CHECK_VBR_BITRATE);
				if(IS_CHECK(IDC_CHECK_VBR_BITRATE)){
					ENABLE_CONTROL(IDC_COMBO_VBR_BITRATE_LOW);
					ENABLE_CONTROL(IDC_COMBO_VBR_BITRATE_HIGH);
				} else {
					DISABLE_CONTROL(IDC_COMBO_VBR_BITRATE_LOW);
					DISABLE_CONTROL(IDC_COMBO_VBR_BITRATE_HIGH);
				}
				UNCHECK(IDC_CHECK_MPEG1AUDIOBITRATE);
				UNCHECK(IDC_CHECK_MPEG2AUDIOBITRATE);
				DISABLE_CONTROL(IDC_COMBO_MPEG1_AUDIO_BITRATE);
				DISABLE_CONTROL(IDC_COMBO_MPEG2_AUDIO_BITRATE);
			} else {
				UNCHECK(IDC_CHECK_VBR_BITRATE);
				DISABLE_CONTROL(IDC_COMBO_VBR);
				DISABLE_CONTROL(IDC_CHECK_VBR_BITRATE);
				DISABLE_CONTROL(IDC_COMBO_VBR_BITRATE_LOW);
				DISABLE_CONTROL(IDC_COMBO_VBR_BITRATE_HIGH);
				if(IS_CHECK(IDC_CHECK_MPEG1AUDIOBITRATE)){
					ENABLE_CONTROL(IDC_COMBO_MPEG1_AUDIO_BITRATE);
				} else {
					DISABLE_CONTROL(IDC_COMBO_MPEG1_AUDIO_BITRATE);
				}
				if(IS_CHECK(IDC_CHECK_MPEG2AUDIOBITRATE)){
					ENABLE_CONTROL(IDC_COMBO_MPEG2_AUDIO_BITRATE);
				} else {
					DISABLE_CONTROL(IDC_COMBO_MPEG2_AUDIO_BITRATE);
				}
			}
			ENABLE_CONTROL(IDC_CHECK_USEPSY);
			ENABLE_CONTROL(IDC_CHECK_VERIFY);
		}
	}
}

static void gogoConfigDialogProcControlReset(HWND hwnd)
{
	// コンボボックスの選択設定
	CB_SETCURSEL_TYPE2(IDC_COMBO_OUTPUT_FORMAT)
	CB_SETCURSEL_TYPE1(IDC_COMBO_MPEG1_AUDIO_BITRATE)
	CB_SETCURSEL_TYPE1(IDC_COMBO_MPEG2_AUDIO_BITRATE)
	CB_SETCURSEL_TYPE2(IDC_COMBO_ENCODE_MODE)
	CB_SETCURSEL_TYPE2(IDC_COMBO_EMPHASIS_TYPE)
	CB_SETCURSEL_TYPE1(IDC_COMBO_VBR_BITRATE_LOW)
	CB_SETCURSEL_TYPE1(IDC_COMBO_VBR_BITRATE_HIGH)
	CB_SETCURSEL_TYPE2(IDC_COMBO_VBR)
	// チェックボックスの設定
	CHECKBOX_SET(IDC_CHECK_DEFAULT)
	CHECKBOX_SET(IDC_CHECK_COMMANDLINE_OPTS)
	CHECKBOX_SET(IDC_CHECK_OUTPUT_FORMAT)
	CHECKBOX_SET(IDC_CHECK_MPEG1AUDIOBITRATE)
	CHECKBOX_SET(IDC_CHECK_MPEG2AUDIOBITRATE)
	CHECKBOX_SET(IDC_CHECK_ENHANCED_LOW_PASS_FILTER)
	CHECKBOX_SET(IDC_CHECK_ENCODE_MODE)
	CHECKBOX_SET(IDC_CHECK_EMPHASIS_TYPE)
	CHECKBOX_SET(IDC_CHECK_OUTFREQ)
	CHECKBOX_SET(IDC_CHECK_MSTHRESHOLD)
	CHECKBOX_SET(IDC_CHECK_USE_CPU_OPTS)
	CHECKBOX_SET(IDC_CHECK_CPUMMX)
	CHECKBOX_SET(IDC_CHECK_CPUSSE)
	CHECKBOX_SET(IDC_CHECK_CPU3DNOW)
	CHECKBOX_SET(IDC_CHECK_CPUE3DNOW)
	CHECKBOX_SET(IDC_CHECK_CPUCMOV)
	CHECKBOX_SET(IDC_CHECK_CPUEMMX)
	CHECKBOX_SET(IDC_CHECK_CPUSSE2)
	CHECKBOX_SET(IDC_CHECK_VBR)
	CHECKBOX_SET(IDC_CHECK_VBR_BITRATE)
	CHECKBOX_SET(IDC_CHECK_USEPSY)
	CHECKBOX_SET(IDC_CHECK_VERIFY)
	CHECKBOX_SET(IDC_CHECK_16KHZ_LOW_PASS_FILTER)
	// エディットの設定
	EDIT_SET(IDC_EDIT_OUTFREQ)
	EDIT_SET(IDC_EDIT_MSTHRESHOLD_THRESHOLD)
	EDIT_SET(IDC_EDIT_MSTHRESHOLD_MSPOWER)
	EDIT_SET(IDC_EDIT_COMMANDLINE_OPTION)
	EDIT_SET(IDC_EDIT_LPF_PARA1)
	EDIT_SET(IDC_EDIT_LPF_PARA2)
	// コントロールの有効 / 無効化
	gogoConfigDialogProcControlEnableDisable(hwnd);
}

static void gogoConfigDialogProcControlApply(HWND hwnd)
{
	// コンボボックスの選択設定
	CB_GETCURSEL_TYPE2(IDC_COMBO_OUTPUT_FORMAT)
	CB_GETCURSEL_TYPE1(IDC_COMBO_MPEG1_AUDIO_BITRATE)
	CB_GETCURSEL_TYPE1(IDC_COMBO_MPEG2_AUDIO_BITRATE)
	CB_GETCURSEL_TYPE2(IDC_COMBO_ENCODE_MODE)
	CB_GETCURSEL_TYPE2(IDC_COMBO_EMPHASIS_TYPE)
	CB_GETCURSEL_TYPE1(IDC_COMBO_VBR_BITRATE_LOW)
	CB_GETCURSEL_TYPE1(IDC_COMBO_VBR_BITRATE_HIGH)
	CB_GETCURSEL_TYPE2(IDC_COMBO_VBR)
	// チェックボックスの設定
	CHECKBOX_GET(IDC_CHECK_DEFAULT)
	CHECKBOX_GET(IDC_CHECK_COMMANDLINE_OPTS)
	CHECKBOX_GET(IDC_CHECK_OUTPUT_FORMAT)
	CHECKBOX_GET(IDC_CHECK_MPEG1AUDIOBITRATE)
	CHECKBOX_GET(IDC_CHECK_MPEG2AUDIOBITRATE)
	CHECKBOX_GET(IDC_CHECK_ENHANCED_LOW_PASS_FILTER)
	CHECKBOX_GET(IDC_CHECK_ENCODE_MODE)
	CHECKBOX_GET(IDC_CHECK_EMPHASIS_TYPE)
	CHECKBOX_GET(IDC_CHECK_OUTFREQ)
	CHECKBOX_GET(IDC_CHECK_MSTHRESHOLD)
	CHECKBOX_GET(IDC_CHECK_USE_CPU_OPTS)
	CHECKBOX_GET(IDC_CHECK_CPUMMX)
	CHECKBOX_GET(IDC_CHECK_CPUSSE)
	CHECKBOX_GET(IDC_CHECK_CPU3DNOW)
	CHECKBOX_GET(IDC_CHECK_CPUE3DNOW)
	CHECKBOX_GET(IDC_CHECK_CPUCMOV)
	CHECKBOX_GET(IDC_CHECK_CPUEMMX)
	CHECKBOX_GET(IDC_CHECK_CPUSSE2)
	CHECKBOX_GET(IDC_CHECK_VBR)
	CHECKBOX_GET(IDC_CHECK_VBR_BITRATE)
	CHECKBOX_GET(IDC_CHECK_USEPSY)
	CHECKBOX_GET(IDC_CHECK_VERIFY)
	CHECKBOX_GET(IDC_CHECK_16KHZ_LOW_PASS_FILTER)
	// エディットの設定
	EDIT_GET_RANGE(IDC_EDIT_OUTFREQ,6,MIN_OUTPUT_RATE,MAX_OUTPUT_RATE)
	EDIT_GET_RANGE(IDC_EDIT_MSTHRESHOLD_THRESHOLD,4,0,100)
	EDIT_GET_RANGE(IDC_EDIT_MSTHRESHOLD_MSPOWER,4,0,100)
	EDIT_GET(IDC_EDIT_COMMANDLINE_OPTION,1024)
	EDIT_GET_RANGE(IDC_EDIT_LPF_PARA1,4,0,100)
	EDIT_GET_RANGE(IDC_EDIT_LPF_PARA2,4,0,100)
	// コントロールの有効 / 無効化
	gogoConfigDialogProcControlEnableDisable(hwnd);
	// リセット
	gogoConfigDialogProcControlReset(hwnd);
}

#undef CB_INFO_TYPE1_BEGIN
#undef CB_INFO_TYPE1_END
#undef CB_INFO_TYPE2_BEGIN
#undef CB_INFO_TYPE2_END
#undef CB_SETCURSEL_TYPE1
#undef CB_SETCURSEL_TYPE2
#undef CB_GETCURSEL_TYPE1
#undef CB_GETCURSEL_TYPE2
#undef CHECKBOX_SET
#undef CHECKBOX_GET
#undef EDIT_SET
#undef EDIT_GET
#undef EDIT_GET_RANGE

#endif

int gogoConfigDialog(void)
{
#ifdef AU_GOGO
  if(!IsWindow(hgogoConfigDailog)) {
    if (PlayerLanguage == LANGUAGE_JAPANESE)
      hgogoConfigDailog = CreateDialog(hInst,MAKEINTRESOURCE(IDD_DIALOG_GOGO),(HWND)hPrefWnd,gogoConfigDialogProc);
    else
      hgogoConfigDailog = CreateDialog(hInst,MAKEINTRESOURCE(IDD_DIALOG_GOGO_EN),(HWND)hPrefWnd,gogoConfigDialogProc);
  }
  ShowWindow(hgogoConfigDailog,SW_SHOW);
#endif
  return 0;
}

#ifdef AU_GOGO

static int gogo_ConfigDialogInfoLock()
{
	return 0;
}
static int gogo_ConfigDialogInfoUnLock()
{
	return 0;
}

volatile gogo_ConfigDialogInfo_t gogo_ConfigDialogInfo;

int gogo_ConfigDialogInfoInit(void)
{
	gogo_ConfigDialogInfo.optIDC_CHECK_DEFAULT = 1;
	gogo_ConfigDialogInfo.optIDC_CHECK_COMMANDLINE_OPTS = 0;
	gogo_ConfigDialogInfo.optIDC_EDIT_COMMANDLINE_OPTION[0] = '\0';
	gogo_ConfigDialogInfo.optIDC_CHECK_OUTPUT_FORMAT = 1;
	gogo_ConfigDialogInfo.optIDC_COMBO_OUTPUT_FORMAT = MC_OUTPUT_NORMAL;
	gogo_ConfigDialogInfo.optIDC_CHECK_MPEG1AUDIOBITRATE = 1;
	gogo_ConfigDialogInfo.optIDC_COMBO_MPEG1_AUDIO_BITRATE = 160;
	gogo_ConfigDialogInfo.optIDC_CHECK_MPEG2AUDIOBITRATE = 1;
	gogo_ConfigDialogInfo.optIDC_COMBO_MPEG2_AUDIO_BITRATE = 80;
	gogo_ConfigDialogInfo.optIDC_CHECK_ENHANCED_LOW_PASS_FILTER = 0;
	strcpy((char *)gogo_ConfigDialogInfo.optIDC_EDIT_LPF_PARA1,"55");
	strcpy((char *)gogo_ConfigDialogInfo.optIDC_EDIT_LPF_PARA2,"70");
	gogo_ConfigDialogInfo.optIDC_CHECK_ENCODE_MODE = 1;
	gogo_ConfigDialogInfo.optIDC_COMBO_ENCODE_MODE = MC_MODE_STEREO;
	gogo_ConfigDialogInfo.optIDC_CHECK_EMPHASIS_TYPE = 1;
	gogo_ConfigDialogInfo.optIDC_COMBO_EMPHASIS_TYPE = MC_EMP_NONE;
	gogo_ConfigDialogInfo.optIDC_CHECK_OUTFREQ = 0;
	strcpy((char *)gogo_ConfigDialogInfo.optIDC_EDIT_OUTFREQ,"44100");
	gogo_ConfigDialogInfo.optIDC_CHECK_MSTHRESHOLD = 0;
	strcpy((char *)gogo_ConfigDialogInfo.optIDC_EDIT_MSTHRESHOLD_THRESHOLD,"75");
	strcpy((char *)gogo_ConfigDialogInfo.optIDC_EDIT_MSTHRESHOLD_MSPOWER,"66");
	gogo_ConfigDialogInfo.optIDC_CHECK_USE_CPU_OPTS = 0;
	gogo_ConfigDialogInfo.optIDC_CHECK_CPUMMX = 0;
	gogo_ConfigDialogInfo.optIDC_CHECK_CPUSSE = 0;
	gogo_ConfigDialogInfo.optIDC_CHECK_CPU3DNOW = 0;
	gogo_ConfigDialogInfo.optIDC_CHECK_CPUE3DNOW = 0;
	gogo_ConfigDialogInfo.optIDC_CHECK_CPUCMOV = 0;
	gogo_ConfigDialogInfo.optIDC_CHECK_CPUEMMX = 0;
	gogo_ConfigDialogInfo.optIDC_CHECK_CPUSSE2 = 0;
	gogo_ConfigDialogInfo.optIDC_CHECK_VBR = 0;
	gogo_ConfigDialogInfo.optIDC_COMBO_VBR = 0;
	gogo_ConfigDialogInfo.optIDC_CHECK_VBR_BITRATE = 0;
	gogo_ConfigDialogInfo.optIDC_COMBO_VBR_BITRATE_LOW = 32;
	gogo_ConfigDialogInfo.optIDC_COMBO_VBR_BITRATE_HIGH = 320;
	gogo_ConfigDialogInfo.optIDC_CHECK_USEPSY = 1;
	gogo_ConfigDialogInfo.optIDC_CHECK_VERIFY = 0;
	gogo_ConfigDialogInfo.optIDC_CHECK_16KHZ_LOW_PASS_FILTER = 1;
	return 0;
}

int gogo_ConfigDialogInfoApply(void)
{
	gogo_ConfigDialogInfoLock();
	if(gogo_ConfigDialogInfo.optIDC_CHECK_DEFAULT>0){
		gogo_opts_reset();
		gogo_ConfigDialogInfoUnLock();
		return 0;
	}
	if(gogo_ConfigDialogInfo.optIDC_CHECK_COMMANDLINE_OPTS>0){
		gogo_opts_reset();
		set_gogo_opts_use_commandline_options((char *)gogo_ConfigDialogInfo.optIDC_EDIT_COMMANDLINE_OPTION);
		gogo_ConfigDialogInfoUnLock();
		return 0;
	}
	if(gogo_ConfigDialogInfo.optIDC_CHECK_OUTPUT_FORMAT>0){
		gogo_opts.optOUTPUT_FORMAT = gogo_ConfigDialogInfo.optIDC_COMBO_OUTPUT_FORMAT;
	}
	if(gogo_ConfigDialogInfo.optIDC_CHECK_MPEG1AUDIOBITRATE>0){
		gogo_opts.optBITRATE1 = gogo_ConfigDialogInfo.optIDC_COMBO_MPEG1_AUDIO_BITRATE;
	}
	if(gogo_ConfigDialogInfo.optIDC_CHECK_MPEG2AUDIOBITRATE>0){
		gogo_opts.optBITRATE2 = gogo_ConfigDialogInfo.optIDC_COMBO_MPEG2_AUDIO_BITRATE;
	}
	if(gogo_ConfigDialogInfo.optIDC_CHECK_ENHANCED_LOW_PASS_FILTER>0){
		gogo_opts.optENHANCEDFILTER_A = atoi((char *)gogo_ConfigDialogInfo.optIDC_EDIT_LPF_PARA1);
		gogo_opts.optENHANCEDFILTER_B = atoi((char *)gogo_ConfigDialogInfo.optIDC_EDIT_LPF_PARA2);
	}
	if(gogo_ConfigDialogInfo.optIDC_CHECK_ENCODE_MODE>0){
		gogo_opts.optENCODEMODE = gogo_ConfigDialogInfo.optIDC_COMBO_ENCODE_MODE;
	}
	if(gogo_ConfigDialogInfo.optIDC_CHECK_EMPHASIS_TYPE>0){
		gogo_opts.optEMPHASIS = gogo_ConfigDialogInfo.optIDC_COMBO_EMPHASIS_TYPE;
	}
	if(gogo_ConfigDialogInfo.optIDC_CHECK_OUTFREQ>0){
		gogo_opts.optOUTFREQ = atoi((char *)gogo_ConfigDialogInfo.optIDC_EDIT_OUTFREQ);
	}
	if(gogo_ConfigDialogInfo.optIDC_CHECK_MSTHRESHOLD>0){
		gogo_opts.optMSTHRESHOLD_threshold = atoi((char *)gogo_ConfigDialogInfo.optIDC_EDIT_MSTHRESHOLD_THRESHOLD);
		gogo_opts.optMSTHRESHOLD_mspower = atoi((char *)gogo_ConfigDialogInfo.optIDC_EDIT_MSTHRESHOLD_MSPOWER);
	}
	if(gogo_ConfigDialogInfo.optIDC_CHECK_USE_CPU_OPTS>0){
		gogo_opts.optUSECPUOPT = 1;
		gogo_opts.optUSEMMX = gogo_ConfigDialogInfo.optIDC_CHECK_CPUMMX;
		gogo_opts.optUSEKNI = gogo_ConfigDialogInfo.optIDC_CHECK_CPUSSE;
		gogo_opts.optUSE3DNOW = gogo_ConfigDialogInfo.optIDC_CHECK_CPU3DNOW;
		gogo_opts.optUSEE3DNOW = gogo_ConfigDialogInfo.optIDC_CHECK_CPUE3DNOW;
		gogo_opts.optUSESSE = gogo_ConfigDialogInfo.optIDC_CHECK_CPUSSE;
		gogo_opts.optUSECMOV = gogo_ConfigDialogInfo.optIDC_CHECK_CPUCMOV;
		gogo_opts.optUSEEMMX = gogo_ConfigDialogInfo.optIDC_CHECK_CPUEMMX;
		gogo_opts.optUSESSE2 = gogo_ConfigDialogInfo.optIDC_CHECK_CPUSSE2;
	} else {
		gogo_opts.optUSECPUOPT = 0;
	}
	if(gogo_ConfigDialogInfo.optIDC_CHECK_VBR>0){
		gogo_opts.optVBR = gogo_ConfigDialogInfo.optIDC_COMBO_VBR;
	}
	if(gogo_ConfigDialogInfo.optIDC_CHECK_VBR_BITRATE>0){
		gogo_opts.optVBRBITRATE_low = gogo_ConfigDialogInfo.optIDC_COMBO_VBR_BITRATE_LOW;
		gogo_opts.optVBRBITRATE_high = gogo_ConfigDialogInfo.optIDC_COMBO_VBR_BITRATE_HIGH;
	}
	if(gogo_ConfigDialogInfo.optIDC_CHECK_USEPSY>0){
		gogo_opts.optUSEPSY = TRUE;
	}
	if(gogo_ConfigDialogInfo.optIDC_CHECK_VERIFY>0){
		gogo_opts.optVERIFY = TRUE;
	}
	if(gogo_ConfigDialogInfo.optIDC_CHECK_16KHZ_LOW_PASS_FILTER>0){
		gogo_opts.optUSELPF16 = TRUE;
	}
//	gogo_opts.optINPFREQ;			// SYSTEM USE(システムで使用するから指定できない)
//	gogo_opts.optSTARTOFFSET;	// SYSTEM USE
//	gogo_opts.optADDTAGnum;		// SYSTEM USE
//	gogo_opts.optADDTAG_len[64];	// SYSTEM USE
//	gogo_opts.optADDTAG_buf[64];	// SYSTEM USE
//	gogo_opts.optCPU;					// PREPAIRING(準備中)
//	gogo_opts.optBYTE_SWAP;			// SYSTEM USE
//	gogo_opts.opt8BIT_PCM;			// SYSTEM USE
//	gogo_opts.optMONO_PCM;		// SYSTEM USE
//	gogo_opts.optTOWNS_SND;			// SYSTEM USE
//	gogo_opts.optTHREAD_PRIORITY;	// PREPARING
//	gogo_opts.optREADTHREAD_PRIORITY;	// PREPARING
//	gogo_opts.optOUTPUTDIR[1024];			// SYSTEM USE
//	gogo_opts.output_name[1024];				// SYSTEM USE
	gogo_ConfigDialogInfoUnLock();
	return 0;
}

#define SEC_GOGO	"gogo"
int gogo_ConfigDialogInfoSaveINI(void)
{
	char *section = SEC_GOGO;
	char *inifile = timidity_output_inifile;
	char buffer[1024];
#if defined(__MINGW32__) || defined(__CYGWIN__)
#define NUMSAVE(name) \
		sprintf(buffer,"%d",gogo_ConfigDialogInfo.name ); \
		WritePrivateProfileString(section, #name ,buffer,inifile);
#define STRSAVE(name) \
		WritePrivateProfileString(section,(char *) #name ,(char *)gogo_ConfigDialogInfo.name ,inifile);
#else
#define NUMSAVE(name) \
		sprintf(buffer,"%d",gogo_ConfigDialogInfo. ## name ); \
		WritePrivateProfileString(section, #name ,buffer,inifile);
#define STRSAVE(name) \
		WritePrivateProfileString(section,(char *) #name ,(char *)gogo_ConfigDialogInfo. ## name ,inifile);
#endif
	NUMSAVE(optIDC_CHECK_DEFAULT)
	NUMSAVE(optIDC_CHECK_COMMANDLINE_OPTS)
	STRSAVE(optIDC_EDIT_COMMANDLINE_OPTION)
	NUMSAVE(optIDC_CHECK_OUTPUT_FORMAT)
	NUMSAVE(optIDC_COMBO_OUTPUT_FORMAT)
	NUMSAVE(optIDC_CHECK_MPEG1AUDIOBITRATE)
	NUMSAVE(optIDC_COMBO_MPEG1_AUDIO_BITRATE)
	NUMSAVE(optIDC_CHECK_MPEG2AUDIOBITRATE)
	NUMSAVE(optIDC_COMBO_MPEG2_AUDIO_BITRATE)
	NUMSAVE(optIDC_CHECK_ENHANCED_LOW_PASS_FILTER)
	STRSAVE(optIDC_EDIT_LPF_PARA1)
	STRSAVE(optIDC_EDIT_LPF_PARA2)
	NUMSAVE(optIDC_CHECK_ENCODE_MODE)
	NUMSAVE(optIDC_COMBO_ENCODE_MODE)
	NUMSAVE(optIDC_CHECK_EMPHASIS_TYPE)
	NUMSAVE(optIDC_COMBO_EMPHASIS_TYPE)
	NUMSAVE(optIDC_CHECK_OUTFREQ)
	STRSAVE(optIDC_EDIT_OUTFREQ)
	NUMSAVE(optIDC_CHECK_MSTHRESHOLD)
	STRSAVE(optIDC_EDIT_MSTHRESHOLD_THRESHOLD)
	STRSAVE(optIDC_EDIT_MSTHRESHOLD_MSPOWER)
	NUMSAVE(optIDC_CHECK_USE_CPU_OPTS)
	NUMSAVE(optIDC_CHECK_CPUMMX)
	NUMSAVE(optIDC_CHECK_CPUSSE)
	NUMSAVE(optIDC_CHECK_CPU3DNOW)
	NUMSAVE(optIDC_CHECK_CPUE3DNOW)
	NUMSAVE(optIDC_CHECK_CPUCMOV)
	NUMSAVE(optIDC_CHECK_CPUEMMX)
	NUMSAVE(optIDC_CHECK_CPUSSE2)
	NUMSAVE(optIDC_CHECK_VBR)
	NUMSAVE(optIDC_COMBO_VBR)
	NUMSAVE(optIDC_CHECK_VBR_BITRATE)
	NUMSAVE(optIDC_COMBO_VBR_BITRATE_LOW)
	NUMSAVE(optIDC_COMBO_VBR_BITRATE_HIGH)
	NUMSAVE(optIDC_CHECK_USEPSY)
	NUMSAVE(optIDC_CHECK_VERIFY)
	NUMSAVE(optIDC_CHECK_16KHZ_LOW_PASS_FILTER)
	WritePrivateProfileString(NULL,NULL,NULL,inifile);		// Write Flush
#undef NUMSAVE
#undef STRSAVE
	return 0;
}
int gogo_ConfigDialogInfoLoadINI(void)
{
	char *section = SEC_GOGO;
	char *inifile = timidity_output_inifile;
	int num;
	char buffer[1024];
#if defined(__MINGW32__) || defined(__CYGWIN__)
#define NUMLOAD(name) \
		num = GetPrivateProfileInt(section, #name ,-1,inifile); \
		if(num!=-1) gogo_ConfigDialogInfo.name = num;
#define STRLOAD(name,len) \
		GetPrivateProfileString(section,(char *) #name ,"",buffer,len,inifile); \
		buffer[len-1] = '\0'; \
		if(buffer[0]!=0) \
			strcpy((char *)gogo_ConfigDialogInfo.name ,buffer);
#else
#define NUMLOAD(name) \
		num = GetPrivateProfileInt(section, #name ,-1,inifile); \
		if(num!=-1) gogo_ConfigDialogInfo. ## name = num;
#define STRLOAD(name,len) \
		GetPrivateProfileString(section,(char *) #name ,"",buffer,len,inifile); \
		buffer[len-1] = '\0'; \
		if(buffer[0]!=0) \
			strcpy((char *)gogo_ConfigDialogInfo. ## name ,buffer);
#endif
	gogo_ConfigDialogInfoLock();
	NUMLOAD(optIDC_CHECK_DEFAULT)
	NUMLOAD(optIDC_CHECK_COMMANDLINE_OPTS)
	STRLOAD(optIDC_EDIT_COMMANDLINE_OPTION,1024)
	NUMLOAD(optIDC_CHECK_OUTPUT_FORMAT)
	NUMLOAD(optIDC_COMBO_OUTPUT_FORMAT)
	NUMLOAD(optIDC_CHECK_MPEG1AUDIOBITRATE)
	NUMLOAD(optIDC_COMBO_MPEG1_AUDIO_BITRATE)
	NUMLOAD(optIDC_CHECK_MPEG2AUDIOBITRATE)
	NUMLOAD(optIDC_COMBO_MPEG2_AUDIO_BITRATE)
	NUMLOAD(optIDC_CHECK_ENHANCED_LOW_PASS_FILTER)
	STRLOAD(optIDC_EDIT_LPF_PARA1,4)
	STRLOAD(optIDC_EDIT_LPF_PARA2,4)
	NUMLOAD(optIDC_CHECK_ENCODE_MODE)
	NUMLOAD(optIDC_COMBO_ENCODE_MODE)
	NUMLOAD(optIDC_CHECK_EMPHASIS_TYPE)
	NUMLOAD(optIDC_COMBO_EMPHASIS_TYPE)
	NUMLOAD(optIDC_CHECK_OUTFREQ)
	STRLOAD(optIDC_EDIT_OUTFREQ,6)
	NUMLOAD(optIDC_CHECK_MSTHRESHOLD)
	STRLOAD(optIDC_EDIT_MSTHRESHOLD_THRESHOLD,4)
	STRLOAD(optIDC_EDIT_MSTHRESHOLD_MSPOWER,4)
	NUMLOAD(optIDC_CHECK_USE_CPU_OPTS)
	NUMLOAD(optIDC_CHECK_CPUMMX)
	NUMLOAD(optIDC_CHECK_CPUSSE)
	NUMLOAD(optIDC_CHECK_CPU3DNOW)
	NUMLOAD(optIDC_CHECK_CPUE3DNOW)
	NUMLOAD(optIDC_CHECK_CPUCMOV)
	NUMLOAD(optIDC_CHECK_CPUEMMX)
	NUMLOAD(optIDC_CHECK_CPUSSE2)
	NUMLOAD(optIDC_CHECK_VBR)
	NUMLOAD(optIDC_COMBO_VBR)
	NUMLOAD(optIDC_CHECK_VBR_BITRATE)
	NUMLOAD(optIDC_COMBO_VBR_BITRATE_LOW)
	NUMLOAD(optIDC_COMBO_VBR_BITRATE_HIGH)
	NUMLOAD(optIDC_CHECK_USEPSY)
	NUMLOAD(optIDC_CHECK_VERIFY)
	NUMLOAD(optIDC_CHECK_16KHZ_LOW_PASS_FILTER)
#undef NUMLOAD
#undef STRLOAD
	gogo_ConfigDialogInfoUnLock();
	return 0;
}

#endif	// AU_GOGO


#ifdef AU_VORBIS
///////////////////////////////////////////////////////////////////////
//
// vorbis ConfigDialog
//
///////////////////////////////////////////////////////////////////////

volatile vorbis_ConfigDialogInfo_t vorbis_ConfigDialogInfo;

// id のコンボボックスの情報の定義
#define CB_INFO_TYPE1_BEGIN(id) static int cb_info_ ## id [] = {
#define CB_INFO_TYPE1_END };
#define CB_INFO_TYPE2_BEGIN(id) static char * cb_info_ ## id [] = {
#define CB_INFO_TYPE2_END };

// cb_info_type1_ＩＤ  cb_info_type2_ＩＤ というふうになる。

// IDC_COMBO_MODE_jp
CB_INFO_TYPE2_BEGIN(IDC_COMBO_MODE_jp)
	"VBR 品質 1 (低)",(char *)1,
	"VBR 品質 2",(char *)2,
	"VBR 品質 3",(char *)3,
	"VBR 品質 4",(char *)4,
	"VBR 品質 4.99",(char *)499,
	"VBR 品質 5",(char *)5,
	"VBR 品質 6",(char *)6,
	"VBR 品質 7",(char *)7,
	"VBR 品質 8 (デフォルト)",(char *)8,
	"VBR 品質 9",(char *)9,
	"VBR 品質 10 (高)",(char *)10,
#if 0
	"デフォルト(約128kbps VBR)",(char *)0,
	"約112kbps VBR",(char *)1,
	"約128kbps VBR",(char *)2,
	"約160kbps VBR",(char *)3,
	"約192kbps VBR",(char *)4,
	"約256kbps VBR",(char *)5,
	"約350kbps VBR",(char *)6,
#endif
	NULL
CB_INFO_TYPE2_END

// IDC_COMBO_MODE_en
CB_INFO_TYPE2_BEGIN(IDC_COMBO_MODE_en)
	"VBR Quality 1 (low)",(char *)1,
	"VBR Quality 2",(char *)2,
	"VBR Quality 3",(char *)3,
	"VBR Quality 4",(char *)4,
	"VBR Quality 4.99",(char *)499,
	"VBR Quality 5",(char *)5,
	"VBR Quality 6",(char *)6,
	"VBR Quality 7",(char *)7,
	"VBR Quality 8 (default)",(char *)8,
	"VBR Quality 9",(char *)9,
	"VBR Quality 10 (high)",(char *)10,
#if 0
	"Default (About 128kbps VBR)",(char *)0,
	"About 112kbps VBR",(char *)1,
	"About 128kbps VBR",(char *)2,
	"About 160kbps VBR",(char *)3,
	"About 192kbps VBR",(char *)4,
	"About 256kbps VBR",(char *)5,
	"About 350kbps VBR",(char *)6,
#endif
	NULL
CB_INFO_TYPE2_END

static char **cb_info_IDC_COMBO_MODE;

// id のコンボボックスを選択の設定する。
#define CB_SETCURSEL_TYPE1(id) \
{ \
	int cb_num; \
	for(cb_num=0;(int)cb_info_ ## id [cb_num]>=0;cb_num++){ \
		SendDlgItemMessage(hwnd,id,CB_SETCURSEL,(WPARAM)0,(LPARAM)0); \
		if(vorbis_ConfigDialogInfo.opt ## id == (int) cb_info_ ## id [cb_num]){ \
			SendDlgItemMessage(hwnd,id,CB_SETCURSEL,(WPARAM)cb_num,(LPARAM)0); \
			break; \
		} \
	} \
}
#define CB_SETCURSEL_TYPE2(id) \
{ \
	int cb_num; \
	for(cb_num=0;(int)cb_info_ ## id [cb_num];cb_num+=2){ \
		SendDlgItemMessage(hwnd,id,CB_SETCURSEL,(WPARAM)0,(LPARAM)0); \
	    if(vorbis_ConfigDialogInfo.opt ## id == (int) cb_info_ ## id [cb_num+1]){ \
			SendDlgItemMessage(hwnd,id,CB_SETCURSEL,(WPARAM)cb_num/2,(LPARAM)0); \
			break; \
		} \
	} \
}
// id のコンボボックスの選択を変数に代入する。
#define CB_GETCURSEL_TYPE1(id) \
{ \
	int cb_num1, cb_num2; \
	cb_num1 = SendDlgItemMessage(hwnd,id,CB_GETCURSEL,(WPARAM)0,(LPARAM)0); \
	for(cb_num2=0;(int)cb_info_ ## id [cb_num2]>=0;cb_num2++) \
		if(cb_num1==cb_num2){ \
			vorbis_ConfigDialogInfo.opt ## id = (int)cb_info_ ## id [cb_num2]; \
			break; \
		} \
}
#define CB_GETCURSEL_TYPE2(id) \
{ \
	int cb_num1, cb_num2; \
	cb_num1 = SendDlgItemMessage(hwnd,id,CB_GETCURSEL,(WPARAM)0,(LPARAM)0); \
	for(cb_num2=0;(int)cb_info_ ## id [cb_num2];cb_num2+=2) \
		if(cb_num1*2==cb_num2){ \
			vorbis_ConfigDialogInfo.opt ## id = (int)cb_info_ ## id [cb_num2+1]; \
			break; \
		} \
}
// チェックされているか。
#define IS_CHECK(id) SendDlgItemMessage(hwnd,id,BM_GETCHECK,0,0)
// チェックする。
#define CHECK(id) SendDlgItemMessage(hwnd,id,BM_SETCHECK,1,0)
// チェックをはずす。
#define UNCHECK(id) SendDlgItemMessage(hwnd,id,BM_SETCHECK,0,0)
// id のチェックボックスを設定する。
#define CHECKBOX_SET(id) \
	if(vorbis_ConfigDialogInfo.opt ## id>0) \
		SendDlgItemMessage(hwnd,id,BM_SETCHECK,1,0); \
	else \
		SendDlgItemMessage(hwnd,id,BM_SETCHECK,0,0); \
// id のチェックボックスを変数に代入する。
#define CHECKBOX_GET(id) \
	if(SendDlgItemMessage(hwnd,id,BM_GETCHECK,0,0)) \
		vorbis_ConfigDialogInfo.opt ## id = 1; \
	else \
		vorbis_ConfigDialogInfo.opt ## id = 0; \
// id のエディットを設定する。
#define EDIT_SET(id) SendDlgItemMessage(hwnd,id,WM_SETTEXT,0,(LPARAM)vorbis_ConfigDialogInfo.opt ## id);
// id のエディットを変数に代入する。
#define EDIT_GET(id,size) SendDlgItemMessage(hwnd,id,WM_GETTEXT,(WPARAM)size,(LPARAM)vorbis_ConfigDialogInfo.opt ## id);
#define EDIT_GET_RANGE(id,size,min,max) \
{ \
	char tmpbuf[1024]; \
	int value; \
	SendDlgItemMessage(hwnd,id,WM_GETTEXT,(WPARAM)size,(LPARAM)vorbis_ConfigDialogInfo.opt ## id); \
	value = atoi((char *)vorbis_ConfigDialogInfo.opt ## id); \
	if(value<min) value = min; \
	if(value>max) value = max; \
	sprintf(tmpbuf,"%d",value); \
	strncpy((char *)vorbis_ConfigDialogInfo.opt ## id,tmpbuf,size); \
	(vorbis_ConfigDialogInfo.opt ## id)[size] = '\0'; \
}
// コントロールの有効化
#define ENABLE_CONTROL(id) EnableWindow(GetDlgItem(hwnd,id),TRUE);
// コントロールの無効化
#define DISABLE_CONTROL(id) EnableWindow(GetDlgItem(hwnd,id),FALSE);


static void vorbisConfigDialogProcControlEnableDisable(HWND hwnd);
static void vorbisConfigDialogProcControlApply(HWND hwnd);
static void vorbisConfigDialogProcControlReset(HWND hwnd);
static int vorbis_ConfigDialogInfoLock();
static int vorbis_ConfigDialogInfoUnLock();
static HANDLE hvorbisConfigDailog = NULL;
static BOOL APIENTRY vorbisConfigDialogProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam)
{
	switch (uMess){
	case WM_INITDIALOG:
	{
		int i;
		// コンボボックスの初期化
		if (PlayerLanguage == LANGUAGE_JAPANESE)
		  cb_info_IDC_COMBO_MODE = cb_info_IDC_COMBO_MODE_jp;
		else
		  cb_info_IDC_COMBO_MODE = cb_info_IDC_COMBO_MODE_en;

		for(i=0;cb_info_IDC_COMBO_MODE[i];i+=2){
			SendDlgItemMessage(hwnd,IDC_COMBO_MODE,CB_INSERTSTRING,(WPARAM)-1,(LPARAM)cb_info_IDC_COMBO_MODE[i]);
		}
		// 設定
		vorbisConfigDialogProcControlReset(hwnd);
	}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCLOSE:
			SendMessage(hwnd,WM_CLOSE,(WPARAM)0,(LPARAM)0);
			break;
		case IDOK:
			vorbisConfigDialogProcControlApply(hwnd);
			SendMessage(hwnd,WM_CLOSE,(WPARAM)0,(LPARAM)0);
			break;
		case IDCANCEL:
			SendMessage(hwnd,WM_CLOSE,(WPARAM)0,(LPARAM)0);
			break;
		case IDC_BUTTON_APPLY:
			vorbisConfigDialogProcControlApply(hwnd);
			break;
		case IDC_CHECK_DEFAULT:
			vorbisConfigDialogProcControlEnableDisable(hwnd);
			break;
		case IDC_COMBO_MODE:
			break;
		default:
			break;
		}
		break;
	case WM_NOTIFY:
		break;
	case WM_SIZE:
		return FALSE;
	case WM_CLOSE:
		vorbis_ConfigDialogInfoSaveINI();
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		hvorbisConfigDailog = NULL;
		break;
	default:
		break;
	}
	return FALSE;
}

// コントロールの有効 / 無効化
static void vorbisConfigDialogProcControlEnableDisable(HWND hwnd)
{
	ENABLE_CONTROL(IDC_CHECK_DEFAULT);
	if(IS_CHECK(IDC_CHECK_DEFAULT)){
		DISABLE_CONTROL(IDC_COMBO_MODE);
	} else {
		ENABLE_CONTROL(IDC_COMBO_MODE);
	}
}

static void vorbisConfigDialogProcControlReset(HWND hwnd)
{
	// コンボボックスの選択設定
	CB_SETCURSEL_TYPE2(IDC_COMBO_MODE)
	// チェックボックスの設定
	CHECKBOX_SET(IDC_CHECK_DEFAULT)
	// エディットの設定
	// コントロールの有効 / 無効化
	vorbisConfigDialogProcControlEnableDisable(hwnd);
}

static void vorbisConfigDialogProcControlApply(HWND hwnd)
{
	// コンボボックスの選択設定
	CB_GETCURSEL_TYPE2(IDC_COMBO_MODE)
	// チェックボックスの設定
	CHECKBOX_GET(IDC_CHECK_DEFAULT)
	// エディットの設定
	// コントロールの有効 / 無効化
	vorbisConfigDialogProcControlEnableDisable(hwnd);
	// リセット
	vorbisConfigDialogProcControlReset(hwnd);
}

#undef CB_INFO_TYPE1_BEGIN
#undef CB_INFO_TYPE1_END
#undef CB_INFO_TYPE2_BEGIN
#undef CB_INFO_TYPE2_END
#undef CB_SETCURSEL_TYPE1
#undef CB_SETCURSEL_TYPE2
#undef CB_GETCURSEL_TYPE1
#undef CB_GETCURSEL_TYPE2
#undef CHECKBOX_SET
#undef CHECKBOX_GET
#undef EDIT_SET
#undef EDIT_GET
#undef EDIT_GET_RANGE

#endif

int vorbisConfigDialog(void)
{
#ifdef AU_VORBIS
  if(!IsWindow(hvorbisConfigDailog)) {
    if (PlayerLanguage == LANGUAGE_JAPANESE)
      hvorbisConfigDailog = CreateDialog(hInst,MAKEINTRESOURCE(IDD_DIALOG_VORBIS),(HWND)hPrefWnd,vorbisConfigDialogProc);
    else
      hvorbisConfigDailog = CreateDialog(hInst,MAKEINTRESOURCE(IDD_DIALOG_VORBIS_EN),(HWND)hPrefWnd,vorbisConfigDialogProc);
  }
  ShowWindow(hvorbisConfigDailog,SW_SHOW);
#endif
	return 0;
}

#ifdef AU_VORBIS

static int vorbis_ConfigDialogInfoLock()
{
	return 0;
}
static int vorbis_ConfigDialogInfoUnLock()
{
	return 0;
}

int vorbis_ConfigDialogInfoInit(void)
{
	vorbis_ConfigDialogInfo.optIDC_CHECK_DEFAULT = 1;
	vorbis_ConfigDialogInfo.optIDC_COMBO_MODE = 0;
	return 0;
}

extern volatile int ogg_vorbis_mode;
int vorbis_ConfigDialogInfoApply(void)
{
	vorbis_ConfigDialogInfoLock();
	if(vorbis_ConfigDialogInfo.optIDC_CHECK_DEFAULT>0){
//		vorbis_opts_reset();
		vorbis_ConfigDialogInfoUnLock();
		return 0;
	}
	ogg_vorbis_mode = vorbis_ConfigDialogInfo.optIDC_COMBO_MODE;
	vorbis_ConfigDialogInfoUnLock();
	return 0;
}

#define SEC_VORBIS	"vorbis"
int vorbis_ConfigDialogInfoSaveINI(void)
{
	char *section = SEC_VORBIS;
	char *inifile = timidity_output_inifile;
	char buffer[1024];
//	int len;
#if defined(__MINGW32__) || defined(__CYGWIN__)
#define NUMSAVE(name) \
		sprintf(buffer,"%d",vorbis_ConfigDialogInfo.name ); \
		WritePrivateProfileString(section, #name ,buffer,inifile);
//#define STRSAVE(name,len) \
//		WritePrivateProfileString(section,(char *) #name ,(char *)vorbis_ConfigDialogInfo.name ,inifile);
#else
#define NUMSAVE(name) \
		sprintf(buffer,"%d",vorbis_ConfigDialogInfo. ## name ); \
		WritePrivateProfileString(section, #name ,buffer,inifile);
//#define STRSAVE(name,len) \
//		WritePrivateProfileString(section,(char *) #name ,(char *)vorbis_ConfigDialogInfo. ## name ,inifile);
#endif
	NUMSAVE(optIDC_CHECK_DEFAULT)
	NUMSAVE(optIDC_COMBO_MODE)
	WritePrivateProfileString(NULL,NULL,NULL,inifile);		// Write Flush
#undef NUMSAVE
//#undef STRSAVE
	return 0;
}
int vorbis_ConfigDialogInfoLoadINI(void)
{
	char *section = SEC_VORBIS;
	char *inifile = timidity_output_inifile;
	int num;
//	char buffer[1024];
#if defined(__MINGW32__) || defined(__CYGWIN__)
#define NUMLOAD(name) \
		num = GetPrivateProfileInt(section, #name ,-1,inifile); \
		if(num!=-1) vorbis_ConfigDialogInfo.name = num;
//#define STRLOAD(name,len) \
//		GetPrivateProfileString(section,(char *) #name ,"",buffer,len,inifile); \
//		buffer[len-1] = '\0'; \
//		if(buffer[0]!=0) \
//			strcpy((char *)vorbis_ConfigDialogInfo.name ,buffer);
#else
#define NUMLOAD(name) \
		num = GetPrivateProfileInt(section, #name ,-1,inifile); \
		if(num!=-1) vorbis_ConfigDialogInfo. ## name = num;
//#define STRLOAD(name,len) \
//		GetPrivateProfileString(section,(char *) #name ,"",buffer,len,inifile); \
//		buffer[len-1] = '\0'; \
//		if(buffer[0]!=0) \
//			strcpy((char *)vorbis_ConfigDialogInfo. ## name ,buffer);
#endif
	vorbis_ConfigDialogInfoLock();
	NUMLOAD(optIDC_CHECK_DEFAULT)
	NUMLOAD(optIDC_COMBO_MODE)
#undef NUMLOAD
//#undef STRLOAD
	vorbis_ConfigDialogInfoUnLock();
	return 0;
}

#endif	// AU_VORBIS
