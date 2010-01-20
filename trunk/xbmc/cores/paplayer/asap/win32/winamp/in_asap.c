/*
 * in_asap.c - ASAP plugin for Winamp
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
#include <commctrl.h>
#include <string.h>

#include "in2.h"
#include "ipc_pe.h"
#include "wa_ipc.h"

#include "asap.h"
#include "gui.h"

// Winamp's equalizer works only with 16-bit samples
#define SUPPORT_EQUALIZER  1

static In_Module mod;

// configuration
#define INI_SECTION  "in_asap"
static const char *ini_file;

// current file
ASAP_State asap;
char current_filename[MAX_PATH] = "";
static char current_filename_with_song[MAX_PATH + 3] = "";
int current_song;
static byte module[ASAP_MODULE_MAX];
static int module_len;
static int duration;
#define channels  asap.module_info.channels

static int playlistLength;

static HANDLE thread_handle = NULL;
static volatile int thread_run = FALSE;
static int paused = 0;
static int seek_needed;

static void writeIniInt(const char *name, int value)
{
	char str[16];
	*appendInt(str, value) = '\0';
	WritePrivateProfileString(INI_SECTION, name, str, ini_file);
}

static void config(HWND hwndParent)
{
	if (settingsDialog(mod.hDllInstance, hwndParent)) {
		writeIniInt("song_length", song_length);
		writeIniInt("silence_seconds", silence_seconds);
		writeIniInt("play_loops", play_loops);
		writeIniInt("mute_mask", mute_mask);
	}
}

static void about(HWND hwndParent)
{
	MessageBox(hwndParent, ASAP_CREDITS "\n" ASAP_COPYRIGHT,
		"About ASAP Winamp plugin " ASAP_VERSION, MB_OK);
}

static int extractSongNumber(const char *s, char *filename)
{
	int i = strlen(s);
	int song = -1;
	if (i > 6 && s[i - 1] >= '0' && s[i - 1] <= '9') {
		if (s[i - 2] == '#') {
			song = s[i - 1] - '1';
			i -= 2;
		}
		else if (s[i - 2] >= '0' && s[i - 2] <= '9' && s[i - 3] == '#') {
			song = (s[i - 2] - '0') * 10 + s[i - 1] - '1';
			i -= 3;
		}
	}
	memcpy(filename, s, i);
	filename[i] = '\0';
	return song;
}

static void expandFileSongs(HWND playlistWnd, int index)
{
	const char *fn;
	fileinfo fi;
	int song;
	ASAP_ModuleInfo module_info;
	char *p;
	int j;
	fn = (const char *) SendMessage(mod.hMainWindow, WM_WA_IPC, index, IPC_GETPLAYLISTFILE);
	song = extractSongNumber(fn, fi.file);
	if (song >= 0 || !ASAP_IsOurFile(fi.file))
		return;
	if (!loadModule(fi.file, module, &module_len))
		return;
	if (!ASAP_GetModuleInfo(&module_info, fi.file, module, module_len))
		return;
	SendMessage(playlistWnd, WM_WA_IPC, IPC_PE_DELETEINDEX, index);
	p = fi.file + strlen(fi.file);
	for (j = 0; j < module_info.songs; j++) {
		COPYDATASTRUCT cds;
		*p = '#';
		*appendInt(p + 1, j + 1) = '\0';
		fi.index = index + j;
		cds.dwData = IPC_PE_INSERTFILENAME;
		cds.lpData = &fi;
		cds.cbData = sizeof(fileinfo);
		SendMessage(playlistWnd, WM_COPYDATA, 0, (LPARAM) &cds);
	}
}

static INT_PTR CALLBACK progressDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_INITDIALOG)
		SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, playlistLength));
	return FALSE;
}

static void expandPlaylistSongs(void)
{
	static BOOL processing = FALSE;
	HWND playlistWnd;
	HWND progressWnd;
	int index;
	if (processing)
		return;
	playlistWnd = (HWND) SendMessage(mod.hMainWindow, WM_WA_IPC, IPC_GETWND_PE, IPC_GETWND);
	if (playlistWnd == NULL)
		return;
	processing = TRUE;
	playlistLength = SendMessage(mod.hMainWindow, WM_WA_IPC, 0, IPC_GETLISTLENGTH);
	progressWnd = CreateDialog(mod.hDllInstance, MAKEINTRESOURCE(IDD_PROGRESS), mod.hMainWindow, progressDialogProc);
	index = playlistLength;
	while (--index >= 0) {
		if ((index & 15) == 0)
			SendDlgItemMessage(progressWnd, IDC_PROGRESS, PBM_SETPOS, playlistLength - index, 0);
		expandFileSongs(playlistWnd, index);
	}
	DestroyWindow(progressWnd);
	processing = FALSE;
}

static void init(void)
{
	ini_file = (const char *) SendMessage(mod.hMainWindow, WM_WA_IPC, 0, IPC_GETINIFILE);
	song_length = GetPrivateProfileInt(INI_SECTION, "song_length", song_length, ini_file);
	silence_seconds = GetPrivateProfileInt(INI_SECTION, "silence_seconds", silence_seconds, ini_file);
	play_loops = GetPrivateProfileInt(INI_SECTION, "play_loops", play_loops, ini_file);
	mute_mask = GetPrivateProfileInt(INI_SECTION, "mute_mask", mute_mask, ini_file);
}

static void quit(void)
{
}

static int title_song;
static ASAP_ModuleInfo title_module_info;

static void getTitle(char *title)
{
	char *p = appendString(title, title_module_info.name);
	if (title_module_info.songs > 1) {
		p = appendString(p, " (song ");
		p = appendInt(p, title_song + 1);
		*p++ = ')';
	}
	*p = '\0';
}

static char *tagFunc(char *tag, void *p)
{
	if (stricmp(tag, "artist") == 0 && title_module_info.author[0] != '\0')
		return strdup(title_module_info.author);
	if (stricmp(tag, "title") == 0) {
		char *title = malloc(strlen(title_module_info.name) + 11);
		if (title != NULL)
			getTitle(title);
		return title;
	}
	return NULL;
}

static void tagFreeFunc(char *tag, void *p)
{
	free(tag);
}

static void getFileInfo(char *file, char *title, int *length_in_ms)
{
	char filename[MAX_PATH];
	if (file == NULL || file[0] == '\0')
		file = current_filename_with_song;
	title_song = extractSongNumber(file, filename);
	if (title_song < 0)
		expandPlaylistSongs();
	if (!loadModule(filename, module, &module_len))
		return;
	if (!ASAP_GetModuleInfo(&title_module_info, filename, module, module_len))
		return;
	if (title_song < 0)
		title_song = title_module_info.default_song;
	if (title != NULL) {
		waFormatTitle fmt_title = {
			NULL, NULL, title, 512, tagFunc, tagFreeFunc
		};
		getTitle(title); // in case IPC_FORMAT_TITLE doesn't work...
		SendMessage(mod.hMainWindow, WM_WA_IPC, (WPARAM) &fmt_title, IPC_FORMAT_TITLE);
	}
	if (length_in_ms != NULL)
		*length_in_ms = getSongDuration(&title_module_info, title_song);
}

static int infoBox(char *file, HWND hwndParent)
{
	char filename[MAX_PATH];
	int song;
	song = extractSongNumber(file, filename);
	showInfoDialog(mod.hDllInstance, hwndParent, filename, song);
	return 0;
}

static int isOurFile(char *fn)
{
	char filename[MAX_PATH];
	extractSongNumber(fn, filename);
	return ASAP_IsOurFile(filename);
}

static DWORD WINAPI playThread(LPVOID dummy)
{
	while (thread_run) {
		static
#if BITS_PER_SAMPLE == 8
			byte
#else
			short
#endif
			buffer[BUFFERED_BLOCKS * 2
#if SUPPORT_EQUALIZER
			* 2
#endif
			];
		int buffered_bytes = BUFFERED_BLOCKS * channels * (BITS_PER_SAMPLE / 8);
		if (seek_needed >= 0) {
			mod.outMod->Flush(seek_needed);
			ASAP_Seek(&asap, seek_needed);
			seek_needed = -1;
		}
		if (mod.outMod->CanWrite() >= buffered_bytes
#if SUPPORT_EQUALIZER
			<< mod.dsp_isactive()
#endif
		) {
			int t;
			buffered_bytes = ASAP_Generate(&asap, buffer, buffered_bytes, BITS_PER_SAMPLE);
			if (buffered_bytes <= 0) {
				mod.outMod->CanWrite();
				if (!mod.outMod->IsPlaying()) {
					PostMessage(mod.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
					return 0;
				}
				Sleep(10);
				continue;
			}
			t = mod.outMod->GetWrittenTime();
			mod.SAAddPCMData(buffer, channels, BITS_PER_SAMPLE, t);
			mod.VSAAddPCMData(buffer, channels, BITS_PER_SAMPLE, t);
#if SUPPORT_EQUALIZER
			t = buffered_bytes / (channels * (BITS_PER_SAMPLE / 8));
			t = mod.dsp_dosamples((short *) buffer, t, BITS_PER_SAMPLE, channels, ASAP_SAMPLE_RATE);
			t *= channels * (BITS_PER_SAMPLE / 8);
			mod.outMod->Write((char *) buffer, t);
#else
			mod.outMod->Write((char *) buffer, buffered_bytes);
#endif
		}
		else
			Sleep(20);
	}
	return 0;
}

static int play(char *fn)
{
	int song;
	int maxlatency;
	DWORD threadId;
	strcpy(current_filename_with_song, fn);
	song = extractSongNumber(fn, current_filename);
	if (!loadModule(current_filename, module, &module_len))
		return -1;
	if (!ASAP_Load(&asap, current_filename, module, module_len))
		return 1;
	if (song < 0)
		song = asap.module_info.default_song;
	duration = playSong(song);
	maxlatency = mod.outMod->Open(ASAP_SAMPLE_RATE, channels, BITS_PER_SAMPLE, -1, -1);
	if (maxlatency < 0)
		return 1;
	mod.SetInfo(BITS_PER_SAMPLE, ASAP_SAMPLE_RATE / 1000, channels, 1);
	mod.SAVSAInit(maxlatency, ASAP_SAMPLE_RATE);
	// the order of VSASetInfo's arguments in in2.h is wrong!
	// http://forums.winamp.com/showthread.php?postid=1841035
	mod.VSASetInfo(ASAP_SAMPLE_RATE, channels);
	mod.outMod->SetVolume(-666);
	seek_needed = -1;
	thread_run = TRUE;
	thread_handle = CreateThread(NULL, 0, playThread, NULL, 0, &threadId);
	if (playing_info)
		updateInfoDialog(current_filename, song);
	return thread_handle != NULL ? 0 : 1;
}

static void pause(void)
{
	paused = 1;
	mod.outMod->Pause(1);
}

static void unPause(void)
{
	paused = 0;
	mod.outMod->Pause(0);
}

static int isPaused(void)
{
	return paused;
}

static void stop(void)
{
	if (thread_handle != NULL) {
		thread_run = FALSE;
		// wait max 10 seconds
		if (WaitForSingleObject(thread_handle, 10 * 1000) == WAIT_TIMEOUT)
			TerminateThread(thread_handle, 0);
		CloseHandle(thread_handle);
		thread_handle = NULL;
	}
	mod.outMod->Close();
	mod.SAVSADeInit();
}

static int getLength(void)
{
	return duration;
}

static int getOutputTime(void)
{
	return mod.outMod->GetOutputTime();
}

static void setOutputTime(int time_in_ms)
{
	seek_needed = time_in_ms;
}

static void setVolume(int volume)
{
	mod.outMod->SetVolume(volume);
}

static void setPan(int pan)
{
	mod.outMod->SetPan(pan);
}

static void eqSet(int on, char data[10], int preamp)
{
}

static In_Module mod = {
	IN_VER,
	"ASAP " ASAP_VERSION,
	0, 0, // filled by Winamp
	"SAP\0Slight Atari Player (*.SAP)\0"
	"CMC;CM3;CMR;CMS;DMC\0Chaos Music Composer (*.CMC;*.CM3;*.CMR;*.CMS;*.DMC)\0"
	"DLT\0Delta Music Composer (*.DLT)\0"
	"MPT;MPD\0Music ProTracker (*.MPT;*.MPD)\0"
	"RMT\0Raster Music Tracker (*.RMT)\0"
	"TMC;TM8\0Theta Music Composer 1.x (*.TMC;*.TM8)\0"
	"TM2\0Theta Music Composer 2.x (*.TM2)\0"
	,
	1,    // is_seekable
	1,    // UsesOutputPlug
	config,
	about,
	init,
	quit,
	getFileInfo,
	infoBox,
	isOurFile,
	play,
	pause,
	unPause,
	isPaused,
	stop,
	getLength,
	getOutputTime,
	setOutputTime,
	setVolume,
	setPan,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // filled by Winamp
	eqSet,
	NULL, // SetInfo
	NULL  // filled by Winamp
};

__declspec(dllexport) In_Module *winampGetInModule2(void)
{
	return &mod;
}
