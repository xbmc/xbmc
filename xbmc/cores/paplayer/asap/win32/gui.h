/*
 * gui.h - settings and file information dialog boxes
 *
 * Copyright (C) 2007-2008  Piotr Fusik
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

#ifdef __cplusplus
extern "C" {
#endif

#define BITS_PER_SAMPLE          16

#define IDC_STATIC     -1
#define IDD_INFO       300
#define IDC_PLAYING    301
#define IDC_FILENAME   302
#define IDC_AUTHOR     303
#define IDC_NAME       304
#define IDC_DATE       305
#define IDC_SONGNO     306
#define IDC_TIME       307
#define IDC_LOOP       308
#define IDC_SAVE       309
#define IDC_CONVERT    310

char *appendString(char *dest, const char *src);
char *appendInt(char *dest, int x);
BOOL loadModule(const char *filename, byte *module, int *module_len);

extern HWND infoDialog;
void showInfoDialog(HINSTANCE hInstance, HWND hwndParent, const char *filename, int song);
void updateInfoDialog(const char *filename, int song);

#ifdef WASAP

#define IDI_APP        101
#define IDI_STOP       102
#define IDI_PLAY       103
#define IDR_TRAYMENU   200
#define IDM_OPEN       201
#define IDM_STOP       202
#define IDM_FILE_INFO  203
#define IDM_SAVE_WAV   204
#define IDM_ABOUT      205
#define IDM_EXIT       206
#define IDM_SONG1      211

#else /* WASAP */

/* config items */
#define DEFAULT_SONG_LENGTH      180
#define DEFAULT_SILENCE_SECONDS  2
/* 576 is a magic number for Winamp, better do not modify it */
#define BUFFERED_BLOCKS          576

#ifndef FOOBAR2000
extern ASAP_State asap;
extern int song_length;
extern int silence_seconds;
extern BOOL play_loops;
extern int mute_mask;
#endif
#ifdef WINAMP
extern char current_filename[MAX_PATH];
extern int current_song;
extern BOOL playing_info;
#endif

/* resource identifiers */
#define IDD_SETTINGS   400
#define IDC_UNLIMITED  401
#define IDC_LIMITED    402
#define IDC_MINUTES    403
#define IDC_SECONDS    404
#define IDC_SILENCE    405
#define IDC_SILSECONDS 406
#define IDC_LOOPS      407
#define IDC_NOLOOPS    408
#define IDC_MUTE1      411
#define IDD_PROGRESS   500
#define IDC_PROGRESS   501

/* functions */
BOOL settingsDialog(HINSTANCE hInstance, HWND hwndParent);
int getSongDuration(const ASAP_ModuleInfo *module_info, int song);
int playSong(int song);

#endif /* WASAP */

#ifdef __cplusplus
}
#endif
