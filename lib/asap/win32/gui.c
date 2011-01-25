/*
 * gui.c - settings and file information dialog boxes
 *
 * Copyright (C) 2007-2009  Piotr Fusik
 *
 * This file is part of ASAP (Another Slight Atari Player),
 * see http://asap.sourceforge.net
 *
 * ASAP is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * ASAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASAP; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <windows.h>
#include <string.h>
#include <tchar.h>

#include "asap.h"
#include "gui.h"

#ifndef WASAP

int song_length = -1;
int silence_seconds = -1;
BOOL play_loops = FALSE;
int mute_mask = 0;
static int saved_mute_mask;
#ifdef WINAMP
BOOL playing_info = FALSE;
#endif

static void enableTimeInput(HWND hDlg, BOOL enable)
{
	EnableWindow(GetDlgItem(hDlg, IDC_MINUTES), enable);
	EnableWindow(GetDlgItem(hDlg, IDC_SECONDS), enable);
}

static void setFocusAndSelect(HWND hDlg, int nID)
{
	HWND hWnd = GetDlgItem(hDlg, nID);
	SetFocus(hWnd);
	SendMessage(hWnd, EM_SETSEL, 0, -1);
}

static BOOL getDlgInt(HWND hDlg, int nID, int *result)
{
	BOOL translated;
	UINT r = GetDlgItemInt(hDlg, nID, &translated, FALSE);
	if (!translated) {
		MessageBox(hDlg, _T("Invalid number"), _T("Error"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	*result = (int) r;
	return TRUE;
}

static INT_PTR CALLBACK settingsDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int i;
	switch (uMsg) {
	case WM_INITDIALOG:
		if (song_length <= 0) {
			CheckRadioButton(hDlg, IDC_UNLIMITED, IDC_LIMITED, IDC_UNLIMITED);
			SetDlgItemInt(hDlg, IDC_MINUTES, DEFAULT_SONG_LENGTH / 60, FALSE);
			SetDlgItemInt(hDlg, IDC_SECONDS, DEFAULT_SONG_LENGTH % 60, FALSE);
			enableTimeInput(hDlg, FALSE);
		}
		else {
			CheckRadioButton(hDlg, IDC_UNLIMITED, IDC_LIMITED, IDC_LIMITED);
			SetDlgItemInt(hDlg, IDC_MINUTES, (UINT) song_length / 60, FALSE);
			SetDlgItemInt(hDlg, IDC_SECONDS, (UINT) song_length % 60, FALSE);
			enableTimeInput(hDlg, TRUE);
		}
		if (silence_seconds <= 0) {
			CheckDlgButton(hDlg, IDC_SILENCE, BST_UNCHECKED);
			SetDlgItemInt(hDlg, IDC_SILSECONDS, DEFAULT_SILENCE_SECONDS, FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_SILSECONDS), FALSE);
		}
		else {
			CheckDlgButton(hDlg, IDC_SILENCE, BST_CHECKED);
			SetDlgItemInt(hDlg, IDC_SILSECONDS, (UINT) silence_seconds, FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_SILSECONDS), TRUE);
		}
		CheckRadioButton(hDlg, IDC_LOOPS, IDC_NOLOOPS, play_loops ? IDC_LOOPS : IDC_NOLOOPS);
		saved_mute_mask = mute_mask;
		for (i = 0; i < 8; i++)
			CheckDlgButton(hDlg, IDC_MUTE1 + i, ((mute_mask >> i) & 1) != 0 ? BST_CHECKED : BST_UNCHECKED);
		return TRUE;
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED) {
			WORD wCtrl = LOWORD(wParam);
			BOOL enabled;
			switch (wCtrl) {
			case IDC_UNLIMITED:
			case IDC_LIMITED:
				enabled = (wCtrl == IDC_LIMITED);
				enableTimeInput(hDlg, enabled);
				if (enabled)
					setFocusAndSelect(hDlg, IDC_MINUTES);
				return TRUE;
			case IDC_SILENCE:
				enabled = (IsDlgButtonChecked(hDlg, IDC_SILENCE) == BST_CHECKED);
				EnableWindow(GetDlgItem(hDlg, IDC_SILSECONDS), enabled);
				if (enabled)
					setFocusAndSelect(hDlg, IDC_SILSECONDS);
				return TRUE;
			case IDC_LOOPS:
			case IDC_NOLOOPS:
				return TRUE;
			case IDC_MUTE1:
			case IDC_MUTE1 + 1:
			case IDC_MUTE1 + 2:
			case IDC_MUTE1 + 3:
			case IDC_MUTE1 + 4:
			case IDC_MUTE1 + 5:
			case IDC_MUTE1 + 6:
			case IDC_MUTE1 + 7:
				i = 1 << (wCtrl - IDC_MUTE1);
				if (IsDlgButtonChecked(hDlg, wCtrl) == BST_CHECKED)
					mute_mask |= i;
				else
					mute_mask &= ~i;
				ASAP_MutePokeyChannels(&asap, mute_mask);
				return TRUE;
			case IDOK:
			{
				int new_song_length;
				if (IsDlgButtonChecked(hDlg, IDC_UNLIMITED) == BST_CHECKED)
					new_song_length = -1;
				else {
					int minutes;
					int seconds;
					if (!getDlgInt(hDlg, IDC_MINUTES, &minutes)
					 || !getDlgInt(hDlg, IDC_SECONDS, &seconds))
						return TRUE;
					new_song_length = 60 * minutes + seconds;
				}
				if (IsDlgButtonChecked(hDlg, IDC_SILENCE) != BST_CHECKED)
					silence_seconds = -1;
				else if (!getDlgInt(hDlg, IDC_SILSECONDS, &silence_seconds))
					return TRUE;
				song_length = new_song_length;
				play_loops = (IsDlgButtonChecked(hDlg, IDC_LOOPS) == BST_CHECKED);
			}
				EndDialog(hDlg, IDOK);
				return TRUE;
			case IDCANCEL:
				mute_mask = saved_mute_mask;
				ASAP_MutePokeyChannels(&asap, mute_mask);
				EndDialog(hDlg, IDCANCEL);
				return TRUE;
			}
		}
		break;
	default:
		break;
	}
	return FALSE;
}

BOOL settingsDialog(HINSTANCE hInstance, HWND hwndParent)
{
	return DialogBox(hInstance, MAKEINTRESOURCE(IDD_SETTINGS), hwndParent, settingsDialogProc) == IDOK;
}

int getSongDuration(const ASAP_ModuleInfo *module_info, int song)
{
	int duration = module_info->durations[song];
	if (duration < 0)
		return 1000 * song_length;
	if (play_loops && module_info->loops[song])
		return 1000 * song_length;
	return duration;
}

int playSong(int song)
{
	int duration = asap.module_info.durations[song];
	if (duration < 0) {
		if (silence_seconds > 0)
			ASAP_DetectSilence(&asap, silence_seconds);
		duration = 1000 * song_length;
	}
	if (play_loops && asap.module_info.loops[song])
		duration = 1000 * song_length;
	ASAP_PlaySong(&asap, song, duration);
	ASAP_MutePokeyChannels(&asap, mute_mask);
	return duration;
}

#endif /* WASAP */

#if defined(WASAP) || defined(WINAMP) || defined(APOLLO)

HWND infoDialog = NULL;
static byte saved_module[ASAP_MODULE_MAX];
static int saved_module_len;
static ASAP_ModuleInfo saved_module_info;
static ASAP_ModuleInfo edited_module_info;
static int edited_song;
static BOOL can_save;
static char convert_filename[MAX_PATH];
static const char *convert_ext;

char *appendString(char *dest, const char *src)
{
	while (*src != '\0')
		*dest++ = *src++;
	return dest;
}

char *appendInt(char *dest, int x)
{
	if (x < 0) {
		*dest++ = '-';
		x = -x;
	}
	if (x >= 10) {
		dest = appendInt(dest, x / 10);
		x %= 10;
	}
	*dest++ = '0' + x;
	return dest;
}

BOOL loadModule(const char *filename, byte *module, int *module_len)
{
	HANDLE fh;
	BOOL ok;
	fh = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (fh == INVALID_HANDLE_VALUE)
		return FALSE;
	ok = ReadFile(fh, module, ASAP_MODULE_MAX, (LPDWORD) module_len, NULL);
	CloseHandle(fh);
	return ok;
}

static void showSongTime(void)
{
	char str[ASAP_DURATION_CHARS];
	ASAP_DurationToString(str, edited_module_info.durations[edited_song]);
	SendDlgItemMessage(infoDialog, IDC_TIME, WM_SETTEXT, 0, (LPARAM) str);
	CheckDlgButton(infoDialog, IDC_LOOP, edited_module_info.loops[edited_song] ? BST_CHECKED : BST_UNCHECKED);
}

static BOOL infoChanged(void)
{
	int i;
	if (strcmp(saved_module_info.author, edited_module_info.author) != 0)
		return TRUE;
	if (strcmp(saved_module_info.name, edited_module_info.name) != 0)
		return TRUE;
	if (strcmp(saved_module_info.date, edited_module_info.date) != 0)
		return TRUE;
	for (i = 0; i < saved_module_info.songs; i++) {
		if (saved_module_info.durations[i] != edited_module_info.durations[i])
			return TRUE;
		if (edited_module_info.durations[i] >= 0
		 && saved_module_info.loops[i] != edited_module_info.loops[i])
			return TRUE;
	}
	return FALSE;
}

static void updateSaveButton(void)
{
	if (can_save)
		EnableWindow(GetDlgItem(infoDialog, IDC_SAVE), infoChanged());
}

static BOOL saveFile(const char *filename, const byte *data, int len)
{
	HANDLE fh;
	DWORD written;
	BOOL ok;
	fh = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fh == INVALID_HANDLE_VALUE)
		return FALSE;
	ok = WriteFile(fh, data, len, &written, NULL);
	CloseHandle(fh);
	return ok;
}

static BOOL doSaveInfo(void)
{
	char filename[MAX_PATH];
	byte out_module[ASAP_MODULE_MAX];
	int out_len;
	out_len = ASAP_SetModuleInfo(&edited_module_info, saved_module, saved_module_len, out_module);
	if (out_len <= 0)
		return FALSE;
	SendDlgItemMessage(infoDialog, IDC_FILENAME, WM_GETTEXT, MAX_PATH, (LPARAM) filename);
	if (!saveFile(filename, out_module, out_len))
		return FALSE;
	saved_module_info = edited_module_info;
	showSongTime();
	EnableWindow(GetDlgItem(infoDialog, IDC_SAVE), FALSE);
	return TRUE;
}

static BOOL saveInfo(void)
{
	int song = edited_module_info.songs;
	while (--song >= 0 && edited_module_info.durations[song] < 0);
	while (--song >= 0)
		if (edited_module_info.durations[song] < 0) {
			MessageBox(infoDialog, "Cannot save file because time not set for all songs", "Error", MB_OK | MB_ICONERROR);
			return FALSE;
		}
	if (!doSaveInfo()) {
		MessageBox(infoDialog, "Cannot save information to file", "Error", MB_OK | MB_ICONERROR);
		return FALSE;
	}
	return TRUE;
}

static BOOL convert(void)
{
	char filename[MAX_PATH];
	byte out_module[ASAP_MODULE_MAX];
	int out_len;
	static OPENFILENAME ofn = {
		sizeof(OPENFILENAME),
		NULL,
		0,
		NULL,
		NULL,
		0,
		0,
		convert_filename,
		MAX_PATH,
		NULL,
		0,
		NULL,
		"Select output file",
		OFN_ENABLESIZING | OFN_EXPLORER | OFN_OVERWRITEPROMPT,
		0,
		0,
		NULL,
		0,
		NULL,
		NULL
	};
	SendDlgItemMessage(infoDialog, IDC_FILENAME, WM_GETTEXT, MAX_PATH, (LPARAM) filename);
	out_len = ASAP_Convert(filename, &edited_module_info, saved_module, saved_module_len, out_module);
	if (out_len <= 0) {
		MessageBox(infoDialog, "Cannot convert file", "Error", MB_OK | MB_ICONERROR);
		return FALSE;
	}
	ofn.hwndOwner = infoDialog;
	ofn.lpstrDefExt = convert_ext;
	if (!GetSaveFileName(&ofn))
		return FALSE;
	if (!saveFile(convert_filename, out_module, out_len)) {
		MessageBox(infoDialog, "Cannot save file", "Error", MB_OK | MB_ICONERROR);
		return FALSE;
	}
	if (strcmp(convert_ext, "sap") == 0)
		saved_module_info = edited_module_info;
	return TRUE;
}

static INT_PTR CALLBACK infoDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char str[ASAP_DURATION_CHARS];
	switch (uMsg) {
	case WM_INITDIALOG:
#ifdef WINAMP
		CheckDlgButton(hDlg, IDC_PLAYING, playing_info ? BST_CHECKED : BST_UNCHECKED);
#endif
		SendDlgItemMessage(hDlg, IDC_AUTHOR, EM_LIMITTEXT, sizeof(edited_module_info.author) - 1, 0);
		SendDlgItemMessage(hDlg, IDC_NAME, EM_LIMITTEXT, sizeof(edited_module_info.name) - 1, 0);
		SendDlgItemMessage(hDlg, IDC_DATE, EM_LIMITTEXT, sizeof(edited_module_info.date) - 1, 0);
		SendDlgItemMessage(hDlg, IDC_TIME, EM_LIMITTEXT, 9, 0);
		return TRUE;
	case WM_COMMAND:
		switch (wParam) {
#ifdef WINAMP
		case MAKEWPARAM(IDC_PLAYING, BN_CLICKED):
			playing_info = (IsDlgButtonChecked(hDlg, IDC_PLAYING) == BST_CHECKED);
			if (playing_info)
				updateInfoDialog(current_filename, current_song);
			return TRUE;
#endif
		case MAKEWPARAM(IDC_AUTHOR, EN_CHANGE):
			SendDlgItemMessage(hDlg, IDC_AUTHOR, WM_GETTEXT, sizeof(edited_module_info.author), (LPARAM) edited_module_info.author);
			updateSaveButton();
			return TRUE;
		case MAKEWPARAM(IDC_NAME, EN_CHANGE):
			SendDlgItemMessage(hDlg, IDC_NAME, WM_GETTEXT, sizeof(edited_module_info.name), (LPARAM) edited_module_info.name);
			updateSaveButton();
			return TRUE;
		case MAKEWPARAM(IDC_DATE, EN_CHANGE):
			SendDlgItemMessage(hDlg, IDC_DATE, WM_GETTEXT, sizeof(edited_module_info.date), (LPARAM) edited_module_info.date);
			updateSaveButton();
			return TRUE;
		case MAKEWPARAM(IDC_TIME, EN_CHANGE):
			SendDlgItemMessage(hDlg, IDC_TIME, WM_GETTEXT, sizeof(str), (LPARAM) str);
			edited_module_info.durations[edited_song] = ASAP_ParseDuration(str);
			updateSaveButton();
			return TRUE;
		case MAKEWPARAM(IDC_LOOP, BN_CLICKED):
			edited_module_info.loops[edited_song] = (IsDlgButtonChecked(hDlg, IDC_LOOP) == BST_CHECKED);
			updateSaveButton();
			return TRUE;
		case MAKEWPARAM(IDC_SONGNO, CBN_SELCHANGE):
			edited_song = SendDlgItemMessage(hDlg, IDC_SONGNO, CB_GETCURSEL, 0, 0);
			showSongTime();
			return TRUE;
		case MAKEWPARAM(IDC_SAVE, BN_CLICKED):
			saveInfo();
			return TRUE;
		case MAKEWPARAM(IDC_CONVERT, BN_CLICKED):
			convert();
			return TRUE;
		case MAKEWPARAM(IDCANCEL, BN_CLICKED):
			if (infoChanged()) {
				BOOL ok;
				switch (MessageBox(hDlg, can_save ? "Save changes?" : "Convert to SAP?", "ASAP", MB_YESNOCANCEL | MB_ICONQUESTION)) {
				case IDYES:
					ok = can_save ? saveInfo() : convert();
					if (!ok)
						return TRUE;
					break;
				case IDCANCEL:
					return TRUE;
				default:
					break;
				}
			}
			DestroyWindow(hDlg);
			infoDialog = NULL;
			return TRUE;
		}
		break;
	default:
		break;
	}
	return FALSE;
}

void showInfoDialog(HINSTANCE hInstance, HWND hwndParent, const char *filename, int song)
{
	if (infoDialog == NULL) {
		edited_module_info = saved_module_info;
		infoDialog = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_INFO), hwndParent, infoDialogProc);
	}
	updateInfoDialog(filename, song);
}

void updateInfoDialog(const char *filename, int song)
{
	BOOL can_edit;
	int i;
	if (infoDialog == NULL || infoChanged())
		return;
	if (!loadModule(filename, saved_module, &saved_module_len)
	 || !ASAP_GetModuleInfo(&saved_module_info, filename, saved_module, saved_module_len)) {
		DestroyWindow(infoDialog);
		infoDialog = NULL;
		return;
	}
	edited_module_info = saved_module_info;
	can_save = ASAP_CanSetModuleInfo(filename);
	convert_ext = ASAP_CanConvert(filename, &edited_module_info, saved_module, saved_module_len);
	can_edit = can_save || convert_ext != NULL;
	SendDlgItemMessage(infoDialog, IDC_AUTHOR, EM_SETREADONLY, !can_edit, 0);
	SendDlgItemMessage(infoDialog, IDC_NAME, EM_SETREADONLY, !can_edit, 0);
	SendDlgItemMessage(infoDialog, IDC_DATE, EM_SETREADONLY, !can_edit, 0);
	SendDlgItemMessage(infoDialog, IDC_FILENAME, WM_SETTEXT, 0, (LPARAM) filename);
	SendDlgItemMessage(infoDialog, IDC_AUTHOR, WM_SETTEXT, 0, (LPARAM) saved_module_info.author);
	SendDlgItemMessage(infoDialog, IDC_NAME, WM_SETTEXT, 0, (LPARAM) saved_module_info.name);
	SendDlgItemMessage(infoDialog, IDC_DATE, WM_SETTEXT, 0, (LPARAM) saved_module_info.date);
	SendDlgItemMessage(infoDialog, IDC_SONGNO, CB_RESETCONTENT, 0, 0);
	EnableWindow(GetDlgItem(infoDialog, IDC_SONGNO), saved_module_info.songs > 1);
	for (i = 1; i <= saved_module_info.songs; i++) {
		char str[16];
		*appendInt(str, i) = '\0';
		SendDlgItemMessage(infoDialog, IDC_SONGNO, CB_ADDSTRING, 0, (LPARAM) str);
	}
	if (song < 0)
		song = saved_module_info.default_song;
	SendDlgItemMessage(infoDialog, IDC_SONGNO, CB_SETCURSEL, song, 0);
	edited_song = song;
	showSongTime();
	SendDlgItemMessage(infoDialog, IDC_TIME, EM_SETREADONLY, !can_edit, 0);
	EnableWindow(GetDlgItem(infoDialog, IDC_LOOP), can_edit);
	EnableWindow(GetDlgItem(infoDialog, IDC_SAVE), FALSE);
	if (convert_ext != NULL) {
		char convert_command[24] = "&Convert to ";
		i = 0;
		do
			convert_command[12 + i] = convert_ext[i] >= 'a' ? convert_ext[i] - 'a' + 'A' : convert_ext[i];
		while (convert_ext[i++] != '\0');
		SendDlgItemMessage(infoDialog, IDC_CONVERT, WM_SETTEXT, 0, (LPARAM) convert_command);
		strcpy(convert_filename, filename);
		ASAP_ChangeExt(convert_filename, convert_ext);
		EnableWindow(GetDlgItem(infoDialog, IDC_CONVERT), TRUE);
	}
	else {
		SendDlgItemMessage(infoDialog, IDC_CONVERT, WM_SETTEXT, 0, (LPARAM) "&Convert");
		EnableWindow(GetDlgItem(infoDialog, IDC_CONVERT), FALSE);
	}
}

#endif
