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

    w32g_subwin2.c: Written by Daisuke Aoki <dai@y7.net>
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

#include "w32g_dib.h"
#include "w32g_wrd.h"
#include "w32g_mag.h"

extern int gdi_lock_ex ( DWORD timeout );
#if 0
static int gdi_lock_result;
#define GDI_LOCK_EX(timeout) ( \
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "GDI_LOCK_EX(%s: %d)", __FILE__, __LINE__ ), \
	gdi_lock_result = gdi_lock_ex(timeout), \
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "GDI_LOCK_EX_RESULT(%d)", gdi_lock_result ), \
	gdi_lock_result \
)
#define GDI_LOCK() { \
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "GDI_LOCK(%s: %d)", __FILE__, __LINE__ ); \
	gdi_lock(); \
}
#define GDI_UNLOCK() { \
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "GDI_UNLOCK(%s: %d)", __FILE__, __LINE__ ); \
	gdi_unlock(); \
}
#else
#define GDI_LOCK() { gdi_lock(); }
#define GDI_LOCK_EX(timeout) gdi_lock_ex(timeout)
#define GDI_UNLOCK() { gdi_unlock(); }
#endif

#if defined(__CYGWIN32__) || defined(__MINGW32__)
#ifndef TPM_TOPALIGN
#define TPM_TOPALIGN	0x0000L
#endif
#endif

extern void VprintfEditCtlWnd(HWND hwnd, char *fmt, va_list argList);
extern void PrintfEditCtlWnd(HWND hwnd, char *fmt, ...);
extern void PutsEditCtlWnd(HWND hwnd, char *str);
extern void ClearEditCtlWnd(HWND hwnd);

#ifndef _mbbtype
#ifndef _MBC_SINGLE
#define _MBC_SINGLE (0)
#endif
#ifndef _MBC_LEAD
#define _MBC_LEAD (1)
#endif
#ifndef _MBC_TRAIL
#define _MBC_TRAIL (2)
#endif
#ifndef _MBC_ILLEGAL
#define _MBC_ILLEGAL (-1)
#endif
#define is_sjis_kanji1(x) ((((unsigned char)(x))>=0x81 && ((unsigned char)(x))<=0x9f) || (((unsigned char)(x))>=0xe0 && ((unsigned char)(x))<=0xfc))
#define is_sjis_kanji2(x) ((((unsigned char)(x))>=0x40 && ((unsigned char)(x))<=0x7e) || (((unsigned char)(x))>=0x80 && ((unsigned char)(x))<=0xfc))
static int ___sjis_mbbtype(unsigned char c, int mbtype)
{
	if(mbtype==_MBC_LEAD){
		if(is_sjis_kanji2(c)) return _MBC_TRAIL; else return _MBC_ILLEGAL;
	} else { if(is_sjis_kanji1(c)) return _MBC_LEAD; else return _MBC_SINGLE; }
}
static int _mbbtype(unsigned char c, int mbtype)
{
	return ___sjis_mbbtype(c,mbtype);
}
#endif


static RGBQUAD RGBtoRGBQUAD ( COLORREF color );
static COLORREF RGBQUADtoRGB ( RGBQUAD rq );

// ****************************************************************************
// Wrd Window

#define MAG_WORK_WIDTH 800
#define MAG_WORK_HEIGHT 640

#define COLOR_MASK_WHITE RGB(0xFF,0xFF,0xFF)
#define COLOR_MASK_BLACK RGB(0x00,0x00,0x00)

WRDWNDINFO WrdWndInfo;

static int WrdWndInfoReset(HWND hwnd);
static int WrdWndInfoApply(void);

w32g_wrd_wnd_t w32g_wrd_wnd;

BOOL SetWrdWndActive(void)
{
	if ( IsWindowVisible(hWrdWnd) ) {
		w32g_wrd_wnd.active = TRUE;
	} else {
		w32g_wrd_wnd.active = FALSE;
	}
	return w32g_wrd_wnd.active;
}

static void wrd_graphic_terminate ( void );
static void wrd_graphic_init ( HDC hdc );
static void wrd_graphic_tone_change ( int index, int tone );
static void wrd_graphic_set_pal ( int index, int pal_index, COLORREF color );
static void wrd_graphic_plane_change ( int active, int display );
static void wrd_graphic_reset ( int index );
static void wrd_graphic_reset_all ( void );
static void wrd_graphic_apply ( RECT *lprc, int index, int lockflag );
static void wrd_graphic_update ( RECT *lprc, int lockflag );
static void wrd_text_update ( int x_from, int y_from, int x_to, int y_to, int lockflag );

static int volatile wrd_graphic_pal_init_flag = 0;

static HANDLE volatile hMutexWrd = NULL;
static BOOL wrd_wnd_lock_ex ( DWORD timeout )
{
	if ( hMutexWrd == NULL ) {
		hMutexWrd = CreateMutex ( NULL, FALSE, NULL );
		if ( hMutexWrd == NULL )
			return FALSE;
	}
	if ( WaitForSingleObject ( hMutexWrd, timeout )== WAIT_FAILED ) {
		return FALSE;
	}
	return TRUE;
}
static BOOL wrd_wnd_lock (void)
{
	return wrd_wnd_lock_ex ( INFINITE );
}
static void wrd_wnd_unlock (void)
{
	ReleaseMutex ( hMutexWrd );
}


BOOL CALLBACK WrdWndProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK WrdCanvasWndProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam);
static int volatile wrd_wnd_initflag = 1;
void TerminateWrdWnd ( void )
{
	wrd_wnd_lock();
	w32g_wrd_wnd.active = FALSE;
	if ( !wrd_wnd_initflag ) {
		DeleteDC ( w32g_wrd_wnd.hmdc );
		DeleteObject ( (HGDIOBJ) w32g_wrd_wnd.hbitmap );
		DeleteObject ( (HGDIOBJ) w32g_wrd_wnd.hbmp_tmask );
		DeleteObject ( (HGDIOBJ) w32g_wrd_wnd.hbmp_work );
		DeleteObject ( (HGDIOBJ) w32g_wrd_wnd.hNullBrush );
		DeleteObject ( (HGDIOBJ) w32g_wrd_wnd.hNullPen );
	}
	wrd_graphic_terminate ();
	memset ( &w32g_wrd_wnd, 0, sizeof (w32g_wrd_wnd_t) );
	wrd_wnd_unlock();
}
void InitWrdWnd(HWND hParentWnd)
{
	WNDCLASS wndclass ;
	HICON hIcon;

	wrd_wnd_lock();
	if ( wrd_wnd_initflag ) {
		memset ( &w32g_wrd_wnd, 0, sizeof (w32g_wrd_wnd_t) );
		wrd_wnd_initflag = 0;
	}
	if (hWrdWnd != NULL) {
		DestroyWindow(hWrdWnd);
		hWrdWnd = NULL;
	}
	TerminateWrdWnd ();

	INILoadWrdWnd();

	w32g_wrd_wnd.hNullBrush = GetStockObject ( NULL_BRUSH );
	w32g_wrd_wnd.hNullPen = GetStockObject ( NULL_PEN );

	hWrdWnd = CreateDialog
		(hInst,MAKEINTRESOURCE(IDD_DIALOG_WRD),hParentWnd,WrdWndProc);
	WrdWndInfoReset(hWrdWnd);
	INILoadWrdWnd();
	ShowWindow(hWrdWnd,SW_HIDE);
	w32g_wrd_wnd.draw_skip = 0;
	w32g_wrd_wnd.font_height = 16; 
	w32g_wrd_wnd.font_width = 8; 
	w32g_wrd_wnd.row = 80; 
	w32g_wrd_wnd.col = 25;
	w32g_wrd_wnd.width = w32g_wrd_wnd.font_width * w32g_wrd_wnd.row; 
	w32g_wrd_wnd.height = w32g_wrd_wnd.font_height * w32g_wrd_wnd.col; 
	w32g_wrd_wnd.pals[W32G_WRDWND_BLACK] = RGB ( 0x00, 0x00, 0x00 );
	w32g_wrd_wnd.pals[W32G_WRDWND_RED] = RGB ( 0xFF, 0x00, 0x00 );
	w32g_wrd_wnd.pals[W32G_WRDWND_BLUE] = RGB ( 0x00, 0x00, 0xFF );
	w32g_wrd_wnd.pals[W32G_WRDWND_PURPLE] = RGB ( 0xFF, 0x00, 0xFF );
	w32g_wrd_wnd.pals[W32G_WRDWND_GREEN] = RGB ( 0x00, 0xFF, 0x00 );
	w32g_wrd_wnd.pals[W32G_WRDWND_LIGHTBLUE] = RGB ( 0x00, 0xFF, 0xFF );
	w32g_wrd_wnd.pals[W32G_WRDWND_YELLOW] = RGB ( 0xFF, 0xFF, 0xFF );
	w32g_wrd_wnd.pals[W32G_WRDWND_WHITE] = RGB ( 0xFF, 0xFF, 0xFF );
	w32g_wrd_wnd.flag = WRD_FLAG_DEFAULT;

	wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_CLASSDC;
	wndclass.lpfnWndProc   = WrdCanvasWndProc ;
	wndclass.cbClsExtra    = 0 ;
	wndclass.cbWndExtra    = 0 ;
	wndclass.hInstance     = hInst ;
	wndclass.hIcon         = NULL;
	wndclass.hCursor       = LoadCursor(0,IDC_ARROW) ;
//	wndclass.hbrBackground = (HBRUSH)(COLOR_SCROLLBAR + 1);
	wndclass.hbrBackground = w32g_wrd_wnd.hNullBrush;
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = "wrd canvas wnd";
	RegisterClass(&wndclass);
  	w32g_wrd_wnd.hwnd = CreateWindowEx(0,"wrd canvas wnd",0,WS_CHILD,
		CW_USEDEFAULT,0,w32g_wrd_wnd.width,w32g_wrd_wnd.height,
		hWrdWnd,0,hInst,0);
	w32g_wrd_wnd.hdc = GetDC(w32g_wrd_wnd.hwnd);

	// 大元
	w32g_wrd_wnd.hbitmap = CreateCompatibleBitmap(w32g_wrd_wnd.hdc,w32g_wrd_wnd.width,w32g_wrd_wnd.height);
	w32g_wrd_wnd.hmdc = CreateCompatibleDC(w32g_wrd_wnd.hdc);
	w32g_wrd_wnd.hgdiobj_hmdcprev = SelectObject(w32g_wrd_wnd.hmdc,w32g_wrd_wnd.hbitmap);
	SelectObject ( w32g_wrd_wnd.hmdc, w32g_wrd_wnd.hNullBrush );
	SelectObject ( w32g_wrd_wnd.hmdc, w32g_wrd_wnd.hNullPen );

	// ワーク
	w32g_wrd_wnd.hbmp_work = CreateCompatibleBitmap(w32g_wrd_wnd.hdc,w32g_wrd_wnd.width,w32g_wrd_wnd.height);

	// テキストマスク
//	w32g_wrd_wnd.hbmp_tmask = CreateBitmap ( w32g_wrd_wnd.width, w32g_wrd_wnd.height, 1, 1, NULL);
	w32g_wrd_wnd.hbmp_tmask = CreateCompatibleBitmap(w32g_wrd_wnd.hdc,w32g_wrd_wnd.width,w32g_wrd_wnd.height);
	{
		HDC hmdc;
		RECT rc;
		HBRUSH hbrush = CreateSolidBrush ( COLOR_MASK_BLACK );
		SetRect ( &rc, 0, 0, w32g_wrd_wnd.width, w32g_wrd_wnd.height );
		hmdc = CreateCompatibleDC ( w32g_wrd_wnd.hmdc );
		SelectObject ( hmdc, w32g_wrd_wnd.hbmp_tmask );
		FillRect ( hmdc, &rc, hbrush );
		SelectObject ( hmdc, w32g_wrd_wnd.hbmp_work );
		FillRect ( hmdc, &rc, hbrush );
		DeleteDC ( hmdc );
		DeleteObject ( hbrush );
	}

	// グラフィック
	wrd_graphic_init ( w32g_wrd_wnd.hdc );

	ReleaseDC(w32g_wrd_wnd.hwnd,w32g_wrd_wnd.hdc);

	{
		char fontname[1024];
		if ( PlayerLanguage == LANGUAGE_JAPANESE )
			strcpy(fontname,"ＭＳ 明朝");
		else
			strcpy(fontname,"Times New Roman");
		w32g_wrd_wnd.hFont = CreateFont(w32g_wrd_wnd.font_height,w32g_wrd_wnd.font_width,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,
			DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
	      	FIXED_PITCH | FF_MODERN	,fontname);
	}
	hIcon = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_TIMIDITY), IMAGE_ICON, 16, 16, 0);
	if (hIcon!=NULL) SendMessage(hWrdWnd,WM_SETICON,FALSE,(LPARAM)hIcon);
	wrd_wnd_unlock();

	WrdWndReset();
	w32g_wrd_wnd.active = FALSE;
	MoveWindow(w32g_wrd_wnd.hwnd,0,0,w32g_wrd_wnd.width,w32g_wrd_wnd.height,TRUE);
	WrdWndClear ( TRUE );
	ShowWindow(w32g_wrd_wnd.hwnd,SW_SHOW);
	UpdateWindow(w32g_wrd_wnd.hwnd);
	UpdateWindow(hWrdWnd);
	WrdWndInfoApply();
}

static void wrd_graphic_terminate ( void )
{
	int index;
	wrd_wnd_lock();
	GdiFlush ();
	for ( index = 0; index < W32G_WRDWND_GRAPHIC_PLANE_MAX; index++ ) {
		if ( w32g_wrd_wnd.graphic_dib[index] != NULL ) {
			dib_free ( w32g_wrd_wnd.graphic_dib[index] );
			w32g_wrd_wnd.graphic_dib[index] = NULL;
		}
	}
	if ( w32g_wrd_wnd.bits_mag_work != NULL )
		free ( w32g_wrd_wnd.bits_mag_work );
	wrd_wnd_unlock();
}

// プレーン index のグラフィックの初期化
static void wrd_graphic_init ( HDC hdc )
{
	int index;

	wrd_wnd_lock();
	wrd_graphic_terminate ();
	for ( index = 0; index < W32G_WRDWND_GRAPHIC_PLANE_MAX; index++ ) {
		w32g_wrd_wnd.graphic_dib[index] = dib_create ( w32g_wrd_wnd.width, w32g_wrd_wnd.height );
		w32g_wrd_wnd.modified_graphic[index] = TRUE;
	}

	w32g_wrd_wnd.bits_mag_work = (char *) safe_malloc ( MAG_WORK_WIDTH * MAG_WORK_HEIGHT );

	w32g_wrd_wnd.index_active = 0;
	w32g_wrd_wnd.index_display = 0;
	wrd_wnd_unlock();
	wrd_graphic_reset_all ();
}

static void wrd_graphic_plane_change ( int active, int display )
{
	wrd_wnd_lock();
	w32g_wrd_wnd.index_active = active;
	w32g_wrd_wnd.index_display = display;
	wrd_wnd_unlock();
}

static void wrd_graphic_reset ( int index )
{
	RECT rc;

	wrd_wnd_lock();
	memset ( w32g_wrd_wnd.graphic_dib[index]->bits, 0, w32g_wrd_wnd.graphic_dib[index]->width * w32g_wrd_wnd.graphic_dib[index]->height );
	dib_set_pals ( w32g_wrd_wnd.graphic_dib[index], w32g_wrd_wnd.default_gpal, W32G_WRDWND_GRAPHIC_PALLETE_MAX );
	SetRect ( &rc, 0, 0, w32g_wrd_wnd.width, w32g_wrd_wnd.height );
	w32g_wrd_wnd.modified_graphic[index] = TRUE;
	wrd_wnd_unlock();
	wrd_graphic_apply ( &rc, index, TRUE );
}

static void wrd_graphic_reset_all ( void )
{
	int i;

	wrd_wnd_lock();
	w32g_wrd_wnd.default_gpal[0] = RGBtoRGBQUAD (RGB ( 0x00, 0x00, 0x00 ));
	w32g_wrd_wnd.default_gpal[1] = RGBtoRGBQUAD (RGB ( 0x00, 0x00, 0x7F ) );
	w32g_wrd_wnd.default_gpal[2] = RGBtoRGBQUAD (RGB ( 0x7F, 0x00, 0x00 ));
	w32g_wrd_wnd.default_gpal[3] = RGBtoRGBQUAD (RGB ( 0x7F, 0x00, 0x7F ));
	w32g_wrd_wnd.default_gpal[4] = RGBtoRGBQUAD (RGB ( 0x00, 0x7F, 0x00 ));
	w32g_wrd_wnd.default_gpal[5] = RGBtoRGBQUAD (RGB ( 0x00, 0x7F, 0x7F ));
	w32g_wrd_wnd.default_gpal[6] = RGBtoRGBQUAD (RGB ( 0x7F, 0x7F, 0x00 ));
	w32g_wrd_wnd.default_gpal[7] = RGBtoRGBQUAD (RGB ( 0x7F, 0x7F, 0x7F ));
	w32g_wrd_wnd.default_gpal[8] = RGBtoRGBQUAD (RGB ( 0x00, 0x00, 0x00 ));
	w32g_wrd_wnd.default_gpal[9] = RGBtoRGBQUAD (RGB ( 0x00, 0x00, 0xFF ));
	w32g_wrd_wnd.default_gpal[10] = RGBtoRGBQUAD (RGB ( 0xFF, 0x00, 0x00 ));
	w32g_wrd_wnd.default_gpal[11] = RGBtoRGBQUAD (RGB ( 0xFF, 0x00, 0xFF ));
	w32g_wrd_wnd.default_gpal[12] = RGBtoRGBQUAD (RGB ( 0x00, 0xFF, 0x00 ));
	w32g_wrd_wnd.default_gpal[13] = RGBtoRGBQUAD (RGB ( 0x00, 0xFF, 0xFF ));
	w32g_wrd_wnd.default_gpal[14] = RGBtoRGBQUAD (RGB ( 0xFF, 0xFF, 0x00 ));
	w32g_wrd_wnd.default_gpal[15] = RGBtoRGBQUAD (RGB ( 0xFF, 0xFF, 0xFF ));
	for ( i = 16; i < W32G_WRDWND_GRAPHIC_PALLETE_MAX; i++ )
		w32g_wrd_wnd.default_gpal[i] = RGBtoRGBQUAD (RGB ( 0x00, 0x00, 0x00 ));
	wrd_wnd_unlock();

	for ( i = 0; i < W32G_WRDWND_GRAPHIC_PLANE_MAX; i ++ ) {
		wrd_graphic_reset ( i );
	}
	wrd_wnd_lock();
	w32g_wrd_wnd.gmode = 0xFFFF;
	w32g_wrd_wnd.fade_from = -1;
	w32g_wrd_wnd.fade_to = -1;
	wrd_wnd_unlock();
}

// プレーン index のグラフィックの lprc 領域を hmdc_graphic へ更新
static void wrd_graphic_apply ( RECT *lprc, int index, int lockflag )
{
#if 0
	if ( WrdWndInfo.GraphicStop ) return;
//	if ( !w32g_wrd_wnd.modified_graphic[index] ) return;

	if ( lockflag ) wrd_wnd_lock();
	w32g_wrd_wnd.modified_graphic[index] = FALSE;
	if ( lockflag ) wrd_wnd_unlock();
#endif
}

// lprc 領域のグラフィックを更新
static void wrd_graphic_update ( RECT *lprc, int lockflag )
{
	if ( WrdWndInfo.GraphicStop ) return;
	if ( w32g_wrd_wnd.draw_skip ) return;
	wrd_wnd_lock();
	if ( lockflag ) GDI_LOCK();
	// 画像処理した関数で行う。
	if ( w32g_wrd_wnd.flag & WRD_FLAG_GRAPHIC ) {
		if ( w32g_wrd_wnd.flag & WRD_FLAG_TEXT ) {
			HDC hmdc_work, hmdc_tmask, hmdc_graphic;
			GdiFlush ();
			hmdc_work = CreateCompatibleDC ( w32g_wrd_wnd.hmdc );
			hmdc_tmask = CreateCompatibleDC ( w32g_wrd_wnd.hmdc );
			hmdc_graphic = CreateCompatibleDC ( w32g_wrd_wnd.hmdc );
			SelectObject ( hmdc_work, w32g_wrd_wnd.hbmp_work );
			SelectObject ( hmdc_tmask, w32g_wrd_wnd.hbmp_tmask );
			SelectObject ( hmdc_graphic, w32g_wrd_wnd.graphic_dib[w32g_wrd_wnd.index_display]->hbmp );
			BitBlt ( w32g_wrd_wnd.hmdc, lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top,
				hmdc_tmask,  lprc->left, lprc->top, SRCAND );
			BitBlt ( hmdc_work, lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top,
				hmdc_tmask,  lprc->left, lprc->top, NOTSRCCOPY );
			BitBlt ( hmdc_work, lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top,
				hmdc_graphic, lprc->left, lprc->top, SRCAND );
			BitBlt ( w32g_wrd_wnd.hmdc, lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top,
				hmdc_work,  lprc->left, lprc->top, SRCPAINT );
			DeleteDC ( hmdc_work );
			DeleteDC ( hmdc_tmask );
			DeleteDC ( hmdc_graphic );
		} else {
			HDC hmdc_graphic;
			hmdc_graphic = CreateCompatibleDC ( w32g_wrd_wnd.hmdc );
			SelectObject ( hmdc_graphic, w32g_wrd_wnd.graphic_dib[w32g_wrd_wnd.index_display]->hbmp );
			BitBlt ( w32g_wrd_wnd.hmdc, lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top,
				hmdc_graphic,  lprc->left, lprc->top, SRCCOPY );
			DeleteDC ( hmdc_graphic );
		}
	}
	if ( lockflag ) GDI_UNLOCK();
	wrd_wnd_unlock();
}

// 領域のテキストを更新
static void wrd_text_update ( int x_from, int y_from, int x_to, int y_to, int lockflag )
{
	RECT rc;

	if ( w32g_wrd_wnd.draw_skip ) return;

	if ( x_from < 0 ) x_from = 0;
	if ( x_from >= w32g_wrd_wnd.row ) x_from = w32g_wrd_wnd.row - 1;
	if ( x_to < 0 ) x_to = 0;
	if ( x_to >= w32g_wrd_wnd.row ) x_to = w32g_wrd_wnd.row - 1;
	if ( y_from < 0 ) y_from = 0;
	if ( y_from >= w32g_wrd_wnd.row ) y_from = w32g_wrd_wnd.col - 1;
	if ( y_to < 0 ) y_to = 0;
	if ( y_to >= w32g_wrd_wnd.row ) y_to = w32g_wrd_wnd.col - 1;

	SetRect ( &rc, x_from * w32g_wrd_wnd.font_width, y_from * w32g_wrd_wnd.font_height,
		(x_to+1) * w32g_wrd_wnd.font_width, (y_to+1) * w32g_wrd_wnd.font_height);

	wrd_wnd_lock();
	if ( lockflag ) GDI_LOCK();
	if ( w32g_wrd_wnd.flag & WRD_FLAG_TEXT ) {
		HDC hmdc_tmask;
		COLORREF forecolor, backcolor, prevforecolor, prevbackcolor;
		char attr;
		HGDIOBJ hgdiobj;
		int x, y;
		forecolor = w32g_wrd_wnd.pals[w32g_wrd_wnd.curforecolor];
		backcolor = w32g_wrd_wnd.pals[w32g_wrd_wnd.curbackcolor];
		prevforecolor = SetTextColor( w32g_wrd_wnd.hmdc, forecolor );
		prevbackcolor = SetBkColor( w32g_wrd_wnd.hmdc, backcolor );
		hgdiobj = SelectObject( w32g_wrd_wnd.hmdc, w32g_wrd_wnd.hFont );
		hmdc_tmask = CreateCompatibleDC ( w32g_wrd_wnd.hmdc );
		SelectObject( hmdc_tmask, w32g_wrd_wnd.hbmp_tmask );
		SelectObject( hmdc_tmask, w32g_wrd_wnd.hFont );
		SelectObject( hmdc_tmask, w32g_wrd_wnd.hNullBrush );
		SelectObject( hmdc_tmask, w32g_wrd_wnd.hNullPen );
		attr = 0;
		SetTextColor( hmdc_tmask, COLOR_MASK_WHITE );
		SetBkColor( hmdc_tmask, COLOR_MASK_BLACK );
		for( y = y_from; y <= y_to; y++ ) {
			for( x = x_from; x <= x_to; x++ ) {
				char mbt = _MBC_SINGLE;
				RECT rc_part;
				if ( forecolor != w32g_wrd_wnd.pals[w32g_wrd_wnd.forecolorbuf[y][x]] ) {
					forecolor = w32g_wrd_wnd.pals[w32g_wrd_wnd.forecolorbuf[y][x]];
					SetTextColor( w32g_wrd_wnd.hmdc, forecolor );
				}
				if ( backcolor != w32g_wrd_wnd.pals[w32g_wrd_wnd.backcolorbuf[y][x]] ) {
					backcolor = w32g_wrd_wnd.pals[w32g_wrd_wnd.backcolorbuf[y][x]];
					SetBkColor( w32g_wrd_wnd.hmdc, backcolor );
				}
				if ( attr != w32g_wrd_wnd.attrbuf[y][x] ) {
					if ( ( attr & W32G_WRDWND_ATTR_REVERSE ) != ( w32g_wrd_wnd.attrbuf[y][x] & W32G_WRDWND_ATTR_REVERSE ) ) {
						if ( w32g_wrd_wnd.attrbuf[y][x] & W32G_WRDWND_ATTR_REVERSE ) {
							SetTextColor( hmdc_tmask, COLOR_MASK_BLACK );
							SetBkColor( hmdc_tmask, COLOR_MASK_WHITE );
						} else {
							SetTextColor( hmdc_tmask, COLOR_MASK_WHITE );
							SetBkColor( hmdc_tmask, COLOR_MASK_BLACK );
						}
					}
					attr = w32g_wrd_wnd.attrbuf[y][x];
				}
				if ( PlayerLanguage == LANGUAGE_JAPANESE && _mbbtype( w32g_wrd_wnd.textbuf[y][x], _MBC_SINGLE ) == _MBC_LEAD ) {
					SetRect ( &rc_part, x * w32g_wrd_wnd.font_width, y * w32g_wrd_wnd.font_height,
						(x + 2) * w32g_wrd_wnd.font_width, (y + 1) * w32g_wrd_wnd.font_height );
					if ( w32g_wrd_wnd.flag & WRD_FLAG_TEXT )
						ExtTextOut( w32g_wrd_wnd.hmdc, rc_part.left, rc_part.top, ETO_OPAQUE | ETO_CLIPPED, &rc_part, w32g_wrd_wnd.textbuf[y] + x, 2, NULL);
					ExtTextOut( hmdc_tmask, rc_part.left, rc_part.top, ETO_OPAQUE | ETO_CLIPPED, &rc_part, w32g_wrd_wnd.textbuf[y] + x, 2, NULL);
					x++;
				} else {
					SetRect ( &rc_part, x * w32g_wrd_wnd.font_width, y * w32g_wrd_wnd.font_height,
						(x + 1) * w32g_wrd_wnd.font_width, (y + 1) * w32g_wrd_wnd.font_height );
					if ( w32g_wrd_wnd.flag & WRD_FLAG_TEXT )
						ExtTextOut( w32g_wrd_wnd.hmdc, rc_part.left, rc_part.top, ETO_OPAQUE | ETO_CLIPPED, &rc_part, w32g_wrd_wnd.textbuf[y] + x, 1, NULL);
					ExtTextOut( hmdc_tmask, rc_part.left, rc_part.top, ETO_OPAQUE | ETO_CLIPPED, &rc_part, w32g_wrd_wnd.textbuf[y] + x, 1, NULL);
				}
			}
		}
		SetTextColor( w32g_wrd_wnd.hmdc, prevforecolor);
		SetBkColor( w32g_wrd_wnd.hmdc, prevbackcolor);
		SelectObject( w32g_wrd_wnd.hmdc, hgdiobj );
		DeleteDC ( hmdc_tmask );
	}
	if ( lockflag ) GDI_UNLOCK();
	wrd_wnd_unlock();
	if ( ! WrdWndInfo.GraphicStop ) {
		wrd_graphic_apply ( &rc, w32g_wrd_wnd.index_display, TRUE );
		wrd_graphic_update ( &rc, 0 );
	}
	InvalidateRect ( w32g_wrd_wnd.hwnd, &rc, FALSE);
}

extern void wrd_graphic_ginit ( void );
extern void wrd_graphic_gcls ( int sw );
extern void wrd_graphic_gscreen ( int active, int display );
extern void wrd_graphic_gon ( int sw );
extern void wrd_graphic_gline ( int x1, int y1, int x2, int y2, int p1, int sw, int p2 );
extern void wrd_graphic_gcircle ( int x, int y, int r, int p1, int sw, int p2 );
extern void wrd_graphic_pload ( char *path );
extern void wrd_graphic_pal_g4r4b4 ( int p, int *g4r4b4, int max );
extern void wrd_graphic_palrev ( int p );
extern void wrd_graphic_apply_pal ( int p );
extern void wrd_graphic_fade ( int p1, int p2, int speed );
extern void wrd_graphic_fadestep ( int v );
extern void wrd_graphic_gmode ( int sw );
extern void wrd_graphic_gmove ( int x1, int y1, int x2, int y2, int xd, int yd, int vs, int vd, int sw );
extern void wrd_graphic_mag ( char *path, int x, int y, int s, int p );
extern void wrd_text_scroll ( int x1, int y1, int x2, int y2, int mode, int color, int c );
extern void wrd_start_skip ( void );
extern void wrd_end_skip ( void );
extern void wrd_graphic_xcopy ( int sx1, int sy1, int sx2, int sy2, int tx, int ty, int ss, int ts, int method,
	 int opt1, int opt2, int opt3, int opt4, int opt5 );

void wrd_graphic_ginit ( void )
{
	RECT rc;
	int index_display_old = w32g_wrd_wnd.index_display;
	if ( WrdWndInfo.GraphicStop ) return;
	if ( !w32g_wrd_wnd.active ) return;
	wrd_graphic_plane_change ( 0, 0 );
	wrd_graphic_reset_all ();
	wrd_wnd_lock();
	mag_deletetab();
	wrd_wnd_unlock();
	SetRect ( &rc, 0, 0, w32g_wrd_wnd.width, w32g_wrd_wnd.height );
	InvalidateRect ( w32g_wrd_wnd.hwnd, &rc, FALSE );
}

void wrd_graphic_gcls ( int sw )
{
	RECT rc;
	int i, size;

	if ( WrdWndInfo.GraphicStop ) return;
	if ( !w32g_wrd_wnd.active ) return;
	wrd_wnd_lock();
	GdiFlush ();
	size = w32g_wrd_wnd.width * w32g_wrd_wnd.height;
	if ( sw == 0 )
		sw = 0xFF;
	for ( i = 0; i < size; i ++ ) {
		w32g_wrd_wnd.graphic_dib[w32g_wrd_wnd.index_active]->bits[i] &= ~sw;
	}
	w32g_wrd_wnd.modified_graphic[w32g_wrd_wnd.index_active] = TRUE;
	SetRect ( &rc, 0, 0, w32g_wrd_wnd.width, w32g_wrd_wnd.height );
	wrd_wnd_unlock();
	if ( w32g_wrd_wnd.index_active == w32g_wrd_wnd.index_display ) {
		wrd_graphic_apply ( &rc, w32g_wrd_wnd.index_display, TRUE );
		wrd_graphic_update ( &rc, TRUE );
		InvalidateRect ( w32g_wrd_wnd.hwnd, &rc, FALSE );
	}
}

void wrd_graphic_gscreen ( int active, int display )
{
	int display_old = w32g_wrd_wnd.index_display;

	if ( WrdWndInfo.GraphicStop ) return;
	if ( !w32g_wrd_wnd.active ) return;
	if ( active < 0 || active >= 2 || display < 0 || display >= 2 )
		return;
	wrd_wnd_lock();
	wrd_graphic_plane_change ( active, display );
	wrd_wnd_unlock();
	if ( display_old != w32g_wrd_wnd.index_display ) {
		RECT rc;
		SetRect ( &rc, 0, 0, w32g_wrd_wnd.width, w32g_wrd_wnd.height );
		wrd_graphic_apply ( &rc, w32g_wrd_wnd.index_display, TRUE );
		wrd_graphic_update ( &rc, TRUE );
		InvalidateRect ( w32g_wrd_wnd.hwnd, &rc, FALSE );
	}
}

void wrd_graphic_gon ( int sw )
{
	int sw_old = w32g_wrd_wnd.flag & WRD_FLAG_GRAPHIC;

	if ( WrdWndInfo.GraphicStop ) return;
	if ( !w32g_wrd_wnd.active ) return;
	if ( sw && !sw_old ) {
		w32g_wrd_wnd.flag |= WRD_FLAG_GRAPHIC;		// 先に設定
		if ( w32g_wrd_wnd.index_active == w32g_wrd_wnd.index_display ) {
			RECT rc;
			SetRect ( &rc, 0, 0, w32g_wrd_wnd.width, w32g_wrd_wnd.height );
			wrd_graphic_apply ( &rc, w32g_wrd_wnd.index_display, TRUE );
			wrd_graphic_update ( &rc, TRUE );
			InvalidateRect ( w32g_wrd_wnd.hwnd, &rc, FALSE );
		}
	} else if ( !sw && sw_old ) {
		if ( w32g_wrd_wnd.index_active == w32g_wrd_wnd.index_display ) {
			RECT rc;
			SetRect ( &rc, 0, 0, w32g_wrd_wnd.width, w32g_wrd_wnd.height );
			wrd_graphic_apply ( &rc, w32g_wrd_wnd.index_display, TRUE );
			wrd_graphic_update ( &rc, TRUE );
			InvalidateRect ( w32g_wrd_wnd.hwnd, &rc, FALSE );
		}
	}
	if ( sw )
		w32g_wrd_wnd.flag |= WRD_FLAG_GRAPHIC;
	else
		w32g_wrd_wnd.flag &= ~WRD_FLAG_GRAPHIC;
}

void wrd_graphic_gline ( int x1, int y1, int x2, int y2, int p1, int sw, int p2 )
{
	int x, y, i;
	RECT rc[10];
	int rc_max = 0;
	char *bits = w32g_wrd_wnd.graphic_dib[w32g_wrd_wnd.index_active]->bits;

	if ( WrdWndInfo.GraphicStop ) return;
	if ( !w32g_wrd_wnd.active ) return;
	if ( x1 > x2 || y1 > y2 || x1 < 0 || y1 < 0 || x2 >= w32g_wrd_wnd.width || y2 >=  w32g_wrd_wnd.height )
		return;
	wrd_wnd_lock();
	GdiFlush ();
	// w32g_wrd_wnd.gmode
	if ( sw >= 1 ) {
		for ( x = x1; x <= x2; x ++ ) 
			bits[ y1 * w32g_wrd_wnd.width + x] = w32g_wrd_wnd.gmode & p1;
		SetRect ( &rc[rc_max++], x1, y1, x2+1, y1+1 );
		for ( y = y1 + 1; y <= y2 - 1; y ++ ) {
			bits[ y * w32g_wrd_wnd.width + x1] = w32g_wrd_wnd.gmode & p1;
			bits[ y * w32g_wrd_wnd.width + x2] = w32g_wrd_wnd.gmode & p1;
		}
		SetRect ( &rc[rc_max++], x1, y1, x1+1, y2+1 );
		SetRect ( &rc[rc_max++], x2, y1, x2+1, y2+1 );
		for ( x = x1; x <= x2; x ++ ) 
			bits[ y2 * w32g_wrd_wnd.width + x] = w32g_wrd_wnd.gmode & p1;
		SetRect ( &rc[rc_max++], x1, y2, x2+1, y2+1 );
		if ( sw == 2 ) {
			for ( y = y1 + 1; y <= y2 - 1; y ++ ) 
				for ( x = x1 + 1; x <= x2 - 1; x ++ ) 
					bits[ y * w32g_wrd_wnd.width + x] = w32g_wrd_wnd.gmode & p2;
			rc_max = 0;
			SetRect ( &rc[rc_max++], x1, y1, x2+1, y2+1 );
		}
	} else if ( sw == 0 ) {
		if ( x1 == x2 ) {
			for ( y = y1 ; y <= y2 ; y ++ ) 
				bits[ y * w32g_wrd_wnd.width + x1] = w32g_wrd_wnd.gmode & p1;
			SetRect ( &rc[rc_max++], x1, y1, x1+1, y2+1 );
		} else if ( y1 == y2 ) {
			for ( x = x1; x <= x2; x ++ ) 
				bits[ y1 * w32g_wrd_wnd.width + x] = w32g_wrd_wnd.gmode & p2;
			SetRect ( &rc[rc_max++], x1, y1, x2+1, y1+1 );
		} else if ( x2 - x1 == y2 - y1 ) {
			for ( y = y1; y <= y2; y ++ ) 
				bits[ y1 * w32g_wrd_wnd.width + (y - y1) + x1] = w32g_wrd_wnd.gmode & p2;
			SetRect ( &rc[rc_max++], x1, y1, x2+1, y2+1 );
		} else if ( x2 - x1 > y2 - y1 ) {
			double r = (y2 - y1) / (x2 -x1), r2;
			int x_min = x1;
			for ( y = y1; y <= y2 - 1; y ++ ) {
				for ( x = x_min; x <= x2; x ++ ) {
					r2 = r * (x - x1) - (y - y1);
					if ( r2 < 0.0 || r2 >= 1.0 ) {
						break;
					} else if ( r2 <= 0.5 ) {
						bits[ y * w32g_wrd_wnd.width + x] = w32g_wrd_wnd.gmode & p1;
						x_min = x + 1;
					} else {
						bits[ (y + 1) * w32g_wrd_wnd.width + x] = w32g_wrd_wnd.gmode & p1;
						x_min = x + 1;
					}
				}
			}
			SetRect ( &rc[rc_max++], x1, y1, x2+1, y2+1 );
		} else if ( x2 - x1 < y2 - y1 ) {
			double r = (x2 - x1) / (y2 -y1), r2;
			int y_min = y1;
			for ( x = x1; x <= x2 - 1; x ++ ) {
				for ( y = y_min; y <= y2; y ++ ) {
					r2 = r * (y - y1) - (x - x1);
					if ( r2 < 0.0 || r2 >= 1.0 ) {
						break;
					} else if ( r2 <= 0.5 ) {
						bits[ y * w32g_wrd_wnd.width + x] = w32g_wrd_wnd.gmode & p1;
						y_min = y + 1;
					} else {
						bits[ y * w32g_wrd_wnd.width + x + 1] = w32g_wrd_wnd.gmode & p1;
						y_min = y + 1;
					}
				}
			}
			SetRect ( &rc[rc_max++], x1, y1, x2+1, y2+1 );
		}
	}
	wrd_wnd_unlock();
	if ( w32g_wrd_wnd.index_active == w32g_wrd_wnd.index_display ) {
		for ( i = 0; i < rc_max; i ++ ) {
			wrd_graphic_apply ( &rc[i], w32g_wrd_wnd.index_display, TRUE );
			wrd_graphic_update ( &rc[i], TRUE );
			InvalidateRect ( w32g_wrd_wnd.hwnd, &rc[i], FALSE );
		}
	}
}

void wrd_graphic_gcircle ( int x, int y, int r, int p1, int sw, int p2 )
{
	if ( WrdWndInfo.GraphicStop ) return;
	if ( !w32g_wrd_wnd.active ) return;
	wrd_wnd_lock();
	wrd_wnd_unlock();
	// w32g_wrd_wnd.gmode
}

void wrd_graphic_pload ( char *path )
{
	int res;

	if ( WrdWndInfo.GraphicStop ) return;
	if ( !w32g_wrd_wnd.active ) return;
	wrd_wnd_lock();
	GdiFlush ();
	res = pho_load_pixel ( w32g_wrd_wnd.graphic_dib[w32g_wrd_wnd.index_active]->bits, w32g_wrd_wnd.width, w32g_wrd_wnd.height, path );
	wrd_wnd_unlock();
	if ( res && w32g_wrd_wnd.index_active == w32g_wrd_wnd.index_display ) {
		RECT rc;
		SetRect ( &rc, 0, 0, w32g_wrd_wnd.width, w32g_wrd_wnd.height );
		wrd_graphic_apply ( &rc, w32g_wrd_wnd.index_display, TRUE );
		wrd_graphic_update ( &rc, TRUE );
		InvalidateRect ( w32g_wrd_wnd.hwnd, &rc, FALSE );
	}
}

static COLORREF g4r4b4_to_rgb ( int g4r4b4 )
{
	return RGB ( ((g4r4b4 & 0x00F0) >> 4 ) << 4, 
		((g4r4b4 & 0x0F00) >> 8 ) << 4, 
		((g4r4b4 & 0x000F) >> 0 ) << 4 );
}

void wrd_graphic_pal_g4r4b4 ( int p, int *g4r4b4, int max )
{
	int i;

	if ( WrdWndInfo.GraphicStop ) return;
	if ( !w32g_wrd_wnd.active ) return;
	if ( p < 0 ) return;
	if ( p >= W32G_WRDWND_GRAPHIC_PALLETE_BUF_MAX )
		return;
	wrd_wnd_lock();
	if ( max > W32G_WRDWND_GRAPHIC_PALLETE_MAX ) {
		max = W32G_WRDWND_GRAPHIC_PALLETE_MAX;
	}
	for ( i = 0; i < max; i++ ) {
		w32g_wrd_wnd.gpal_buff[p][i] = RGBtoRGBQUAD ( g4r4b4_to_rgb( g4r4b4[i] ) );
	}
	wrd_wnd_unlock();
	if ( p == 0 ) {
		RECT rc;
#if 1
		for ( i = 0; i < W32G_WRDWND_GRAPHIC_PLANE_MAX; i++ ) {
			dib_set_pals ( w32g_wrd_wnd.graphic_dib[i], w32g_wrd_wnd.gpal_buff[p], max );
		}
#else
		dib_set_pals ( w32g_wrd_wnd.graphic_dib[w32g_wrd_wnd.index_display], w32g_wrd_wnd.gpal_buff[p], max );
#endif
		SetRect ( &rc, 0, 0, w32g_wrd_wnd.width, w32g_wrd_wnd.height );
		wrd_graphic_apply ( &rc, w32g_wrd_wnd.index_display, TRUE );
		wrd_graphic_update ( &rc, TRUE );
		InvalidateRect(w32g_wrd_wnd.hwnd, &rc, FALSE);
	}
}

extern DWORD volatile dwMainThreadId;
void wrd_graphic_palrev ( int p )
{
	int i;

	if ( WrdWndInfo.GraphicStop ) return;
	if ( !w32g_wrd_wnd.active ) return;
	if ( p < 0 ) return;
	if ( p >= W32G_WRDWND_GRAPHIC_PALLETE_BUF_MAX )
		return;
	wrd_wnd_lock();
	for ( i = 0; i <W32G_WRDWND_GRAPHIC_PALLETE_MAX; i++ ) {
		w32g_wrd_wnd.gpal_buff[p][i].rgbBlue = 0xFF - w32g_wrd_wnd.gpal_buff[p][i].rgbBlue;
		w32g_wrd_wnd.gpal_buff[p][i].rgbGreen = 0xFF - w32g_wrd_wnd.gpal_buff[p][i].rgbGreen;
		w32g_wrd_wnd.gpal_buff[p][i].rgbRed = 0xFF - w32g_wrd_wnd.gpal_buff[p][i].rgbRed;
	}
	wrd_wnd_unlock();
	if ( p == 0 ) {
		RECT rc;
#if 1
		for ( i = 0; i < W32G_WRDWND_GRAPHIC_PLANE_MAX; i++ ) {
			dib_set_pals ( w32g_wrd_wnd.graphic_dib[i], w32g_wrd_wnd.gpal_buff[p], W32G_WRDWND_GRAPHIC_PALLETE_MAX );
		}
#else
		dib_set_pals ( w32g_wrd_wnd.graphic_dib[w32g_wrd_wnd.index_display], w32g_wrd_wnd.gpal_buff[p], W32G_WRDWND_GRAPHIC_PALLETE_MAX );
#endif
		SetRect ( &rc, 0, 0, w32g_wrd_wnd.width, w32g_wrd_wnd.height );
		wrd_graphic_apply ( &rc, w32g_wrd_wnd.index_display, TRUE );
		wrd_graphic_update ( &rc, TRUE );
		InvalidateRect ( w32g_wrd_wnd.hwnd, &rc, FALSE );
	}
}

void wrd_graphic_apply_pal ( int p )
{
	int i;
	RECT rc;

	if ( WrdWndInfo.GraphicStop ) return;
	if ( !w32g_wrd_wnd.active ) return;
	if ( p < 0 ) return;
	if ( p >= W32G_WRDWND_GRAPHIC_PALLETE_BUF_MAX )
		return;
	wrd_wnd_lock();
#if 1
	for ( i = 0; i < W32G_WRDWND_GRAPHIC_PLANE_MAX; i++ ) {
		dib_set_pals ( w32g_wrd_wnd.graphic_dib[i], w32g_wrd_wnd.gpal_buff[p], W32G_WRDWND_GRAPHIC_PALLETE_MAX );
	}
#else
	dib_set_pals ( w32g_wrd_wnd.graphic_dib[w32g_wrd_wnd.index_display], w32g_wrd_wnd.gpal_buff[p], W32G_WRDWND_GRAPHIC_PALLETE_MAX );
#endif
	SetRect ( &rc, 0, 0, w32g_wrd_wnd.width, w32g_wrd_wnd.height );
	wrd_wnd_unlock();
	wrd_graphic_apply ( &rc, w32g_wrd_wnd.index_display, TRUE );
	wrd_graphic_update ( &rc, TRUE );
	InvalidateRect ( w32g_wrd_wnd.hwnd, &rc, FALSE );
}

void wrd_graphic_fade ( int p1, int p2, int speed )
{

	if ( WrdWndInfo.GraphicStop ) return;
	if ( !w32g_wrd_wnd.active ) return;
	if ( p1 < 0 ) return;
	if ( p1 >= W32G_WRDWND_GRAPHIC_PALLETE_BUF_MAX ) return;
	if ( p2 < 0 ) return;
	if ( p2 >= W32G_WRDWND_GRAPHIC_PALLETE_BUF_MAX ) return;
	wrd_wnd_lock();
	w32g_wrd_wnd.fade_from = p1;
	w32g_wrd_wnd.fade_to = p2;
	wrd_wnd_unlock();
	if ( speed == 0 ) {
		wrd_graphic_apply_pal ( p2 );
	} else{
//		wrd_graphic_apply_pal ( p1 );
	}
}

void wrd_graphic_fadestep ( int v )
{
	int i;
	RECT rc;
	RGBQUAD gpal[W32G_WRDWND_GRAPHIC_PALLETE_MAX];
	double v1, v2;

	if ( WrdWndInfo.GraphicStop ) return;
	if ( !w32g_wrd_wnd.active ) return;
	if ( w32g_wrd_wnd.fade_from < 0 || w32g_wrd_wnd.fade_to < 0 )
		return;
	wrd_wnd_lock();
	v2 = (double) v / WRD_MAXFADESTEP;
	v1 = 1.0 - v2;
	for ( i = 0; i < W32G_WRDWND_GRAPHIC_PALLETE_MAX; i++ ) {
		RGBQUAD *rq1 = &w32g_wrd_wnd.gpal_buff[w32g_wrd_wnd.fade_from][i];
		RGBQUAD *rq2 = &w32g_wrd_wnd.gpal_buff[w32g_wrd_wnd.fade_to][i];
		gpal[i].rgbBlue = (char) ( v1 * rq1->rgbBlue + v2 * rq2->rgbBlue ); 
		gpal[i].rgbGreen = (char) ( v1 * rq1->rgbGreen + v2 * rq2->rgbGreen );
		gpal[i].rgbRed = (char) ( v1 * rq1->rgbRed + v2 * rq2->rgbRed );
		gpal[i].rgbReserved = 0;
	}
#if 1
	for ( i = 0; i < W32G_WRDWND_GRAPHIC_PLANE_MAX; i++ ) {
		dib_set_pals ( w32g_wrd_wnd.graphic_dib[i], gpal, W32G_WRDWND_GRAPHIC_PALLETE_MAX );
	}
#else
	dib_set_pals ( w32g_wrd_wnd.graphic_dib[w32g_wrd_wnd.index_display], gpal, W32G_WRDWND_GRAPHIC_PALLETE_MAX );
#endif
	SetRect ( &rc, 0, 0, w32g_wrd_wnd.width, w32g_wrd_wnd.height );
	wrd_wnd_unlock();
	wrd_graphic_apply ( &rc, w32g_wrd_wnd.index_display, TRUE );
	wrd_graphic_update ( &rc, TRUE );
#if 1
	InvalidateRect ( w32g_wrd_wnd.hwnd, &rc, FALSE );
#else
	{ // パレットの変化で全画面を更新しないで済むようにチェックしてみる。けど、重いので不採用。
#define BITS_DIV 10
		int j;
		char *bits;
		int mod_pal[W32G_WRDWND_GRAPHIC_PALLETE_MAX];
		char bits_use_pal[BITS_DIV][BITS_DIV];
		for ( i = 0; i < W32G_WRDWND_GRAPHIC_PALLETE_MAX; i++ ) {
			if ( memcmp ( &w32g_wrd_wnd.gpal_buff[w32g_wrd_wnd.fade_from][i],
				&w32g_wrd_wnd.gpal_buff[w32g_wrd_wnd.fade_to][i], sizeof (RGBQUAD) ) != 0 )
				mod_pal[i] = 1;
			else
				mod_pal[i] = 0;
		}
		bits = w32g_wrd_wnd.graphic_dib[w32g_wrd_wnd.index_display]->bits;
		for ( i = 0; i < BITS_DIV; i ++ ) {
			for ( j = 0; j < BITS_DIV; j ++ ) {
				bits_use_pal[i][j] = 0;
			}
		}
		for ( i = 0; i < 640; i ++ ) {
			for ( j = 0; j < 400; j ++ ) {
				char c = bits [ j * w32g_wrd_wnd.width + i ];
				if ( c >= 0 && c <= 0x0F && mod_pal[ c ] )
					bits_use_pal[BITS_DIV*i/640][BITS_DIV*j/400] = 1;
			}
		}
		for ( i = 0; i < BITS_DIV; i ++ ) {
			for ( j = 0; j < BITS_DIV; j ++ ) {
				if ( bits_use_pal[i][j] )
					SetRect ( &rc, i * 640/BITS_DIV, j * 400/BITS_DIV, ( i + 1) * 640/BITS_DIV, ( j + 1) * 400/BITS_DIV );
					InvalidateRect ( w32g_wrd_wnd.hwnd, &rc, FALSE );
			}
		}
	}
#endif
}

void wrd_graphic_gmode ( int sw )
{
	w32g_wrd_wnd.gmode = sw;
}

void wrd_graphic_gmove ( int x1, int y1, int x2, int y2, int xd, int yd, int vs, int vd, int sw )
{
	int x, y;

	if ( WrdWndInfo.GraphicStop ) return;
	if ( !w32g_wrd_wnd.active ) return;
	if ( vs < 0 || vs >= 2 || vd < 0 || vd >= 2 ) return;
	if ( x1 < 0 || x1 >= w32g_wrd_wnd.width ) return;
	if ( y1 < 0 || y1 >= w32g_wrd_wnd.height ) return;
	if ( xd < 0 || xd >= w32g_wrd_wnd.width ) return;
	if ( yd < 0 || yd >= w32g_wrd_wnd.height ) return;
	wrd_wnd_lock();
	GdiFlush ();
	x1 = ( ( x1 + 7 ) / 8 ) * 8;
	x2 = ( ( x2 + 7 ) / 8 ) * 8;
	xd = ( ( xd + 7 ) / 8 ) * 8;
	if ( xd + x2 - x1 >= w32g_wrd_wnd.width ) {
		int d =  w32g_wrd_wnd.width - 1 - xd;
		x2 = x1 + d; 
	}
	if ( yd + y2 - y1 >= w32g_wrd_wnd.height ) {
		int d =  w32g_wrd_wnd.height - 1 - yd;
		y2 = y1 + d; 
	}
	switch ( sw ) {
	default:
	case 0:	// COPY
		for ( y = y1; y <= y2; y ++ ) {
			int i_src = y * w32g_wrd_wnd.width;
			int i_dest = (yd + y - y1) * w32g_wrd_wnd.width;
			if ( w32g_wrd_wnd.gmode >= 0x0F ) {
				memcpy ( &w32g_wrd_wnd.graphic_dib[vd]->bits[i_dest + xd],
					&w32g_wrd_wnd.graphic_dib[vs]->bits[i_src + x1], x2 - x1 + 1 );
			} else {
				for ( x = x1; x <= x2; x ++ ) {
					int i_dest_tmp = i_dest + xd + x - x1;
					int i_src_tmp = i_src + x;
					w32g_wrd_wnd.graphic_dib[vd]->bits[i_dest_tmp]
						= w32g_wrd_wnd.graphic_dib[vs]->bits[i_src_tmp] & w32g_wrd_wnd.gmode;
				}
			}
		}
		break;
	case 1:	// SWAP
		for ( y = y1; y <= y2; y ++ ) {
			int i_src = y * w32g_wrd_wnd.width;
			int i_dest = (yd + y - y1) * w32g_wrd_wnd.width;
			if ( w32g_wrd_wnd.gmode >= 0x0F ) {
				char buff[640+1];
				int d = x2 - x1 + 1;
				memcpy ( buff, &w32g_wrd_wnd.graphic_dib[vd]->bits[i_dest + xd], d );
				memcpy ( &w32g_wrd_wnd.graphic_dib[vd]->bits[i_dest + xd],
					&w32g_wrd_wnd.graphic_dib[vs]->bits[i_src + x1], d );
				memcpy ( &w32g_wrd_wnd.graphic_dib[vs]->bits[i_src + x1], buff, d );
			} else {
				for ( x = x1; x <= x2; x ++ ) {
					int i_dest_tmp = i_dest + xd + x - x1;
					int i_src_tmp = i_src + x;
					char t = w32g_wrd_wnd.graphic_dib[vd]->bits[i_dest_tmp];
					w32g_wrd_wnd.graphic_dib[vd]->bits[i_dest_tmp]
						= ( w32g_wrd_wnd.graphic_dib[vd]->bits[i_dest_tmp] & ~w32g_wrd_wnd.gmode )
							| ( w32g_wrd_wnd.graphic_dib[vs]->bits[i_src_tmp] & w32g_wrd_wnd.gmode );
					w32g_wrd_wnd.graphic_dib[vs]->bits[i_src_tmp]
						= ( w32g_wrd_wnd.graphic_dib[vs]->bits[i_src_tmp]  & ~w32g_wrd_wnd.gmode )
							| t & w32g_wrd_wnd.gmode;
				}
			}
		}
		break;
	}
	wrd_wnd_unlock();
	if ( w32g_wrd_wnd.index_active == w32g_wrd_wnd.index_display ) {
		RECT rc;
		SetRect ( &rc, xd, yd, xd + x2 - x1, yd + y2 - y1 );
		wrd_graphic_apply ( &rc, w32g_wrd_wnd.index_display, TRUE );
		wrd_graphic_update ( &rc, TRUE );
		InvalidateRect ( w32g_wrd_wnd.hwnd, &rc, FALSE );
	}
}

void wrd_graphic_mag ( char *path, int x, int y, int s, int p )
{
	int x_orig = x, y_orig = y;
	int size = w32g_wrd_wnd.width * w32g_wrd_wnd.height;
	magdata *mh;
	int width, height;

	if ( WrdWndInfo.GraphicStop ) return;
	if ( !w32g_wrd_wnd.active ) return;
	mh = mag_create ( path );
	if ( x_orig == WRD_NOARG )
		x_orig = 0;
	if ( y_orig == WRD_NOARG )
		y_orig = 0;
	if ( mh == NULL )
		return;
	width = mh->xend - mh->xorig + 1;
	height = mh->yend - mh->yorig + 1;
	if (MAG_WORK_WIDTH < width )
		return;
	if ( MAG_WORK_HEIGHT < height )
		return;
	if ( s <= 0 )
		return;
	wrd_wnd_lock();
	GdiFlush ();
	if ( wrd_graphic_pal_init_flag == 0 ) { /* MIMPI BUG ? */
		if ( p == 1 ) p = 0;
		wrd_graphic_pal_init_flag = 1;
	}
	if ( p == 0 || p == 1 ) {
		if ( s == 1 && x_orig == 0 && y_orig == 0 && width <= w32g_wrd_wnd.width && height <= w32g_wrd_wnd.height ) {
			mag_load_pixel ( w32g_wrd_wnd.graphic_dib[w32g_wrd_wnd.index_active]->bits,
				w32g_wrd_wnd.width, w32g_wrd_wnd.height, mh );
		} else {
#if 1
			mag_load_pixel ( w32g_wrd_wnd.bits_mag_work,
				MAG_WORK_WIDTH, MAG_WORK_HEIGHT, mh );
			for ( y = 0; y < height; y ++ ) {
				int dest_index = (y_orig + y/s) * w32g_wrd_wnd.width;
				int src_index = y * MAG_WORK_WIDTH;
				long v[MAG_WORK_WIDTH];
				for ( x = 0; x < width; x ++ )
					v[x] = 0;
				for ( x = 0; x < width; x ++ ) {
					v[x/s] += w32g_wrd_wnd.bits_mag_work[ src_index + x ];
				}
				for ( x = 0; x < width; x ++ ) {
					if ( v[x/s] >= 0 ) {
						int i_tmp = dest_index + x_orig + x/s;
						if ( i_tmp < MAG_WORK_WIDTH * MAG_WORK_HEIGHT )
							w32g_wrd_wnd.graphic_dib[w32g_wrd_wnd.index_active]->bits[i_tmp] = ( v[x/s] / s / s ) & 0x0F;
						v[x/s] = -1;
					}
				}
			}
			width /= s;
			height /= s;
#else
			mag_load_pixel ( w32g_wrd_wnd.bits_mag_work,
				MAG_WORK_WIDTH, MAG_WORK_HEIGHT, mh );
			for ( y = 0; y < height; y ++ ) {
				for ( x = 0; x < width; x ++ ) {
					w32g_wrd_wnd.graphic_dib[w32g_wrd_wnd.index_active]->bits[(y_orig + y) * w32g_wrd_wnd.width + x_orig + x]
						= w32g_wrd_wnd.bits_mag_work[ y * MAG_WORK_WIDTH + x];
				}
			}
#endif
		}
	}
	wrd_wnd_unlock();
	wrd_graphic_pal_g4r4b4 ( 17, mh->pal, 16 );
	if ( w32g_wrd_wnd.index_active == 0 )
		wrd_graphic_pal_g4r4b4 ( 18, mh->pal, 16 );
	if ( w32g_wrd_wnd.index_active == 1 )
		wrd_graphic_pal_g4r4b4 ( 19, mh->pal, 16 );
	if ( p == 0 || p == 2 ) {
		wrd_graphic_pal_g4r4b4 ( 0, mh->pal, 16 );
	} else {
		// wrd_graphic_pal_g4r4b4() を実行しないと領域が更新されない。
		if ( w32g_wrd_wnd.index_active == w32g_wrd_wnd.index_display ) {
			RECT rc;
			SetRect ( &rc, x_orig, y_orig, width, height );
			wrd_graphic_apply ( &rc, w32g_wrd_wnd.index_display, TRUE );
			wrd_graphic_update ( &rc, TRUE );
			InvalidateRect ( w32g_wrd_wnd.hwnd, &rc, FALSE );
		}
	}
}

void wrd_text_ton ( int sw )
{
	int sw_old = w32g_wrd_wnd.flag & WRD_FLAG_TEXT;

	if ( !w32g_wrd_wnd.active ) return;
	if ( sw && !sw_old ) {
		w32g_wrd_wnd.flag |= WRD_FLAG_TEXT;		// 先に設定
		if ( w32g_wrd_wnd.index_active == w32g_wrd_wnd.index_display ) {
			wrd_text_update ( 0, 0, w32g_wrd_wnd.width, w32g_wrd_wnd.height, TRUE );
		}
	} else if ( !sw && sw_old ) {
		if ( w32g_wrd_wnd.index_active == w32g_wrd_wnd.index_display ) {
			wrd_text_update ( 0, 0, w32g_wrd_wnd.width, w32g_wrd_wnd.height, TRUE );
		}
	}
	if ( sw )
		w32g_wrd_wnd.flag |= WRD_FLAG_TEXT;
	else
		w32g_wrd_wnd.flag &= ~WRD_FLAG_TEXT;
}

void wrd_text_scroll ( int x1, int y1, int x2, int y2, int mode, int color, int c )
{
	int x, y;
	x1--; x2--; y1--; y2--;
	if ( !w32g_wrd_wnd.active ) return;
	if ( x1 > x2 ) return;
	if ( y1 > y2 ) return;
	if ( x1 < 0 ) x1 = 0; if ( x2 < 0 ) x2 = 0;
	if ( x1 >= w32g_wrd_wnd.row ) x1 = w32g_wrd_wnd.row - 1;
	if ( x2 >= w32g_wrd_wnd.row ) x2 = w32g_wrd_wnd.row - 1;
	if ( y1 < 0 ) y1 = 0; if ( y2 < 0 ) y2 = 0;
	if ( y1 >= w32g_wrd_wnd.row ) y1 = w32g_wrd_wnd.col - 1;
	if ( y2 >= w32g_wrd_wnd.row ) y2 = w32g_wrd_wnd.col - 1;
	wrd_wnd_lock();
	if ( mode == 0 ) {
		int dx = x2 - x1 + 1;
		for ( y = y1+1; y <= y2; y ++ ) {
			memcpy ( &w32g_wrd_wnd.textbuf [ y - 1 ][x1], 
				&w32g_wrd_wnd.textbuf [ y ][x1], dx );
			memcpy ( &w32g_wrd_wnd.forecolorbuf [ y - 1 ][x1], 
				&w32g_wrd_wnd.forecolorbuf [ y ][x1], dx );
			memcpy ( &w32g_wrd_wnd.backcolorbuf [ y - 1 ][x1], 
				&w32g_wrd_wnd.backcolorbuf [ y ][x1], dx );
			memcpy ( &w32g_wrd_wnd.attrbuf [ y - 1 ][x1], 
				&w32g_wrd_wnd.attrbuf [ y ][x1], dx );
		}
		WrdWndCurStateSaveAndRestore ( 1 );
		WrdWndSetAttr98 ( color );
		for ( x = x1; x <= x2; x ++ ) {
			w32g_wrd_wnd.textbuf [ y2][x] = c;
			w32g_wrd_wnd.forecolorbuf [ y2][x] = w32g_wrd_wnd.curforecolor;
			w32g_wrd_wnd.backcolorbuf [ y2][x] = w32g_wrd_wnd.curbackcolor;
			w32g_wrd_wnd.attrbuf [ y2][x] = w32g_wrd_wnd.curattr;
		}
		WrdWndCurStateSaveAndRestore ( 0 );
	} else if ( mode == 1 ) {
		int dx = x2 - x1 + 1;
		for ( y = y2 - 1; y >= y1; y -- ) {
			memcpy ( &w32g_wrd_wnd.textbuf [ y + 1 ][x1], 
				&w32g_wrd_wnd.textbuf [ y ][x1], dx );
			memcpy ( &w32g_wrd_wnd.forecolorbuf [ y + 1 ][x1], 
				&w32g_wrd_wnd.forecolorbuf [ y ], dx );
			memcpy ( &w32g_wrd_wnd.backcolorbuf [ y + 1 ][x1], 
				&w32g_wrd_wnd.backcolorbuf [ y ][x1], dx );
			memcpy ( &w32g_wrd_wnd.attrbuf [ y + 1 ][x1], 
				&w32g_wrd_wnd.attrbuf [ y ][x1], dx );
		}
		WrdWndCurStateSaveAndRestore ( 1 );
		WrdWndSetAttr98 ( color );
		for ( x = x1; x <= x2; x ++ ) {
			w32g_wrd_wnd.textbuf [ y1][x] = c;
			w32g_wrd_wnd.forecolorbuf [ y1][x] = w32g_wrd_wnd.curforecolor;
			w32g_wrd_wnd.backcolorbuf [ y1][x] = w32g_wrd_wnd.curbackcolor;
			w32g_wrd_wnd.attrbuf [ y1][x] = w32g_wrd_wnd.curattr;
		}
		WrdWndCurStateSaveAndRestore ( 0 );
	} else if ( mode == 2 ) {
		for ( y = y1; y <= y2; y ++ ) {
			for ( x = x1+1; x <= x2; x ++ ) {
				w32g_wrd_wnd.textbuf [ y ][ x ] 
					= w32g_wrd_wnd.textbuf [ y ][ x - 1];
				w32g_wrd_wnd.forecolorbuf[ y ][ x ] 
					= w32g_wrd_wnd.forecolorbuf [ y ][ x - 1];
				w32g_wrd_wnd.backcolorbuf [ y ][ x ] 
					= w32g_wrd_wnd.backcolorbuf [ y ][ x - 1];
				w32g_wrd_wnd.attrbuf [ y ][ x ] 
					= w32g_wrd_wnd.attrbuf [ y ][ x - 1];
			}
		}
		WrdWndCurStateSaveAndRestore ( 1 );
		WrdWndSetAttr98 ( color );
		for ( y = y1; y <= y2; y ++ ) {
			w32g_wrd_wnd.textbuf [ y ][ x1] = c;
			w32g_wrd_wnd.forecolorbuf [ y ][ x1] = w32g_wrd_wnd.curforecolor;
			w32g_wrd_wnd.backcolorbuf [ y ][ x1] = w32g_wrd_wnd.curbackcolor;
			w32g_wrd_wnd.attrbuf [ y ][ x1] = w32g_wrd_wnd.curattr;
		}
		WrdWndCurStateSaveAndRestore ( 0 );
	} else if ( mode == 3 ) {
		for ( y = y1; y <= y2; y ++ ) {
			for ( x = x2 - 1; x >= x1; x -- ) {
				w32g_wrd_wnd.textbuf [ y ][ x ] 
					= w32g_wrd_wnd.textbuf [ y ][ x + 1];
				w32g_wrd_wnd.forecolorbuf[ y ][ x ] 
					= w32g_wrd_wnd.forecolorbuf [ y ][ x + 1];
				w32g_wrd_wnd.backcolorbuf [ y ][ x ] 
					= w32g_wrd_wnd.backcolorbuf [ y ][ x + 1];
				w32g_wrd_wnd.attrbuf [ y ][ x ] 
					= w32g_wrd_wnd.attrbuf [ y ][ x + 1];
			}
		}
		WrdWndCurStateSaveAndRestore ( 1 );
		WrdWndSetAttr98 ( color );
		for ( y = y1; y <= y2; y ++ ) {
			w32g_wrd_wnd.textbuf [ y ][ x2] = c;
			w32g_wrd_wnd.forecolorbuf [ y ][ x2] = w32g_wrd_wnd.curforecolor;
			w32g_wrd_wnd.backcolorbuf [ y ][ x2] = w32g_wrd_wnd.curbackcolor;
			w32g_wrd_wnd.attrbuf [ y ][ x2] = w32g_wrd_wnd.curattr;
		}
		WrdWndCurStateSaveAndRestore ( 0 );
	}
	wrd_wnd_unlock();
	wrd_text_update ( x1, y1, x2, y2, TRUE );
}

void wrd_start_skip ( void )
{
	wrd_wnd_lock();
	w32g_wrd_wnd.draw_skip = 1;
	wrd_wnd_unlock();
}

void wrd_end_skip ( void )
{
	w32g_wrd_wnd.draw_skip = 0;
	wrd_text_update ( 0, 0, w32g_wrd_wnd.width, w32g_wrd_wnd.height, TRUE );
}

void wrd_graphic_xcopy ( int sx1, int sy1, int sx2, int sy2, int tx, int ty, int ss, int ts, int method,
	 int opt1, int opt2, int opt3, int opt4, int opt5 )
{
#if 0
	int x, y, d, size = w32g_wrd_wnd.width * w32g_wrd_wnd.height;

	if ( WrdWndInfo.GraphicStop ) return;
	if ( !w32g_wrd_wnd.active ) return;

	if ( ss < 0 || ss >= 2 || ts < 0 || ts >= 2 ) return;
	if ( sx1 < 0 || sx1 >= w32g_wrd_wnd.width ) return;
	if ( sy1 < 0 || sy1 >= w32g_wrd_wnd.height ) return;
	if ( tx < 0 || tx >= w32g_wrd_wnd.width ) return;
	if ( ty < 0 || ty >= w32g_wrd_wnd.height ) return;
	wrd_wnd_lock();
	GdiFlush ();
	if ( tx + sx2 - sx1 >= w32g_wrd_wnd.width ) {
		d =  w32g_wrd_wnd.width - 1 - tx;
		sx2 = sx1 + d; 
	}
	if ( ty + sy2 - sy1 >= w32g_wrd_wnd.height ) {
		d =  w32g_wrd_wnd.height - 1 - ty;
		sy2 = sy1 + d; 
	}
	switch ( method ) {
	case 0:	// COPY
		d = sx2 - sx1 + 1;
		for ( y = sy1; y <= sy2; y ++ ) {
			int i_src = y * w32g_wrd_wnd.width;
			int i_dest = ( ty + y - sy1) * w32g_wrd_wnd.width;
			memcpy ( &w32g_wrd_wnd.graphic_dib[ts]->bits[i_dest + tx],
				&w32g_wrd_wnd.graphic_dib[ss]->bits[i_src + sx1], d );
		}
		break;
	case 1:	// COPY EXCEPT 0
		for ( y = sy1; y <= sy2; y ++ ) {
			int i_src = y * w32g_wrd_wnd.width;
			int i_dest = ( ty + y - sy1) * w32g_wrd_wnd.width;
			for ( x = sx1; x <= sx2; x ++ ) {
				int i_src_tmp = i_src + x;
				int i_dest_tmp = i_dest + tx + x - sx1;
				char c = w32g_wrd_wnd.graphic_dib[ss]->bits[i_src_tmp];
				if ( c != 0 )
					w32g_wrd_wnd.graphic_dib[ts]->bits[i_dest_tmp] = c;
			}
		}
		break;
	case 2:	// XOR
		for ( y = sy1; y <= sy2; y ++ ) {
			int i_src = y * w32g_wrd_wnd.width;
			int i_dest = ( ty + y - sy1) * w32g_wrd_wnd.width;
			for ( x = sx1; x <= sx2; x ++ ) {
				int i_src_tmp = i_src + x;
				int i_dest_tmp = i_dest + tx + x - sx1;
				w32g_wrd_wnd.graphic_dib[ts]->bits[i_dest_tmp] ^= w32g_wrd_wnd.graphic_dib[ss]->bits[i_src_tmp];
			}
		}
		break;
	case 3:	// AND
		for ( y = sy1; y <= sy2; y ++ ) {
			int i_src = y * w32g_wrd_wnd.width;
			int i_dest = ( ty + y - sy1) * w32g_wrd_wnd.width;
			for ( x = sx1; x <= sx2; x ++ ) {
				int i_src_tmp = i_src + x;
				int i_dest_tmp = i_dest + tx + x - sx1;
				w32g_wrd_wnd.graphic_dib[ts]->bits[i_dest_tmp] &= w32g_wrd_wnd.graphic_dib[ss]->bits[i_src_tmp];
			}
		}
		break;
	case 4: // OR
		for ( y = sy1; y <= sy2; y ++ ) {
			int i_src = y * w32g_wrd_wnd.width;
			int i_dest = ( ty + y - sy1) * w32g_wrd_wnd.width;
			for ( x = sx1; x <= sx2; x ++ ) {
				int i_src_tmp = i_src + x;
				int i_dest_tmp = i_dest + tx + x - sx1;
				w32g_wrd_wnd.graphic_dib[ts]->bits[i_dest_tmp] |= w32g_wrd_wnd.graphic_dib[ss]->bits[i_src_tmp];
			}
		}
		break;
	case 5:	// X REVERSE
		for ( y = sy1; y <= sy2; y ++ ) {
			int i_src = y * w32g_wrd_wnd.width;
			int i_dest = ( ty + y - sy1) * w32g_wrd_wnd.width;
			for ( x = sx1; x <= sx2; x ++ ) {
				int i_src_tmp = i_src + x;
				int i_dest_tmp = i_dest + tx + sx2 - x;
				w32g_wrd_wnd.graphic_dib[ts]->bits[i_dest_tmp] |= w32g_wrd_wnd.graphic_dib[ss]->bits[i_src_tmp];
			}
		}
		break;
	case 6:	// Y REVERSE
		d = sx2 - sx1 + 1;
		for ( y = sy1; y <= sy2; y ++ ) {
			int i_src = ( sy1 + sy2 - y ) * w32g_wrd_wnd.width;
			int i_dest = ( ty + y - sy1) * w32g_wrd_wnd.width;
			memcpy ( &w32g_wrd_wnd.graphic_dib[ts]->bits[i_dest + tx],
				&w32g_wrd_wnd.graphic_dib[ss]->bits[i_src + sx1], d );
		}
		break;
	case 7:	// X Y REVERSE
		for ( y = sy1; y <= sy2; y ++ ) {
			int i_src = ( sy1 + sy2 - y ) * w32g_wrd_wnd.width;
			int i_dest = ( ty + y - sy1) * w32g_wrd_wnd.width;
			for ( x = sx1; x <= sx2; x ++ ) {
				int i_src_tmp = i_src + x;
				int i_dest_tmp = i_dest + tx + sx2 - x;
				w32g_wrd_wnd.graphic_dib[ts]->bits[i_dest_tmp] |= w32g_wrd_wnd.graphic_dib[ss]->bits[i_src_tmp];
			}
		}
		break;
	case 8:	// COPY2
		for ( y = sy1; y <= sy2; y ++ ) {
			int i_src = y * w32g_wrd_wnd.width;
			int i_dest = ( ty + y - sy1) * w32g_wrd_wnd.width;
			for ( x = sx1; x <= sx2; x ++ ) {
				int i_src_tmp = i_src + x;
				int i_dest_tmp = i_dest + tx + x - sx1;
				char c = w32g_wrd_wnd.graphic_dib[ss]->bits[i_src_tmp];
				if ( c != 0 )
					w32g_wrd_wnd.graphic_dib[ts]->bits[i_dest_tmp] = c;
				else {
					i_src_tmp = ( opt2 + y - sy1) * w32g_wrd_wnd.width + opt1 + x - sx1;
					if ( 0 <= i_src_tmp && i_src_tmp < size )
						w32g_wrd_wnd.graphic_dib[ts]->bits[i_dest_tmp] = w32g_wrd_wnd.graphic_dib[ss]->bits[i_src_tmp];
				}
			}
		}
		break;
	case 9:	// ちょっとわからなかった。
		break;
	case 10:	// COPY opt1, opt2
		if ( opt1 < 0 || opt2 < 0 )
			break;
		d = sx2 - sx1 + 1;
		for ( y = sy1; y <= sy2; y ++ ) {
			int i_src, i_dest;
			if ( (y - sy1) % ( opt1 + opt2 ) >= opt2 )
				continue;
			i_src = y * w32g_wrd_wnd.width;
			i_dest = ( ty + y - sy1) * w32g_wrd_wnd.width;
			memcpy ( &w32g_wrd_wnd.graphic_dib[ts]->bits[i_dest + tx],
				&w32g_wrd_wnd.graphic_dib[ss]->bits[i_src + sx1], d );
		}
		break;
	case 11:	// Clipping Copy	ふつうのコピーで代用。
		d = sx2 - sx1 + 1;
		for ( y = sy1; y <= sy2; y ++ ) {
			int i_src = y * w32g_wrd_wnd.width;
			int i_dest = ( ty + y - sy1) * w32g_wrd_wnd.width;
			memcpy ( &w32g_wrd_wnd.graphic_dib[ts]->bits[i_dest + tx],
				&w32g_wrd_wnd.graphic_dib[ss]->bits[i_src + sx1], d );
		}
		break;
	case 12: // PLANE COPY
		if ( opt1 < 0 || opt1 >= 4 || opt2 < 0 || opt2 >= 4 )
			break;
		for ( y = sy1; y <= sy2; y ++ ) {
			int i_src = y * w32g_wrd_wnd.width;
			int i_dest = ( ty + y - sy1) * w32g_wrd_wnd.width;
			for ( x = sx1; x <= sx2; x ++ ) {
				int i_src_tmp = i_src + x;
				int i_dest_tmp = i_dest + tx + x - sx1;
				char c = w32g_wrd_wnd.graphic_dib[ss]->bits[i_src_tmp];
				c = ( ( c & ( 1 << opt1) ) >> opt1 ) << opt2;
				w32g_wrd_wnd.graphic_dib[ts]->bits[i_dest_tmp] = c;
			}
		}
		break;
	default:
		break;
	}
	wrd_wnd_unlock();
	if ( w32g_wrd_wnd.index_active == w32g_wrd_wnd.index_display ) {
		RECT rc;
		SetRect ( &rc, tx, ty, tx + sx2 - sx1, ty + sy2 - sy1 );
		wrd_graphic_apply ( &rc, w32g_wrd_wnd.index_display, TRUE );
		wrd_graphic_update ( &rc, TRUE );
		InvalidateRect ( w32g_wrd_wnd.hwnd, &rc, FALSE );
	}
#endif
}

void WrdWndReset(void)
{
	int i;
	wrd_wnd_lock();
	w32g_wrd_wnd.curposx = 0;
	w32g_wrd_wnd.curposy = 0;
	w32g_wrd_wnd.curforecolor = W32G_WRDWND_WHITE;
	w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
	w32g_wrd_wnd.curattr = 0;
	for ( i = 0; i < w32g_wrd_wnd.col; i++ ) {
		memset ( w32g_wrd_wnd.textbuf[i], 0x20, w32g_wrd_wnd.row );
		memset ( w32g_wrd_wnd.forecolorbuf[i], w32g_wrd_wnd.curforecolor, w32g_wrd_wnd.row );
		memset ( w32g_wrd_wnd.backcolorbuf[i], w32g_wrd_wnd.curbackcolor, w32g_wrd_wnd.row );
		memset ( w32g_wrd_wnd.attrbuf[i], w32g_wrd_wnd.curattr, w32g_wrd_wnd.row);
	}
	wrd_graphic_pal_init_flag = 0;
	wrd_wnd_unlock();
	wrd_text_update ( 0, 0, w32g_wrd_wnd.row - 1, w32g_wrd_wnd.col - 1, TRUE );
	wrd_graphic_ginit ();
}

void WrdWndCurStateSaveAndRestore(int saveflag)
{
	static int saved_curposx = 0;
	static int saved_curposy = 0;
	static int saved_curforecolor = W32G_WRDWND_WHITE;
	static int saved_curbackcolor = W32G_WRDWND_BLACK;
	static int saved_curattr = 0;
	if ( saveflag ) {
		saved_curforecolor = w32g_wrd_wnd.curforecolor;
		saved_curbackcolor = w32g_wrd_wnd.curbackcolor;
		saved_curattr = w32g_wrd_wnd.curattr;
		saved_curposx = w32g_wrd_wnd.curposx;
		saved_curposy = w32g_wrd_wnd.curposy;
	} else {
		w32g_wrd_wnd.curforecolor = saved_curforecolor;
		w32g_wrd_wnd.curbackcolor = saved_curbackcolor;
		w32g_wrd_wnd.curattr = saved_curattr;
		WrdWndGoto( saved_curposx, saved_curposy );
	}
}

// from_from 行から from_to 行までを to_from 行を先頭にコピー。
void WrdWndCopyLineS ( int from_from, int from_to, int to_from, int lockflag )
{
	int y, to_to;
	if ( !w32g_wrd_wnd.active ) return;
	if ( to_from >= w32g_wrd_wnd.col ) return;
	if ( to_from == from_from ) return;
	if ( from_to < from_from ) return;
	if ( to_from < 0 ) {
		from_from -= to_from;
		from_to -= to_from;
		to_from = 0;
	}
	to_to = to_from + from_to - from_from;
	if ( to_to >= w32g_wrd_wnd.col )
		to_to = w32g_wrd_wnd.col - 1;
	if ( lockflag ) wrd_wnd_lock();
	if ( to_from < from_from ) {
		for ( y = to_from; y <= to_to; y++ ) {
			int y_from = from_from + y - to_from;
			if ( y_from < 0 || y_from >= w32g_wrd_wnd.col ) {
				memset ( w32g_wrd_wnd.textbuf[y], 0x20, w32g_wrd_wnd.row );
				memset ( w32g_wrd_wnd.forecolorbuf[y], W32G_WRDWND_BLACK, w32g_wrd_wnd.row );
				memset ( w32g_wrd_wnd.backcolorbuf[y], W32G_WRDWND_BLACK, w32g_wrd_wnd.row );
				memset ( w32g_wrd_wnd.attrbuf[y], 0, w32g_wrd_wnd.row);
			} else {
				memcpy ( w32g_wrd_wnd.textbuf[y], w32g_wrd_wnd.textbuf[y_from], w32g_wrd_wnd.row );
				memcpy ( w32g_wrd_wnd.forecolorbuf[y], w32g_wrd_wnd.forecolorbuf[y_from], w32g_wrd_wnd.row );
				memcpy ( w32g_wrd_wnd.backcolorbuf[y], w32g_wrd_wnd.backcolorbuf[y_from], w32g_wrd_wnd.row );
				memcpy ( w32g_wrd_wnd.attrbuf[y], w32g_wrd_wnd.attrbuf[y_from], w32g_wrd_wnd.row );
			}
		}
	} else {
		for ( y = to_to; y >= to_from; y-- ) {
			int y_from = from_from + y - to_from;
			if ( y_from < 0 || y_from >= w32g_wrd_wnd.col ) {
				memset ( w32g_wrd_wnd.textbuf[y], 0x20, w32g_wrd_wnd.row );
				memset ( w32g_wrd_wnd.forecolorbuf[y], W32G_WRDWND_BLACK, w32g_wrd_wnd.row );
				memset ( w32g_wrd_wnd.backcolorbuf[y], W32G_WRDWND_BLACK, w32g_wrd_wnd.row );
				memset ( w32g_wrd_wnd.attrbuf[y], 0, w32g_wrd_wnd.row);
			} else {
				memcpy ( w32g_wrd_wnd.textbuf[y], w32g_wrd_wnd.textbuf[y_from], w32g_wrd_wnd.row );
				memcpy ( w32g_wrd_wnd.forecolorbuf[y], w32g_wrd_wnd.forecolorbuf[y_from], w32g_wrd_wnd.row );
				memcpy ( w32g_wrd_wnd.backcolorbuf[y], w32g_wrd_wnd.backcolorbuf[y_from], w32g_wrd_wnd.row );
				memcpy ( w32g_wrd_wnd.attrbuf[y], w32g_wrd_wnd.attrbuf[y_from], w32g_wrd_wnd.row );
			}
		}
	}
	if ( lockflag ) wrd_wnd_unlock();
	wrd_text_update ( 0, to_from, w32g_wrd_wnd.row - 1, to_to, lockflag );
}

// from 行を to 行にコピー。
void WrdWndCopyLine ( int from, int to, int lockflag )
{
	if ( !w32g_wrd_wnd.active ) return;
	WrdWndCopyLineS ( from, from, to, lockflag );
}

// from行から to 行までクリア
void WrdWndClearLineFromTo(int from, int to, int lockflag)
{
	int i;
	if ( !w32g_wrd_wnd.active ) return;
	if ( from < 0 ) from = 0;
	if ( from >= w32g_wrd_wnd.col ) from = w32g_wrd_wnd.col - 1;
	if ( to < 0 ) to = 0;
	if ( to >= w32g_wrd_wnd.col ) to = w32g_wrd_wnd.col - 1;
	if ( to < from ) return;
	if ( lockflag ) wrd_wnd_lock();
	for ( i = from; i <= to; i++ ) {
		memset(w32g_wrd_wnd.textbuf[i], 0x20, w32g_wrd_wnd.row);
		memset(w32g_wrd_wnd.forecolorbuf[i], W32G_WRDWND_BLACK, w32g_wrd_wnd.row);
		memset(w32g_wrd_wnd.backcolorbuf[i], W32G_WRDWND_BLACK, w32g_wrd_wnd.row);
		memset(w32g_wrd_wnd.attrbuf[i], 0, w32g_wrd_wnd.row);
	}
	if ( lockflag ) wrd_wnd_unlock();
	wrd_text_update ( 0, from, w32g_wrd_wnd.row - 1, to, lockflag );
}

// from 行を to 行に移動。
void WrdWndMoveLine(int from, int to, int lockflag)
{
	if ( !w32g_wrd_wnd.active ) return;
	if ( from == to ) return;
	if ( from < 0 || from >= w32g_wrd_wnd.col ) return;
	if ( to < 0 || to >= w32g_wrd_wnd.col ) return;
	WrdWndCopyLine ( from, to, lockflag );
	WrdWndClearLineFromTo ( from,from, lockflag );
}

// スクロールダウンする。
void WrdWndScrollDown(int lockflag)
{
	if ( !w32g_wrd_wnd.active ) return;
	WrdWndCopyLineS ( 0, w32g_wrd_wnd.col - 1, 1, lockflag );
	WrdWndClearLineFromTo ( 0, 0, lockflag );
}

// スクロールアップする。
void WrdWndScrollUp(int lockflag)
{
	if ( !w32g_wrd_wnd.active ) return;
	WrdWndCopyLineS ( 1, w32g_wrd_wnd.col - 1, 0, lockflag );
	WrdWndClearLineFromTo ( w32g_wrd_wnd.col - 1, w32g_wrd_wnd.col - 1, lockflag );
}

// 画面消去
void WrdWndClear(int lockflag)
{
	if ( !w32g_wrd_wnd.active ) return;
	WrdWndClearLineFromTo ( 0, w32g_wrd_wnd.col - 1, lockflag );
}

// 文字出力
void WrdWndPutString(char *str, int lockflag)
{
	if ( !w32g_wrd_wnd.active ) return;
	WrdWndPutStringN(str, strlen(str),lockflag);
}

// 文字出力(n文字)
void WrdWndPutStringN(char *str, int n, int lockflag)
{
	int i;

	if ( !w32g_wrd_wnd.active ) return;
	if ( lockflag ) wrd_wnd_lock();
	for(;;){
		if ( w32g_wrd_wnd.curposx + n <= w32g_wrd_wnd.row ) {
			memcpy( &w32g_wrd_wnd.textbuf[w32g_wrd_wnd.curposy][w32g_wrd_wnd.curposx], str, n );
			memset( &w32g_wrd_wnd.forecolorbuf[w32g_wrd_wnd.curposy][w32g_wrd_wnd.curposx],
				w32g_wrd_wnd.curforecolor, n );
			memset( &w32g_wrd_wnd.backcolorbuf[w32g_wrd_wnd.curposy][w32g_wrd_wnd.curposx],
				w32g_wrd_wnd.curbackcolor, n );
			memset( &w32g_wrd_wnd.attrbuf[w32g_wrd_wnd.curposy][w32g_wrd_wnd.curposx],
				w32g_wrd_wnd.curattr, n );
			if ( lockflag ) wrd_wnd_unlock();
			wrd_text_update ( w32g_wrd_wnd.curposx, w32g_wrd_wnd.curposy,
				w32g_wrd_wnd.curposx + n - 1, w32g_wrd_wnd.curposy, lockflag );
			if ( lockflag ) wrd_wnd_lock();
			w32g_wrd_wnd.curposx += n;
			if ( w32g_wrd_wnd.curposx >= w32g_wrd_wnd.row ) {
				w32g_wrd_wnd.curposx = 0;
				w32g_wrd_wnd.curposy++;
				if ( w32g_wrd_wnd.curposy >= w32g_wrd_wnd.col ) {
					if ( lockflag ) wrd_wnd_unlock();
					WrdWndScrollUp ( lockflag );
					if ( lockflag ) wrd_wnd_lock();
					w32g_wrd_wnd.curposy = w32g_wrd_wnd.col - 1;
				}
			}
			break;
		} else {
			int len = w32g_wrd_wnd.row - w32g_wrd_wnd.curposx;
			char mbt = _MBC_SINGLE;
			if ( PlayerLanguage == LANGUAGE_JAPANESE ) {
				for ( i=0; i < len; i++ ) {
					mbt = _mbbtype ( str[i], mbt );
				}
				if ( mbt == _MBC_LEAD )
					len -= 1;
			}
			memcpy( &w32g_wrd_wnd.textbuf[w32g_wrd_wnd.curposy][w32g_wrd_wnd.curposx], str, len );
			memset( &w32g_wrd_wnd.forecolorbuf[w32g_wrd_wnd.curposy][w32g_wrd_wnd.curposx],
				w32g_wrd_wnd.curforecolor, len );
			memset( &w32g_wrd_wnd.backcolorbuf[w32g_wrd_wnd.curposy][w32g_wrd_wnd.curposx],
				w32g_wrd_wnd.curbackcolor, len );
			memset( &w32g_wrd_wnd.attrbuf[w32g_wrd_wnd.curposy][w32g_wrd_wnd.curposx],
				w32g_wrd_wnd.curattr, len );
			if ( mbt == _MBC_LEAD ) {
				w32g_wrd_wnd.textbuf[w32g_wrd_wnd.curposy][w32g_wrd_wnd.row-1] = 0x20;
				w32g_wrd_wnd.forecolorbuf[w32g_wrd_wnd.curposy][w32g_wrd_wnd.row-1] = w32g_wrd_wnd.curforecolor;
				w32g_wrd_wnd.backcolorbuf[w32g_wrd_wnd.curposy][w32g_wrd_wnd.row-1] = w32g_wrd_wnd.curbackcolor;
				w32g_wrd_wnd.attrbuf[w32g_wrd_wnd.curposy][w32g_wrd_wnd.row-1] = 0;
			}
			if ( lockflag ) wrd_wnd_unlock();
			wrd_text_update ( w32g_wrd_wnd.curposx, w32g_wrd_wnd.curposy,
				w32g_wrd_wnd.curposx + len - 1, w32g_wrd_wnd.curposy, lockflag );
			if ( lockflag ) wrd_wnd_lock();
			n -= len;
			str += len;
			w32g_wrd_wnd.curposx = 0;
			w32g_wrd_wnd.curposy++;
			if ( w32g_wrd_wnd.curposy >= w32g_wrd_wnd.col ) {
				if ( lockflag ) wrd_wnd_unlock();
				WrdWndScrollUp(lockflag);
				if ( lockflag ) wrd_wnd_lock();
				w32g_wrd_wnd.curposy = w32g_wrd_wnd.col - 1;
			}
		}
	}
	if ( lockflag ) wrd_wnd_unlock();
}

// left == TRUE : 行の左消去
// left != TRUE : 行の右消去
void WrdWndLineClearFrom(int left, int lockflag)
{
	if ( !w32g_wrd_wnd.active ) return;
	if ( lockflag ) wrd_wnd_lock();
	if ( left ) {
		memset( w32g_wrd_wnd.textbuf[w32g_wrd_wnd.curposy] , 0x20 , w32g_wrd_wnd.curposx + 1 );
		memset( w32g_wrd_wnd.forecolorbuf[w32g_wrd_wnd.curposy], W32G_WRDWND_BLACK, w32g_wrd_wnd.curposx + 1 );
		memset( w32g_wrd_wnd.backcolorbuf[w32g_wrd_wnd.curposy], W32G_WRDWND_BLACK, w32g_wrd_wnd.curposx + 1 );
		memset( w32g_wrd_wnd.attrbuf[w32g_wrd_wnd.curposy], 0, w32g_wrd_wnd.curposx + 1 );
		if ( lockflag ) wrd_wnd_unlock();
		wrd_text_update ( 0, w32g_wrd_wnd.curposy, 
			w32g_wrd_wnd.curposx - 1, w32g_wrd_wnd.curposy, lockflag );
	} else {
		memset( &w32g_wrd_wnd.textbuf[w32g_wrd_wnd.curposy][w32g_wrd_wnd.curposx],
			0x20 , w32g_wrd_wnd.row - w32g_wrd_wnd.curposx );
		memset( &w32g_wrd_wnd.forecolorbuf[w32g_wrd_wnd.curposy][w32g_wrd_wnd.curposx],
			W32G_WRDWND_BLACK, w32g_wrd_wnd.row - w32g_wrd_wnd.curposx );
		memset( &w32g_wrd_wnd.backcolorbuf[w32g_wrd_wnd.curposy][w32g_wrd_wnd.curposx],
			W32G_WRDWND_BLACK, w32g_wrd_wnd.row - w32g_wrd_wnd.curposx );
		memset( &w32g_wrd_wnd.attrbuf[w32g_wrd_wnd.curposy][w32g_wrd_wnd.curposx],
			0, w32g_wrd_wnd.row - w32g_wrd_wnd.curposx );
		if ( lockflag ) wrd_wnd_unlock();
		wrd_text_update ( w32g_wrd_wnd.curposx, w32g_wrd_wnd.curposy,
			w32g_wrd_wnd.row - 1, w32g_wrd_wnd.curposy, lockflag );
	}
}

// PC98 のアトリビュートで設定
void WrdWndSetAttr98(int attr)
{
	if ( !w32g_wrd_wnd.active ) return;
	switch ( attr ) {
	case 0:	// 規定値
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_WHITE;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;
	case 1: // ハイライト
		w32g_wrd_wnd.curattr = 0;
		break;
	case 2: // バーティカルライン
		w32g_wrd_wnd.curattr = 0;
		break;
	case 4: // アンダーライン
		w32g_wrd_wnd.curattr = 0;
		break;
	case 5: // ブリンク
		w32g_wrd_wnd.curattr = 0;
		break;
	case 7: // リバース
		{
			char tmp = w32g_wrd_wnd.curbackcolor;
			w32g_wrd_wnd.curbackcolor = w32g_wrd_wnd.curforecolor;
			w32g_wrd_wnd.curforecolor = tmp;
			w32g_wrd_wnd.curattr = 0;
			w32g_wrd_wnd.curattr |= W32G_WRDWND_ATTR_REVERSE;
		}
		break;
	case 8: // シークレット
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;
	case 16:	// 黒
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;
	case 17:	// 赤
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_RED;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;
	case 18:	// 青
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_BLUE;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;
	case 19:	// 紫
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_PURPLE;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;	
	case 20:		// 緑
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_GREEN;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;
	case 21:	// 黄色
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_YELLOW;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;
	case 22:	// 水色
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_LIGHTBLUE;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;
	case 23: // 白
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_WHITE;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;
	case 30:	// 黒
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;
	case 31:	// 赤
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_RED;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;
	case 32:	// 緑
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_GREEN;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;
	case 33:	// 黄色
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_YELLOW;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;
	case 34:	// 青
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_BLUE;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;
	case 35:	// 紫
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_PURPLE;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;
	case 36:	// 水色
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_LIGHTBLUE;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;
	case 37:	// 白
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_WHITE;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;
	case 40:	// 黒反転
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		w32g_wrd_wnd.curattr |= W32G_WRDWND_ATTR_REVERSE;
		break;
	case 41:	// 赤反転
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_RED;
		w32g_wrd_wnd.curattr = 0;
		w32g_wrd_wnd.curattr |= W32G_WRDWND_ATTR_REVERSE;
		break;
	case 42:	// 緑反転
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_GREEN;
		w32g_wrd_wnd.curattr = 0;
		w32g_wrd_wnd.curattr |= W32G_WRDWND_ATTR_REVERSE;
		break;
	case 43:	// 黄色反転
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_YELLOW;
		w32g_wrd_wnd.curattr = 0;
		w32g_wrd_wnd.curattr |= W32G_WRDWND_ATTR_REVERSE;
		break;
	case 44:	// 青反転
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLUE;
		w32g_wrd_wnd.curattr = 0;
		w32g_wrd_wnd.curattr |= W32G_WRDWND_ATTR_REVERSE;
		break;
	case 45:	// 紫反転
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_PURPLE;
		w32g_wrd_wnd.curattr = 0;
		w32g_wrd_wnd.curattr |= W32G_WRDWND_ATTR_REVERSE;
		break;
	case 46:	// 水色反転
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_LIGHTBLUE;
		w32g_wrd_wnd.curattr = 0;
		w32g_wrd_wnd.curattr |= W32G_WRDWND_ATTR_REVERSE;
		break;
	case 47:	// 白反転
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_WHITE;
		w32g_wrd_wnd.curattr = 0;
		w32g_wrd_wnd.curattr |= W32G_WRDWND_ATTR_REVERSE;
		break;
	default:
		w32g_wrd_wnd.curforecolor = W32G_WRDWND_WHITE;
		w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
		w32g_wrd_wnd.curattr = 0;
		break;
	}
}

// アトリビュートのリセット
void WrdWndSetAttrReset(void)
{
	if ( !w32g_wrd_wnd.active ) return;
	w32g_wrd_wnd.curforecolor = W32G_WRDWND_WHITE;
	w32g_wrd_wnd.curbackcolor = W32G_WRDWND_BLACK;
	w32g_wrd_wnd.curattr = 0;
}

// カーソルポジションの移動
void WrdWndGoto(int x, int y)
{
	if ( !w32g_wrd_wnd.active ) return;
	if ( x < 0 ) x = 0;
	if ( x >= w32g_wrd_wnd.row ) x = w32g_wrd_wnd.row - 1;
	if ( y < 0 ) y = 0;
	if ( y >= w32g_wrd_wnd.col ) y = w32g_wrd_wnd.col - 1;
	w32g_wrd_wnd.curposx = x;
	w32g_wrd_wnd.curposy = y;
}

void WrdWndPaintAll(int lockflag)
{
	if ( !w32g_wrd_wnd.active ) return;
	wrd_text_update ( 0, 0, w32g_wrd_wnd.row - 1, w32g_wrd_wnd.col - 1, TRUE );
}

// SetInvalidateRect() は WM_PAINT を呼ぶ可能性がある。
void WrdWndPaintDo(int flag)
{
	RECT rc;
	if ( flag ) InvalidateRect( w32g_wrd_wnd.hwnd,NULL, FALSE );
	if ( GetUpdateRect(w32g_wrd_wnd.hwnd, &rc, FALSE) ) {
		PAINTSTRUCT ps;
		if ( wrd_wnd_lock_ex ( 0 ) == TRUE ) {
			if ( GDI_LOCK_EX(0) == 0 ) {
				w32g_wrd_wnd.hdc = BeginPaint(w32g_wrd_wnd.hwnd, &ps);
				BitBlt(w32g_wrd_wnd.hdc,rc.left,rc.top,rc.right,rc.bottom,w32g_wrd_wnd.hmdc,rc.left,rc.top,SRCCOPY);
				EndPaint(w32g_wrd_wnd.hwnd, &ps);
				GDI_UNLOCK(); // gdi_lock
			} else {
				InvalidateRect ( w32g_wrd_wnd.hwnd, &rc, FALSE );
			}
		} else {
				InvalidateRect ( w32g_wrd_wnd.hwnd, &rc, FALSE );
		}
		wrd_wnd_unlock();
	}
}

#define IDM_GRAPHIC_STOP 3531
#define IDM_GRAPHIC_START 3532

BOOL CALLBACK
WrdCanvasWndProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam)
{
	static HMENU hPopupMenu = NULL;
	switch (uMess)
	{
		case WM_CREATE:
			break;
		case WM_PAINT:
	      	WrdWndPaintDo(FALSE);
	    	return 0;
		case WM_DROPFILES:
			SendMessage(hMainWnd,WM_DROPFILES,wParam,lParam);
			return 0;
		case WM_DESTROY:
			if ( hPopupMenu != NULL )
				DestroyMenu ( hPopupMenu );
			hPopupMenu = NULL;
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDM_GRAPHIC_STOP:
				WrdWndInfo.GraphicStop = 1;
				break;
			case IDM_GRAPHIC_START:
				WrdWndInfo.GraphicStop = 0;
				break;
			default:
				break;
			}
			break;
		case WM_RBUTTONDOWN:
			{
			if ( LOWORD(lParam ) != HTCAPTION ){
				POINT point;
				int res;
				if ( hPopupMenu != NULL )
					DestroyMenu ( hPopupMenu );
				hPopupMenu = CreatePopupMenu();
				if ( WrdWndInfo.GraphicStop ) {
					AppendMenu(hPopupMenu,MF_STRING,IDM_GRAPHIC_START,"Graphic Start");
				} else {
					AppendMenu(hPopupMenu,MF_STRING,IDM_GRAPHIC_STOP,"Graphic Stop");
				}
				GetCursorPos(&point);
				SetForegroundWindow ( hwnd );
				res = TrackPopupMenu ( hPopupMenu, TPM_TOPALIGN|TPM_LEFTALIGN,
					point.x, point.y, 0, hwnd, NULL );
				PostMessage ( hwnd, WM_NULL, 0, 0 );
				DestroyMenu ( hPopupMenu );
				hPopupMenu = NULL;
			}
			}
			break;		
		default:
			return DefWindowProc(hwnd,uMess,wParam,lParam) ;
	}
	return 0L;
}

extern void MainWndUpdateWrdButton(void);

BOOL CALLBACK
WrdWndProc(HWND hwnd, UINT uMess, WPARAM wParam, LPARAM lParam)
{
	switch (uMess){
	case WM_INITDIALOG:
		SetWindowPosSize(GetDesktopWindow(),hwnd,WrdWndInfo.PosX, WrdWndInfo.PosY );
		return FALSE;
	case WM_DESTROY:
		TerminateWrdWnd ();
		INISaveWrdWnd();
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCLOSE:
			ShowWindow(hwnd, SW_HIDE);
			MainWndUpdateWrdButton();
			break;
		default:
			return FALSE;
		}
		case WM_MOVE:
			// WrdWndInfo.PosX = (int) LOWORD(lParam);
			// WrdWndInfo.PosY = (int) HIWORD(lParam);
			{
				RECT rc;
				GetWindowRect(hwnd,&rc);
				WrdWndInfo.PosX = rc.left;
				WrdWndInfo.PosY = rc.top;
			}
			break;
		case WM_CLOSE:
			ShowWindow(hWrdWnd, SW_HIDE);
			MainWndUpdateWrdButton();
			break;
		case WM_DROPFILES:
			SendMessage(hMainWnd,WM_DROPFILES,wParam,lParam);
			return 0;
		default:
			return FALSE;
	}
	return FALSE;
}

static int WrdWndInfoReset(HWND hwnd)
{
	memset(&WrdWndInfo,0,sizeof(WRDWNDINFO));
	WrdWndInfo.PosX = - 1;
	WrdWndInfo.PosY = - 1;
	WrdWndInfo.GraphicStop = 1;
	return 0;
}

static int WrdWndInfoApply(void)
{
	return 0;
}

static RGBQUAD RGBtoRGBQUAD ( COLORREF color )
{
	RGBQUAD rq;
	rq.rgbRed = (unsigned char) ( ( color & 0x000000FFL ) >> 0 );
	rq.rgbGreen = (unsigned char) ( ( color & 0x0000FF00L ) >> 8 );
	rq.rgbBlue = (unsigned char) ( ( color & 0x00FF0000L ) >> 16 );
	rq.rgbReserved = 0;
	return rq;
}

static COLORREF RGBQUADtoRGB ( RGBQUAD rq )
{
	return RGB ( rq.rgbRed, rq.rgbGreen, rq.rgbBlue );
}

extern int PosSizeSave;

#define SEC_WRDWND "WrdWnd"
int INISaveWrdWnd(void)
{
	char *section = SEC_WRDWND;
	char *inifile = TIMIDITY_WINDOW_INI_FILE;
	char buffer[256];
	if ( PosSizeSave ) {
		if ( WrdWndInfo.PosX >= 0 || WrdWndInfo.PosY >= 0 ) {
			if ( WrdWndInfo.PosX < 0 )
				WrdWndInfo.PosX = 0;
			if ( WrdWndInfo.PosY < 0 )
				WrdWndInfo.PosY = 0;
		}
		sprintf(buffer,"%d",WrdWndInfo.PosX);
		if ( WrdWndInfo.PosX >= 0 )
		WritePrivateProfileString(section,"PosX",buffer,inifile);
		sprintf(buffer,"%d",WrdWndInfo.PosY);
		if ( WrdWndInfo.PosY >= 0 )
		WritePrivateProfileString(section,"PosY",buffer,inifile);
	}
	sprintf(buffer,"%d",WrdWndInfo.GraphicStop);
	WritePrivateProfileString(section,"GraphicStop",buffer,inifile);
	WritePrivateProfileString(NULL,NULL,NULL,inifile);		// Write Flush
	return 0;
}

int INILoadWrdWnd(void)
{
	char *section = SEC_WRDWND;
	char *inifile = TIMIDITY_WINDOW_INI_FILE;
	int num;
	num = GetPrivateProfileInt(section,"PosX",-1,inifile);
	WrdWndInfo.PosX = num;
	num = GetPrivateProfileInt(section,"PosY",-1,inifile);
	WrdWndInfo.PosY = num;
	num = GetPrivateProfileInt(section,"GraphicStop",1,inifile);
	WrdWndInfo.GraphicStop = num;
	return 0;
}

void w32_wrd_ctl_event(CtlEvent *e)
{
}


