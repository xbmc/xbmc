/*
** FAAD - Freeware Advanced Audio Decoder
** Copyright (C) 2002 M. Bakker
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** $Id: QCDFAAD.c,v 1.2 2003/04/28 19:04:35 menno Exp $
** based on menno's in_faad.dll plugin for Winamp
**
** The tag function has been removed because QCD supports ID3v1 & ID3v2 very well
** About how to tagging: Please read the "ReadMe.txt" first
**/

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <mmreg.h>
#include <commctrl.h>
#include <shellapi.h>
#include <stdlib.h>
#include <stdio.h>
#include "QCDInputDLL.h"

#include "resource.h"

#include <faad.h>
#include <aacinfo.h>
#include <filestream.h>
//#include <id3v2tag.h>

static char app_name[] = "QCDFAAD";

faadAACInfo file_info;

faacDecHandle hDecoder;
faacDecFrameInfo frameInfo;

HINSTANCE		hInstance;
HWND			hwndPlayer, hwndConfig, hwndAbout;
QCDModInitIn	sQCDCallbacks, *QCDCallbacks;
BOOL			oldAPIs = 0;
static char	lastfn[MAX_PATH]; // currently playing file (used for getting info on the current file)
int file_length; // file length, in bytes
int paused; // are we paused?
int seek_needed; // if != -1, it is the point that the decode thread should seek to, in ms.

char *sample_buffer; // sample buffer
unsigned char *buffer; // input buffer
unsigned char *memmap_buffer; // input buffer for whole file
long memmap_index;

long buffercount, fileread, bytecount;

// seek table for ADTS header files
unsigned long *seek_table = NULL;
int seek_table_length=0;

int killPlayThread = 0; // the kill switch for the decode thread
HANDLE play_thread_handle = INVALID_HANDLE_VALUE; // the handle to the decode thread
FILE_STREAM *infile;

/* Function definitions */
int id3v2_tag(unsigned char *buffer);
DWORD WINAPI PlayThread(void *b); // the decode thread procedure

// general funcz
static void show_error(const char *message,...)
{
	char foo[512];
	va_list args;
	va_start(args, message);
	vsprintf(foo, message, args);
	va_end(args);
	MessageBox(hwndPlayer, foo, "FAAD Plug-in Error", MB_ICONSTOP);
}


// 1= use vbr display, 0 = use average bitrate. This value only controls what shows up in the
// configuration form. Also- Streaming uses an on-the-fly bitrate display regardless of this value.
long m_variable_bitrate_display=0;
long m_priority = 5;
long m_memmap_file = 0;
static char INI_FILE[MAX_PATH];

char *priority_text[] = {	"",
							"Decode Thread Priority: Lowest",
							"Decode Thread Priority: Lower",
							"Decode Thread Priority: Normal",
							"Decode Thread Priority: Higher",
							"Decode Thread Priority: Highest (default)"
						};

long priority_table[] = {0, THREAD_PRIORITY_LOWEST, THREAD_PRIORITY_BELOW_NORMAL, THREAD_PRIORITY_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_HIGHEST};

long current_file_mode = 0;

int PlayThread_memmap();
int PlayThread_file();

static void _r_s(char *name,char *data, int mlen)
{
	char buf[10];
	strcpy(buf,data);
	GetPrivateProfileString(app_name,name,buf,data,mlen,INI_FILE);
}

#define RS(x) (_r_s(#x,x,sizeof(x)))
#define WS(x) (WritePrivateProfileString(app_name,#x,x,INI_FILE))

void config_read()
{
    char variable_bitrate_display[10];
    char priority[10];
    char memmap_file[10];
    char local_buffer_size[10];
    char stream_buffer_size[10];

    strcpy(variable_bitrate_display, "1");
    strcpy(priority, "5");
    strcpy(memmap_file, "0");
    strcpy(local_buffer_size, "128");
    strcpy(stream_buffer_size, "64");

    RS(variable_bitrate_display);
    RS(priority);
    RS(memmap_file);
    RS(local_buffer_size);
    RS(stream_buffer_size);

    m_priority = atoi(priority);
    m_variable_bitrate_display = atoi(variable_bitrate_display);
    m_memmap_file = atoi(memmap_file);
    m_local_buffer_size = atoi(local_buffer_size);
    m_stream_buffer_size = atoi(stream_buffer_size);
}

void config_write()
{
    char variable_bitrate_display[10];
    char priority[10];
    char memmap_file[10];
    char local_buffer_size[10];
    char stream_buffer_size[10];

    itoa(m_priority, priority, 10);
    itoa(m_variable_bitrate_display, variable_bitrate_display, 10);
    itoa(m_memmap_file, memmap_file, 10);
    itoa(m_local_buffer_size, local_buffer_size, 10);
    itoa(m_stream_buffer_size, stream_buffer_size, 10);

    WS(variable_bitrate_display);
    WS(priority);
    WS(memmap_file);
    WS(local_buffer_size);
    WS(stream_buffer_size);
}

//-----------------------------------------------------------------------------

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD fdwReason, LPVOID pRes)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		hInstance = hInst;
	return TRUE;
}

//-----------------------------------------------------------------------------
//old entrypoint api
PLUGIN_API BOOL QInputModule(QCDModInitIn *ModInit, QCDModInfo *ModInfo)
{
	ModInit->version					= PLUGIN_API_VERSION;
	ModInit->toModule.ShutDown			= ShutDown;
	ModInit->toModule.GetTrackExtents	= GetTrackExtents;
	ModInit->toModule.GetMediaSupported = GetMediaSupported;
	ModInit->toModule.GetCurrentPosition= GetCurrentPosition;
	ModInit->toModule.Play				= Play;
	ModInit->toModule.Pause				= Pause;
	ModInit->toModule.Stop				= Stop;
	ModInit->toModule.SetVolume			= SetVolume;
	ModInit->toModule.About				= About;
	ModInit->toModule.Configure			= Configure;
	QCDCallbacks = ModInit;

	ModInfo->moduleString = "FAAD Plugin v1.0b";
	/* read config */
	QCDCallbacks->Service(opGetPluginSettingsFile, INI_FILE, MAX_PATH, 0);

	config_read();
	ModInfo->moduleExtensions = "AAC";

	hwndPlayer = (HWND)ModInit->Service(opGetParentWnd, 0, 0, 0);
	lastfn[0] = 0;
	play_thread_handle = INVALID_HANDLE_VALUE;

	oldAPIs = 1;

	return TRUE;
}

//-----------------------------------------------------------------------------

PLUGIN_API QCDModInitIn* INPUTDLL_ENTRY_POINT()
{
	sQCDCallbacks.version						= PLUGIN_API_VERSION;
	sQCDCallbacks.toModule.Initialize			= Initialize;
	sQCDCallbacks.toModule.ShutDown				= ShutDown;
	sQCDCallbacks.toModule.GetTrackExtents		= GetTrackExtents;
	sQCDCallbacks.toModule.GetMediaSupported	= GetMediaSupported;
	sQCDCallbacks.toModule.GetCurrentPosition	= GetCurrentPosition;
	sQCDCallbacks.toModule.Play					= Play;
	sQCDCallbacks.toModule.Pause				= Pause;
	sQCDCallbacks.toModule.Stop					= Stop;
	sQCDCallbacks.toModule.SetVolume			= SetVolume;
	sQCDCallbacks.toModule.About				= About;
	sQCDCallbacks.toModule.Configure			= Configure;

	QCDCallbacks = &sQCDCallbacks;
	return &sQCDCallbacks;
}

//----------------------------------------------------------------------------

BOOL CALLBACK config_dialog_proc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char tmp[10];

    switch (message)
	{
    case WM_INITDIALOG:
        /* Set priority slider range and previous position */
		SendMessage(GetDlgItem(hwndDlg, THREAD_PRIORITY_SLIDER), TBM_SETRANGE, TRUE, MAKELONG(1, 5)); 
		SendMessage(GetDlgItem(hwndDlg, THREAD_PRIORITY_SLIDER), TBM_SETPOS, TRUE, m_priority);
		SetDlgItemText(hwndDlg, IDC_STATIC2, priority_text[m_priority]);
		
		/* Put a limit to the amount of characters allowed in the buffer boxes */
		SendMessage(GetDlgItem(hwndDlg, LOCAL_BUFFER_TXT), EM_LIMITTEXT, 4, 0);
		SendMessage(GetDlgItem(hwndDlg, STREAM_BUFFER_TXT), EM_LIMITTEXT, 4, 0);

	    if(m_variable_bitrate_display)
			SendMessage(GetDlgItem(hwndDlg, VARBITRATE_CHK), BM_SETCHECK, BST_CHECKED, 0);
	    if(m_memmap_file)
			SendMessage(GetDlgItem(hwndDlg, IDC_MEMMAP), BM_SETCHECK, BST_CHECKED, 0);

		itoa(m_local_buffer_size, tmp, 10);
		SetDlgItemText(hwndDlg, LOCAL_BUFFER_TXT, tmp);

		itoa(m_stream_buffer_size, tmp, 10);
		SetDlgItemText(hwndDlg, STREAM_BUFFER_TXT, tmp);

		return TRUE;

	case WM_HSCROLL:

		/* Thread priority slider moved */
		if(GetDlgItem(hwndDlg, THREAD_PRIORITY_SLIDER) == (HWND) lParam)
		{
			int tmp;
			tmp = SendMessage(GetDlgItem(hwndDlg, THREAD_PRIORITY_SLIDER), TBM_GETPOS, 0, 0);

			if(tmp > 0)
			{
				m_priority = tmp;

                SetDlgItemText(hwndDlg, IDC_STATIC2, priority_text[m_priority]);

				if(play_thread_handle)
					SetThreadPriority(play_thread_handle, priority_table[m_priority]);
			}
		}

		return TRUE;

    case WM_COMMAND:

		if(HIWORD(wParam) == BN_CLICKED)
		{
			if(GetDlgItem(hwndDlg, VARBITRATE_CHK) == (HWND) lParam)
			{
				/* Variable Bitrate checkbox hit */
				m_variable_bitrate_display = SendMessage(GetDlgItem(hwndDlg, VARBITRATE_CHK), BM_GETCHECK, 0, 0); 
			}
			if(GetDlgItem(hwndDlg, IDC_MEMMAP) == (HWND) lParam)
			{
				/* Variable Bitrate checkbox hit */
				m_memmap_file = SendMessage(GetDlgItem(hwndDlg, IDC_MEMMAP), BM_GETCHECK, 0, 0); 
			}
		}

        switch (LOWORD(wParam))
		{
			case OK_BTN:
				/* User hit OK, save buffer settings (all others are set on command) */
				GetDlgItemText(hwndDlg, LOCAL_BUFFER_TXT, tmp, 5);
				m_local_buffer_size = atol(tmp);

				GetDlgItemText(hwndDlg, STREAM_BUFFER_TXT, tmp, 5);
				m_stream_buffer_size = atol(tmp);

                config_write();

				EndDialog(hwndDlg, wParam);
				return TRUE;
			case RESET_BTN:
				SendMessage(GetDlgItem(hwndDlg, VARBITRATE_CHK), BM_SETCHECK, BST_CHECKED, 0);
				m_variable_bitrate_display = 1;
				SendMessage(GetDlgItem(hwndDlg, IDC_MEMMAP), BM_SETCHECK, BST_UNCHECKED, 0);
				m_memmap_file = 0;
				SendMessage(GetDlgItem(hwndDlg, THREAD_PRIORITY_SLIDER), TBM_SETPOS, TRUE, 5);
				m_priority = 5;
				SetDlgItemText(hwndDlg, IDC_STATIC2, priority_text[5]);
				SetDlgItemText(hwndDlg, LOCAL_BUFFER_TXT, "128");
				m_local_buffer_size = 128;
				SetDlgItemText(hwndDlg, STREAM_BUFFER_TXT, "64");
				m_stream_buffer_size = 64;
				return TRUE;
			case IDCANCEL:
			case CANCEL_BTN:
				/* User hit Cancel or the X, just close without saving buffer settings */
				DestroyWindow(hwndDlg);
				return TRUE;
        }
    }
    return FALSE;
}


void Configure(int flags)
{
	if(!IsWindow(hwndConfig))
		hwndConfig = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_CONFIG), hwndPlayer, config_dialog_proc);
	ShowWindow(hwndConfig, SW_NORMAL);
}

//-----------------------------------------------------------------------------
// proc of "About Dialog"
INT_PTR CALLBACK about_dialog_proc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static RECT rcLOGO, rcMail1, rcMail2/*, rcMail3*/;
	POINT ptMouse;
	static char szPluginVer[] = "QCD FAAD Input Plug-in v1.0b\nCompiled on " __TIME__ ", " __DATE__;
	static char szFLACVer[] = "Using: FAAD2 v "FAAD2_VERSION" by";

	switch (uMsg)
	{
	case WM_INITDIALOG:
	case WM_MOVE:
		GetWindowRect(GetDlgItem(hwndDlg, IDC_LOGO), &rcLOGO);
		GetWindowRect(GetDlgItem(hwndDlg, IDC_MAIL1), &rcMail1);
		GetWindowRect(GetDlgItem(hwndDlg, IDC_MAIL2), &rcMail2);
//		GetWindowRect(GetDlgItem(hwndDlg, IDC_MAIL2), &rcMail3);

		SetDlgItemText(hwndDlg, IDC_PLUGINVER, szPluginVer);
		SetDlgItemText(hwndDlg, IDC_FAADVER, szFLACVer);
		
		return TRUE;
	case WM_MOUSEMOVE:
		ptMouse.x = LOWORD(lParam);
		ptMouse.y = HIWORD(lParam);
		ClientToScreen(hwndDlg, &ptMouse);
		if( (ptMouse.x >= rcLOGO.left && ptMouse.x <= rcLOGO.right && 
			ptMouse.y >= rcLOGO.top && ptMouse.y<= rcLOGO.bottom) 
			||
			(ptMouse.x >= rcMail1.left && ptMouse.x <= rcMail1.right && 
			ptMouse.y >= rcMail1.top && ptMouse.y<= rcMail1.bottom) 
			||
			(ptMouse.x >= rcMail2.left && ptMouse.x <= rcMail2.right && 
			ptMouse.y >= rcMail2.top && ptMouse.y<= rcMail2.bottom) 
/*			||
			(ptMouse.x >= rcMail3.left && ptMouse.x <= rcMail3.right && 
			ptMouse.y >= rcMail3.top && ptMouse.y<= rcMail3.bottom)*/ )
			SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(32649)));
		else
			SetCursor(LoadCursor(NULL, IDC_ARROW));

		return TRUE;
	case WM_LBUTTONDOWN:
		ptMouse.x = LOWORD(lParam);
		ptMouse.y = HIWORD(lParam);
		ClientToScreen(hwndDlg, &ptMouse);
		if(ptMouse.x >= rcLOGO.left && ptMouse.x <= rcLOGO.right && 
			ptMouse.y >= rcLOGO.top && ptMouse.y<= rcLOGO.bottom)
			ShellExecute(0, NULL, "http://www.audiocoding.com", NULL,NULL, SW_NORMAL);
		else if(ptMouse.x >= rcMail1.left && ptMouse.x <= rcMail1.right && 
			ptMouse.y >= rcMail1.top && ptMouse.y<= rcMail1.bottom)
			ShellExecute(0, NULL, "mailto:shaohao@elong.com", NULL,NULL, SW_NORMAL);
		else if(ptMouse.x >= rcMail2.left && ptMouse.x <= rcMail2.right && 
			ptMouse.y >= rcMail2.top && ptMouse.y<= rcMail2.bottom)
			ShellExecute(0, NULL, "mailto:menno@audiocoding.com", NULL,NULL, SW_NORMAL);
/*		else if(ptMouse.x >= rcMail3.left && ptMouse.x <= rcMail3.right && 
			ptMouse.y >= rcMail3.top && ptMouse.y<= rcMail3.bottom)
			ShellExecute(0, NULL, "I don't know", NULL,NULL, SW_NORMAL);
*/
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
		default:
			DestroyWindow(hwndDlg);
			return TRUE;
		}
	}
	return FALSE;
}

void About(int flags)
{
	if(!IsWindow(hwndAbout))
		hwndAbout = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_ABOUT), hwndPlayer, about_dialog_proc); 
	ShowWindow(hwndAbout, SW_SHOW);
}

//-----------------------------------------------------------------------------

BOOL Initialize(QCDModInfo *ModInfo, int flags)
{
	hwndPlayer = (HWND)QCDCallbacks->Service(opGetParentWnd, 0, 0, 0);

	lastfn[0] = 0;
	seek_needed = -1;
	paused = 0;
	play_thread_handle = INVALID_HANDLE_VALUE;

	/* read config */
	QCDCallbacks->Service(opGetPluginSettingsFile, INI_FILE, MAX_PATH, 0);
    config_read();

	ModInfo->moduleString = "FAAD Plugin v1.0b";
	ModInfo->moduleExtensions = "AAC";

    /* Initialize winsock, necessary for streaming */
    WinsockInit();

	// insert menu item into plugin menu
//	QCDCallbacks->Service(opSetPluginMenuItem, hInstance, IDD_CONFIG, (long)"FAAD Plug-in");

	return TRUE;
}

//----------------------------------------------------------------------------

void ShutDown(int flags)
{
	Stop(lastfn, STOPFLAG_FORCESTOP);

    if(buffer)
		LocalFree(buffer);

    /* Deallocate winsock */
    WinsockDeInit();

	// delete the inserted plugin menu
//	QCDCallbacks->Service(opSetPluginMenuItem, hInstance, 0, 0);
}

//-----------------------------------------------------------------------------

BOOL GetMediaSupported(LPCSTR medianame, MediaInfo *mediaInfo)
{
	FILE_STREAM *in;
	faadAACInfo tmp;
	char *ch = strrchr(medianame, '.');

	if (!medianame || !*medianame)
		return FALSE;

	if(!ch)
		return (lstrlen(medianame) > 2); // no extension defaults to me (if not drive letter)

   /* Finally fixed */
    if(StringComp(ch, ".aac", 4) == 0)
    {
		in = open_filestream((char *)medianame);
		
		if(in != NULL && mediaInfo)
		{
			if(in->http)
			{
				/* No seeking in http streams */
				mediaInfo->mediaType = DIGITAL_STREAM_MEDIA;
				mediaInfo->op_canSeek = FALSE;
			}
			else
			{
				mediaInfo->mediaType = DIGITAL_FILE_MEDIA;
			    get_AAC_format((char *)medianame, &tmp, NULL, NULL, 1);
				if(tmp.headertype == 2) /* ADTS header - seekable */
					mediaInfo->op_canSeek = TRUE;
				else
					mediaInfo->op_canSeek = FALSE; /* ADIF or Headerless - not seekable */
			}
			
			close_filestream(in);
			return TRUE;
		}
		else
		{
			close_filestream(in);
			return FALSE;
		}
	}
	else
		return FALSE;		
}

unsigned long samplerate, channels;

int play_memmap(char *fn)
{
    int tagsize = 0;

    infile = open_filestream(fn);
    
	if (infile == NULL)
        return 1;

    fileread = filelength_filestream(infile);

    memmap_buffer = (char*)LocalAlloc(LPTR, fileread);
    read_buffer_filestream(infile, memmap_buffer, fileread);

    /* skip id3v2 tag */
    memmap_index = id3v2_tag(memmap_buffer);

    hDecoder = faacDecOpen();

	/* Copy the configuration dialog setting and use it as the default */
	/* initialize the decoder, and get samplerate and channel info */

    if( (buffercount = faacDecInit(hDecoder, memmap_buffer + memmap_index,
        fileread - memmap_index - 1, &samplerate, &channels)) < 0 )
    {
		show_error("Error opening input file");
		return 1;
    }

    memmap_index += buffercount;

    PlayThread_memmap();

    return 0;
}

int play_file(char *fn)
{
    int k;
    int tagsize;

    ZeroMemory(buffer, 768*2);

     infile = open_filestream(fn);
    
	if (infile == NULL)
        return 1;

	fileread = filelength_filestream(infile);

    buffercount = bytecount = 0;
    read_buffer_filestream(infile, buffer, 768*2);

    tagsize = id3v2_tag(buffer);

	/* If we find a tag, run right over it */
    if(tagsize)
	{
        if(infile->http)
        {
            int i;
            /* Crude way of doing this, but I believe its fast enough to not make a big difference */
            close_filestream(infile);
            infile = open_filestream(fn);

            for(i=0; i < tagsize; i++)
                read_byte_filestream(infile);
        }
        else
            seek_filestream(infile, tagsize, FILE_BEGIN);

        bytecount = tagsize;
        buffercount = 0;
        read_buffer_filestream(infile, buffer, 768*2);
    }

    hDecoder = faacDecOpen();

	/* Copy the configuration dialog setting and use it as the default */
	/* initialize the decoder, and get samplerate and channel info */

    if((buffercount = faacDecInit(hDecoder, buffer, 768*2, &samplerate, &channels)) < 0)
    {
		show_error("Error opening input file");
		return 1;
    }

    if(buffercount > 0)
	{
		bytecount += buffercount;

		for (k = 0; k < (768*2 - buffercount); k++)
			buffer[k] = buffer[k + buffercount];

		read_buffer_filestream(infile, buffer + (768*2) - buffercount, buffercount);
		buffercount = 0;
	}

    PlayThread_file();

    return 0;
}


//-----------------------------------------------------------------------------

BOOL Play(LPCSTR medianame, int playfrom, int playto, int flags)
{
	if(stricmp(lastfn, medianame) != 0)
	{
		sQCDCallbacks.toPlayer.OutputStop(STOPFLAG_PLAYDONE);
		Stop(lastfn, STOPFLAG_PLAYDONE);
	}

	if(paused)
	{
		// Update the player controls to reflect the new unpaused state
		sQCDCallbacks.toPlayer.OutputPause(0);

		Pause(medianame, PAUSE_DISABLED);

		if (playfrom >= 0)
			seek_needed = playfrom;
	}
	else if(play_thread_handle != INVALID_HANDLE_VALUE)
	{
		seek_needed = playfrom;
		return TRUE;
	}
	else
	{
		int thread_id;

		// alloc the input buffer
		buffer = (unsigned char*)LocalAlloc(LPTR, 768*2);

		current_file_mode = m_memmap_file;
		
		if(current_file_mode)
		{
			if(play_memmap((char *)medianame))
				return FALSE;
		}
		else
		{
			if(play_file((char *)medianame))
				return FALSE;
		}
		
		if(seek_table)
		{
			free(seek_table);
			seek_table = NULL;
			seek_table_length = 0;
		}
		
		get_AAC_format((char *)medianame, &file_info, &seek_table, &seek_table_length, 0);
		
		seek_needed = playfrom > 0 ? playfrom : -1;
		killPlayThread = 0;
		strcpy(lastfn,medianame);

		/*
		To RageAmp: This is really needed, because aacinfo isn't very accurate on ADIF files yet.
		Can be fixed though :-)
		*/
		file_info.sampling_rate = samplerate;
		file_info.channels = frameInfo.channels;

		play_thread_handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) PlayThread, (void *) &killPlayThread, 0, &thread_id);
		if(!play_thread_handle)
			return FALSE;
		
		// Note: This line seriously slows down start up time
		if(m_priority != 3) // if the priority in config window is set to normal, there is nothing to reset!
			SetThreadPriority(play_thread_handle, priority_table[m_priority]);
		
    }

	return TRUE;
}

//-----------------------------------------------------------------------------

BOOL Pause(LPCSTR medianame, int flags)
{
	if(QCDCallbacks->toPlayer.OutputPause(flags))
	{
		// send back pause/unpause notification
		QCDCallbacks->toPlayer.PlayPaused(medianame, flags);
		paused = flags;
		return TRUE;
	}
	return FALSE;
}

//-----------------------------------------------------------------------------

BOOL Stop(LPCSTR medianame, int flags)
{
	if(medianame && *medianame && stricmp(lastfn, medianame) == 0)
	{
		sQCDCallbacks.toPlayer.OutputStop(flags);
		
		killPlayThread = 1;
		if(play_thread_handle != INVALID_HANDLE_VALUE)
		{
			if(WaitForSingleObject(play_thread_handle, INFINITE) == WAIT_TIMEOUT)
			{
//				MessageBox(hwndPlayer, "FAAD thread kill timeout", "debug", 0);
				TerminateThread(play_thread_handle,0);
			}
			CloseHandle(play_thread_handle);
			play_thread_handle = INVALID_HANDLE_VALUE;
		}

		if (oldAPIs)
			QCDCallbacks->toPlayer.PlayStopped(lastfn);
		
		lastfn[0] = 0;
	}

	return TRUE;
}

int aac_seek(int pos_ms, int *sktable)
{
    double offset_sec;

    offset_sec = pos_ms / 1000.0;
    if(!current_file_mode)
    {
        seek_filestream(infile, sktable[(int)(offset_sec+0.5)], FILE_BEGIN);

        bytecount = sktable[(int)(offset_sec+0.5)];
        buffercount = 0;
        read_buffer_filestream(infile, buffer, 768*2);
    }
	else
	{
        memmap_index = sktable[(int)(offset_sec+0.5)];
    }

    return 0;
}

//-----------------------------------------------------------------------------

void SetVolume(int levelleft, int levelright, int flags)
{
	QCDCallbacks->toPlayer.OutputSetVol(levelleft, levelright, flags);
}

//-----------------------------------------------------------------------------

BOOL GetCurrentPosition(LPCSTR medianame, long *track, long *offset)
{
	return QCDCallbacks->toPlayer.OutputGetCurrentPosition((UINT*)offset, 0);
}

//-----------------------------------------------------------------------------

BOOL GetTrackExtents(LPCSTR medianame, TrackExtents *ext, int flags)
{
    faadAACInfo tmp;

    if(get_AAC_format((char*)medianame, &tmp, NULL, NULL, 1))
		return FALSE;

	ext->track = 1;
	ext->start = 0;
	ext->end = tmp.length;
	ext->bytesize = tmp.bitrate * tmp.length;
	ext->unitpersec = 1000;

	return TRUE;
}

//--------------------------for play thread-------------------------------------

int last_frame;

DWORD WINAPI PlayThread(void *b)
{
    BOOL done = FALSE, updatePos = FALSE;
	int decode_pos_ms = 0; // current decoding position, in milliseconds
    int l;
	int decoded_frames=0;
	int br_calc_frames=0;
	int br_bytes_consumed=0;
    unsigned long bytesconsumed;

    last_frame = 0;

	if(!done)
	{
		// open outputdevice
		WAVEFORMATEX wf;
		wf.wFormatTag = WAVE_FORMAT_PCM;
		wf.cbSize = 0;
		wf.nChannels = file_info.channels;
		wf.wBitsPerSample = 16;
		wf.nSamplesPerSec = file_info.sampling_rate;
		wf.nBlockAlign = wf.nChannels * wf.wBitsPerSample / 8;
		wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
		if (!QCDCallbacks->toPlayer.OutputOpen(lastfn, &wf))
		{
			show_error("Error: Failed openning output plugin!");
			done = TRUE; // cannot open sound device
		}
	}

    while (! *((int *)b) )
    {
 		/********************** SEEK ************************/
       if (!done && seek_needed >= 0)
	   {
            int seconds;

            // Round off to a second
            seconds = seek_needed - (seek_needed%1000);
			QCDCallbacks->toPlayer.OutputFlush(decode_pos_ms);
            aac_seek(seconds, seek_table);
            decode_pos_ms = seconds;
			decoded_frames = 0;
			br_calc_frames = 0;
			br_bytes_consumed = 0;

			seek_needed = -1;
			updatePos = 1;
	   }

		/********************* QUIT *************************/
		if (done)
		{
			if (QCDCallbacks->toPlayer.OutputDrain(0) && !(seek_needed >= 0))
			{
				play_thread_handle = INVALID_HANDLE_VALUE;
				QCDCallbacks->toPlayer.OutputStop(STOPFLAG_PLAYDONE);
				QCDCallbacks->toPlayer.PlayDone(lastfn);
			}
			else if (seek_needed >= 0)
			{
				done = FALSE;
				continue;
			}
			break;
		}

        /******************* DECODE TO BUFFER ****************/
		else
        {
			if (current_file_mode)
				bytesconsumed = PlayThread_memmap();
			else
				bytesconsumed = PlayThread_file();

			if(last_frame)
				done = TRUE;
			else
			{

				decoded_frames++;
				br_calc_frames++;
				br_bytes_consumed += bytesconsumed;

				/* Update the variable bitrate about every second */
				if(m_variable_bitrate_display && br_calc_frames == 43)
				{
					AudioInfo vai;
					vai.struct_size = sizeof(AudioInfo);
					vai.frequency = file_info.sampling_rate;
					vai.bitrate = (int)((br_bytes_consumed * 8) / (decoded_frames / 43.07));
					vai.mode = (channels == 2) ? 0 : 3;
					vai.layer = 0;
					vai.level = file_info.version;
					QCDCallbacks->Service(opSetAudioInfo, &vai, sizeof(AudioInfo), 0);

					br_calc_frames = 0;
				}

                if (!killPlayThread && (frameInfo.samples > 0))
                {
                    //update the time display
					if (updatePos)
					{
						QCDCallbacks->toPlayer.PositionUpdate(decode_pos_ms);
						updatePos = 0;
					}
					
					{
						WriteDataStruct wd;

						l = frameInfo.samples * sizeof(short);

						decode_pos_ms += (1024*1000)/file_info.sampling_rate;
						
						wd.bytelen = l;
						wd.data = sample_buffer;
						wd.markerend = 0;
						wd.markerstart = decode_pos_ms;
						wd.bps = 16;
						wd.nch = frameInfo.channels;
						wd.numsamples =l/file_info.channels/(16/8);
						wd.srate = file_info.sampling_rate;
						
						if (!QCDCallbacks->toPlayer.OutputWrite(&wd))
							done = TRUE;
					}
				}
			}
		}
        Sleep(10);
    }

	// close up
	play_thread_handle = INVALID_HANDLE_VALUE;

	faacDecClose(hDecoder);
	hDecoder = INVALID_HANDLE_VALUE;
	close_filestream(infile);
	infile = NULL;

	if(seek_table)
	{
		free(seek_table);
		seek_table = NULL;
		seek_table_length = 0;
	}

	if(buffer)
	{
		LocalFree(buffer);
		buffer = NULL;
	}
	if(memmap_buffer)
	{
		LocalFree(memmap_buffer);
		memmap_buffer = NULL;
	}

    return 0;
}

// thread play funcs
int PlayThread_memmap()
{
    sample_buffer = (char*)faacDecDecode(hDecoder, &frameInfo,
        memmap_buffer + memmap_index, fileread - memmap_index - 1);
    if (frameInfo.error)
    {
//        show_error(faacDecGetErrorMessage(frameInfo.error));
        last_frame = 1;
    }

    memmap_index += frameInfo.bytesconsumed;
    if (memmap_index >= fileread)
        last_frame = 1;

    return frameInfo.bytesconsumed;
}

int PlayThread_file()
{
    int k;

    if (buffercount > 0)
	{
        for (k = 0; k < (768*2 - buffercount); k++)
            buffer[k] = buffer[k + buffercount];

        read_buffer_filestream(infile, buffer + (768*2) - buffercount, buffercount);
        buffercount = 0;
    }

    sample_buffer = (char*)faacDecDecode(hDecoder, &frameInfo, buffer, 768*2);
    if (frameInfo.error)
    {
//        show_error(faacDecGetErrorMessage(frameInfo.error));
        last_frame = 1;
    }

    buffercount += frameInfo.bytesconsumed;

    bytecount += frameInfo.bytesconsumed;
    if (bytecount >= fileread)
        last_frame = 1;

    return frameInfo.bytesconsumed;
}

// tag
int id3v2_tag(unsigned char *buffer)
{
    if (StringComp(buffer, "ID3", 3) == 0)
	{
        unsigned long tagsize;

        /* high bit is not used */
        tagsize = (buffer[6] << 21) | (buffer[7] << 14) |
            (buffer[8] <<  7) | (buffer[9] <<  0);

        tagsize += 10;

        return tagsize;
    }
	else
	{
        return 0;
    }
}