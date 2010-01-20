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

    w32g_subwin.c: Written by Daisuke Aoki <dai@y7.net>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <stddef.h>
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
#include <shlobj.h>
#include <commctrl.h>
#include <windowsx.h>
#include "w32g_res.h"
#include "w32g_utl.h"
#include "w32g_pref.h"
#include "w32g_subwin.h"
#include "w32g_ut2.h"

#if defined(__CYGWIN32__) || defined(__MINGW32__)
#ifndef TPM_TOPALIGN
#define TPM_TOPALIGN	0x0000L
#endif
#endif

extern void MainWndToggleConsoleButton(void);
extern void MainWndUpdateConsoleButton(void);
extern void MainWndToggleTracerButton(void);
extern void MainWndUpdateTracerButton(void);
extern void MainWndToggleListButton(void);
extern void MainWndUpdateListButton(void);
extern void MainWndToggleDocButton(void);
extern void MainWndUpdateDocButton(void);
extern void MainWndToggleWrdButton(void);
extern void MainWndUpdateWrdButton(void);
extern void MainWndToggleSoundSpecButton(void);
extern void MainWndUpdateSoundSpecButton(void);
extern void ShowSubWindow(HWND hwnd,int showflag);
extern void ToggleSubWindow(HWND hwnd);

extern void VprintfEditCtlWnd(HWND hwnd, char *fmt, va_list argList);
extern void PrintfEditCtlWnd(HWND hwnd, char *fmt, ...);
extern void PutsEditCtlWnd(HWND hwnd, char *str);
extern void ClearEditCtlWnd(HWND hwnd);

extern char *nkf_convert(char *si,char *so,int maxsize,char *in_mode,char *out_mode);

void InitListSearchWnd(HWND hParentWnd);
void ShowListSearch(void);
void HideListSearch(void);

// ***************************************************************************
//
// Console Window
//
// ***************************************************************************

// ---------------------------------------------------------------------------
// variables
static int ConsoleWndMaxSize = 64 * 1024;
static HFONT hFontConsoleWnd = NULL;
CONSOLEWNDINFO ConsoleWndInfo;

// ---------------------------------------------------------------------------
// prototypes of functions
static BOOL CALLBACK ConsoleWndProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam);
static void ConsoleWndAllUpdate(void);
static void ConsoleWndVerbosityUpdate(void);
static void ConsoleWndVerbosityApply(void);
static void ConsoleWndValidUpdate(void);
static void ConsoleWndValidApply(void);
static void ConsoleWndVerbosityApplyIncDec(int num);
static int ConsoleWndInfoReset(HWND hwnd);
static int ConsoleWndInfoApply(void);

// ---------------------------------------------------------------------------
// Global Functions

// Initialization
void InitConsoleWnd(HWND hParentWnd)
{
	if (hConsoleWnd != NULL) {
		DestroyWindow(hConsoleWnd);
		hConsoleWnd = NULL;
	}
	INILoadConsoleWnd();
	switch(PlayerLanguage){
  	case LANGUAGE_ENGLISH:
		hConsoleWnd = CreateDialog
  			(hInst,MAKEINTRESOURCE(IDD_DIALOG_CONSOLE_EN),hParentWnd,ConsoleWndProc);
		break;
 	default:
	case LANGUAGE_JAPANESE:
		hConsoleWnd = CreateDialog
  			(hInst,MAKEINTRESOURCE(IDD_DIALOG_CONSOLE),hParentWnd,ConsoleWndProc);
	break;
	}
	ConsoleWndInfoReset(hConsoleWnd);
	ShowWindow(hConsoleWnd,SW_HIDE);
	ConsoleWndInfoReset(hConsoleWnd);
	UpdateWindow(hConsoleWnd);
	ConsoleWndVerbosityApplyIncDec(0);
	CheckDlgButton(hConsoleWnd, IDC_CHECKBOX_VALID, ConsoleWndFlag);
	Edit_LimitText(GetDlgItem(hConsoleWnd,IDC_EDIT), ConsoleWndMaxSize);
	ConsoleWndInfoApply();
}

// Window Procedure
static BOOL CALLBACK
ConsoleWndProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam)
{
	switch (uMess){
	case WM_INITDIALOG:
		PutsConsoleWnd("Console Window\n");
		ConsoleWndAllUpdate();
		SetWindowPosSize(GetDesktopWindow(),hwnd,ConsoleWndInfo.PosX, ConsoleWndInfo.PosY );
		return FALSE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCLOSE:
			ShowWindow(hwnd, SW_HIDE);
			MainWndUpdateConsoleButton();
			break;
		case IDCLEAR:
			ClearConsoleWnd();
			break;
		case IDC_CHECKBOX_VALID:
			ConsoleWndValidApply();
			break;
		case IDC_BUTTON_VERBOSITY:
			ConsoleWndVerbosityApply();
			break;
		case IDC_BUTTON_INC:
			ConsoleWndVerbosityApplyIncDec(1);
			break;
		case IDC_BUTTON_DEC:
			ConsoleWndVerbosityApplyIncDec(-1);
			break;
		default:
			break;
		}
		switch (HIWORD(wParam)) {
		case EN_ERRSPACE:
			ClearConsoleWnd();
			PutsConsoleWnd("### EN_ERRSPACE -> Clear! ###\n");
			break;
		default:
			break;
		}
		break;
	case WM_SIZE:
		ConsoleWndAllUpdate();
		return FALSE;
	case WM_MOVE:
//		ConsoleWndInfo.PosX = (int) LOWORD(lParam);
//		ConsoleWndInfo.PosY = (int) HIWORD(lParam);
		{
			RECT rc;
			GetWindowRect(hwnd,&rc);
			ConsoleWndInfo.PosX = rc.left;
			ConsoleWndInfo.PosY = rc.top;
		}
		break;
	// See PreDispatchMessage() in w32g2_main.c
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		int nVirtKey = (int)wParam;
		switch(nVirtKey){
			case VK_ESCAPE:
				SendMessage(hwnd,WM_CLOSE,0,0);
				break;
		}
	}
		break;
	case WM_DESTROY:
		INISaveConsoleWnd();
		break;
	case WM_CLOSE:
		ShowSubWindow(hConsoleWnd,0);
//		ShowWindow(hConsoleWnd, SW_HIDE);
		MainWndUpdateConsoleButton();
		break;
	case WM_SETFOCUS:
		HideCaret(hwnd);
		break;
	case WM_KILLFOCUS:
		ShowCaret(hwnd);
		break;
	default:
		return FALSE;
	}
	return FALSE;
}

// puts()
void PutsConsoleWnd(char *str)
{
	HWND hwnd;
	if(!IsWindow(hConsoleWnd) || !ConsoleWndFlag)
		return;
	hwnd = GetDlgItem(hConsoleWnd,IDC_EDIT);
	PutsEditCtlWnd(hwnd,str);
}

// printf()
void PrintfConsoleWnd(char *fmt, ...)
{
	HWND hwnd;
	va_list ap;
	if(!IsWindow(hConsoleWnd) || !ConsoleWndFlag)
		return;
	hwnd = GetDlgItem(hConsoleWnd,IDC_EDIT);
	va_start(ap, fmt);
	VprintfEditCtlWnd(hwnd,fmt,ap);
	va_end(ap);
}

// Clear
void ClearConsoleWnd(void)
{
	HWND hwnd;
	if(!IsWindow(hConsoleWnd))
		return;
	hwnd = GetDlgItem(hConsoleWnd,IDC_EDIT);
	ClearEditCtlWnd(hwnd);
}

// ---------------------------------------------------------------------------
// Static Functions

static void ConsoleWndAllUpdate(void)
{
	ConsoleWndVerbosityUpdate();
	ConsoleWndValidUpdate();
	Edit_LimitText(GetDlgItem(hConsoleWnd,IDC_EDIT_VERBOSITY),3);
	Edit_LimitText(GetDlgItem(hConsoleWnd,IDC_EDIT),ConsoleWndMaxSize);
}

static void ConsoleWndValidUpdate(void)
{
	if(ConsoleWndFlag)
		CheckDlgButton(hConsoleWnd, IDC_CHECKBOX_VALID, 1);
	else
		CheckDlgButton(hConsoleWnd, IDC_CHECKBOX_VALID, 0);
}

static void ConsoleWndValidApply(void)
{
	if(IsDlgButtonChecked(hConsoleWnd,IDC_CHECKBOX_VALID))
		ConsoleWndFlag = 1;
	else
		ConsoleWndFlag = 0;
}

static void ConsoleWndVerbosityUpdate(void)
{
	SetDlgItemInt(hConsoleWnd,IDC_EDIT_VERBOSITY,(UINT)ctl->verbosity, TRUE);
}

static void ConsoleWndVerbosityApply(void)
{
	char buffer[64];
	HWND hwnd;
	hwnd = GetDlgItem(hConsoleWnd,IDC_EDIT_VERBOSITY);
	if(!IsWindow(hConsoleWnd)) return;
	if(Edit_GetText(hwnd,buffer,60)<=0) return;
	ctl->verbosity = atoi(buffer);
	ConsoleWndVerbosityUpdate();
}

static void ConsoleWndVerbosityApplyIncDec(int num)
{
	if(!IsWindow(hConsoleWnd)) return;
	ctl->verbosity += num;
	RANGE(ctl->verbosity, -1, 4);
	ConsoleWndVerbosityUpdate();
}

static int ConsoleWndInfoReset(HWND hwnd)
{
	memset(&ConsoleWndInfo,0,sizeof(CONSOLEWNDINFO));
	ConsoleWndInfo.PosX = - 1;
	ConsoleWndInfo.PosY = - 1;
	return 0;
}

static int ConsoleWndInfoApply(void)
{
	return 0;
}



// ****************************************************************************
//
// List Window
//
// ****************************************************************************

// ---------------------------------------------------------------------------
// Macros
#define IDM_LISTWND_REMOVE		4101
#define IDM_LISTWND_PLAY  		4102
#define IDM_LISTWND_REFINE 		4103
#define IDM_LISTWND_UNIQ 		4104
#define IDM_LISTWND_CLEAR 		4105
#define IDM_LISTWND_CHOOSEFONT	4106
#define IDM_LISTWND_CURRENT 4107
#define IDM_LISTWND_SEARCH 4108

// ---------------------------------------------------------------------------
// Variables
LISTWNDINFO ListWndInfo;
HWND hListSearchWnd = NULL;

// ---------------------------------------------------------------------------
// Prototypes
static BOOL CALLBACK ListWndProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam);
static int ListWndInfoReset(HWND hwnd);
static int ListWndInfoApply(void);
static int ListWndSetFontListBox(char *fontName, int fontWidth, int fontHeght);
static int ResetListWnd(void);
static int ClearListWnd(void);
static int UniqListWnd(void);
static int RefineListWnd(void);
static int DelListWnd(int nth);

// ---------------------------------------------------------------------------
// Grobal Functions
void InitListWnd(HWND hParentWnd)
{
	if (hListWnd != NULL) {
		DestroyWindow(hListWnd);
		hListWnd = NULL;
	}
	ListWndInfoReset(hListWnd);
	INILoadListWnd();
	switch(PlayerLanguage){
	case LANGUAGE_ENGLISH:
		hListWnd = CreateDialog
			(hInst,MAKEINTRESOURCE(IDD_DIALOG_SIMPLE_LIST_EN),hParentWnd,ListWndProc);
		break;
	default:
	case LANGUAGE_JAPANESE:
		hListWnd = CreateDialog
			(hInst,MAKEINTRESOURCE(IDD_DIALOG_SIMPLE_LIST),hParentWnd,ListWndProc);
		break;
	}
	ListWndInfoReset(hListWnd);
	ListWndInfo.hPopupMenu = CreatePopupMenu();
	switch(PlayerLanguage){
	case LANGUAGE_JAPANESE:
		AppendMenu(ListWndInfo.hPopupMenu,MF_STRING,IDM_LISTWND_PLAY,"演奏");
		AppendMenu(ListWndInfo.hPopupMenu,MF_STRING,IDC_BUTTON_DOC,"ドキュメント");
		AppendMenu(ListWndInfo.hPopupMenu,MF_SEPARATOR,0,0);
		AppendMenu(ListWndInfo.hPopupMenu,MF_STRING,IDM_LISTWND_CURRENT,"現在位置");
		AppendMenu(ListWndInfo.hPopupMenu,MF_STRING,IDM_LISTWND_SEARCH,"検索");
		AppendMenu(ListWndInfo.hPopupMenu,MF_SEPARATOR,0,0);
		AppendMenu(ListWndInfo.hPopupMenu,MF_STRING,IDM_LISTWND_REMOVE,"削除");
		AppendMenu(ListWndInfo.hPopupMenu,MF_SEPARATOR,0,0);
		AppendMenu(ListWndInfo.hPopupMenu,MF_STRING,IDM_LISTWND_CHOOSEFONT,"フォントの選択");
		break;
 	default:
  	case LANGUAGE_ENGLISH:
		AppendMenu(ListWndInfo.hPopupMenu,MF_STRING,IDM_LISTWND_PLAY,"Play");
		AppendMenu(ListWndInfo.hPopupMenu,MF_STRING,IDC_BUTTON_DOC,"Doc");
		AppendMenu(ListWndInfo.hPopupMenu,MF_SEPARATOR,0,0);
		AppendMenu(ListWndInfo.hPopupMenu,MF_STRING,IDM_LISTWND_CURRENT,"Current item");
		AppendMenu(ListWndInfo.hPopupMenu,MF_STRING,IDM_LISTWND_SEARCH,"Search");
		AppendMenu(ListWndInfo.hPopupMenu,MF_SEPARATOR,0,0);
		AppendMenu(ListWndInfo.hPopupMenu,MF_STRING,IDM_LISTWND_REMOVE,"Remove");
		AppendMenu(ListWndInfo.hPopupMenu,MF_SEPARATOR,0,0);
		AppendMenu(ListWndInfo.hPopupMenu,MF_STRING,IDM_LISTWND_CHOOSEFONT,"Choose Font");
		break;
	}

	INILoadListWnd();
	ListWndInfoApply();
	ShowWindow(ListWndInfo.hwnd,SW_HIDE);
	UpdateWindow(ListWndInfo.hwnd);
	w32g_send_rc(RC_EXT_UPDATE_PLAYLIST, 0);
}

// ---------------------------------------------------------------------------
// Static Functions

void SetNumListWnd(int cursel, int nfiles);

#define WM_CHOOSEFONT_DIAG	(WM_APP+100)
#define WM_LIST_SEARCH_DIAG	(WM_APP+101)
#define LISTWND_HORIZONTALEXTENT 1600

static BOOL CALLBACK
ListWndProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam)
{
	static BOOL ListSearchWndShow;
	switch (uMess){
	case WM_INITDIALOG:
		ListSearchWndShow = 0;
		InitListSearchWnd(hwnd);
		SendDlgItemMessage(hwnd,IDC_LISTBOX_PLAYLIST,
			LB_SETHORIZONTALEXTENT,(WPARAM)LISTWND_HORIZONTALEXTENT,0);
		w32g_send_rc(RC_EXT_UPDATE_PLAYLIST, 0);
		SetWindowPosSize(GetDesktopWindow(),hwnd,ListWndInfo.PosX, ListWndInfo.PosY );
		return FALSE;
	case WM_DESTROY:
		{
		RECT rc;
		GetWindowRect(hwnd,&rc);
		ListWndInfo.Width = rc.right - rc.left;
		ListWndInfo.Height = rc.bottom - rc.top;
		}
		DestroyMenu(ListWndInfo.hPopupMenu);
		ListWndInfo.hPopupMenu = NULL;
		INISaveListWnd();
		break;
		/* マウス入力がキャプチャされていないための処理 */
	case WM_SETCURSOR:
		switch(HIWORD(lParam)){
		case WM_RBUTTONDOWN:
			if(LOWORD(lParam)!=HTCAPTION){	// タイトルバーにないとき
				POINT point;
				int res;
				GetCursorPos(&point);
				SetForegroundWindow ( hwnd );
				res = TrackPopupMenu(ListWndInfo.hPopupMenu,TPM_TOPALIGN|TPM_LEFTALIGN,
					point.x,point.y,0,hwnd,NULL);
				PostMessage ( hwnd, WM_NULL, 0, 0 );
				return TRUE;
			}
			break;
		default:
			break;
		}
		break;
	case WM_CHOOSEFONT_DIAG:
		{
			char fontName[64];
			int fontHeight;
			int fontWidth;
			strcpy(fontName,ListWndInfo.fontName);
			fontHeight = ListWndInfo.fontHeight;
			fontWidth = ListWndInfo.fontWidth;
			if(DlgChooseFont(hwnd,fontName,&fontHeight,&fontWidth)==0){
				ListWndSetFontListBox(fontName,fontWidth,fontHeight);
			}
		}
		break;
	case WM_LIST_SEARCH_DIAG:
		ShowListSearch();
		break;
	case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case IDCLOSE:
				ShowWindow(hwnd, SW_HIDE);
				MainWndUpdateListButton();
				break;
			case LBN_DBLCLK:
				SendMessage(hwnd,WM_COMMAND,(WPARAM)IDM_LISTWND_PLAY,0);
				return FALSE;
			case LBN_SELCHANGE:
				{
				int idListBox = (int) LOWORD(wParam);
				HWND hwndListBox = (HWND) lParam;
				int selected, nfiles, cursel;
				w32g_get_playlist_index(&selected,&nfiles,&cursel);
				SetNumListWnd(cursel,nfiles);
				return FALSE;
				}
			default:
				break;
			}
			switch (LOWORD(wParam)) {
			case IDC_BUTTON_CLEAR:
				if(MessageBox(hListWnd,"Clear playlist?","Playlist",
							  MB_YESNO)==IDYES)
					w32g_send_rc(RC_EXT_CLEAR_PLAYLIST, 0);
				return FALSE;
			case IDC_BUTTON_REFINE:
				if(MessageBox(hListWnd,
							  "Remove unsupported file types from the playlist?",
							  "Playlist",MB_YESNO) == IDYES)
					w32g_send_rc(RC_EXT_REFINE_PLAYLIST, 0);
				return FALSE;
			case IDC_BUTTON_UNIQ:
				if(MessageBox(hListWnd,
							  "Remove the same files from the playlist and make files of the playlist unique?",
							  "Playlist",MB_YESNO)==IDYES)
					w32g_send_rc(RC_EXT_UNIQ_PLAYLIST, 0);
				return FALSE;
			case IDM_LISTWND_REMOVE:
				w32g_send_rc(RC_EXT_DELETE_PLAYLIST, 0);
				break;
			case IDC_BUTTON_DOC: {
					int cursel;
					w32g_get_playlist_index(NULL, NULL, &cursel);
					w32g_send_rc(RC_EXT_OPEN_DOC, cursel);
				}
				break;
			case IDM_LISTWND_PLAY:
				{
					int new_cursel =  SendDlgItemMessage(hwnd,IDC_LISTBOX_PLAYLIST,LB_GETCURSEL,0,0);
					int selected, nfiles, cursel;
					w32g_get_playlist_index(&selected, &nfiles, &cursel);
					if ( nfiles <= new_cursel ) new_cursel = nfiles - 1;
					if ( new_cursel >= 0 )
						w32g_send_rc(RC_EXT_JUMP_FILE, new_cursel );
				}
				return FALSE;
			case IDM_LISTWND_CHOOSEFONT:
				{
 					SendMessage(hwnd,WM_CHOOSEFONT_DIAG,0,0);
				}
				return FALSE;
			case IDM_LISTWND_CURRENT:
				{
					int selected, nfiles, cursel;
					w32g_get_playlist_index(&selected, &nfiles, &cursel);
					SendDlgItemMessage(hwnd,IDC_LISTBOX_PLAYLIST,
						LB_SETCURSEL,(WPARAM)selected,0);
					SetNumListWnd(selected,nfiles);
				}
				return FALSE;
			case IDM_LISTWND_SEARCH:
				{
					SendMessage(hwnd,WM_LIST_SEARCH_DIAG,0,0);
				}
				return FALSE;
			default:
				break;
			}
			break;
			case WM_VKEYTOITEM:
				{
					UINT vkey = (UINT)LOWORD(wParam);
					int nCaretPos = (int)HIWORD(wParam);
					switch(vkey){
					case VK_SPACE:
					case VK_RETURN:
						w32g_send_rc(RC_EXT_JUMP_FILE, nCaretPos);
						return -2;
					case 0x50:	// VK_P
						SendMessage(hMainWnd,WM_COMMAND,MAKEWPARAM(IDM_PREV,0),0);
						return -2;
					case 0x4e:	// VK_N
						SendMessage(hMainWnd,WM_COMMAND,MAKEWPARAM(IDM_NEXT,0),0);
						return -2;
					case 0x45:	// VK_E
						SendMessage(hMainWnd,WM_COMMAND,MAKEWPARAM(IDM_STOP,0),0);
						return -2;
					case 0x53:	// VK_S
						SendMessage(hMainWnd,WM_COMMAND,MAKEWPARAM(IDM_PAUSE,0),0);
						return -2;
					case VK_ESCAPE:
						SendMessage(hListWnd,WM_COMMAND,MAKEWPARAM(0,IDCLOSE),0);
						return -2;
					case 0x51:	// VK_Q
						if(MessageBox(hListWnd,"Quit TiMidity?","TiMidity",MB_ICONQUESTION|MB_YESNO)==IDYES)
							SendMessage(hMainWnd,WM_CLOSE,0,0);
						return -2;
					case VK_BACK:
						w32g_send_rc(RC_EXT_DELETE_PLAYLIST, -1);
						return -2;
					case 0x44:	// VK_D
						w32g_send_rc(RC_EXT_DELETE_PLAYLIST, 0);
						return -2;
					case VK_DELETE:
						w32g_send_rc(RC_EXT_ROTATE_PLAYLIST, -1);
						return -2;
					case VK_INSERT:
						w32g_send_rc(RC_EXT_ROTATE_PLAYLIST, 1);
						return -2;
					case 0x46:	// VK_F
						return -2;
					case 0x42:	// VK_B
						return -2;
					case 0x4D:	// VK_M
						SendMessage(hListWnd,WM_COMMAND,MAKEWPARAM(IDC_BUTTON_REFINE,0),0);
						return -2;
					case 0x43:	// VK_C
						SendMessage(hListWnd,WM_COMMAND,MAKEWPARAM(IDC_BUTTON_CLEAR,0),0);
						return -2;
					case 0x55:	// VK_U
						SendMessage(hListWnd,WM_COMMAND,MAKEWPARAM(IDC_BUTTON_UNIQ,0),0);
						return -2;
					case 0x56:	// VK_V
						SendMessage(hListWnd,WM_COMMAND,MAKEWPARAM(IDC_BUTTON_DOC,0),0);
						return -2;
					case 0x57:	// VK_W
						SendMessage(hMainWnd,WM_COMMAND,MAKEWPARAM(IDM_WRD,0),0);
						return -2;
					case VK_F1:
					case 0x48:	// VK_H
						if ( PlayerLanguage == LANGUAGE_JAPANESE ){
						MessageBox(hListWnd,
							"キーコマンド\n"
							"リストウインドウコマンド\n"
							"  ESC: ヘルプを閉じる      H: ヘルプを出す\n"
							"  V: ドキュメントを見る      W: WRD ウインドウを開く\n"
							"プレイヤーコマンド\n"
							"  SPACE/ENTER: 演奏開始    E: 停止    S: 一時停止\n"
							"  P: 前の曲    N: 次の曲\n"
							"プレイリスト操作コマンド\n"
							"  M: MIDIファイル以外を削除    U: 重複ファイルを削除\n"
							"  C: プレイリストのクリア\n"
							"  D: カーソルの曲を削除    BS: カーソルの前の曲を削除\n"
							"  INS: カーソルの曲をリストの最後に移す (Push)\n"
							"  DEL: リストの最後の曲をカーソルの前に挿入 (Pop)\n"
							"TiMidity コマンド\n"
							"  Q: 終了\n"
							,"ヘルプ", MB_OK);
						} else {
						MessageBox(hListWnd,
							"Usage of key.\n"
							"List window command.\n"
							"  ESC: Close Help      H: Help\n"
							"  V: View Document   W: Open WRD window\n"
							"Player command.\n"
							"  SPACE/ENTER: PLAY    E: Stop    S: Pause\n"
							"  P: Prev    N: Next\n"
							"Playlist command.\n"
							"  M: Refine playlist    U: Uniq playlist\n"
							"  C: Clear playlist\n"
							"  D: Remove playlist    BS: Remove previous playlist\n"
							"  INS: Push Playlist    DEL: Pop Playlist\n"
							"TiMidity command.\n"
							"  Q: Quit\n"
							,"Help", MB_OK);
						}
						return -2;
					default:
						break;
			}
			return -1;
		}
	case WM_SIZE:
		switch(wParam){
		case SIZE_MAXIMIZED:
		case SIZE_RESTORED:
			{		// なんか意味なく面倒(^^;;
				int x,y,cx,cy;
				int maxHeight = 0;
				int center, idControl;
				HWND hwndChild;
				RECT rcParent, rcChild, rcRest;
				int nWidth = LOWORD(lParam);
				int nHeight = HIWORD(lParam);
				GetWindowRect(hwnd,&rcParent);
				cx = rcParent.right-rcParent.left;
				cy  = rcParent.bottom-rcParent.top;
				if(cx < 380)
					MoveWindow(hwnd,rcParent.left,rcParent.top,380,cy,TRUE);
				if(cy < 200)
					MoveWindow(hwnd,rcParent.left,rcParent.top,cx,200,TRUE);
				GetClientRect(hwnd,&rcParent);
				rcRest.left = rcParent.left; rcRest.right = rcParent.right;

				// IDC_EDIT_NUM
				idControl = IDC_EDIT_NUM;
				hwndChild = GetDlgItem(hwnd,idControl);
				GetWindowRect(hwndChild,&rcChild);
				cx = rcChild.right-rcChild.left;
				cy = rcChild.bottom-rcChild.top;
				x = rcParent.left;
				y = rcParent.bottom - cy;
				MoveWindow(hwndChild,x,y,cx,cy,TRUE);
				if(cy>maxHeight) maxHeight = cy;
				rcRest.left += cx;
				// IDC_BUTTON_DOC
				idControl = IDC_BUTTON_DOC;
				hwndChild = GetDlgItem(hwnd,idControl);
				GetWindowRect(hwndChild,&rcChild);
				cx = rcChild.right-rcChild.left;
				cy = rcChild.bottom-rcChild.top;
				x = rcRest.left + 10;
				y = rcParent.bottom - cy;
				MoveWindow(hwndChild,x,y,cx,cy,TRUE);
				if(cy>maxHeight) maxHeight = cy;
				rcRest.left += cx;
				// IDC_BUTTON_CLEAR
				idControl = IDC_BUTTON_CLEAR;
				hwndChild = GetDlgItem(hwnd,idControl);
				GetWindowRect(hwndChild,&rcChild);
				cx = rcChild.right-rcChild.left;
				cy = rcChild.bottom-rcChild.top;
				x = rcParent.right - cx - 5;
				y = rcParent.bottom - cy ;
				MoveWindow(hwndChild,x,y,cx,cy,TRUE);
				if(cy>maxHeight) maxHeight = cy;
				rcRest.right -= cx + 5;
				// IDC_BUTTON_UNIQ
				center = rcRest.left + (int)((rcRest.right - rcRest.left)*0.52);
				idControl = IDC_BUTTON_UNIQ;
				hwndChild = GetDlgItem(hwnd,idControl);
				GetWindowRect(hwndChild,&rcChild);
				cx = rcChild.right-rcChild.left;
				cy = rcChild.bottom-rcChild.top;
				x = center - cx;
				y = rcParent.bottom - cy;
				MoveWindow(hwndChild,x,y,cx,cy,TRUE);
				if(cy>maxHeight) maxHeight = cy;
				// IDC_BUTTON_REFINE
				idControl = IDC_BUTTON_REFINE;
				hwndChild = GetDlgItem(hwnd,idControl);
				GetWindowRect(hwndChild,&rcChild);
				cx = rcChild.right-rcChild.left;
				cy = rcChild.bottom-rcChild.top;
				x = center + 3;
				y = rcParent.bottom - cy;
				MoveWindow(hwndChild,x,y,cx,cy,TRUE);
				if(cy>maxHeight) maxHeight = cy;
				// IDC_LISTBOX_PLAYLIST
				idControl = IDC_LISTBOX_PLAYLIST;
				hwndChild = GetDlgItem(hwnd,idControl);
				cx = rcParent.right - rcParent.left;
				cy = rcParent.bottom - rcParent.top - maxHeight - 3;
				x  = rcParent.left;
				y = rcParent.top;
				MoveWindow(hwndChild,x,y,cx,cy,TRUE);
				InvalidateRect(hwnd,&rcParent,FALSE);
				UpdateWindow(hwnd);
				GetWindowRect(hwnd,&rcParent);
				ListWndInfo.Width = rcParent.right - rcParent.left;
				ListWndInfo.Height = rcParent.bottom - rcParent.top;
				break;
			}
		case SIZE_MINIMIZED:
		case SIZE_MAXHIDE:
		case SIZE_MAXSHOW:
		default:
			break;
		}
		break;
	case WM_MOVE:
//		ListWndInfo.PosX = (int) LOWORD(lParam);
//		ListWndInfo.PosY = (int) HIWORD(lParam);
		{
			RECT rc;
			GetWindowRect(hwnd,&rc);
			ListWndInfo.PosX = rc.left;
			ListWndInfo.PosY = rc.top;
		}
		break;
	// See PreDispatchMessage() in w32g2_main.c
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		int nVirtKey = (int)wParam;
		switch(nVirtKey){
			case VK_ESCAPE:
				SendMessage(hwnd,WM_CLOSE,0,0);
				break;
		}
	}
		break;
	case WM_CLOSE:
		ShowSubWindow(hListWnd,0);
//		ShowWindow(hListWnd, SW_HIDE);
		MainWndUpdateListButton();
		break;
	case WM_SHOWWINDOW:
	{
		BOOL fShow = (BOOL)wParam;
		if ( fShow ) {
			if ( ListSearchWndShow ) {
				ShowListSearch();
			} else {
				HideListSearch();
			}
		} else {
			if ( IsWindowVisible ( hListSearchWnd ) )
				ListSearchWndShow = TRUE;
			else
				ListSearchWndShow = FALSE;
			HideListSearch();
		}
	}
		break;
	case WM_DROPFILES:
		SendMessage(hMainWnd,WM_DROPFILES,wParam,lParam);
		return 0;
	default:
		return FALSE;
	}
	return FALSE;
}

static int ListWndInfoReset(HWND hwnd)
{
	memset(&ListWndInfo,0,sizeof(LISTWNDINFO));
	ListWndInfo.PosX = - 1;
	ListWndInfo.PosY = - 1;
	ListWndInfo.Height = 400;
	ListWndInfo.Width = 400;
	ListWndInfo.hPopupMenu = NULL;
	ListWndInfo.hwnd = hwnd;
	if ( hwnd != NULL )
		ListWndInfo.hwndListBox = GetDlgItem(hwnd,IDC_LISTBOX_PLAYLIST);
	strcpy(ListWndInfo.fontNameEN,"Times New Roman");
	strcpy(ListWndInfo.fontNameJA,"ＭＳ 明朝");
	ListWndInfo.fontHeight = 12;
	ListWndInfo.fontWidth = 6;
	ListWndInfo.fontFlags = FONT_FLAGS_FIXED;
	switch(PlayerLanguage){
	case LANGUAGE_ENGLISH:
		ListWndInfo.fontName = ListWndInfo.fontNameEN;
		break;
	default:
	case LANGUAGE_JAPANESE:
		ListWndInfo.fontName = ListWndInfo.fontNameJA;
		break;
	}
	return 0;
}
static int ListWndInfoApply(void)
{
	RECT rc;
	HFONT hFontPre = NULL;
	DWORD fdwPitch = (ListWndInfo.fontFlags&FONT_FLAGS_FIXED)?FIXED_PITCH:VARIABLE_PITCH;	
	DWORD fdwItalic = (ListWndInfo.fontFlags&FONT_FLAGS_ITALIC)?TRUE:FALSE;
	HFONT hFont =
		CreateFont(ListWndInfo.fontHeight,ListWndInfo.fontWidth,0,0,FW_DONTCARE,fdwItalic,FALSE,FALSE,
			DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
	      	fdwPitch | FF_DONTCARE,ListWndInfo.fontName);
	if(hFont != NULL){
		hFontPre = ListWndInfo.hFontListBox;
		ListWndInfo.hFontListBox = hFont;
		SendMessage(ListWndInfo.hwndListBox,WM_SETFONT,(WPARAM)ListWndInfo.hFontListBox,(LPARAM)MAKELPARAM(TRUE,0));
	}
	GetWindowRect(ListWndInfo.hwnd,&rc);
	MoveWindow(ListWndInfo.hwnd,rc.left,rc.top,ListWndInfo.Width,ListWndInfo.Height,TRUE);
//	InvalidateRect(hwnd,&rc,FALSE);
//	UpdateWindow(hwnd);
	if(hFontPre!=NULL) CloseHandle(hFontPre);
	INISaveListWnd();
	return 0;
}

static int ListWndSetFontListBox(char *fontName, int fontWidth, int fontHeight)
{
	strcpy(ListWndInfo.fontName,fontName);
	ListWndInfo.fontWidth = fontWidth;
	ListWndInfo.fontHeight = fontHeight;
	ListWndInfoApply();
	return 0;
}

void SetNumListWnd(int cursel, int nfiles)
{
	char buff[64];
	sprintf(buff,"%04d/%04d",cursel+1,nfiles);
	SetDlgItemText(hListWnd,IDC_EDIT_NUM,buff);
}


#if 0
// ***************************************************************************
// Tracer Window

BOOL CALLBACK TracerWndProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam);
void InitTracerWnd(HWND hParentWnd)
{
	if (hTracerWnd != NULL) {
		DestroyWindow(hTracerWnd);
		hTracerWnd = NULL;
	}
	hTracerWnd = CreateDialog
		(hInst,MAKEINTRESOURCE(IDD_DIALOG_TRACER),hParentWnd,TracerWndProc);
	ShowWindow(hTracerWnd,SW_HIDE);
	UpdateWindow(hTracerWnd);
}

BOOL CALLBACK
TracerWndProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam)
{
	switch (uMess){
	case WM_INITDIALOG:
		return FALSE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCLOSE:
			ShowWindow(hwnd, SW_HIDE);
			MainWndUpdateTracerButton();
			break;
		default:
			return FALSE;
		}
		case WM_SIZE:
			return FALSE;
		case WM_CLOSE:
			ShowWindow(hTracerWnd, SW_HIDE);
			MainWndUpdateTracerButton();
			break;
		default:
			return FALSE;
	}
	return FALSE;
}
#endif



//****************************************************************************
// Doc Window

#define IDM_DOCWND_CHOOSEFONT 4232
int DocWndIndependent = 0; /* Independent document viewer mode.(独立ドキュメントビュワーモード) */
int DocWndAutoPopup = 0;
DOCWNDINFO DocWndInfo;

static BOOL CALLBACK DocWndProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam);
static void InitDocEditWnd(HWND hParentWnd);
static void DocWndConvertText(char *in, int in_size, char *out, int out_size);
static void DocWndSetText(char *text, int text_size);
static void DocWndSetInfo(char *info, char *filename);
static void DocWndInfoInit(void);
static int DocWndInfoLock(void);
static void DocWndInfoUnLock(void);

void InitDocWnd(HWND hParentWnd);
void DocWndInfoReset(void);
void DocWndAddDocFile(char *filename);
void DocWndSetMidifile(char *filename);
void DocWndReadDoc(int num);
void DocWndReadDocNext(void);
void DocWndReadDocPrev(void);

static int DocWndInfoReset2(HWND hwnd);
static int DocWndInfoApply(void);
static int DocWndSetFontEdit(char *fontName, int fontWidth, int fontHeight);

void InitDocWnd(HWND hParentWnd)
{
	HMENU hMenu;
	HICON hIcon;
	if (hDocWnd != NULL) {
		DestroyWindow(hDocWnd);
		hDocWnd = NULL;
	}
	DocWndInfoReset2(hDocWnd);
	INILoadDocWnd();
	switch(PlayerLanguage){
  	case LANGUAGE_ENGLISH:
		hDocWnd = CreateDialog
			(hInst,MAKEINTRESOURCE(IDD_DIALOG_DOC_EN),hParentWnd,DocWndProc);
		break;
 	default:
	case LANGUAGE_JAPANESE:
		hDocWnd = CreateDialog
			(hInst,MAKEINTRESOURCE(IDD_DIALOG_DOC),hParentWnd,DocWndProc);
	break;
	}
	hIcon = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_TIMIDITY), IMAGE_ICON, 16, 16, 0);
	if (hIcon!=NULL) SendMessage(hDocWnd,WM_SETICON,FALSE,(LPARAM)hIcon);
	DocWndInfoReset2(hDocWnd);
	hMenu = GetSystemMenu(DocWndInfo.hwnd,FALSE);
	switch(PlayerLanguage){
	case LANGUAGE_JAPANESE:
		AppendMenu(hMenu,MF_SEPARATOR,0,0);
		AppendMenu(hMenu,MF_STRING,IDM_DOCWND_CHOOSEFONT,"フォントの選択");
		break;
 	default:
  	case LANGUAGE_ENGLISH:
		AppendMenu(hMenu,MF_SEPARATOR,0,0);
		AppendMenu(hMenu,MF_STRING,IDM_DOCWND_CHOOSEFONT,"Choose Font");
		break;
	}
	DocWndInfoReset2(hDocWnd);
	INILoadDocWnd();
	DocWndInfoApply();
	ShowWindow(hDocWnd,SW_HIDE);
	UpdateWindow(hDocWnd);
	EnableWindow(GetDlgItem(hDocWnd,IDC_BUTTON_PREV),FALSE);
	EnableWindow(GetDlgItem(hDocWnd,IDC_BUTTON_NEXT),FALSE);
}

static BOOL CALLBACK
DocWndProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam)
{
	switch (uMess){
	case WM_INITDIALOG:
		PutsDocWnd("Doc Window\n");
		DocWndInfoInit();
		SetWindowPosSize(GetDesktopWindow(),hwnd,DocWndInfo.PosX, DocWndInfo.PosY );
		return FALSE;
	case WM_DESTROY:
		{
		RECT rc;
		GetWindowRect(hwnd,&rc);
		DocWndInfo.Width = rc.right - rc.left;
		DocWndInfo.Height = rc.bottom - rc.top;
		}
		INISaveDocWnd();
		break;
	case WM_SYSCOMMAND:
		switch(wParam){
		case IDM_DOCWND_CHOOSEFONT:
		{
			char fontName[64];
			int fontHeight;
			int fontWidth;
			strcpy(fontName,DocWndInfo.fontName);
			fontHeight = DocWndInfo.fontHeight;
			fontWidth = DocWndInfo.fontWidth;
			if(DlgChooseFont(hwnd,fontName,&fontHeight,&fontWidth)==0){
				DocWndSetFontEdit(fontName,fontWidth,fontHeight);
			}
			break;
		}
			break;
		default:
			break;
		}
    case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCLOSE:
			ShowWindow(hwnd, SW_HIDE);
			MainWndUpdateDocButton();
			break;
        case IDCLEAR:
			ClearDocWnd();
			break;
		default:
			break;
		}
		switch (LOWORD(wParam)) {
		case IDC_BUTTON_NEXT:
			DocWndReadDocNext();
			break;
		case IDC_BUTTON_PREV:
			DocWndReadDocPrev();
			break;
		default:
			break;
		}
		return FALSE;
	// See PreDispatchMessage() in w32g2_main.c
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		int nVirtKey = (int)wParam;
		switch(nVirtKey){
			case VK_ESCAPE:
				SendMessage(hwnd,WM_CLOSE,0,0);
				break;
		}
	}
		break;
	case WM_CLOSE:
		ShowSubWindow(hDocWnd,0);
//		ShowWindow(hDocWnd, SW_HIDE);
		MainWndUpdateDocButton();
		break;
	case WM_SIZE:
		switch(wParam){
		case SIZE_MAXIMIZED:
		case SIZE_RESTORED:
			{		// なんか意味なく面倒(^^;;
				int x,y,cx,cy;
				int max = 0;
				int width;
				RECT rcParent;
				RECT rcEDIT_INFO, rcEDIT_FILENAME, rcBUTTON_PREV, rcBUTTON_NEXT, rcEDIT;
				HWND hwndEDIT_INFO, hwndEDIT_FILENAME, hwndBUTTON_PREV, hwndBUTTON_NEXT, hwndEDIT;
				int nWidth = LOWORD(lParam);
				int nHeight = HIWORD(lParam);
				GetWindowRect(hwnd,&rcParent);
				cx = rcParent.right-rcParent.left;
				cy  = rcParent.bottom-rcParent.top;
				if(cx < 300)
					MoveWindow(hwnd,rcParent.left,rcParent.top,300,cy,TRUE);
				if(cy < 200)
					MoveWindow(hwnd,rcParent.left,rcParent.top,cx,200,TRUE);
				GetClientRect(hwnd,&rcParent);
				hwndEDIT = GetDlgItem(hwnd,IDC_EDIT);
				hwndEDIT_INFO = GetDlgItem(hwnd,IDC_EDIT_INFO);
				hwndEDIT_FILENAME = GetDlgItem(hwnd,IDC_EDIT_FILENAME);
				hwndBUTTON_PREV = GetDlgItem(hwnd,IDC_BUTTON_PREV);
				hwndBUTTON_NEXT = GetDlgItem(hwnd,IDC_BUTTON_NEXT);
				GetWindowRect(hwndEDIT,&rcEDIT);
				GetWindowRect(hwndEDIT_INFO,&rcEDIT_INFO);
				GetWindowRect(hwndEDIT_FILENAME,&rcEDIT_FILENAME);
				GetWindowRect(hwndBUTTON_PREV,&rcBUTTON_PREV);
				GetWindowRect(hwndBUTTON_NEXT,&rcBUTTON_NEXT);
				width = rcParent.right - rcParent.left;
				cx = rcBUTTON_NEXT.right-rcBUTTON_NEXT.left;
				cy = rcBUTTON_NEXT.bottom-rcBUTTON_NEXT.top;
				x = rcParent.right - cx - 5;
				y = rcParent.bottom - cy;
				MoveWindow(hwndBUTTON_NEXT,x,y,cx,cy,TRUE);
				width -= cx + 5;
				if(cy>max) max = cy;
				cx = rcBUTTON_PREV.right-rcBUTTON_PREV.left;
				cy = rcBUTTON_PREV.bottom-rcBUTTON_PREV.top;
				x  -= cx + 5;
				y = rcParent.bottom - cy;
				MoveWindow(hwndBUTTON_PREV,x,y,cx,cy,TRUE);
				width -= cx;
				if(cy>max) max = cy;
				width -= 5;
//				cx = rcEDIT_INFO.right-rcEDIT_INFO.left;
				cx = (int)(width * 0.36);
				cy = rcEDIT_INFO.bottom-rcEDIT_INFO.top;
				x = rcParent.left;
				y = rcParent.bottom - cy;
				MoveWindow(hwndEDIT_INFO,x,y,cx,cy,TRUE);
				if(cy>max) max = cy;
				x += cx + 5;
//				cx = rcEDIT_FILENAME.right-rcEDIT_FILENAME.left;
				cx = (int)(width * 0.56);
				cy = rcEDIT_FILENAME.bottom-rcEDIT_FILENAME.top;
				y = rcParent.bottom - cy;
				MoveWindow(hwndEDIT_FILENAME,x,y,cx,cy,TRUE);
				if(cy>max) max = cy;
				cx = rcParent.right - rcParent.left;
				cy = rcParent.bottom - rcParent.top - max - 5;
				x  = rcParent.left;
				y = rcParent.top;
				MoveWindow(hwndEDIT,x,y,cx,cy,TRUE);
				InvalidateRect(hwnd,&rcParent,FALSE);
				UpdateWindow(hwnd);
				GetWindowRect(hwnd,&rcParent);
				DocWndInfo.Width = rcParent.right - rcParent.left;
				DocWndInfo.Height = rcParent.bottom - rcParent.top;
				break;
			}
		case SIZE_MINIMIZED:
		case SIZE_MAXHIDE:
		case SIZE_MAXSHOW:
		default:
			break;
		}
		break;
	case WM_MOVE:
//		DocWndInfo.PosX = (int) LOWORD(lParam);
//		DocWndInfo.PosY = (int) HIWORD(lParam);
		{
			RECT rc;
			GetWindowRect(hwnd,&rc);
			DocWndInfo.PosX = rc.left;
			DocWndInfo.PosY = rc.top;
		}
		break;
	default:
		return FALSE;
	}
	return FALSE;
}

static int DocWndInfoReset2(HWND hwnd)
{
//	memset(&DocWndInfo,0,sizeof(DOCWNDINFO));
	DocWndInfo.PosX = - 1;
	DocWndInfo.PosY = - 1;
	DocWndInfo.Height = 400;
	DocWndInfo.Width = 400;
	DocWndInfo.hPopupMenu = NULL;
	DocWndInfo.hwnd = hwnd;
	if ( hwnd != NULL )
	DocWndInfo.hwndEdit = GetDlgItem(hwnd,IDC_EDIT);
	strcpy(DocWndInfo.fontNameEN,"Times New Roman");
	strcpy(DocWndInfo.fontNameJA,"ＭＳ 明朝");
	DocWndInfo.fontHeight = 12;
	DocWndInfo.fontWidth = 6;
	DocWndInfo.fontFlags = FONT_FLAGS_FIXED;
	switch(PlayerLanguage){
	case LANGUAGE_ENGLISH:
		DocWndInfo.fontName = DocWndInfo.fontNameEN; 
		break;
	default:
	case LANGUAGE_JAPANESE:
		DocWndInfo.fontName = DocWndInfo.fontNameJA; 
		break;
	}
	return 0;
}
static int DocWndInfoApply(void)
{
	RECT rc;
	HFONT hFontPre = NULL;
	DWORD fdwPitch = (DocWndInfo.fontFlags&FONT_FLAGS_FIXED)?FIXED_PITCH:VARIABLE_PITCH;	
	DWORD fdwItalic = (DocWndInfo.fontFlags&FONT_FLAGS_ITALIC)?TRUE:FALSE;
	HFONT hFont =
		CreateFont(DocWndInfo.fontHeight,DocWndInfo.fontWidth,0,0,FW_DONTCARE,fdwItalic,FALSE,FALSE,
			DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
	      	fdwPitch | FF_DONTCARE,DocWndInfo.fontName);
	if(hFont != NULL){
		hFontPre = DocWndInfo.hFontEdit;
		DocWndInfo.hFontEdit = hFont;
		SendMessage(DocWndInfo.hwndEdit,WM_SETFONT,(WPARAM)DocWndInfo.hFontEdit,(LPARAM)MAKELPARAM(TRUE,0));
	}
	GetWindowRect(DocWndInfo.hwnd,&rc);
	MoveWindow(DocWndInfo.hwnd,rc.left,rc.top,DocWndInfo.Width,DocWndInfo.Height,TRUE);
//	InvalidateRect(hwnd,&rc,FALSE);
//	UpdateWindow(hwnd);
	if(hFontPre!=NULL) CloseHandle(hFontPre);
	INISaveDocWnd();
	return 0;
}

static int DocWndSetFontEdit(char *fontName, int fontWidth, int fontHeight)
{
	strcpy(DocWndInfo.fontName,fontName);
	DocWndInfo.fontWidth = fontWidth;
	DocWndInfo.fontHeight = fontHeight;
	DocWndInfoApply();
	return 0;
}

static char ControlCode[] = "@ABCDEFGHIJKLMNOPQRS";
static void DocWndConvertText(char *in, int in_size, char *out, int out_size)
{
	char *buffer = (char *)safe_malloc(sizeof(char)*out_size);
	int buffer_size = out_size;
	int i=0, j=0;
	int nl = 0;

// Convert Return Code CR, LF -> CR+LF ,
//         Control Code -> ^? (^@, ^A, ^B, ...).
// stage1:
	for(;;){
		if(i>=in_size || j>=buffer_size-1)
			goto stage1_end;
		if(nl==13){
			if(in[i]==13){
				if(j>=buffer_size-2)
					goto stage1_end;
				buffer[j++] = 13;
				buffer[j++] = 10;
				i++;
				nl = 13;
				continue;
			}
			if(in[i]==10){
				if(j>=buffer_size-2)
					goto stage1_end;
				buffer[j++] = 13;
				buffer[j++] = 10;
				i++;
				nl = 0;
				continue;
			}
			if(j>=buffer_size-2)
				goto stage1_end;
			buffer[j++] = 13;
			buffer[j++] = 10;
			if(in[i]>=0 && in[i]<=0x1f && in[i]!='\t'){
				if(j>=buffer_size-2)
					goto stage1_end;
				buffer[j++] = '^';
				buffer[j++] = ControlCode[in[i]];
			} else {
				if(j>=buffer_size-1)
					goto stage1_end;
				buffer[j++] = in[i];
			}
			i++;
			nl = 0;
			continue;
		}
		if(nl==10){
			if(in[i]==13||in[i]==10){
				if(j>=buffer_size-2)
					goto stage1_end;
				buffer[j++] = 13;
				buffer[j++] = 10;
				nl = in[i];
				i++;
				continue;
			}
			if(j>=buffer_size-2)
				goto stage1_end;
			buffer[j++] = 13;
			buffer[j++] = 10;
			if(in[i]>=0 && in[i]<=0x1f && in[i]!='\t'){
				if(j>=buffer_size-2)
					goto stage1_end;
				buffer[j++] = '^';
				buffer[j++] = ControlCode[in[i]];
			} else {
				if(j>=buffer_size-1)
					goto stage1_end;
				buffer[j++] = in[i];
			}
			i++;
			nl = 0;
			continue;
		}
		if(in[i]==13||in[i]==10){
			nl = in[i];
			i++;
			continue;
		}
		if(in[i]>=0 && in[i]<=0x1f && in[i]!='\t'){
			if(j>=buffer_size-2)
				goto stage1_end;
			buffer[j++] = '^';
			buffer[j++] = ControlCode[in[i]];
		} else {
			if(j>=buffer_size-1)
				goto stage1_end;
			buffer[j++] = in[i];
		}
		i++;
		nl = 0;
		continue;
	}
stage1_end:
	buffer[j] = '\0';
// Convert KANJI Code.
// stage2:
#ifndef MAX2
#define MAX2(x,y) ((x)>=(y)?(x):(y))
#endif
	switch(PlayerLanguage){
  	case LANGUAGE_ENGLISH:
 	default:
		strncpy(out,buffer,MAX2(buffer_size-1,out_size-1));
		out[out_size-1] = '\0';
		break;
	case LANGUAGE_JAPANESE:
		strncpy(out,buffer,MAX2(buffer_size-1,out_size-1));
		nkf_convert(buffer,out,out_size-1,NULL,"SJIS");
		out[out_size-1] = '\0';
		break;
	}
}

#define BUFFER_SIZE (1024*64)
static void DocWndSetText(char *text, int text_size)
{
	char buffer[BUFFER_SIZE];
	int buffer_size = BUFFER_SIZE;
	if(!IsWindow(hDocWnd) || !DocWndFlag)
		return;
	if(DocWndInfoLock()==FALSE)
		return;
//	Edit_SetText(GetDlgItem(hDocWnd,IDC_EDIT),text);
	DocWndConvertText(text,text_size,buffer,buffer_size);
	Edit_SetText(GetDlgItem(hDocWnd,IDC_EDIT),buffer);
	DocWndInfoUnLock();
}

static void DocWndSetInfo(char *info, char *filename)
{
	int buffer_size = BUFFER_SIZE;
	if(!IsWindow(hDocWnd) || !DocWndFlag)
		return;
	if(DocWndInfoLock()==FALSE)
		return;
	Edit_SetText(GetDlgItem(hDocWnd,IDC_EDIT_INFO),info);
	Edit_SetText(GetDlgItem(hDocWnd,IDC_EDIT_FILENAME),filename);
	DocWndInfoUnLock();
}

// *.doc *.txt *.hed archive#*.doc archive#*.txt archive#*.hed 

static void DocWndInfoInit(void)
{
//	DocWndInfo.hMutex = NULL;
//	DocWndInfo.hMutex = CreateMutex(NULL,TRUE,NULL);
	DocWndInfo.DocFileCur = 0;
	DocWndInfo.DocFileMax = 0;
	DocWndInfo.Text = NULL;
	DocWndInfo.TextSize = 0;
	EnableWindow(GetDlgItem(hDocWnd,IDC_BUTTON_PREV),FALSE);
	EnableWindow(GetDlgItem(hDocWnd,IDC_BUTTON_NEXT),FALSE);
//	if(DocWndInfo.hMutex!=NULL)
//		DocWndInfoUnLock();
}

// Success -> TRUE   Failure -> FALSE 
static int DocWndInfoLock(void)
{
#if 0
	DWORD dwRes;
	if(DocWndInfo.hMutex==NULL)
		return FALSE;
	dwRes = WaitForSingleObject(DocWndInfo.hMutex,10000);
	if(dwRes==WAIT_OBJECT_0	|| dwRes==WAIT_ABANDONED)
		return TRUE;
	else
		return FALSE;
#else
	return TRUE;
#endif
}

static void DocWndInfoUnLock(void)
{
//	ReleaseMutex(DocWndInfo.hMutex);
}

void DocWndInfoReset(void)
{
	if(DocWndInfoLock()==FALSE)
		return;
	DocWndInfo.DocFileCur = 0;
	DocWndInfo.DocFileMax = 0;
	if(DocWndInfo.Text!=NULL){
		free(DocWndInfo.Text);
		DocWndInfo.Text = NULL;
	}
	DocWndInfo.TextSize = 0;
	DocWndSetInfo("","");
	DocWndSetText("",0);
// end:
	DocWndInfoUnLock();
}

void DocWndAddDocFile(char *filename)
{
	struct timidity_file *tf = open_file(filename,0,0);
#ifdef W32GUI_DEBUG
PrintfDebugWnd("DocWndAddDocFile <- [%s]\n",filename);
#endif
	if(tf==NULL)
		return;
	close_file(tf);
	if(DocWndInfoLock()==FALSE)
		return;
	if(DocWndInfo.DocFileMax>=DOCWND_DOCFILEMAX-1)
		goto end;
	DocWndInfo.DocFileMax++;
	strncpy(DocWndInfo.DocFile[DocWndInfo.DocFileMax-1],filename,MAXPATH);
	DocWndInfo.DocFile[DocWndInfo.DocFileMax-1][MAXPATH-1] = '\0';
	if(DocWndInfo.DocFileCur==1)
		EnableWindow(GetDlgItem(hDocWnd,IDC_BUTTON_PREV),FALSE);
	else
		EnableWindow(GetDlgItem(hDocWnd,IDC_BUTTON_PREV),TRUE);
	if(DocWndInfo.DocFileCur==DocWndInfo.DocFileMax)
		EnableWindow(GetDlgItem(hDocWnd,IDC_BUTTON_NEXT),FALSE);
	else
		EnableWindow(GetDlgItem(hDocWnd,IDC_BUTTON_NEXT),TRUE);
#ifdef W32GUI_DEBUG
PrintfDebugWnd("DocWndAddDocFile -> (%d)[%s]\n",DocWndInfo.DocFileMax-1,DocWndInfo.DocFile[DocWndInfo.DocFileMax-1]);
#endif
end:
	DocWndInfoUnLock();
}

void DocWndSetMidifile(char *filename)
{
	char buffer[MAXPATH+10];
	char *p;
	if(DocWndInfoLock()==FALSE)
		return;
	strncpy(buffer,filename,MAXPATH-1);
	buffer[MAXPATH-1] = '\0';
	p = strrchr(buffer,'.');
	if(p==NULL)
		goto end;
	*p = '\0';
	strcat(buffer,".txt");
	DocWndAddDocFile(buffer);
	*p = '\0';
	strcat(buffer,".doc");
	DocWndAddDocFile(buffer);
	*p = '\0';
	strcat(buffer,".hed");
	DocWndAddDocFile(buffer);
	p = strrchr(buffer,'#');
	if(p==NULL)
		goto end;
	*p = '\0';
	strcat(buffer,"readme.txt");
	DocWndAddDocFile(buffer);
	*p = '\0';
	strcat(buffer,"readme.1st");
	DocWndAddDocFile(buffer);
	*p = '\0';
	strcat(buffer,"歌詞.txt");
	DocWndAddDocFile(buffer);
end:
	DocWndInfoUnLock();
}

#define DOCWNDDOCSIZEMAX (64*1024)
void DocWndReadDoc(int num)
{
	struct timidity_file *tf;
	if(DocWndInfoLock()==FALSE)
		return;
	if(num<1)
		num = 1;
	if(num>DocWndInfo.DocFileMax)
		num = DocWndInfo.DocFileMax;
	if(num==DocWndInfo.DocFileCur)
		goto end;
	DocWndInfo.DocFileCur = num;
	tf = open_file(DocWndInfo.DocFile[DocWndInfo.DocFileCur-1],1,10);
	if(tf==NULL)
		goto end;
	if(DocWndInfo.Text!=NULL){
		free(DocWndInfo.Text);
		DocWndInfo.Text = NULL;
	}
	DocWndInfo.Text = (char *)safe_malloc(sizeof(char)*DOCWNDDOCSIZEMAX);
	DocWndInfo.Text[0] = '\0';
	DocWndInfo.TextSize = tf_read(DocWndInfo.Text,1,DOCWNDDOCSIZEMAX-1,tf);
	DocWndInfo.Text[DocWndInfo.TextSize] = '\0';
	close_file(tf);
	{
		char info[1024];
		char *filename;
		char *p1, *p2, *p3;
		p1 = DocWndInfo.DocFile[DocWndInfo.DocFileCur-1];
		p2 = pathsep_strrchr(p1);
		p3 = strrchr(p1,'#');
		if(p3!=NULL){
			sprintf(info,"(%02d/%02d) %s",DocWndInfo.DocFileCur,DocWndInfo.DocFileMax,p3+1);
			filename = p2 + 1;
		} else if(p2!=NULL){
			sprintf(info,"(%02d/%02d) %s",DocWndInfo.DocFileCur,DocWndInfo.DocFileMax,p2+1);
			filename = p2 + 1;
		} else {
			sprintf(info,"(%02d/%02d) %s",DocWndInfo.DocFileCur,DocWndInfo.DocFileMax,p1+1);
			filename = p1;
		}
		DocWndSetInfo(info,filename);
	}
	DocWndSetText(DocWndInfo.Text,DocWndInfo.TextSize);
end:
	if(DocWndInfo.DocFileCur==1)
		EnableWindow(GetDlgItem(hDocWnd,IDC_BUTTON_PREV),FALSE);
	else
		EnableWindow(GetDlgItem(hDocWnd,IDC_BUTTON_PREV),TRUE);
	if(DocWndInfo.DocFileCur==DocWndInfo.DocFileMax)
		EnableWindow(GetDlgItem(hDocWnd,IDC_BUTTON_NEXT),FALSE);
	else
		EnableWindow(GetDlgItem(hDocWnd,IDC_BUTTON_NEXT),TRUE);
	DocWndInfoUnLock();
}

void DocWndReadDocNext(void)
{
	int num;
	if(DocWndInfoLock()==FALSE)
		return;
	num = DocWndInfo.DocFileCur + 1;
	if(num>DocWndInfo.DocFileMax)
		num = DocWndInfo.DocFileMax;
	DocWndReadDoc(num);
	DocWndInfoUnLock();
}

void DocWndReadDocPrev(void)
{
	int num;
	if(DocWndInfoLock()==FALSE)
		return;
	num = DocWndInfo.DocFileCur - 1;
	if(num<1)
		num = 1;
	DocWndReadDoc(num);
	DocWndInfoUnLock();
}

void PutsDocWnd(char *str)
{
	HWND hwnd;
	if(!IsWindow(hDocWnd) || !DocWndFlag)
		return;
	hwnd = GetDlgItem(hDocWnd,IDC_EDIT);
	PutsEditCtlWnd(hwnd,str);
}

void PrintfDocWnd(char *fmt, ...)
{
	HWND hwnd;
	va_list ap;
	if(!IsWindow(hDocWnd) || !DocWndFlag)
		return;
	hwnd = GetDlgItem(hDocWnd,IDC_EDIT);
	va_start(ap, fmt);
	VprintfEditCtlWnd(hwnd,fmt,ap);
	va_end(ap);
}

void ClearDocWnd(void)
{
	HWND hwnd;
	if(!IsWindow(hDocWnd))
		return;
	hwnd = GetDlgItem(hDocWnd,IDC_EDIT);
	ClearEditCtlWnd(hwnd);
}

//****************************************************************************
// List Search Dialog
#define ListSearchStringMax 256
static char ListSearchString[ListSearchStringMax];

BOOL CALLBACK ListSearchWndProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam);
void InitListSearchWnd(HWND hParentWnd)
{
	strcpy(ListSearchString,"");
	if (hListSearchWnd != NULL) {
		DestroyWindow(hListSearchWnd);
		hListSearchWnd = NULL;
	}
	switch(PlayerLanguage){
	case LANGUAGE_JAPANESE:
		hListSearchWnd = CreateDialog
			(hInst,MAKEINTRESOURCE(IDD_DIALOG_ONE_LINE),hParentWnd,ListSearchWndProc);
		break;
	default:
	case LANGUAGE_ENGLISH:
		hListSearchWnd = CreateDialog
			(hInst,MAKEINTRESOURCE(IDD_DIALOG_ONE_LINE_EN),hParentWnd,ListSearchWndProc);
		break;
	}
	ShowWindow(hListSearchWnd,SW_HIDE);
	UpdateWindow(hListSearchWnd);
}

#define ListSearchStringBuffSize 1024*2

BOOL CALLBACK
ListSearchWndProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam)
{
	switch (uMess){
	case WM_INITDIALOG:
		switch(PlayerLanguage){
		case LANGUAGE_JAPANESE:
			SendMessage(hwnd,WM_SETTEXT,0,(LPARAM)"プレイリストの検索");
			SendMessage(GetDlgItem(hwnd,IDC_STATIC_HEAD),WM_SETTEXT,0,(LPARAM)"検索キーワードを入れてください。");
			SendMessage(GetDlgItem(hwnd,IDC_STATIC_TAIL),WM_SETTEXT,0,(LPARAM)"");
			SendMessage(GetDlgItem(hwnd,IDC_BUTTON_1),WM_SETTEXT,0,(LPARAM)"検索");
			SendMessage(GetDlgItem(hwnd,IDC_BUTTON_2),WM_SETTEXT,0,(LPARAM)"次を検索");
			SendMessage(GetDlgItem(hwnd,IDC_BUTTON_3),WM_SETTEXT,0,(LPARAM)"閉じる");
			break;
		default:
		case LANGUAGE_ENGLISH:
			SendMessage(hwnd,WM_SETTEXT,0,(LPARAM)"Playlist Search");
			SendMessage(GetDlgItem(hwnd,IDC_STATIC_HEAD),WM_SETTEXT,0,(LPARAM)"Enter search keyword.");
			SendMessage(GetDlgItem(hwnd,IDC_STATIC_TAIL),WM_SETTEXT,0,(LPARAM)"");
			SendMessage(GetDlgItem(hwnd,IDC_BUTTON_1),WM_SETTEXT,0,(LPARAM)"SEACH");
			SendMessage(GetDlgItem(hwnd,IDC_BUTTON_2),WM_SETTEXT,0,(LPARAM)"NEXT SEARCH");
			SendMessage(GetDlgItem(hwnd,IDC_BUTTON_3),WM_SETTEXT,0,(LPARAM)"CLOSE");
			break;
		}
		return FALSE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCLOSE:
			ShowWindow(hwnd, SW_HIDE);
			break;
		case IDC_BUTTON_1:
		case IDC_BUTTON_2:
		{
			int selected, nfiles, cursel;
			SendDlgItemMessage(hwnd,IDC_EDIT_ONE_LINE,
				WM_GETTEXT,(WPARAM)250,(LPARAM)ListSearchString);
			w32g_get_playlist_index(&selected, &nfiles, &cursel);
			if ( LOWORD(wParam) == IDC_BUTTON_2 )
				cursel++;
			if ( strlen ( ListSearchString ) > 0 ) {
				char buff[ListSearchStringBuffSize];
				for ( ; cursel < nfiles; cursel ++ ) {
					int result = SendDlgItemMessage(hListWnd,IDC_LISTBOX_PLAYLIST,
						LB_GETTEXTLEN,(WPARAM)cursel, 0 );
					if ( result < ListSearchStringBuffSize ) {
						result = SendDlgItemMessage(hListWnd,IDC_LISTBOX_PLAYLIST,
							LB_GETTEXT,(WPARAM)cursel,(LPARAM)buff);
						if ( result == LB_ERR ) {
							cursel = LB_ERR;
							break;
						}
						if ( strstr ( buff, ListSearchString ) != NULL ) {
							break;
						}
					} else if ( result == LB_ERR ) {
						cursel = LB_ERR;
						break;
					}
				}
				if ( cursel >= nfiles ) {
					cursel = LB_ERR;
				}
			} else {
				cursel = LB_ERR;
			}
			if ( cursel != LB_ERR ) {
				SendDlgItemMessage(hListWnd,IDC_LISTBOX_PLAYLIST,
					LB_SETCURSEL,(WPARAM)cursel,0);
				SetNumListWnd(cursel,nfiles);
				if ( LOWORD(wParam) == IDC_BUTTON_1 )
					HideListSearch();
			}
		}
			break;
		case IDC_BUTTON_3:
			HideListSearch();
			break;
		default:
			return FALSE;
	}
		break;
	case WM_CLOSE:
		ShowWindow(hListSearchWnd, SW_HIDE);
		break;
	default:
		return FALSE;
	}
	return FALSE;
}

void ShowListSearch(void)
{
	ShowWindow(hListSearchWnd, SW_SHOW);
}
void HideListSearch(void)
{
	ShowWindow(hListSearchWnd, SW_HIDE);
}

//****************************************************************************
// SoundSpec Window

BOOL CALLBACK SoundSpecWndProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam);
void InitSoundSpecWnd(HWND hParentWnd)
{
	if (hSoundSpecWnd != NULL) {
		DestroyWindow(hSoundSpecWnd);
		hSoundSpecWnd = NULL;
	}
	hSoundSpecWnd = CreateDialog
		(hInst,MAKEINTRESOURCE(IDD_DIALOG_SOUNDSPEC),hParentWnd,SoundSpecWndProc);
	ShowWindow(hSoundSpecWnd,SW_HIDE);
	UpdateWindow(hSoundSpecWnd);
}

BOOL CALLBACK
SoundSpecWndProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam)
{
	switch (uMess){
	case WM_INITDIALOG:
		return FALSE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCLOSE:
			ShowWindow(hwnd, SW_HIDE);
			MainWndUpdateSoundSpecButton();
			break;
		default:
			return FALSE;
		}
		case WM_SIZE:
			return FALSE;
		case WM_CLOSE:
			ShowWindow(hSoundSpecWnd, SW_HIDE);
			MainWndUpdateSoundSpecButton();
			break;
		default:
			return FALSE;
	}
	return FALSE;
}

void w32g_open_doc(int close_if_no_doc)
{
	if(close_if_no_doc==1 && DocWndInfo.DocFileMax <= 0)
		ShowSubWindow(hDocWnd, 0);
	else
	{
		DocWndReadDoc(1);
		if(close_if_no_doc!=2)
			ShowSubWindow(hDocWnd, 1);
	}
}

void w32g_setup_doc(int idx)
{
	char *filename;

	DocWndInfoReset();
	if((filename = w32g_get_playlist(idx)) == NULL)
		return;
	DocWndSetMidifile(filename);
}
