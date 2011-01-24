/*
 * wasap.c - Another Slight Atari Player for Win32 systems
 *
 * Copyright (C) 2005-2009  Piotr Fusik
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
#include <shellapi.h>

#include "asap.h"
#include "gui.h"

#define APP_TITLE        "WASAP"
#define WND_CLASS_NAME   "WASAP"
#define BUFFERED_BLOCKS  4096

static ASAP_State asap;
static int songs = 0;
static char current_filename[MAX_PATH] = "";
static int current_song;

static HWND hWnd;


/* WaveOut ---------------------------------------------------------------- */

/* double-buffering, *2 for 16-bit, *2 for stereo */
static byte buffer[2][BUFFERED_BLOCKS * 2 * 2];
static HWAVEOUT hwo = INVALID_HANDLE_VALUE;
static WAVEHDR wh[2] = {
	{ buffer[0], 0, 0, 0, 0, 0, NULL, 0 },
	{ buffer[1], 0, 0, 0, 0, 0, NULL, 0 },
};
static BOOL playing = FALSE;

static void WaveOut_Stop(void)
{
	if (playing) {
		playing = FALSE;
		waveOutReset(hwo);
	}
}

static void WaveOut_Write(LPWAVEHDR pwh)
{
	if (playing) {
		int len = ASAP_Generate(&asap, pwh->lpData, pwh->dwBufferLength, BITS_PER_SAMPLE);
		if (len < (int) pwh->dwBufferLength
		 || waveOutWrite(hwo, pwh, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			/* calling StopPlayback() here causes a deadlock */
			PostMessage(hWnd, WM_COMMAND, IDM_STOP, 0);
		}
	}
}

static void CALLBACK WaveOut_Proc(HWAVEOUT hwo2, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	if (uMsg == WOM_DONE)
		WaveOut_Write((LPWAVEHDR) dwParam1);
}

static int WaveOut_Open(int channels)
{
	WAVEFORMATEX wfx;
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = channels;
	wfx.nSamplesPerSec = ASAP_SAMPLE_RATE;
	wfx.nBlockAlign = channels * (BITS_PER_SAMPLE / 8);
	wfx.nAvgBytesPerSec = ASAP_SAMPLE_RATE * wfx.nBlockAlign;
	wfx.wBitsPerSample = BITS_PER_SAMPLE;
	wfx.cbSize = 0;
	if (waveOutOpen(&hwo, WAVE_MAPPER, &wfx, (DWORD) WaveOut_Proc, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
		return FALSE;
	wh[1].dwBufferLength = wh[0].dwBufferLength = BUFFERED_BLOCKS * wfx.nBlockAlign;
	if (waveOutPrepareHeader(hwo, &wh[0], sizeof(wh[0])) != MMSYSERR_NOERROR
	 || waveOutPrepareHeader(hwo, &wh[1], sizeof(wh[1])) != MMSYSERR_NOERROR)
		return FALSE;
	return TRUE;
}

static void WaveOut_Start(void)
{
	playing = TRUE;
	WaveOut_Write(&wh[0]);
	WaveOut_Write(&wh[1]);
}

static void WaveOut_Close(void)
{
	if (hwo == INVALID_HANDLE_VALUE)
		return;
	WaveOut_Stop();
	if (wh[0].dwFlags & WHDR_PREPARED)
		waveOutUnprepareHeader(hwo, &wh[0], sizeof(wh[0]));
	if (wh[1].dwFlags & WHDR_PREPARED)
		waveOutUnprepareHeader(hwo, &wh[1], sizeof(wh[1]));
	waveOutClose(hwo);
	hwo = INVALID_HANDLE_VALUE;
}


/* Tray ------------------------------------------------------------------- */

#define MYWM_NOTIFYICON  (WM_APP + 1)
static NOTIFYICONDATA nid = {
	sizeof(NOTIFYICONDATA),
	NULL,
	0,
	NIF_ICON | NIF_MESSAGE | NIF_TIP,
	MYWM_NOTIFYICON,
	NULL,
	APP_TITLE
};
static UINT taskbarCreatedMessage;

static void Tray_Modify(HICON hIcon)
{
	char *p;
	nid.hIcon = hIcon;
	/* we need to be careful because szTip is only 64 characters */
	/* 5 */
	p = appendString(nid.szTip, APP_TITLE);
	if (songs > 0) {
		const char *pb;
		const char *pe;
		for (pe = current_filename; *pe != '\0'; pe++);
		for (pb = pe; pb > current_filename && pb[-1] != '\\' && pb[-1] != '/'; pb--);
		/* 2 */
		*p++ = ':';
		*p++ = ' ';
		/* max 33 */
		if (pe - pb <= 33)
			p = appendString(p, pb);
		else {
			memcpy(p, pb, 30);
			p = appendString(p + 30, "...");
		}
		if (songs > 1) {
			/* 7 */
			p = appendString(p, " (song ");
			/* max 3 */
			p = appendInt(p, current_song + 1);
			/* 4 */
			p = appendString(p, " of ");
			/* max 3 */
			p = appendInt(p, songs);
			/* 1 */
			*p++ = ')';
		}
	}
	/* 1 */
	*p = '\0';
	Shell_NotifyIcon(NIM_MODIFY, &nid);
}


/* GUI -------------------------------------------------------------------- */

static BOOL errorShown = FALSE;
static HINSTANCE hInst;
static HICON hStopIcon;
static HICON hPlayIcon;
static HMENU hTrayMenu;
static HMENU hSongMenu;

static void ShowError(const char *message)
{
	errorShown = TRUE;
	MessageBox(hWnd, message, APP_TITLE, MB_OK | MB_ICONERROR);
	errorShown = FALSE;
}

static void ShowAbout(void)
{
	MSGBOXPARAMS mbp = {
		sizeof(MSGBOXPARAMS),
		hWnd,
		hInst,
		ASAP_CREDITS
		"WASAP icons (C) 2005 Lukasz Sychowicz\n\n"
		ASAP_COPYRIGHT,
		APP_TITLE " " ASAP_VERSION,
		MB_OK | MB_USERICON,
		MAKEINTRESOURCE(IDI_APP),
		0,
		NULL,
		LANG_NEUTRAL
	};
	MessageBoxIndirect(&mbp);
}

static void ClearSongsMenu(void)
{
	int n = GetMenuItemCount(hSongMenu);
	while (--n >= 0)
		DeleteMenu(hSongMenu, n, MF_BYPOSITION);
}

static void SetSongsMenu(int n)
{
	int i;
	for (i = 1; i <= n; i++) {
		char str[16];
		*appendInt(str, i) = '\0';
		AppendMenu(hSongMenu, MF_ENABLED | MF_STRING, IDM_SONG1 + i - 1, str);
	}
}

static void StopPlayback(void)
{
	WaveOut_Stop();
	Tray_Modify(hStopIcon);
}

static BOOL DoLoad(ASAP_State *asap, byte module[], int module_len)
{
	if (!ASAP_Load(asap, current_filename, module, module_len)) {
		ShowError("Unsupported file format");
		return FALSE;
	}
	return TRUE;
}

static void LoadAndPlay(int song)
{
	byte module[ASAP_MODULE_MAX];
	int module_len;
	int duration;
	if (!loadModule(current_filename, module, &module_len))
		return;
	if (songs > 0) {
		ClearSongsMenu();
		StopPlayback();
		songs = 0;
		EnableMenuItem(hTrayMenu, IDM_SAVE_WAV, MF_BYCOMMAND | MF_GRAYED);
	}
	if (!DoLoad(&asap, module, module_len))
		return;
	if (!WaveOut_Open(asap.module_info.channels)) {
		ShowError("Error initalizing WaveOut");
		return;
	}
	if (song < 0)
		song = asap.module_info.default_song;
	songs = asap.module_info.songs;
	EnableMenuItem(hTrayMenu, IDM_SAVE_WAV, MF_BYCOMMAND | MF_ENABLED);
	updateInfoDialog(current_filename, song);
	SetSongsMenu(songs);
	CheckMenuRadioItem(hSongMenu, 0, songs - 1, song, MF_BYPOSITION);
	current_song = song;
	duration = asap.module_info.durations[song];
	if (asap.module_info.loops[song])
		duration = -1;
	ASAP_PlaySong(&asap, song, duration);
	Tray_Modify(hPlayIcon);
	WaveOut_Start();
}

static int opening = FALSE;

static void SelectAndLoadFile(void)
{
	static OPENFILENAME ofn = {
		sizeof(OPENFILENAME),
		NULL,
		0,
		"All supported\0"
		"*.sap;*.cmc;*.cm3;*.cmr;*.cms;*.dmc;*.dlt;*.mpt;*.mpd;*.rmt;*.tmc;*.tm8;*.tm2\0"
		"Slight Atari Player (*.sap)\0"
		"*.sap\0"
		"Chaos Music Composer (*.cmc;*.cm3;*.cmr;*.cms;*.dmc)\0"
		"*.cmc;*.cm3;*.cmr;*.cms;*.dmc\0"
		"Delta Music Composer (*.dlt)\0"
		"*.dlt\0"
		"Music ProTracker (*.mpt;*.mpd)\0"
		"*.mpt;*.mpd\0"
		"Raster Music Tracker (*.rmt)\0"
		"*.rmt\0"
		"Theta Music Composer 1.x (*.tmc;*.tm8)\0"
		"*.tmc;*.tm8\0"
		"Theta Music Composer 2.x (*.tm2)\0"
		"*.tm2\0"
		"\0",
		NULL,
		0,
		1,
		current_filename,
		MAX_PATH,
		NULL,
		0,
		NULL,
		"Select 8-bit Atari music",
		OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY
			| OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
		0,
		0,
		NULL,
		0,
		NULL,
		NULL
	};
	opening = TRUE;
	ofn.hwndOwner = hWnd;
	if (GetOpenFileName(&ofn))
		LoadAndPlay(-1);
	opening = FALSE;
}

static char wav_filename[MAX_PATH];

static BOOL DoSaveWav(ASAP_State *asap)
{
	HANDLE fh;
	byte buffer[8192];
	DWORD len;
	fh = CreateFile(wav_filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fh == INVALID_HANDLE_VALUE)
		return FALSE;
	ASAP_GetWavHeader(asap, buffer, BITS_PER_SAMPLE);
	len = ASAP_WAV_HEADER_BYTES;
	while (len > 0) {
		if (!WriteFile(fh, buffer, len, &len, NULL)) {
			CloseHandle(fh);
			return FALSE;
		}
		len = ASAP_Generate(asap, buffer, sizeof(buffer), BITS_PER_SAMPLE);
	}
	CloseHandle(fh);
	return TRUE;
}

static void SaveWav(void)
{
	byte module[ASAP_MODULE_MAX];
	int module_len;
	ASAP_State asap;
	int duration;
	static OPENFILENAME ofn = {
		sizeof(OPENFILENAME),
		NULL,
		0,
		"WAV files (*.wav)\0*.wav\0\0",
		NULL,
		0,
		0,
		wav_filename,
		MAX_PATH,
		NULL,
		0,
		NULL,
		"Select output file",
		OFN_ENABLESIZING | OFN_EXPLORER | OFN_OVERWRITEPROMPT,
		0,
		0,
		"wav",
		0,
		NULL,
		NULL
	};
	if (!loadModule(current_filename, module, &module_len))
		return;
	if (!DoLoad(&asap, module, module_len))
		return;
	duration = asap.module_info.durations[current_song];
	if (duration < 0) {
		if (MessageBox(hWnd,
			"This song has unknown duration.\n"
			"Use \"File information\" to update the source file with the correct duration.\n"
			"Do you want to save 3 minutes?",
			"Unknown duration", MB_YESNO | MB_ICONWARNING) != IDYES)
			return;
		duration = 180000;
	}
	ASAP_PlaySong(&asap, current_song, duration);
	strcpy(wav_filename, current_filename);
	ASAP_ChangeExt(wav_filename, "wav");
	ofn.hwndOwner = hWnd;
	if (!GetSaveFileName(&ofn))
		return;
	if (!DoSaveWav(&asap))
		ShowError("Cannot save file");
}

static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int idc;
	POINT pt;
	PCOPYDATASTRUCT pcds;
	switch (msg) {
	case WM_COMMAND:
		idc = LOWORD(wParam);
		switch (idc) {
		case IDM_OPEN:
			SelectAndLoadFile();
			break;
		case IDM_STOP:
			StopPlayback();
			break;
		case IDM_FILE_INFO:
			showInfoDialog(hInst, hWnd, current_filename, current_song);
			break;
		case IDM_SAVE_WAV:
			SaveWav();
			break;
		case IDM_ABOUT:
			ShowAbout();
			break;
		case IDM_EXIT:
			PostQuitMessage(0);
			break;
		default:
			if (idc >= IDM_SONG1 && idc < IDM_SONG1 + songs)
				LoadAndPlay(idc - IDM_SONG1);
			break;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case MYWM_NOTIFYICON:
		if (opening) {
			SetForegroundWindow(GetLastActivePopup(hWnd));
			break;
		}
		switch (lParam) {
		case WM_LBUTTONDOWN:
			SelectAndLoadFile();
			break;
		case WM_MBUTTONDOWN:
			if (songs <= 1)
				break;
			/* FALLTHROUGH */
		case WM_RBUTTONUP:
			GetCursorPos(&pt);
			SetForegroundWindow(hWnd);
			TrackPopupMenu(lParam == WM_MBUTTONDOWN ? hSongMenu : hTrayMenu,
				TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON,
				pt.x, pt.y, 0, hWnd, NULL);
			PostMessage(hWnd, WM_NULL, 0, 0);
			break;
		default:
			break;
		}
		break;
	case WM_COPYDATA:
		pcds = (PCOPYDATASTRUCT) lParam;
		if (pcds->dwData == 'O' && pcds->cbData <= sizeof(current_filename)) {
			if (errorShown) {
				HWND hChild = GetLastActivePopup(hWnd);
				if (hChild != hWnd)
					SendMessage(hChild, WM_CLOSE, 0, 0);
			}
			memcpy(current_filename, pcds->lpData, pcds->cbData);
			LoadAndPlay(-1);
		}
		break;
	default:
		if (msg == taskbarCreatedMessage)
			Shell_NotifyIcon(NIM_ADD, &nid);
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char *pb;
	char *pe;
	WNDCLASS wc;
	HMENU hMainMenu;
	MSG msg;

	for (pb = lpCmdLine; *pb == ' ' || *pb == '\t'; pb++);
	for (pe = pb; *pe != '\0'; pe++);
	while (--pe > pb && (*pe == ' ' || *pe == '\t'));
	/* Now pb and pe point at respectively the first and last non-blank
	   character in lpCmdLine. If pb > pe then the command line is blank. */
	if (*pb == '"' && *pe == '"')
		pb++;
	else
		pe++;
	*pe = '\0';
	/* Now pb contains the filename, if any, specified on the command line. */

	hWnd = FindWindow(WND_CLASS_NAME, NULL);
	if (hWnd != NULL) {
		/* an instance of WASAP is already running */
		if (*pb != '\0') {
			/* pass the filename */
			COPYDATASTRUCT cds = { 'O', (DWORD) (pe + 1 - pb), pb };
			SendMessage(hWnd, WM_COPYDATA, (WPARAM) NULL, (LPARAM) &cds);
		}
		else {
			/* bring the open dialog to top */
			HWND hChild = GetLastActivePopup(hWnd);
			if (hChild != hWnd)
				SetForegroundWindow(hChild);
		}
		return 0;
	}

	hInst = hInstance;

	wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WND_CLASS_NAME;
	RegisterClass(&wc);

	hWnd = CreateWindow(WND_CLASS_NAME,
		APP_TITLE,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	hStopIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_STOP));
	hPlayIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PLAY));
	hMainMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_TRAYMENU));
	hTrayMenu = GetSubMenu(hMainMenu, 0);
	hSongMenu = CreatePopupMenu();
	InsertMenu(hTrayMenu, 1, MF_BYPOSITION | MF_ENABLED | MF_STRING | MF_POPUP,
		(UINT_PTR) hSongMenu, "So&ng");
	SetMenuDefaultItem(hTrayMenu, 0, TRUE);
	nid.hWnd = hWnd;
	nid.hIcon = hStopIcon;
	Shell_NotifyIcon(NIM_ADD, &nid);
	taskbarCreatedMessage = RegisterWindowMessage("TaskbarCreated");

	if (*pb != '\0') {
		memcpy(current_filename, pb, pe + 1 - pb);
		LoadAndPlay(-1);
	}
	else
		SelectAndLoadFile();

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (infoDialog == NULL || !IsDialogMessage(infoDialog, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	WaveOut_Close();
	Shell_NotifyIcon(NIM_DELETE, &nid);
	DestroyMenu(hMainMenu);
	return 0;
}
