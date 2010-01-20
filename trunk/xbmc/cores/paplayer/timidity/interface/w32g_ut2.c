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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"
#include "controls.h"
#include "recache.h"
#ifdef SUPPORT_SOUNDSPEC
#include "soundspec.h"
#endif /* SUPPORT_SOUNDSPEC */
#include "wrd.h"
#include "w32g.h"
#include "w32g_ut2.h"

char *timidity_window_inifile;

// ****************************************************************************
// DlgChooseFont
// hwnd: Owner Window of This Dialog
// hwndFontChange: Window to Change Font
// hFontPre: Previous Font of hwndFontChange (Call CloseHandle())
int DlgChooseFontAndApply(HWND hwnd, HWND hwndFontChange, HFONT hFontPre, char *fontname, int *fontheight, int *fontwidth)
{
	LOGFONT lf;
	CHOOSEFONT cf;
	HFONT hFont;
	memset(&lf,0,sizeof(LOGFONT));
	memset(&cf,0,sizeof(CHOOSEFONT));

//	lf.lfHeight = 16;
//	lf.lfWidth = 8;
	strcpy(lf.lfFaceName,"ＭＳ 明朝");
    cf.lStructSize = sizeof(CHOOSEFONT);
    cf.hwndOwner = hwnd;
//    cf.hDC = NULL;
    cf.lpLogFont = &lf;
//    cf.iPointSize = 16;
//    cf.Flags = CF_ANSIONLY | CF_FORCEFONTEXIST ;
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;;
//    cf.rgbColors = RGB(0,0,0);
//    cf.lCustData = NULL;
//    cf.lpfnHook = NULL;
//    cf.lpTemplateName = NULL;
//    cf.hInstance = 0;
//    cf.lpszStyle = NULL;
    cf.nFontType = SCREEN_FONTTYPE;
//    cf.nSizeMin = 4;
//    cf.nSizeMax = 72;
	ChooseFont(&cf);

//	if(ChooseFont(&cf)==TRUE)
//		return -1;
	if(hFontPre!=NULL)
		CloseHandle(hFontPre);
	hFont = CreateFontIndirect(&lf);
	SendMessage(hwndFontChange,WM_SETFONT,(WPARAM)hFont,(LPARAM)MAKELPARAM(TRUE,0));
	if(fontname!=NULL) strcpy(fontname,lf.lfFaceName);
	if(fontheight!=NULL) *fontheight = lf.lfHeight;
	if(fontwidth!=NULL) *fontwidth = lf.lfWidth;
	return 0;
}

int DlgChooseFont(HWND hwnd, char *fontName, int *fontHeight, int *fontWidth)
{
	LOGFONT lf;
	CHOOSEFONT cf;

	memset(&lf,0,sizeof(LOGFONT));
	if(fontHeight!=NULL) lf.lfHeight = *fontHeight;
	if(fontWidth!=NULL) lf.lfWidth = *fontWidth;
	if(fontName!=NULL) strcpy(lf.lfFaceName,fontName);

	memset(&cf,0,sizeof(CHOOSEFONT));
    cf.lStructSize = sizeof(CHOOSEFONT);
    cf.hwndOwner = hwnd;
//    cf.hDC = NULL;
    cf.lpLogFont = &lf;
//    cf.iPointSize = 16;
//    cf.Flags = CF_ANSIONLY | CF_FORCEFONTEXIST ;
    cf.Flags = CF_ANSIONLY | CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
//    cf.rgbColors = RGB(0,0,0);
//    cf.lCustData = NULL;
//    cf.lpfnHook = NULL;
//    cf.lpTemplateName = NULL;
//    cf.hInstance = 0;
//    cf.lpszStyle = NULL;
    cf.nFontType = SCREEN_FONTTYPE;
//    cf.nSizeMin = 4;
//    cf.nSizeMax = 72;
	if(ChooseFont(&cf)!=TRUE)
		return -1;

	if(fontName!=NULL) strcpy(fontName,lf.lfFaceName);
	if(fontHeight!=NULL) *fontHeight = abs(lf.lfHeight);
	if(fontWidth!=NULL) *fontWidth = lf.lfWidth;
	return 0;
}

/**********************************************************************/
void SetWindowPosSize ( HWND parent_hwnd, HWND hwnd, int x, int y )
{
	RECT rc1, rc2;
	int width1, height1;
	int width2, height2;
	if ( GetWindowRect ( hwnd, &rc1 ) ) {
		width1 = rc1.right - rc1.left;
		height1 = rc1.bottom - rc1.top;
		if ( x >= 0 ) {
			rc1.right = rc1.right - rc1.left + x;
			rc1.left = x;
		} else {
//			rc1.right = rc1.right - rc1.left;
//			rc1.left = 0;
		}
		if ( y >= 0 ) {
			rc1.bottom = rc1.bottom - rc1.top + y;
			rc1.top = y;
		} else {
//			rc1.bottom = rc1.bottom - rc1.top;
//			rc1.top = 0;
		}
		if ( GetClientRect ( parent_hwnd, &rc2 ) ) {
			width2 = rc2.right - rc2.left;
			height2 = rc2.bottom - rc2.top;
			if ( rc1.left < rc2.left ) rc1.left = rc2.left;
			if ( rc1.left > rc2.right ) rc1.left = rc2.right;
			if ( rc1.top < rc2.top ) rc1.top = rc2.top;
			if ( rc1.top > rc2.bottom ) rc1.top = rc2.bottom;
			if ( width1 > width2 ) width1 = width2;
			if ( height1 > height2 ) height1 = height2;
			MoveWindow ( hwnd, rc1.left, rc1.top, width1, height1, TRUE );
		}
	}
}

/**********************************************************************/
BOOL PosSizeSave = TRUE;

#define SEC_MAINWND "MainWnd"
int INISaveMainWnd(void)
{
	char *section = SEC_MAINWND;
	char *inifile = TIMIDITY_WINDOW_INI_FILE;
	char buffer[256];
	if ( PosSizeSave ) {
		sprintf(buffer,"%d",MainWndInfo.PosX);
		if ( MainWndInfo.PosX >= 0 || MainWndInfo.PosY >= 0 ) {
			if ( MainWndInfo.PosX < 0 )
				MainWndInfo.PosX = 0;
			if ( MainWndInfo.PosY < 0 )
				MainWndInfo.PosY = 0;
		}
		if ( MainWndInfo.PosX >= 0 )
		WritePrivateProfileString(section,"PosX",buffer,inifile);
		sprintf(buffer,"%d",MainWndInfo.PosY);
		if ( MainWndInfo.PosY >= 0 )
		WritePrivateProfileString(section,"PosY",buffer,inifile);
	}
	sprintf(buffer,"%d",MainWndInfo.CanvasMode);
	WritePrivateProfileString(section,"CanvasMode",buffer,inifile);
	WritePrivateProfileString(NULL,NULL,NULL,inifile);		// Write Flush
	return 0;
}

int INILoadMainWnd(void)
{
	char *section = SEC_MAINWND;
	char *inifile = TIMIDITY_WINDOW_INI_FILE;
	int num;
	num = GetPrivateProfileInt(section,"PosX",-1,inifile);
	MainWndInfo.PosX = num;
	num = GetPrivateProfileInt(section,"PosY",-1,inifile);
	MainWndInfo.PosY = num;
	num = GetPrivateProfileInt(section,"CanvasMode",-1,inifile);
	MainWndInfo.CanvasMode = num;
	return 0;
}

#define SEC_LISTWND "ListWnd"
int INISaveListWnd(void)
{
	char *section = SEC_LISTWND;
	char *inifile = TIMIDITY_WINDOW_INI_FILE;
	char buffer[256];
	if ( PosSizeSave ) {
		if ( ListWndInfo.PosX >= 0 || ListWndInfo.PosY >= 0 ) {
			if ( ListWndInfo.PosX < 0 )
				ListWndInfo.PosX = 0;
			if ( ListWndInfo.PosY < 0 )
				ListWndInfo.PosY = 0;
		}
		sprintf(buffer,"%d",ListWndInfo.PosX);
		if ( ListWndInfo.PosX >= 0 )
		WritePrivateProfileString(section,"PosX",buffer,inifile);
		sprintf(buffer,"%d",ListWndInfo.PosY);
		if ( ListWndInfo.PosY >= 0 )
		WritePrivateProfileString(section,"PosY",buffer,inifile);
		sprintf(buffer,"%d",ListWndInfo.Width);
		WritePrivateProfileString(section,"Width",buffer,inifile);
		sprintf(buffer,"%d",ListWndInfo.Height);
		WritePrivateProfileString(section,"Height",buffer,inifile);
	}
	WritePrivateProfileString(section,"fontNameEN",ListWndInfo.fontNameEN,inifile);
	WritePrivateProfileString(section,"fontNameJA",ListWndInfo.fontNameJA,inifile);
	sprintf(buffer,"%d",ListWndInfo.fontWidth);
	WritePrivateProfileString(section,"fontWidth",buffer,inifile);
	sprintf(buffer,"%d",ListWndInfo.fontHeight);
	WritePrivateProfileString(section,"fontHeight",buffer,inifile);
	sprintf(buffer,"%d",ListWndInfo.fontFlags);
	WritePrivateProfileString(section,"fontFlags",buffer,inifile);
	WritePrivateProfileString(NULL,NULL,NULL,inifile);		// Write Flush
	return 0;
}

int INILoadListWnd(void)
{
	char *section = SEC_LISTWND;
	char *inifile = TIMIDITY_WINDOW_INI_FILE;
	int num;
	char buffer[64];
	num = GetPrivateProfileInt(section,"PosX",-1,inifile);
	ListWndInfo.PosX = num;
	num = GetPrivateProfileInt(section,"PosY",-1,inifile);
	ListWndInfo.PosY = num;
	num = GetPrivateProfileInt(section,"Width",-1,inifile);
	if(num!=-1) ListWndInfo.Width = num;
	num = GetPrivateProfileInt(section,"Height",-1,inifile);
	if(num!=-1) ListWndInfo.Height = num;
	GetPrivateProfileString(section,"fontNameEN","",buffer,32,inifile);
	if(buffer[0]!=0) strcpy(ListWndInfo.fontNameEN,buffer);
	GetPrivateProfileString(section,"fontNameJA","",buffer,32,inifile);
	if(buffer[0]!=0) strcpy(ListWndInfo.fontNameJA,buffer);
	num = GetPrivateProfileInt(section,"fontWidth",-1,inifile);
	if(num!=-1) ListWndInfo.fontWidth = num;
	num = GetPrivateProfileInt(section,"fontHeight",-1,inifile);
	if(num!=-1) ListWndInfo.fontHeight = num;
	num = GetPrivateProfileInt(section,"fontFlags",-1,inifile);
	if(num!=-1) ListWndInfo.fontFlags = num;
	return 0;
}

#define SEC_DOCWND "DocWnd"
int INISaveDocWnd(void)
{
	char *section = SEC_DOCWND;
	char *inifile = TIMIDITY_WINDOW_INI_FILE;
	char buffer[256];
	if ( PosSizeSave ) {
		if ( DocWndInfo.PosX >= 0 || DocWndInfo.PosY >= 0 ) {
			if ( DocWndInfo.PosX < 0 )
				DocWndInfo.PosX = 0;
			if ( DocWndInfo.PosY < 0 )
				DocWndInfo.PosY = 0;
		}
		sprintf(buffer,"%d",DocWndInfo.PosX);
		if ( DocWndInfo.PosX >= 0 )
		WritePrivateProfileString(section,"PosX",buffer,inifile);
		sprintf(buffer,"%d",DocWndInfo.PosY);
		if ( DocWndInfo.PosY >= 0 )
		WritePrivateProfileString(section,"PosY",buffer,inifile);
		sprintf(buffer,"%d",DocWndInfo.Width);
		WritePrivateProfileString(section,"Width",buffer,inifile);
		sprintf(buffer,"%d",DocWndInfo.Height);
		WritePrivateProfileString(section,"Height",buffer,inifile);
	}
	WritePrivateProfileString(section,"fontNameEN",DocWndInfo.fontNameEN,inifile);
	WritePrivateProfileString(section,"fontNameJA",DocWndInfo.fontNameJA,inifile);
	sprintf(buffer,"%d",DocWndInfo.fontWidth);
	WritePrivateProfileString(section,"fontWidth",buffer,inifile);
	sprintf(buffer,"%d",DocWndInfo.fontHeight);
	WritePrivateProfileString(section,"fontHeight",buffer,inifile);
	sprintf(buffer,"%d",DocWndInfo.fontFlags);
	WritePrivateProfileString(section,"fontFlags",buffer,inifile);
	WritePrivateProfileString(NULL,NULL,NULL,inifile);		// Write Flush
	return 0;
}

int INILoadDocWnd(void)
{
	char *section = SEC_DOCWND;
	char *inifile = TIMIDITY_WINDOW_INI_FILE;
	int num;
	char buffer[64];
	num = GetPrivateProfileInt(section,"PosX",-1,inifile);
	DocWndInfo.PosX = num;
	num = GetPrivateProfileInt(section,"PosY",-1,inifile);
	DocWndInfo.PosY = num;
	num = GetPrivateProfileInt(section,"Width",-1,inifile);
	if(num!=-1) DocWndInfo.Width = num;
	num = GetPrivateProfileInt(section,"Height",-1,inifile);
	if(num!=-1) DocWndInfo.Height = num;
	GetPrivateProfileString(section,"fontNameEN","",buffer,32,inifile);
	if(buffer[0]!=0) strcpy(DocWndInfo.fontNameEN,buffer);
	GetPrivateProfileString(section,"fontNameJA","",buffer,32,inifile);
	if(buffer[0]!=0) strcpy(DocWndInfo.fontNameJA,buffer);
	num = GetPrivateProfileInt(section,"fontWidth",-1,inifile);
	if(num!=-1) DocWndInfo.fontWidth = num;
	num = GetPrivateProfileInt(section,"fontHeight",-1,inifile);
	if(num!=-1) DocWndInfo.fontHeight = num;
	num = GetPrivateProfileInt(section,"fontFlags",-1,inifile);
	if(num!=-1) DocWndInfo.fontFlags = num;
	return 0;
}

#define SEC_CONSOLEWND "ConsoleWnd"
int INISaveConsoleWnd(void)
{
	char *section = SEC_CONSOLEWND;
	char *inifile = TIMIDITY_WINDOW_INI_FILE;
	char buffer[256];
	if ( PosSizeSave ) {
		if ( ConsoleWndInfo.PosX >= 0 || ConsoleWndInfo.PosY >= 0 ) {
			if ( ConsoleWndInfo.PosX < 0 )
				ConsoleWndInfo.PosX = 0;
			if ( ConsoleWndInfo.PosY < 0 )
				ConsoleWndInfo.PosY = 0;
		}
		sprintf(buffer,"%d",ConsoleWndInfo.PosX);
		if ( ConsoleWndInfo.PosX >= 0 )
		WritePrivateProfileString(section,"PosX",buffer,inifile);
		sprintf(buffer,"%d",ConsoleWndInfo.PosY);
		if ( ConsoleWndInfo.PosY >= 0 )
		WritePrivateProfileString(section,"PosY",buffer,inifile);
	}
	WritePrivateProfileString(NULL,NULL,NULL,inifile);		// Write Flush
	return 0;
}

int INILoadConsoleWnd(void)
{
	char *section = SEC_CONSOLEWND;
	char *inifile = TIMIDITY_WINDOW_INI_FILE;
	int num;
	num = GetPrivateProfileInt(section,"PosX",-1,inifile);
	ConsoleWndInfo.PosX = num;
	num = GetPrivateProfileInt(section,"PosY",-1,inifile);
	ConsoleWndInfo.PosY = num;
	return 0;
}


/**********************************************************************/
// プロセス間通信用にメールスロットのサーバースレッドを用意する

#define TIMIDITY_MAILSLOT "\\\\.\\mailslot\\timiditypp_mailslot_ver_1_0"

// メールスロットに渡される形式
// ヘッダ
// コマンド名
// オプション数
// オプション１
// オプション２
//   ...

// ヘッダ
#define MC_HEADER	"TiMidity++Win32GUI Mailslot-1.0"	
// コマンド名
// TiMidity の終了
#define MC_TERMINATE	"Terminate"
// ファイルを指定
#define MC_FILES "Files Argc Argv"
// オプション１ : ファイル名１
//   ...
// プレイリストのクリア
#define MC_PLAYLIST_CLEAR	"Playlist Clear"
// 演奏開始
#define MC_PLAY			"Play"
// 次のファイルの演奏
#define MC_PLAY_NEXT	"Play Next"
// 前のファイルの演奏
#define MC_PLAY_PREV	"Play Prev"
// 演奏停止
#define MC_STOP	"Stop"
// 演奏一時停止
#define MC_PAUSE	"Pause"
// TiMidity の状態を指定メールスロットに送信
#define MC_SEND_TIMIDITY_INFO	"Send TiMidity Info"
// オプション１ : メールスロット名
// オプション２ : 状態１
//   ...
// 状態
// "PlayFileName:～" : 演奏ファイル名
// "PlayTile:～"		: 演奏タイトル名
// "PlayStatus:～"		: 演奏状態(～:PLAY,STOP,PAUSE)

static HANDLE hMailslot = NULL;

void w32gMailslotThread(void);

int w32gStartMailslotThread(void)
{
	DWORD dwThreadID;
	HANDLE hThread;
	hThread = (HANDLE)crt_beginthreadex(NULL,0,(LPTHREAD_START_ROUTINE)w32gMailslotThread,NULL,0,&dwThreadID);
	if((unsigned long)hThread==-1){
		return FALSE;	// Error!
	}
	return TRUE;
}

int ReadFromMailslot(HANDLE hmailslot, char *buffer, int *size)
{
	DWORD dwMessageSize, dwMessageNum, dwMessageReadSize;
	BOOL bRes;
	int i;
	bRes = GetMailslotInfo(hmailslot,NULL,&dwMessageSize,&dwMessageNum,(LPDWORD)NULL);
	if(bRes==FALSE || dwMessageSize==MAILSLOT_NO_MESSAGE)
		return FALSE;
	for(i=0;i<10;i++){
		bRes = ReadFile(hMailslot,buffer,dwMessageSize,&dwMessageReadSize,(LPOVERLAPPED)NULL);
#ifdef W32GUI_DEBUG
PrintfDebugWnd("[%s]\n",buffer);
#endif
		if(bRes==TRUE){
			break;
		}
		Sleep(300);
	}
	if(bRes==TRUE){
		*size = (int)dwMessageSize;
		return TRUE;
	} else
		return FALSE;
}
// 無視する版
void ReadFromMailslotIgnore(HANDLE hmailslot, int num)
{
	int i;
	char buffer[10240];
	int size;
	for(i=0;i<num;i++){
		if(ReadFromMailslot(hmailslot,buffer,&size)==FALSE)
			return;
	}
	return;
}
// メールスロットに書き込む
HANDLE *OpenMailslot(void)
{
	HANDLE hFile;
	hFile = CreateFile(TIMIDITY_MAILSLOT,GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,(HANDLE) NULL);
	if(hFile==INVALID_HANDLE_VALUE)
		return NULL;
	else
		return hFile;
}
void CloseMailslot(HANDLE hmailslot)
{
	CloseHandle(hmailslot);
}
int WriteMailslot(HANDLE hmailslot, char *buffer, int size)
{
	DWORD dwWrittenSize;
	BOOL bRes;
	bRes = WriteFile(hmailslot,buffer,(DWORD)lstrlen(buffer) + 1,&dwWrittenSize,(LPOVERLAPPED)NULL);
	if(bRes==FALSE){
		return FALSE;
	}
	return TRUE;
}

int isURLFile(char *filename);
extern volatile DWORD dwWindowThreadID;
volatile argc_argv_t MailslotArgcArgv;
volatile int MailslotThreadTeminateFlag = FALSE; 
void w32gMailslotThread(void)
{
	int i;
	char buffer[1024];
	int size;
	MailslotArgcArgv.argc = 0;
	MailslotArgcArgv.argv = NULL;
	for(i=0;i<10;i++){
		hMailslot = CreateMailslot(TIMIDITY_MAILSLOT,0,MAILSLOT_WAIT_FOREVER,(LPSECURITY_ATTRIBUTES)NULL);
		if (hMailslot != INVALID_HANDLE_VALUE) {
			break;
		}
		hMailslot = NULL;
		Sleep(300);
	}
	if(hMailslot==NULL){
		return;
	}
	for(;;){
		Sleep(1000);
		if(MailslotThreadTeminateFlag==TRUE){
			if(hMailslot!=NULL)
				CloseHandle(hMailslot);
			break;
		}
		for(;;){
			Sleep(200);
			if(ReadFromMailslot(hMailslot,buffer,&size)==FALSE){
				Sleep(1000);
				continue;
			}
			if(strcasecmp(buffer,MC_HEADER)!=0){
				continue;
			}
			if(ReadFromMailslot(hMailslot,buffer,&size)==FALSE){
				Sleep(1000);
				continue;
			}
			if(strcasecmp(buffer,MC_TERMINATE)==0){
				CloseHandle(hMailslot);
			    w32g_send_rc(RC_STOP, 0);
			    w32g_send_rc(RC_QUIT, 0);
//				PostThreadMessage(dwWindowThreadID,WM_CLOSE,0,0);
//				PostThreadMessage(dwWindowThreadID,WM_QUIT,0,0);
				Sleep(500);
				return;
			}
			if(strcasecmp(buffer,MC_FILES)==0){
				char **files;
				int nfiles;
				int flag = TRUE;
				if(ReadFromMailslot(hMailslot,buffer,&size)==FALSE){
					continue;
				}
				nfiles = atoi(buffer);
				// MailslotArgcArgv　が初期化されていなかったら処理途中として無視
				if(MailslotArgcArgv.argc!=0 || MailslotArgcArgv.argv!=NULL){
					ReadFromMailslotIgnore(hMailslot,nfiles);
					continue;
				}
				files = (char **)malloc(sizeof(char *)*nfiles);
				if(files==NULL){
					ReadFromMailslotIgnore(hMailslot,nfiles);
					continue;
				}
				for(i=0;i<nfiles;i++){
					if(ReadFromMailslot(hMailslot,buffer,&size)==FALSE){
						flag = FALSE;
						break;
					}
					files[i] = (char *)malloc(sizeof(char)*(size+1));
					if(files[i]==NULL){
						int j;
						ReadFromMailslotIgnore(hMailslot,nfiles-i-1);
						for(j=0;j<i;j++){
							free(files[j]);
						}
						flag = FALSE;
						break;
					}
					strncpy(files[i],buffer,size);
					files[i][size] = 0;
				}
				if(flag==FALSE){
					free(files);
					continue;
				}
				MailslotArgcArgv.argc = nfiles;
				MailslotArgcArgv.argv = files;
				// files は別のところで解放してくれる
				w32g_send_rc(RC_EXT_LOAD_FILES_AND_PLAY,(int32)&MailslotArgcArgv);
//				w32g_send_rc(RC_EXT_LOAD_FILE,(int32)files[0]);
				continue;
			}
			if(strcasecmp(buffer,MC_PLAYLIST_CLEAR)==0){
				int param_num;
				if(ReadFromMailslot(hMailslot,buffer,&size)==FALSE){
					continue;
				}
				param_num = atoi(buffer);
				w32g_send_rc(RC_EXT_CLEAR_PLAYLIST,0);
				ReadFromMailslotIgnore(hMailslot,param_num);
				continue;
			}
			if(strcasecmp(buffer,MC_PLAY)==0){
				int param_num;
				if(ReadFromMailslot(hMailslot,buffer,&size)==FALSE){
					continue;
				}
				param_num = atoi(buffer);
				w32g_send_rc(RC_LOAD_FILE,0);
				ReadFromMailslotIgnore(hMailslot,param_num);
				continue;
			}
			if(strcasecmp(buffer,MC_PLAY_NEXT)==0){
				int param_num;
				if(ReadFromMailslot(hMailslot,buffer,&size)==FALSE){
					continue;
				}
				param_num = atoi(buffer);
				w32g_send_rc(RC_NEXT,0);
				ReadFromMailslotIgnore(hMailslot,param_num);
				continue;
			}
			if(strcasecmp(buffer,MC_PLAY_PREV)==0){
				int param_num;
				if(ReadFromMailslot(hMailslot,buffer,&size)==FALSE){
					continue;
				}
				param_num = atoi(buffer);
				w32g_send_rc(RC_REALLY_PREVIOUS,0);
				ReadFromMailslotIgnore(hMailslot,param_num);
				continue;
			}
			if(strcasecmp(buffer,MC_STOP)==0){
				int param_num;
				if(ReadFromMailslot(hMailslot,buffer,&size)==FALSE){
					continue;
				}
				param_num = atoi(buffer);
				w32g_send_rc(RC_STOP,0);
				ReadFromMailslotIgnore(hMailslot,param_num);
				continue;
			}
			if(strcasecmp(buffer,MC_PAUSE)==0){
				int param_num;
				if(ReadFromMailslot(hMailslot,buffer,&size)==FALSE){
					continue;
				}
				param_num = atoi(buffer);
				w32g_send_rc(RC_PAUSE,0);
				ReadFromMailslotIgnore(hMailslot,param_num);
				continue;
			}
			if(strcasecmp(buffer,MC_SEND_TIMIDITY_INFO)==0){
				int param_num;
				if(ReadFromMailslot(hMailslot,buffer,&size)==FALSE){
					continue;
				}
				param_num = atoi(buffer);
				ReadFromMailslotIgnore(hMailslot,param_num);
				// 何もしない
				continue;
			}
		}
	}
}

#define TIMIDTY_MUTEX_NAME "TiMidity_pp_Win32GUI_ver_1_0_0"
static HANDLE hMutexTiMidity = NULL;
// TiMidity が唯一なることを主張します
// その証拠の Mutex を hMutexTiMidity に保持します
int UniqTiMidity(void)
{
	hMutexTiMidity = CreateMutex(NULL,TRUE,TIMIDTY_MUTEX_NAME);
	if(hMutexTiMidity!=NULL && GetLastError()==0){
		return TRUE;
	}
	if(GetLastError()==ERROR_ALREADY_EXISTS){
		;
	}
	if(hMutexTiMidity!=NULL){
		CloseHandle(hMutexTiMidity);
	}
	hMutexTiMidity = NULL;
	return FALSE;
}

// すでに TiMidity が存在するか
int ExistOldTiMidity(void)
{
	HANDLE hMutex = CreateMutex(NULL,TRUE,TIMIDTY_MUTEX_NAME);
	if(GetLastError()==ERROR_ALREADY_EXISTS){
		if(hMutex!=NULL)
	CloseHandle(hMutex);
	return TRUE;
	}
	if(hMutex!=NULL)
		CloseHandle(hMutex);
	return FALSE;
}

// 何回か唯一の TiMidity になろうとします
int TryUniqTiMidity(int num)
{
	int i;
	for(i=0;i<num;i++){
		if(UniqTiMidity()==TRUE){
			return TRUE;
		}
		Sleep(1000);
	}
	return FALSE;
}

int SendFilesToOldTiMidity(int nfiles, char **files)
{
	int i;
	HANDLE hmailslot;
	char buffer[1024];
	int size;
	hmailslot = OpenMailslot();
	if(hmailslot==NULL)
		return FALSE;
	strcpy(buffer,MC_HEADER);
	size = strlen(buffer); WriteMailslot(hmailslot,buffer,size);
	strcpy(buffer,MC_FILES);
	size = strlen(buffer); WriteMailslot(hmailslot,buffer,size);
	sprintf(buffer,"%d",nfiles);
	size = strlen(buffer); WriteMailslot(hmailslot,buffer,size);
	for(i=0;i<nfiles;i++){
		char filepath[1024];
		char *p;
//		if(url_check_type(files[i])==-1 && GetFullPathName(files[i],1000,filepath,&p)!=0){
		if(isURLFile(files[i])==FALSE && GetFullPathName(files[i],1000,filepath,&p)!=0){
			size = strlen(filepath); WriteMailslot(hmailslot,filepath,size);
		} else {
			size = strlen(files[i]); WriteMailslot(hmailslot,files[i],size);
		}
	}
	CloseMailslot(hmailslot);
	return TRUE;
}

int SendCommandNoParamOldTiMidity(char *command)
{
	HANDLE hmailslot;
	char buffer[1024];
	int size;
	hmailslot = OpenMailslot();
	if(hmailslot==NULL)
		return FALSE;
	strcpy(buffer,MC_HEADER);
	size = strlen(buffer); WriteMailslot(hmailslot,buffer,size);
	strcpy(buffer,command);
	size = strlen(buffer); WriteMailslot(hmailslot,buffer,size);
	strcpy(buffer,"0");
	size = strlen(buffer); WriteMailslot(hmailslot,buffer,size);
	CloseMailslot(hmailslot);
	return TRUE;
}

int TerminateOldTiMidity(void)
{
	return  SendCommandNoParamOldTiMidity(MC_TERMINATE);
}
int ClearPlaylistOldTiMidity(void)
{
	return  SendCommandNoParamOldTiMidity(MC_PLAYLIST_CLEAR);
}
int PlayOldTiMidity(void)
{
	return  SendCommandNoParamOldTiMidity(MC_PLAY);
}

int PlayNextOldTiMidity(void)
{
	return  SendCommandNoParamOldTiMidity(MC_PLAY_NEXT);
}
int PlayPrevOldTiMidity(void)
{
	return  SendCommandNoParamOldTiMidity(MC_PLAY_PREV);
}
int StopOldTiMidity(void)
{
	return  SendCommandNoParamOldTiMidity(MC_STOP);
}
int PauseOldTiMidity(void)
{
	return  SendCommandNoParamOldTiMidity(MC_PAUSE);
}

// ２重起動時の処理
// opt==0 : ファイルを古い TiMidity に渡して自分は終了。古い TiMidity がないときは自分が起動。
//                古いプレイリストはクリアする。
// opt==1 : ファイルを古い TiMidity に渡して自分は終了。古い TiMidity がないときは自分が起動。
//               古いプレイリストはクリアしない。
// opt==2 : 古い TiMidity を終了して、自分が演奏する
// opt==3 : 自分は何もせず終了
// opt==4 : 古い TiMidity を終了して、自分は何もせず終了
// opt==5 : ２重に起動する
// 自分が終了するべきときは FALSE を返す
// 自分が終了するべきでないときは TRUE を返す
int w32gSecondTiMidity(int opt, int argc, char **argv)
{
	int i;
	switch(opt){
	case 0:
	case 1:
		if(ExistOldTiMidity()==TRUE){
			if(opt==0)
				ClearPlaylistOldTiMidity();
			SendFilesToOldTiMidity(argc > 0 ? argc-1 : 0, argv+1);
			return FALSE;
		} else {
			if(TryUniqTiMidity(20)==TRUE){
				w32gStartMailslotThread();
				return TRUE;
			}
			return FALSE;
		}
	case 2:
		if(ExistOldTiMidity()==TRUE){
			for(i=0;i<=20;i++){
				TerminateOldTiMidity();
				if(UniqTiMidity()==TRUE){
					w32gStartMailslotThread();
					return TRUE;
				}
				Sleep(1000);
			}
		} else {
			if(TryUniqTiMidity(20)==TRUE){
				w32gStartMailslotThread();
				return TRUE;
			}
		}
		return FALSE;
	case 3:
		return FALSE;
	case 4:
		if(ExistOldTiMidity()==TRUE){
			for(i=0;i<=20;i++){
				TerminateOldTiMidity();
				if(ExistOldTiMidity()==FALSE){
					return FALSE;
				}
				Sleep(1000);
			}
		}
		return FALSE;
	case 5:
		return TRUE;
	default:
		return FALSE;
	}
}

// w32gSecondTiMidity() の後処理
int w32gSecondTiMidityExit(void)
{
	MailslotThreadTeminateFlag = TRUE;
	Sleep(300);
	if(hMailslot!=NULL)
		CloseHandle(hMailslot);
	ReleaseMutex(hMutexTiMidity);
	CloseHandle(hMutexTiMidity);
	return 0;
}

// Before it call timidity_start_initialize()
int isURLFile(char *filename)
{
	if(strncasecmp(filename,"http://",7)==0
		|| strncasecmp(filename,"ftp://",6)==0
		|| strncasecmp(filename,"news://",7)==0
		|| strncasecmp(filename,"file:",5)==0
		|| strncasecmp(filename,"dir:",4)==0){
		return TRUE;
	} else {
		return FALSE;
	}
}
