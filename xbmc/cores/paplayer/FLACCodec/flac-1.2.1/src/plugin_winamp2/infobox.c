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
#include <stdio.h>
#include "FLAC/all.h"
#include "share/alloc.h"
#include "plugin_common/all.h"
#include "infobox.h"
#include "configure.h"
#include "resource.h"


typedef struct
{
	char filename[MAX_PATH];
	FLAC__StreamMetadata *tags;
} LOCALDATA;

static char buffer[8192];
static char *genres = NULL;
static DWORD genresSize = 0, genresCount = 0;
static BOOL genresChanged = FALSE, isNT;

static const char infoTitle[] = "FLAC File Info";

/*
 *  Genres
 */

/* TODO: write genres in utf-8 ? */

static __inline int GetGenresFileName(char *buffer, int size)
{
	char *c;

	if (!GetModuleFileName(NULL, buffer, size))
		return 0;
	c = strrchr(buffer, '\\');
	if (!c) return 0;
	strcpy(c+1, "genres.txt");

	return 1;
}

static void LoadGenres()
{
	HANDLE hFile;
	DWORD  spam;
	char  *c;

	FLAC__ASSERT(0 != genres);

	if (!GetGenresFileName(buffer, sizeof(buffer))) return;
	/* load file */
	hFile = CreateFile(buffer, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return;
	genresSize = GetFileSize(hFile, 0);
	if (genresSize && (genres = (char*)safe_malloc_add_2op_(genresSize, /*+*/2)))
	{
		if (!ReadFile(hFile, genres, genresSize, &spam, NULL) || spam!=genresSize)
		{
			free(genres);
			genres = NULL;
		}
		else
		{
			genres[genresSize] = 0;
			genres[genresSize+1] = 0;
			/* replace newlines */
			genresChanged = FALSE;
			genresCount = 1;

			for (c=genres; *c; c++)
			{
				if (*c == 10)
				{
					*c = 0;
					if (*(c+1))
						genresCount++;
					else genresSize--;
				}
			}
		}
	}

	CloseHandle(hFile);
}

static void SaveGenres(HWND hlist)
{
	HANDLE hFile;
	DWORD  spam;
	int i, count, len;

	if (!GetGenresFileName(buffer, sizeof(buffer))) return;
	/* write file */
	hFile = CreateFile(buffer, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return;

	count = SendMessage(hlist, CB_GETCOUNT, 0, 0);
	for (i=0; i<count; i++)
	{
		SendMessage(hlist, CB_GETLBTEXT, i, (LPARAM)buffer);
		len = strlen(buffer);
		if (i != count-1)
		{
			buffer[len] = 10;
			len++;
		}
		WriteFile(hFile, buffer, len, &spam, NULL);
	}

	CloseHandle(hFile);
}

static void AddGenre(HWND hwnd, const char *genre)
{
	HWND hgen = GetDlgItem(hwnd, IDC_GENRE);

	if (SendMessage(hgen, CB_FINDSTRINGEXACT, -1, (LPARAM)genre) == CB_ERR)
	{
		genresChanged = TRUE;
		SendMessage(hgen, CB_ADDSTRING, 0, (LPARAM)genre);
	}
}

static void InitGenres(HWND hwnd)
{
	HWND hgen = GetDlgItem(hwnd, IDC_GENRE);
	char *c;

	/* set text length limit to 64 chars */
	SendMessage(hgen, CB_LIMITTEXT, 64, 0);
	/* try to load genres */
	if (!genres)
		LoadGenres(hgen);
	/* add the to list */
	if (genres)
	{
		SendMessage(hgen, CB_INITSTORAGE, genresCount, genresSize);

		for (c = genres; *c; c += strlen(c)+1)
			SendMessage(hgen, CB_ADDSTRING, 0, (LPARAM)c);
	}
}

static void DeinitGenres(HWND hwnd, BOOL final)
{
	if (genresChanged && hwnd)
	{
		SaveGenres(GetDlgItem(hwnd, IDC_GENRE));
		genresChanged = FALSE;
		final = TRUE;
	}
	if (final)
	{
		free(genres);
		genres = 0;
	}
}

static wchar_t *AnsiToWide(const char *src)
{
	int len;
	wchar_t *dest;

	FLAC__ASSERT(0 != src);

	len = strlen(src) + 1;
	/* copy */
	dest = (wchar_t*)safe_malloc_mul_2op_(len, /*times*/sizeof(wchar_t));
	if (dest) mbstowcs(dest, src, len);
	return dest;
}

/*
 *  Infobox helpers
 */

#define SetText(x,y)            ucs2 = FLAC_plugin__tags_get_tag_ucs2(data->tags, y); \
                                WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, ucs2, -1, buffer, sizeof(buffer), NULL, NULL); \
                                if(ucs2) free(ucs2); \
                                SetDlgItemText(hwnd, x, buffer)

#define GetText(x,y)            GetDlgItemText(hwnd, x, buffer, sizeof(buffer));                        \
                                if (*buffer) { ucs2 = AnsiToWide(buffer); FLAC_plugin__tags_set_tag_ucs2(data->tags, y, ucs2, /*replace_all=*/false); free(ucs2); } \
                                else FLAC_plugin__tags_delete_tag(data->tags, y)

#define SetTextW(x,y)           ucs2 = FLAC_plugin__tags_get_tag_ucs2(data->tags, y); \
                                SetDlgItemTextW(hwnd, x, ucs2); \
                                free(ucs2)

#define GetTextW(x,y)           GetDlgItemTextW(hwnd, x, (WCHAR*)buffer, sizeof(buffer)/2);                     \
                                if (*(WCHAR*)buffer) FLAC_plugin__tags_set_tag_ucs2(data->tags, y, (WCHAR*)buffer, /*replace_all=*/false); \
                                else FLAC_plugin__tags_delete_tag(data->tags, y)


static BOOL InitInfoboxInfo(HWND hwnd, const char *file)
{
	LOCALDATA *data = LocalAlloc(LPTR, sizeof(LOCALDATA));
	wchar_t *ucs2;
	FLAC__StreamMetadata streaminfo;
	DWORD    length, bps, ratio, rg;
	LONGLONG filesize;

	SetWindowLong(hwnd, GWL_USERDATA, (LONG)data);
	/* file name */
	strncpy(data->filename, file, sizeof(data->filename));
	SetDlgItemText(hwnd, IDC_NAME, file);
	/* stream data and vorbis comment */
	filesize = FileSize(file);
	if (!filesize) return FALSE;
	if (!FLAC__metadata_get_streaminfo(file, &streaminfo))
		return FALSE;
	ReadTags(file, &data->tags, false);

	length = (DWORD)(streaminfo.data.stream_info.total_samples / streaminfo.data.stream_info.sample_rate);
	bps = (DWORD)(filesize / (125*streaminfo.data.stream_info.total_samples/streaminfo.data.stream_info.sample_rate));
	ratio = bps*1000000 / (streaminfo.data.stream_info.sample_rate*streaminfo.data.stream_info.channels*streaminfo.data.stream_info.bits_per_sample);
	rg  = FLAC_plugin__tags_get_tag_utf8(data->tags, "REPLAYGAIN_TRACK_GAIN") ? 1 : 0;
	rg |= FLAC_plugin__tags_get_tag_utf8(data->tags, "REPLAYGAIN_ALBUM_GAIN") ? 2 : 0;

	sprintf(buffer, "Sample rate: %d Hz\nChannels: %d\nBits per sample: %d\nMin block size: %d\nMax block size: %d\n"
	                "File size: %I64d bytes\nTotal samples: %I64d\nLength: %d:%02d\nAvg. bitrate: %d\nCompression ratio: %d.%d%%\n"
	                "ReplayGain: %s\n",
	    streaminfo.data.stream_info.sample_rate, streaminfo.data.stream_info.channels, streaminfo.data.stream_info.bits_per_sample,
	    streaminfo.data.stream_info.min_blocksize, streaminfo.data.stream_info.max_blocksize, filesize, streaminfo.data.stream_info.total_samples,
	    length/60, length%60, bps, ratio/10, ratio%10,
	    rg==3 ? "track gain\nReplayGain: album gain" : rg==2 ? "album gain" : rg==1 ? "track gain" : "not present");

	SetDlgItemText(hwnd, IDC_INFO, buffer);
	/* tag */
	if (isNT)
	{
		SetTextW(IDC_TITLE,   "TITLE");
		SetTextW(IDC_ARTIST,  "ARTIST");
		SetTextW(IDC_ALBUM,   "ALBUM");
		SetTextW(IDC_COMMENT, "COMMENT");
		SetTextW(IDC_YEAR,    "DATE");
		SetTextW(IDC_TRACK,   "TRACKNUMBER");
		SetTextW(IDC_GENRE,   "GENRE");
	}
	else
	{
		SetText(IDC_TITLE,   "TITLE");
		SetText(IDC_ARTIST,  "ARTIST");
		SetText(IDC_ALBUM,   "ALBUM");
		SetText(IDC_COMMENT, "COMMENT");
		SetText(IDC_YEAR,    "DATE");
		SetText(IDC_TRACK,   "TRACKNUMBER");
		SetText(IDC_GENRE,   "GENRE");
	}

	return TRUE;
}

static void __inline SetTag(HWND hwnd, const char *filename, FLAC__StreamMetadata *tags)
{
	strcpy(buffer, infoTitle);

	if (FLAC_plugin__tags_set(filename, tags))
		strcat(buffer, " [Updated]");
	else strcat(buffer, " [Failed]");

	SetWindowText(hwnd, buffer);
}

static void UpdateTag(HWND hwnd)
{
	LOCALDATA *data = (LOCALDATA*)GetWindowLong(hwnd, GWL_USERDATA);
	wchar_t *ucs2;

	/* get fields */
	if (isNT)
	{
		GetTextW(IDC_TITLE,   "TITLE");
		GetTextW(IDC_ARTIST,  "ARTIST");
		GetTextW(IDC_ALBUM,   "ALBUM");
		GetTextW(IDC_COMMENT, "COMMENT");
		GetTextW(IDC_YEAR,    "DATE");
		GetTextW(IDC_TRACK,   "TRACKNUMBER");
		GetTextW(IDC_GENRE,   "GENRE");

		ucs2 = FLAC_plugin__tags_get_tag_ucs2(data->tags, "GENRE");
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, ucs2, -1, buffer, sizeof(buffer), NULL, NULL);
		free(ucs2);
	}
	else
	{
		GetText(IDC_TITLE,   "TITLE");
		GetText(IDC_ARTIST,  "ARTIST");
		GetText(IDC_ALBUM,   "ALBUM");
		GetText(IDC_COMMENT, "COMMENT");
		GetText(IDC_YEAR,    "DATE");
		GetText(IDC_TRACK,   "TRACKNUMBER");
		GetText(IDC_GENRE,   "GENRE");
	}

	/* update genres list (buffer should contain genre) */
	if (buffer[0]) AddGenre(hwnd, buffer);

	/* write tag */
	SetTag(hwnd, data->filename, data->tags);
}

static void RemoveTag(HWND hwnd)
{
	LOCALDATA *data = (LOCALDATA*)GetWindowLong(hwnd, GWL_USERDATA);
	FLAC_plugin__tags_delete_all(data->tags);

	SetDlgItemText(hwnd, IDC_TITLE,   "");
	SetDlgItemText(hwnd, IDC_ARTIST,  "");
	SetDlgItemText(hwnd, IDC_ALBUM,   "");
	SetDlgItemText(hwnd, IDC_COMMENT, "");
	SetDlgItemText(hwnd, IDC_YEAR,    "");
	SetDlgItemText(hwnd, IDC_TRACK,   "");
	SetDlgItemText(hwnd, IDC_GENRE,   "");

	SetTag(hwnd, data->filename, data->tags);
}


static INT_PTR CALLBACK InfoProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	/* init */
	case WM_INITDIALOG:
		SetWindowText(hwnd, infoTitle);
		InitGenres(hwnd);
		/* init fields */
		if (!InitInfoboxInfo(hwnd, (const char*)lParam))
			PostMessage(hwnd, WM_CLOSE, 0, 0);
		return TRUE;
	/* destroy */
	case WM_DESTROY:
		{
			LOCALDATA *data = (LOCALDATA*)GetWindowLong(hwnd, GWL_USERDATA);
			FLAC_plugin__tags_destroy(&data->tags);
			LocalFree(data);
			DeinitGenres(hwnd, FALSE);
		}
		break;
	/* commands */
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		/* ok/cancel */
		case IDOK:
		case IDCANCEL:
			EndDialog(hwnd, LOWORD(wParam));
			return TRUE;
		/* save */
		case IDC_UPDATE:
			UpdateTag(hwnd);
			break;
		/* remove */
		case IDC_REMOVE:
			RemoveTag(hwnd);
			break;
		}
		break;
	}

	return 0;
}

/*
 *  Helpers
 */

ULONGLONG FileSize(const char *fileName)
{
	LARGE_INTEGER res;
	HANDLE hFile = CreateFile(fileName, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE) return 0;
	res.LowPart = GetFileSize(hFile, &res.HighPart);
	CloseHandle(hFile);
	return res.QuadPart;
}

static __inline char *GetFileName(const char *fullname)
{
	const char *c = fullname + strlen(fullname) - 1;

	while (c > fullname)
	{
		if (*c=='\\' || *c=='/')
		{
			c++;
			break;
		}
		c--;
	}

	return (char*)c;
}

void ReadTags(const char *fileName, FLAC__StreamMetadata **tags, BOOL forDisplay)
{
	if(FLAC_plugin__tags_get(fileName, tags)) {

		/* add file name */
		if (forDisplay)
		{
			char *c;
			wchar_t *ucs2;
			ucs2 = AnsiToWide(fileName);
			FLAC_plugin__tags_set_tag_ucs2(*tags, "filepath", ucs2, /*replace_all=*/true);
			free(ucs2);

			strcpy(buffer, GetFileName(fileName));
			if (c = strrchr(buffer, '.')) *c = 0;
			ucs2 = AnsiToWide(buffer);
			FLAC_plugin__tags_set_tag_ucs2(*tags, "filename", ucs2, /*replace_all=*/true);
			free(ucs2);
		}
	}
}

/*
 *  Front-end
 */

void InitInfobox()
{
	isNT = !(GetVersion() & 0x80000000);
}

void DeinitInfobox()
{
	DeinitGenres(NULL, true);
}

void DoInfoBox(HINSTANCE inst, HWND hwnd, const char *filename)
{
	DialogBoxParam(inst, MAKEINTRESOURCE(IDD_INFOBOX), hwnd, InfoProc, (LONG)filename);
}
