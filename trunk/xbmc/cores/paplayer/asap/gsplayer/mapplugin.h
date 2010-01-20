/*
 *  GSPlayer - The audio player for WindowsCE
 *  Copyright (C) 2003-2005  Y.Nagamidori
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

// Simply Decoder Plug-in
#ifndef __MAPPLUGIN_H__
#define __MAPPLUGIN_H__

typedef struct _MAP_PLUGIN_EQ {
	BOOL bEnable;	
	int nPreamp;	// 0 - 64 (center: 31)
	int nEq[10];	// 0 - 64 (center: 31)
} MAP_PLUGIN_EQ;

typedef struct _MAP_PLUGIN_FILE_INFO {
	int		nChannels;
	int		nSampleRate;
	int		nBitsPerSample;
	int		nAvgBitrate;	// kbps
	int		nDuration;		// ms
} MAP_PLUGIN_FILE_INFO;

typedef struct _MAP_PLUGIN_STREMING_INFO
{
	int		nChannels;
	int		nSampleRate;
	int		nBitsPerSample;
	int		nAvgBitrate;	// kbps
} MAP_PLUGIN_STREMING_INFO;

#define MAX_PLUGIN_TAG_STR		255
typedef struct _MAP_PLUGIN_FILETAG
{
	TCHAR szTrack[MAX_PLUGIN_TAG_STR];
	TCHAR szArtist[MAX_PLUGIN_TAG_STR];
	TCHAR szAlbum[MAX_PLUGIN_TAG_STR];
	TCHAR szComment[MAX_PLUGIN_TAG_STR];
	TCHAR szGenre[MAX_PLUGIN_TAG_STR];
	int nYear;
	int nTrackNum;
} MAP_PLUGIN_FILETAG;

#define PLUGIN_RET_ERROR	-1
#define PLUGIN_RET_SUCCESS	0
#define PLUGIN_RET_EOF		1
#define PLUGIN_DEC_VER		0x00000101

#define PLUGIN_FUNC_DECFILE				0x00000001
#define PLUGIN_FUNC_DECSTREAMING		0x00000002
#define PLUGIN_FUNC_EQ					0x00000004
#define PLUGIN_FUNC_SEEKFILE			0x00000008
#define PLUGIN_FUNC_FILETAG				0x00000010

typedef struct _MAP_DEC_PLUGIN {
	DWORD	dwVersion;	// PLUGIN_DEC_VER
	DWORD	dwChar;		// sizeof(TCHAR)
	DWORD	dwUser; // for Plug-in author
	void (WINAPI *Init)();
	void (WINAPI *Quit)();
	DWORD (WINAPI *GetFunc)();
	BOOL (WINAPI *GetPluginName)(LPTSTR pszName); // MAX_PATH
	BOOL (WINAPI *SetEqualizer)(MAP_PLUGIN_EQ* pEQ);
	void (WINAPI *ShowConfigDlg)(HWND hwndParent);

	// for file
	BOOL (WINAPI *GetFileExtCount)();		
	BOOL (WINAPI *GetFileExt)(int nIndex, LPTSTR pszExt, LPTSTR pszExtDesc); // MAX_PATH
	BOOL (WINAPI *IsValidFile)(LPCTSTR pszFile);
	BOOL (WINAPI *OpenFile)(LPCTSTR pszFile, MAP_PLUGIN_FILE_INFO* pInfo);
	long (WINAPI *SeekFile)(long lTime); // ms
	BOOL (WINAPI *StartDecodeFile)();
	int  (WINAPI *DecodeFile)(WAVEHDR* pHdr);
	void (WINAPI *StopDecodeFile)();
	void (WINAPI *CloseFile)();
	BOOL (WINAPI *GetTag)(MAP_PLUGIN_FILETAG* pTag);
	BOOL (WINAPI *GetFileTag)(LPCTSTR pszFile, MAP_PLUGIN_FILETAG* pTag);

	// for streaming
	BOOL (WINAPI *OpenStreaming)(LPBYTE pbBuf, DWORD cbBuf, MAP_PLUGIN_STREMING_INFO* pInfo);
	int (WINAPI *DecodeStreaming)(LPBYTE pbInBuf, DWORD cbInBuf, DWORD* pcbInUsed, WAVEHDR* pHdr);
	void (WINAPI *CloseStreaming)();

} MAP_DEC_PLUGIN;


#ifdef __cplusplus
extern "C" {
#endif 

__declspec(dllexport) MAP_DEC_PLUGIN* WINAPI mapGetDecoder();

#ifdef __cplusplus
};
#endif 

#endif // __MAPPLUGIN_H__
