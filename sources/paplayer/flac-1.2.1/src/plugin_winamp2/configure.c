/* in_flac - Winamp2 FLAC input plugin
 * Copyright (C) 2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "configure.h"
#include "tagz.h"
#include "resource.h"
#include "share/alloc.h"


static char buffer[256];
static char ini_name[MAX_PATH];

/*
 *  Read/write config
 */

#define RI(x, def)          (x = GetPrivateProfileInt("FLAC", #x, def, ini_name))
#define WI(x)               WritePrivateProfileString("FLAC", #x, itoa(x, buffer, 10), ini_name)
#define RS(x, n, def)       GetPrivateProfileString("FLAC", #x, def, x, n, ini_name)
#define WS(x)               WritePrivateProfileString("FLAC", #x, x, ini_name)

static const char default_format[] = "[%artist% - ]$if2(%title%,%filename%)";
static const char default_sep[] = ", ";

static wchar_t *convert_ansi_to_wide_(const char *src)
{
	int len;
	wchar_t *dest;

	FLAC__ASSERT(0 != src);

	len = strlen(src) + 1;
	/* copy */
	dest = safe_malloc_mul_2op_(len, /*times*/sizeof(wchar_t));
	if (dest) mbstowcs(dest, src, len);
	return dest;
}

void InitConfig()
{
	char *p;

	GetModuleFileName(NULL, ini_name, sizeof(ini_name));
	p = strrchr(ini_name, '.');
	if (!p) p = ini_name + strlen(ini_name);
	strcpy(p, ".ini");

	flac_cfg.title.tag_format_w = NULL;
}

void ReadConfig()
{
	RS(flac_cfg.title.tag_format, sizeof(flac_cfg.title.tag_format), default_format);
	if (flac_cfg.title.tag_format_w)
		free(flac_cfg.title.tag_format_w);
	flac_cfg.title.tag_format_w = convert_ansi_to_wide_(flac_cfg.title.tag_format);
	/* @@@ FIXME: trailing spaces */
	RS(flac_cfg.title.sep, sizeof(flac_cfg.title.sep), default_sep);
	RI(flac_cfg.tag.reserve_space, 1);

	RI(flac_cfg.display.show_bps, 1);
	RI(flac_cfg.output.misc.stop_err, 0);
	RI(flac_cfg.output.replaygain.enable, 1);
	RI(flac_cfg.output.replaygain.album_mode, 0);
	RI(flac_cfg.output.replaygain.hard_limit, 0);
	RI(flac_cfg.output.replaygain.preamp, 0);
	RI(flac_cfg.output.resolution.normal.dither_24_to_16, 0);
	RI(flac_cfg.output.resolution.replaygain.dither, 0);
	RI(flac_cfg.output.resolution.replaygain.noise_shaping, 1);
	RI(flac_cfg.output.resolution.replaygain.bps_out, 16);
}

void WriteConfig()
{
	WS(flac_cfg.title.tag_format);
	WI(flac_cfg.tag.reserve_space);
	WS(flac_cfg.title.sep);

	WI(flac_cfg.display.show_bps);
	WI(flac_cfg.output.misc.stop_err);
	WI(flac_cfg.output.replaygain.enable);
	WI(flac_cfg.output.replaygain.album_mode);
	WI(flac_cfg.output.replaygain.hard_limit);
	WI(flac_cfg.output.replaygain.preamp);
	WI(flac_cfg.output.resolution.normal.dither_24_to_16);
	WI(flac_cfg.output.resolution.replaygain.dither);
	WI(flac_cfg.output.resolution.replaygain.noise_shaping);
	WI(flac_cfg.output.resolution.replaygain.bps_out);
}

/*
 *  Dialog
 */

#define PREAMP_RANGE            24

#define Check(x,y)              CheckDlgButton(hwnd, x, y ? BST_CHECKED : BST_UNCHECKED)
#define GetCheck(x)             (IsDlgButtonChecked(hwnd, x)==BST_CHECKED)
#define GetSel(x)               SendDlgItemMessage(hwnd, x, CB_GETCURSEL, 0, 0)
#define GetPos(x)               SendDlgItemMessage(hwnd, x, TBM_GETPOS, 0, 0)
#define Enable(x,y)             EnableWindow(GetDlgItem(hwnd, x), y)

static INT_PTR CALLBACK GeneralProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	/* init */
	case WM_INITDIALOG:
		SendDlgItemMessage(hwnd, IDC_TITLE, EM_LIMITTEXT, 255, 0);
		SendDlgItemMessage(hwnd, IDC_SEP, EM_LIMITTEXT, 15, 0);

		SetDlgItemText(hwnd, IDC_TITLE, flac_cfg.title.tag_format);
		SetDlgItemText(hwnd, IDC_SEP, flac_cfg.title.sep);
		Check(IDC_ID3V1, 0);
/*!		Check(IDC_RESERVE, flac_cfg.tag.reserve_space); */
		Check(IDC_BPS, flac_cfg.display.show_bps);
		Check(IDC_ERRORS, flac_cfg.output.misc.stop_err);
		return TRUE;
	/* commands */
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		/* ok */
		case IDOK:
			GetDlgItemText(hwnd, IDC_TITLE, flac_cfg.title.tag_format, sizeof(flac_cfg.title.tag_format));
			if (flac_cfg.title.tag_format_w)
				free(flac_cfg.title.tag_format_w);
			GetDlgItemText(hwnd, IDC_SEP, flac_cfg.title.sep, sizeof(flac_cfg.title.sep));
			flac_cfg.title.tag_format_w = convert_ansi_to_wide_(flac_cfg.title.tag_format);

/*!			flac_cfg.tag.reserve_space = GetCheck(IDC_RESERVE); */
			flac_cfg.display.show_bps = GetCheck(IDC_BPS);
			flac_cfg.output.misc.stop_err = GetCheck(IDC_ERRORS);
			break;
		/* reset */
		case IDC_RESET:
			Check(IDC_ID3V1, 0);
			Check(IDC_RESERVE, 1);
			Check(IDC_BPS, 1);
			Check(IDC_ERRORS, 0);
			/* fall throught */
		/* default */
		case IDC_TAGZ_DEFAULT:
			SetDlgItemText(hwnd, IDC_TITLE, default_format);
			break;
		/* help */
		case IDC_TAGZ_HELP:
			MessageBox(hwnd, tagz_manual, "Help", 0);
			break;
		}
		break;
	}

	return 0;
}


static void UpdatePreamp(HWND hwnd, HWND hamp)
{
	int pos = SendMessage(hamp, TBM_GETPOS, 0, 0) - PREAMP_RANGE;
	sprintf(buffer, "%d dB", pos);
	SetDlgItemText(hwnd, IDC_PA, buffer);
}

static void UpdateRG(HWND hwnd)
{
	int on = GetCheck(IDC_ENABLE);
	Enable(IDC_ALBUM, on);
	Enable(IDC_LIMITER, on);
	Enable(IDC_PREAMP, on);
	Enable(IDC_PA, on);
}

static void UpdateDither(HWND hwnd)
{
	int on = GetCheck(IDC_DITHERRG);
	Enable(IDC_SHAPE, on);
}

static INT_PTR CALLBACK OutputProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	/* init */
	case WM_INITDIALOG:
		Check(IDC_ENABLE, flac_cfg.output.replaygain.enable);
		Check(IDC_ALBUM, flac_cfg.output.replaygain.album_mode);
		Check(IDC_LIMITER, flac_cfg.output.replaygain.hard_limit);
		Check(IDC_DITHER, flac_cfg.output.resolution.normal.dither_24_to_16);
		Check(IDC_DITHERRG, flac_cfg.output.resolution.replaygain.dither);
		/* prepare preamp slider */
		{
			HWND hamp = GetDlgItem(hwnd, IDC_PREAMP);
			SendMessage(hamp, TBM_SETRANGE, 1, MAKELONG(0, PREAMP_RANGE*2));
			SendMessage(hamp, TBM_SETPOS, 1, flac_cfg.output.replaygain.preamp+PREAMP_RANGE);
			UpdatePreamp(hwnd, hamp);
		}
		/* fill comboboxes */
		{
			HWND hlist = GetDlgItem(hwnd, IDC_TO);
			SendMessage(hlist, CB_ADDSTRING, 0, (LPARAM)"16 bps");
			SendMessage(hlist, CB_ADDSTRING, 0, (LPARAM)"24 bps");
			SendMessage(hlist, CB_SETCURSEL, flac_cfg.output.resolution.replaygain.bps_out/8 - 2, 0);

			hlist = GetDlgItem(hwnd, IDC_SHAPE);
			SendMessage(hlist, CB_ADDSTRING, 0, (LPARAM)"None");
			SendMessage(hlist, CB_ADDSTRING, 0, (LPARAM)"Low");
			SendMessage(hlist, CB_ADDSTRING, 0, (LPARAM)"Medium");
			SendMessage(hlist, CB_ADDSTRING, 0, (LPARAM)"High");
			SendMessage(hlist, CB_SETCURSEL, flac_cfg.output.resolution.replaygain.noise_shaping, 0);
		}
		UpdateRG(hwnd);
		UpdateDither(hwnd);
		return TRUE;
	/* commands */
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		/* ok */
		case IDOK:
			flac_cfg.output.replaygain.enable = GetCheck(IDC_ENABLE);
			flac_cfg.output.replaygain.album_mode = GetCheck(IDC_ALBUM);
			flac_cfg.output.replaygain.hard_limit = GetCheck(IDC_LIMITER);
			flac_cfg.output.replaygain.preamp = GetPos(IDC_PREAMP) - PREAMP_RANGE;
			flac_cfg.output.resolution.normal.dither_24_to_16 = GetCheck(IDC_DITHER);
			flac_cfg.output.resolution.replaygain.dither = GetCheck(IDC_DITHERRG);
			flac_cfg.output.resolution.replaygain.noise_shaping = GetSel(IDC_SHAPE);
			flac_cfg.output.resolution.replaygain.bps_out = (GetSel(IDC_TO)+2)*8;
			break;
		/* reset */
		case IDC_RESET:
			Check(IDC_ENABLE, 1);
			Check(IDC_ALBUM, 0);
			Check(IDC_LIMITER, 0);
			Check(IDC_DITHER, 0);
			Check(IDC_DITHERRG, 0);

			SendDlgItemMessage(hwnd, IDC_PREAMP, TBM_SETPOS, 1, PREAMP_RANGE);
			SendDlgItemMessage(hwnd, IDC_TO, CB_SETCURSEL, 0, 0);
			SendDlgItemMessage(hwnd, IDC_SHAPE, CB_SETCURSEL, 1, 0);

			UpdatePreamp(hwnd, GetDlgItem(hwnd, IDC_PREAMP));
			UpdateRG(hwnd);
			UpdateDither(hwnd);
			break;
		/* active check-boxes */
		case IDC_ENABLE:
			UpdateRG(hwnd);
			break;
		case IDC_DITHERRG:
			UpdateDither(hwnd);
			break;
		}
		break;
	/* scroller */
	case WM_HSCROLL:
		if (GetDlgCtrlID((HWND)lParam)==IDC_PREAMP)
			UpdatePreamp(hwnd, (HWND)lParam);
		return 0;
	}

	return 0;
}

#define NUM_PAGES       2

typedef struct
{
	HWND htab;
	HWND hdlg;
	RECT r;
	HWND all[NUM_PAGES];
} LOCALDATA;

static void ScreenToClientRect(HWND hwnd, RECT *rect)
{
	POINT pt = { rect->left, rect->top };
	ScreenToClient(hwnd, &pt);
	rect->left = pt.x;
	rect->top  = pt.y;

	pt.x = rect->right;
	pt.y = rect->bottom;
	ScreenToClient(hwnd, &pt);
	rect->right  = pt.x;
	rect->bottom = pt.y;
}

static void SendCommand(HWND hwnd, int command)
{
	LOCALDATA *data = (LOCALDATA*)GetWindowLong(hwnd, GWL_USERDATA);
	SendMessage(data->hdlg, WM_COMMAND, command, 0);
}

static void BroadcastCommand(HWND hwnd, int command)
{
	LOCALDATA *data = (LOCALDATA*)GetWindowLong(hwnd, GWL_USERDATA);
	int i;

	for (i=0; i<NUM_PAGES; i++)
		SendMessage(data->all[i], WM_COMMAND, command, 0);
}

static void OnSelChange(HWND hwnd)
{
	LOCALDATA *data = (LOCALDATA*)GetWindowLong(hwnd, GWL_USERDATA);
	int index = TabCtrl_GetCurSel(data->htab);
	if (index < 0) return;
	/* hide previous */
	if (data->hdlg)
		ShowWindow(data->hdlg, SW_HIDE);
	/* display */
	data->hdlg = data->all[index];
	SetWindowPos(data->hdlg, HWND_TOP, data->r.left, data->r.top, data->r.right-data->r.left, data->r.bottom-data->r.top, SWP_SHOWWINDOW);
	SetFocus(hwnd);
}

static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static activePage = 0;

	switch (msg)
	{
	/* init */
	case WM_INITDIALOG:
		{
			LOCALDATA *data = LocalAlloc(LPTR, sizeof(LOCALDATA));
			HINSTANCE inst = (HINSTANCE)lParam;
			TCITEM item;

			/* init */
			SetWindowLong(hwnd, GWL_USERDATA, (LONG)data);
			data->htab = GetDlgItem(hwnd, IDC_TABS);
			data->hdlg = NULL;
			/* add pages */
			item.mask = TCIF_TEXT;
			data->all[0] = CreateDialog(inst, MAKEINTRESOURCE(IDD_CONFIG_GENERAL), hwnd, GeneralProc);
			item.pszText = "General";
			TabCtrl_InsertItem(data->htab, 0, &item);

			data->all[1] = CreateDialog(inst, MAKEINTRESOURCE(IDD_CONFIG_OUTPUT), hwnd, OutputProc);
			item.pszText = "Output";
			TabCtrl_InsertItem(data->htab, 1, &item);
			/* get rect (after adding pages) */
			GetWindowRect(data->htab, &data->r);
			ScreenToClientRect(hwnd, &data->r);
			TabCtrl_AdjustRect(data->htab, 0, &data->r);
			/* simulate item change */
			TabCtrl_SetCurSel(data->htab, activePage);
			OnSelChange(hwnd);
		}
		return TRUE;
	/* destory */
	case WM_DESTROY:
		{
			LOCALDATA *data = (LOCALDATA*)GetWindowLong(hwnd, GWL_USERDATA);
			int i;

			activePage = TabCtrl_GetCurSel(data->htab);

			for (i=0; i<NUM_PAGES; i++)
				DestroyWindow(data->all[i]);

			LocalFree(data);
		}
		break;
	/* commands */
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		/* ok/cancel */
		case IDOK:
			BroadcastCommand(hwnd, IDOK);
			/* fall through */
		case IDCANCEL:
			EndDialog(hwnd, LOWORD(wParam));
			return TRUE;
		case IDC_RESET:
			SendCommand(hwnd, IDC_RESET);
			break;
		}
		break;
	/* notification */
	case WM_NOTIFY:
		if (LOWORD(wParam) == IDC_TABS)
		{
			NMHDR *hdr = (NMHDR*)lParam;

			switch (hdr->code)
			{
			case TCN_SELCHANGE:
				OnSelChange(hwnd);
				break;
			}
		}
		break;
	}

	return 0;
}


int DoConfig(HINSTANCE inst, HWND parent)
{
	return DialogBoxParam(inst, MAKEINTRESOURCE(IDD_CONFIG), parent, DialogProc, (LONG)inst) == IDOK;
}
