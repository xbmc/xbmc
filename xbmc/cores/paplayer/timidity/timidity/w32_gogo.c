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

    w32_gogo.c

    Functions to use gogo.dll for mp3 gogo (Windows 95/98/NT).

    Orignal source : stub.c by ＰＥＮ＠ＭａｒｉｎｅＣａｔ and へるみ.

    Modified by Daisuke Aoki <dai@y7.net>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "interface.h"

#ifdef AU_GOGO_DLL

#include <stdio.h>

#ifdef __W32__
#include <stdlib.h>
#include <io.h>
#include <windows.h>
#include <process.h>
#include <windowsx.h>
#include <winuser.h>
#include <stdio.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <fcntl.h>

/* #include <musenc.h>		/* for gogo */
#include <gogo/gogo.h>		/* for gogo */

#include "w32_gogo.h"

#include "gogo_a.h"

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"

static	HINSTANCE	hModule = NULL;
typedef MERET (*me_init)(void);
typedef MERET (*me_setconf)(UPARAM mode, UPARAM dwPara1, UPARAM dwPara2 );
typedef MERET (*me_getconf)(UPARAM mode, void *para1 );
typedef MERET (*me_detect)();
typedef MERET (*me_procframe)();
typedef MERET (*me_close)();
typedef MERET (*me_end)();
typedef MERET (*me_getver)( unsigned long *vercode,  char *verstring );
typedef MERET (*me_haveunit)( unsigned long *unit );

static	me_init		mpge_init = NULL;
static  me_setconf	mpge_setconf = NULL;
static	me_getconf	mpge_getconf = NULL;
static	me_detect	mpge_detector = NULL;
static	me_procframe mpge_processframe = NULL;
static	me_close	mpge_close = NULL;
static	me_end		mpge_end = NULL;
static	me_getver	mpge_getver = NULL;
static	me_haveunit mpge_haveunit = NULL;

int MPGE_available = 0;

// DLLの読み込み(最初の1回目のみ)とワークエリアの初期化を行います。
MERET	MPGE_initializeWork(void)
{
	if( hModule == NULL ){
		// (DLLが読み込まれていない場合)
		// カレントディレクトリ、及びsystemディレクトリのGOGO.DLLの読み込み
		hModule = LoadLibrary("gogo.dll");
		if( hModule == NULL ){			// DLLが見つからない場合
			#define Key		HKEY_CURRENT_USER
			#define SubKey "Software\\MarineCat\\GOGO_DLL"
			HKEY	hKey;
			DWORD	dwType, dwKeySize;
			LONG	lResult;
			static	char	*szName = "INSTPATH";
			char	szPathName[ _MAX_PATH + 8];
			dwKeySize = sizeof( szPathName );

			// レジストリ項目の HEY_CURENT_USER\Software\MarineCat\GOGO_DLLキー以下の 
			// INSTPATH (REG_SZ)を取得します。
			if( RegOpenKeyEx(
					Key,
					SubKey,
					0,
					KEY_ALL_ACCESS,
					&hKey ) == ERROR_SUCCESS 
			){
				lResult = RegQueryValueEx(
					hKey,
					szName,
					0,
					&dwType,
					(BYTE *)szPathName,
					&dwKeySize);
				RegCloseKey(hKey);
				if( lResult == ERROR_SUCCESS && REG_SZ == dwType ){
					// レジストリから取得したパスで再度DLLの読み込みを試みる
					hModule = LoadLibrary( szPathName );
				}
			}
			#undef Key
			#undef SubKey
		}
		// DLLが見つからない
		if( hModule == NULL ){
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "can not find gogo.dll.");
			return ME_INTERNALERROR;
//			MessageBox( "DLLの読み込みを失敗しました。\nDLLをEXEファイルと同じディレクトリへ複写してください\n");
//			fprintf( stderr,"DLLの読み込みを失敗しました。\nDLLをEXEファイルと同じディレクトリへ複写してください\n");
//			exit( -1 );
		}
		
		// エクスポート関数の取得
		mpge_init = (me_init )GetProcAddress( hModule, "MPGE_initializeWork" );
		mpge_setconf = (me_setconf )GetProcAddress( hModule, "MPGE_setConfigure" );
		mpge_getconf = (me_getconf )GetProcAddress( hModule, "MPGE_getConfigure" );
		mpge_detector = (me_detect )GetProcAddress( hModule, "MPGE_detectConfigure" );
		mpge_processframe = (me_procframe )GetProcAddress( hModule, "MPGE_processFrame" );
		mpge_close   = (me_close )GetProcAddress( hModule, "MPGE_closeCoder" );
		mpge_end	 = (me_end )GetProcAddress( hModule, "MPGE_endCoder" );
		mpge_getver	 = (me_getver )GetProcAddress( hModule, "MPGE_getVersion" );
		mpge_haveunit= (me_haveunit )GetProcAddress( hModule, "MPGE_getUnitStates" );
	}

	// すべての関数が正常か確認する
	if( mpge_init && mpge_setconf && mpge_getconf &&
		mpge_detector && mpge_processframe && mpge_end && mpge_getver && mpge_haveunit ){
		MPGE_available = 1;
		return (mpge_init)();
	}

	// エラー
	//fprintf( stderr, "DLLの内容を正しく識別することが出来ませんでした\n");
	FreeLibrary( hModule );
	hModule = NULL;
	MPGE_available = 0;
	//exit( -1 );

	return ME_NOERR;
}

MERET	MPGE_terminateWork(void)	// 強制終了
{
	mpge_init = NULL;
	mpge_setconf = NULL;
	mpge_getconf = NULL;
	mpge_detector = NULL;
	mpge_processframe = NULL;
	mpge_close   = NULL;
	mpge_end	 = NULL;
	mpge_getver	 = NULL;
	mpge_haveunit= NULL;
	if(hModule)
		FreeLibrary( hModule );
	hModule = NULL;
	MPGE_available = 0;

	return ME_NOERR;
}


MERET	MPGE_setConfigure(UPARAM mode, UPARAM dwPara1, UPARAM dwPara2 )
{
	if(!mpge_setconf)
		return ME_INTERNALERROR;
	return (mpge_setconf)( mode, dwPara1, dwPara2 );
}

MERET	MPGE_getConfigure(UPARAM mode, void *para1 )
{
	if(!mpge_getconf)
		return ME_INTERNALERROR;
	return (mpge_getconf)( mode, para1 );
}

MERET	MPGE_detectConfigure(void)
{
	if(!mpge_detector)
		return ME_INTERNALERROR;
	return (mpge_detector)();
}

MERET	MPGE_processFrame(void)
{
	if(!mpge_processframe)
		return ME_INTERNALERROR;
	return (mpge_processframe)();
}

MERET	MPGE_closeCoder(void)
{
	if(!mpge_close)
		return ME_INTERNALERROR;
	return (mpge_close)();
}

MERET	MPGE_endCoder(void)
{
	MERET val;
	if(!mpge_end)
		return ME_INTERNALERROR;
	val = (mpge_end)();
	if( val == ME_NOERR ){
		mpge_setconf = NULL;
		mpge_getconf = NULL;
		mpge_detector = NULL;
		mpge_processframe = NULL;
		mpge_close   = NULL;
		mpge_end	 = NULL;
		mpge_getver	 = NULL;
		mpge_haveunit= NULL;
		FreeLibrary( hModule );		// DLL開放
		hModule = NULL;
		MPGE_available = 0;
	}
	return val;
}

MERET	MPGE_getVersion( unsigned long *vercode,  char *verstring )
{
	if(!mpge_getver)
		return ME_INTERNALERROR;
	return (mpge_getver)( vercode, verstring );
}

MERET	MPGE_getUnitStates( unsigned long *unit)
{
	if(!mpge_haveunit)
		return ME_INTERNALERROR;
	return	(mpge_haveunit)( unit );
}

int gogo_dll_check(void)
{
	HANDLE hDLL = NULL;
	if(hModule)
		return 1;
	hDLL = LoadLibrary("gogo.dll");
	if(hDLL){
		FreeLibrary(hDLL);
		return 1;
	} else {
#define Key		HKEY_CURRENT_USER
#define SubKey "Software\\MarineCat\\GOGO_DLL"
		HKEY	hKey;
		DWORD	dwType, dwKeySize;
		LONG	lResult;
		static	char	*szName = "INSTPATH";
		char	szPathName[ _MAX_PATH + 8];
		dwKeySize = sizeof( szPathName );
		if( RegOpenKeyEx(Key,SubKey,0,KEY_ALL_ACCESS,&hKey ) == ERROR_SUCCESS ){
			lResult = RegQueryValueEx(hKey,szName,0,&dwType,(BYTE *)szPathName,&dwKeySize);
			RegCloseKey(hKey);
			if( lResult == ERROR_SUCCESS && REG_SZ == dwType ){
				hDLL = LoadLibrary( szPathName );
			}
		}
#undef Key
#undef SubKey
	}
	if(hDLL){
		FreeLibrary(hDLL);
		return 1;
	}
	return 0;
}

#endif /* AU_GOGO_DLL */
