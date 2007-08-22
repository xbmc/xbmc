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
** $Id: in_faad.c,v 1.4 2002/11/01 11:19:36 menno Exp $
**/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include "resource.h"

#include <faad.h>
#include <stdlib.h>
#include <stdio.h>
#include <aacinfo.h>
#include <filestream.h>
#include <id3v2tag.h>

#include "in2.h"


static char app_name[] = "AudioCoding.com AAC Player";

// post this to the main window at end of file (after playback has stopped)
#define WM_WA_AAC_EOF WM_USER+2

faadAACInfo file_info;

faacDecHandle hDecoder;
faacDecFrameInfo frameInfo;

In_Module mod; // the output module (declared near the bottom of this file)
char *lastfn; // currently playing file (used for getting info on the current file)
int file_length; // file length, in bytes
int decode_pos_ms; // current decoding position, in milliseconds
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

int killPlayThread=0; // the kill switch for the decode thread
int PlayThreadAlive=0; // 1=play thread still running
HANDLE play_thread_handle=INVALID_HANDLE_VALUE; // the handle to the decode thread
FILE_STREAM *infile;

/* Function definitions */
int id3v2_tag(unsigned char *buffer);
DWORD WINAPI PlayThread(void *b); // the decode thread procedure

BOOL CALLBACK config_dialog_proc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);


// 1= use vbr display, 0 = use average bitrate. This value only controls what shows up in the
// configuration form. Also- Streaming uses an on-the-fly bitrate display regardless of this value.
long m_variable_bitrate_display=0;
long m_priority = 5;
long m_memmap_file = 0;
char m_format_string[50] = "%1 - %2";
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



static void config_init()
{
	char *p=INI_FILE;
	GetModuleFileName(NULL,INI_FILE,sizeof(INI_FILE));
	while (*p) p++;
	while (p >= INI_FILE && *p != '.') p--;
	strcpy(p+1,"ini");
}

void config_read()
{
    char variable_bitrate_display[10];
    char priority[10];
    char memmap_file[10];
    char local_buffer_size[10];
    char stream_buffer_size[10];

	config_init();

    strcpy(variable_bitrate_display, "1");
    strcpy(priority, "4");
    strcpy(memmap_file, "0");
    strcpy(local_buffer_size, "128");
    strcpy(stream_buffer_size, "64");

    RS(variable_bitrate_display);
    RS(priority);
    RS(m_format_string);
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
    WS(m_format_string);
    WS(memmap_file);
    WS(local_buffer_size);
    WS(stream_buffer_size);
}

void config(HWND hwndParent)
{
	DialogBox(mod.hDllInstance, MAKEINTRESOURCE(IDD_CONFIG), hwndParent, config_dialog_proc);
}

BOOL CALLBACK config_dialog_proc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char tmp[10];

    switch (message)
	{
    case WM_INITDIALOG:
        
		SetDlgItemText(hwndDlg, IDC_FORMAT, m_format_string);

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

				GetDlgItemText(hwndDlg, IDC_FORMAT, m_format_string, 50);

                config_write();

			case IDCANCEL:
			case CANCEL_BTN:
				/* User hit Cancel or the X, just close without saving buffer settings */
				EndDialog(hwndDlg, wParam);
            return TRUE;
        }
    }
    return FALSE;
}

void about(HWND hwndParent)
{
    MessageBox(hwndParent,"AudioCoding.com AAC Player\n"
        "Compiled on: " __DATE__ "\n"
        "http://www.audiocoding.com\n"
        "Copyright 2002 AudioCoding.com",
        "About AAC Player",MB_OK);
}

void init()
{
	char plugin_path[1024];
	int i;

    config_read();

	hInstance_for_id3editor = mod.hDllInstance;

    buffer = (unsigned char*)LocalAlloc(LPTR, 768*2);
    lastfn = (char*)LocalAlloc(LPTR, 1024*sizeof(char));

    /* Initialize winsock, necessary for streaming */
    WinsockInit();
}

void quit()
{
    if (lastfn) LocalFree(lastfn);
    if (buffer) LocalFree(buffer);

    /* Deallocate winsock */
    WinsockDeInit();

	if(seek_table)
	{
		free(seek_table);
		seek_table = NULL;
		seek_table_length = 0;
	}
}

int isourfile(char *fn)
{
    /* Finally fixed */
    if(StringComp(fn + strlen(fn) - 3, "aac", 3) == 0)
    {
        return 1;
    }

    return 0;
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

    if ((buffercount = faacDecInit(hDecoder, memmap_buffer + memmap_index,
        fileread - memmap_index - 1, &samplerate, &channels)) < 0)
    {
		MessageBox(mod.hMainWindow, "Error opening input file\n", "FAAD Error", MB_OK);
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
        {
            seek_filestream(infile, tagsize, FILE_BEGIN);
        }

        bytecount = tagsize;
        buffercount = 0;
        read_buffer_filestream(infile, buffer, 768*2);
    }

    hDecoder = faacDecOpen();

	/* Copy the configuration dialog setting and use it as the default */
	/* initialize the decoder, and get samplerate and channel info */

    if((buffercount = faacDecInit(hDecoder, buffer, 768*2, &samplerate, &channels)) < 0)
    {
		MessageBox(mod.hMainWindow, "Error opening input file\n", "FAAD Error", MB_OK);
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

int play(char *fn)
{
    int maxlatency;
    int thread_id;

    current_file_mode = m_memmap_file;

    if (current_file_mode) {
        if (play_memmap(fn))
            return -1;
    } else {
        if (play_file(fn))
            return -1;
    }

	if(seek_table)
	{
		free(seek_table);
		seek_table = NULL;
		seek_table_length = 0;
	}

	get_AAC_format(fn, &file_info, &seek_table, &seek_table_length, 0);

	if(infile->http)
	{
		/* No seeking in http streams */
		mod.is_seekable = 0;
	}
	else
	{
		if (file_info.headertype == 2) /* ADTS header - seekable */
			mod.is_seekable = 1;
		else
			mod.is_seekable = 0; /* ADIF or Headerless - not seekable */
	}

    strcpy(lastfn,fn);
    paused=0;
    decode_pos_ms=0;
    seek_needed=-1;

    /*
      To RageAmp: This is really needed, because aacinfo isn't very accurate on ADIF files yet.
                  Can be fixed though :-)
    */
    file_info.sampling_rate = samplerate;
    file_info.channels = frameInfo.channels;

    maxlatency = mod.outMod->Open(file_info.sampling_rate, file_info.channels, 16, -1,-1);

    if (maxlatency < 0) // error opening device
    {
        return -1;
    }

    // initialize vis stuff
    mod.SAVSAInit(maxlatency, file_info.sampling_rate);
    mod.VSASetInfo(file_info.sampling_rate, file_info.channels);

    mod.SetInfo(file_info.bitrate/1000, file_info.sampling_rate/1000, file_info.channels,1);

    mod.outMod->SetVolume(-666); // set the output plug-ins default volume

    killPlayThread = 0;

    if((play_thread_handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) PlayThread, (void *) &killPlayThread, 0, &thread_id)) == NULL)
    {
        MessageBox(mod.hMainWindow, "Fatal error: Cannot create playback thread\n", "FAAD Error", MB_OK);
        return -1;
    }

    // Note: This line seriously slows down start up time
	if(m_priority != 3) // if the priority in config window is set to normal, there is nothing to reset!
	    SetThreadPriority(play_thread_handle, priority_table[m_priority]);

    return 0;
}

void pause() { paused=1; mod.outMod->Pause(1); }
void unpause() { paused=0; mod.outMod->Pause(0); }
int ispaused() { return paused; }

void stop()
{
    killPlayThread=1;

    if (play_thread_handle != INVALID_HANDLE_VALUE)
    {
        if (WaitForSingleObject(play_thread_handle, INFINITE) == WAIT_TIMEOUT)
            TerminateThread(play_thread_handle,0);
        CloseHandle(play_thread_handle);
        play_thread_handle = INVALID_HANDLE_VALUE;
    }

    faacDecClose(hDecoder);
    close_filestream(infile);
    mod.outMod->Close();
    mod.SAVSADeInit();

    if (memmap_buffer) LocalFree(memmap_buffer);
	if(seek_table)
	{
		free(seek_table);
		seek_table = NULL;
		seek_table_length = 0;
	}
}

int getlength()
{
    return file_info.length;
}

int getoutputtime()
{
    return decode_pos_ms+(mod.outMod->GetOutputTime()-mod.outMod->GetWrittenTime());
}

void setoutputtime(int time_in_ms)
{
    if (file_info.headertype == 2)
        seek_needed=time_in_ms;
    else
        seek_needed = -1;
}

int aac_seek(int pos_ms, int *sktable)
{
    double offset_sec;

    offset_sec = pos_ms / 1000.0;
    if (!current_file_mode)
    {
        seek_filestream(infile, sktable[(int)(offset_sec+0.5)], FILE_BEGIN);

        bytecount = sktable[(int)(offset_sec+0.5)];
        buffercount = 0;
        read_buffer_filestream(infile, buffer, 768*2);
    } else {
        memmap_index = sktable[(int)(offset_sec+0.5)];
    }

    return 0;
}

VOID EnableCheckbox(HWND checkBox, BOOL bEnable, BOOL bChecked)
{
    if (bEnable) {
        EnableWindow(checkBox, TRUE);
        SendMessage(checkBox, BM_SETCHECK, bChecked?BST_CHECKED:BST_UNCHECKED, 0);
    } else {
        EnableWindow(checkBox, FALSE);
        SendMessage(checkBox, BM_SETCHECK, bChecked?BST_CHECKED:BST_UNCHECKED, 0);
    }
}

void setvolume(int volume) { mod.outMod->SetVolume(volume); }
void setpan(int pan) { mod.outMod->SetPan(pan); }

char info_fn[255];
BOOL bFileChanged;

BOOL CALLBACK info_dialog_proc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    faadAACInfo format;
    char *tmp_string;
    char info[1024];
    LV_COLUMN lvc;
    BOOL bResult;

    switch (message) {
    case WM_INITDIALOG:
        /* Set up the list control for the ID3 tag */
        
        /* Initialize the LV_COLUMN structure. */
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvc.fmt = LVCFMT_LEFT;

        /* Add the columns. */
        lvc.iSubItem = 0;
        lvc.cx = 100;
        lvc.pszText = "Frame";
        ListView_InsertColumn(GetDlgItem(hwndDlg, IDC_ID3LIST), 0, &lvc);
        lvc.iSubItem = 1;
        lvc.cx = 250;
        lvc.pszText = "Data";
        ListView_InsertColumn(GetDlgItem(hwndDlg, IDC_ID3LIST), 1, &lvc);

        /* get AAC info */
        get_AAC_format(info_fn, &format, NULL, NULL, 1);

        switch(format.headertype)
        {
        case 0: /* Headerless */
            tmp_string = "RAW"; break;
        case 1: /* ADIF */
            tmp_string = "ADIF"; break;
        case 2: /* ADTS */
            tmp_string = "ADTS"; break;
        }
        SetDlgItemText(hwndDlg, IDC_HEADER, tmp_string);

        if (format.object_type == 0 /* Main */)
            tmp_string = "Main";
        else if (format.object_type == 1 /* Low Complexity */)
            tmp_string = "Low Complexity";
        else if (format.object_type == 2 /* SSR */)
            tmp_string = "SSR (unsupported)";
        else if (format.object_type == 3 /* LTP */)
            tmp_string = "Main LTP";
        SetDlgItemText(hwndDlg, IDC_PROFILE, tmp_string);

        if (format.version == 2) tmp_string = "MPEG2";
        else tmp_string = "MPEG4";
        SetDlgItemText(hwndDlg, IDC_VERSION, tmp_string);

        wsprintf(info, "%d bps", format.bitrate);
        SetDlgItemText(hwndDlg, IDC_BITRATE, info);

        wsprintf(info, "%d Hz", format.sampling_rate);
        SetDlgItemText(hwndDlg, IDC_SAMPLERATE, info);

        wsprintf(info, "%d ch", format.channels);
        SetDlgItemText(hwndDlg, IDC_CHANNELS, info);

        FillID3List(hwndDlg, GetDlgItem(hwndDlg, IDC_ID3LIST), info_fn);
        if (ListView_GetItemCount(GetDlgItem(hwndDlg, IDC_ID3LIST)) == 0)
            EnableWindow(GetDlgItem(hwndDlg, IDC_ID3V2TAG), FALSE);
        else
            EnableCheckbox(GetDlgItem(hwndDlg, IDC_ID3V2TAG), TRUE, TRUE);
        bFileChanged = FALSE;

        return TRUE;

    case WM_NOTIFY:
        
        /* Branch depending on the specific notification message. */
        switch (((LPNMHDR) lParam)->code) { 
            
        /* Process LVN_GETDISPINFO to supply information about */
        /* callback items. */
        case LVN_GETDISPINFO:
            List_OnGetDispInfo((LV_DISPINFO *)lParam);
            break;
        case NM_DBLCLK:
            bResult = List_EditData(hwndDlg, GetDlgItem(hwndDlg, IDC_ID3LIST));
            if (bResult)
                EnableCheckbox(GetDlgItem(hwndDlg, IDC_ID3V2TAG), TRUE, TRUE);
            bFileChanged = bFileChanged ? bFileChanged : bResult;
            break;
        }
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_ADDSTFRAMES:
            bResult = List_AddStandardFrames(hwndDlg, GetDlgItem(hwndDlg, IDC_ID3LIST));
            if (bResult)
                EnableCheckbox(GetDlgItem(hwndDlg, IDC_ID3V2TAG), TRUE, TRUE);
            bFileChanged = bFileChanged ? bFileChanged : bResult;
            return TRUE;
        case IDC_ADDFRAME:
            bResult = List_AddFrame(hwndDlg, GetDlgItem(hwndDlg, IDC_ID3LIST));
            if (bResult)
                EnableCheckbox(GetDlgItem(hwndDlg, IDC_ID3V2TAG), TRUE, TRUE);
            bFileChanged = bFileChanged ? bFileChanged : bResult;
            return TRUE;
        case IDC_DELFRAME:
            bResult = List_DeleteSelected(hwndDlg, GetDlgItem(hwndDlg, IDC_ID3LIST));
            if (ListView_GetItemCount(GetDlgItem(hwndDlg, IDC_ID3LIST)) == 0)
                EnableCheckbox(GetDlgItem(hwndDlg, IDC_ID3V2TAG), FALSE, FALSE);
            bFileChanged = bFileChanged ? bFileChanged : bResult;
            return TRUE;
        case IDC_EDITFRAME:
            bResult = List_EditData(hwndDlg, GetDlgItem(hwndDlg, IDC_ID3LIST));
            if (bResult)
                EnableCheckbox(GetDlgItem(hwndDlg, IDC_ID3V2TAG), TRUE, TRUE);
            bFileChanged = bFileChanged ? bFileChanged : bResult;
            return TRUE;
        case IDC_ID3V2TAG:
            bFileChanged = TRUE;
            return TRUE;

        case IDC_CLOSE:
        case IDCANCEL:
            if (bFileChanged == TRUE) {
                if (MessageBox(hwndDlg, "Save changes?", "Save changes", MB_YESNO) == IDYES)
                    List_SaveID3(hwndDlg, GetDlgItem(hwndDlg, IDC_ID3LIST), info_fn);
            }
            EndDialog(hwndDlg, wParam);
            return TRUE;
        }
    }
    return FALSE;
}

int infoDlg(char *fn, HWND hwnd)
{
    lstrcpy(info_fn, fn);

    DialogBox(mod.hDllInstance, MAKEINTRESOURCE(IDD_INFO),
        hwnd, info_dialog_proc);

    return 0;
}

LPTSTR PathFindFileName(LPCTSTR pPath)
{
    LPCTSTR pT;

    for (pT = pPath; *pPath; pPath = CharNext(pPath)) {
        if ((pPath[0] == TEXT('\\') || pPath[0] == TEXT(':')) && pPath[1] && (pPath[1] != TEXT('\\')))
            pT = pPath + 1;
    }

    return (LPTSTR)pT;   // const -> non const
}

int getsonglength(char *fn)
{
    faadAACInfo tmp;

    get_AAC_format(fn, &tmp, NULL, NULL, 1);

    return tmp.length;
}

void getfileinfo(char *filename, char *title, int *length_in_ms)
{
    if (!filename || !*filename)  // currently playing file
    {
        if (length_in_ms)
            *length_in_ms=getlength();

        if (title)
            GetID3FileTitle(lastfn, title, m_format_string);
    }
    else // some other file
    {
        if (length_in_ms)
            *length_in_ms = getsonglength(filename);

        if (title)
            GetID3FileTitle(filename, title, m_format_string);
    }
}

void eq_set(int on, char data[10], int preamp)
{
}

int last_frame;

int PlayThread_memmap()
{
    sample_buffer = (char*)faacDecDecode(hDecoder, &frameInfo,
        memmap_buffer + memmap_index, fileread - memmap_index - 1);
    if (frameInfo.error)
    {
        MessageBox(NULL, faacDecGetErrorMessage(frameInfo.error), "FAAD Error", MB_OK);
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

    if (buffercount > 0) {
        for (k = 0; k < (768*2 - buffercount); k++)
            buffer[k] = buffer[k + buffercount];

        read_buffer_filestream(infile, buffer + (768*2) - buffercount, buffercount);
        buffercount = 0;
    }

    sample_buffer = (char*)faacDecDecode(hDecoder, &frameInfo, buffer, 768*2);
    if (frameInfo.error)
    {
        MessageBox(NULL, faacDecGetErrorMessage(frameInfo.error), "FAAD Error", MB_OK);
        last_frame = 1;
    }

    buffercount += frameInfo.bytesconsumed;

    bytecount += frameInfo.bytesconsumed;
    if (bytecount >= fileread)
        last_frame = 1;

    return frameInfo.bytesconsumed;
}

DWORD WINAPI PlayThread(void *b)
{
    int done=0;
    int l;
	int decoded_frames=0;
	int br_calc_frames=0;
	int br_bytes_consumed=0;
    unsigned long bytesconsumed;

	PlayThreadAlive = 1;
    last_frame = 0;

    while (! *((int *)b) )
    {
        if (seek_needed != -1)
        {
            int seconds;

            // Round off to a second
            seconds = seek_needed - (seek_needed%1000);
            mod.outMod->Flush(decode_pos_ms);
            aac_seek(seconds, seek_table);
            decode_pos_ms = seconds;
			seek_needed = -1;
			decoded_frames = 0;
			br_calc_frames = 0;
			br_bytes_consumed = 0;
        }

        if (done)
        {
            mod.outMod->CanWrite();

            if (!mod.outMod->IsPlaying())
            {
                PostMessage(mod.hMainWindow,WM_WA_AAC_EOF,0,0);
                PlayThreadAlive = 0;
                return 0;
            }

            Sleep(10);
        }
        //assume that max channels is 2.
        else if (mod.outMod->CanWrite() >= ((1024*file_info.channels*sizeof(short))<<(mod.dsp_isactive()?1:0)))
        {
            if(last_frame)
            {
                done=1;
            }
            else
            {
                if (current_file_mode)
                    bytesconsumed = PlayThread_memmap();
                else
                    bytesconsumed = PlayThread_file();

				decoded_frames++;
				br_calc_frames++;
				br_bytes_consumed += bytesconsumed;

				/* Update the variable bitrate about every second */
				if(m_variable_bitrate_display && br_calc_frames == 43)
				{
					int br;

					br = (int)((br_bytes_consumed * 8) / (decoded_frames / 43.07));

					mod.SetInfo(br/1000, file_info.sampling_rate/1000, file_info.channels, 1);
					br_calc_frames = 0;
				}

                if (!killPlayThread && (frameInfo.samples > 0))
                {
                    mod.SAAddPCMData(sample_buffer,file_info.channels, 16, decode_pos_ms);
                    mod.VSAAddPCMData(sample_buffer,file_info.channels, 16, decode_pos_ms);
                    decode_pos_ms+=(1024*1000)/file_info.sampling_rate;

                    if (mod.dsp_isactive())
                        l=mod.dsp_dosamples((short *)sample_buffer,frameInfo.samples*sizeof(short)/file_info.channels/(16/8),16,file_info.channels,file_info.sampling_rate)*(file_info.channels*(16/8));
                    else
                        l = frameInfo.samples*sizeof(short);

                    mod.outMod->Write(sample_buffer, l);
                }
            }
        }
        else
        {
            Sleep(10);
        }
    }

	if(seek_table)
	{
		free(seek_table);
		seek_table = NULL;
		seek_table_length = 0;
	}
    
	PlayThreadAlive = 0;
	
    return 0;
}

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

In_Module mod =
{
    IN_VER,
    "AudioCoding.com AAC Player: " __DATE__,
    0,  // hMainWindow
    0,  // hDllInstance
    "AAC\0AAC File (*.AAC)\0"
    ,
    1, // is_seekable
    1, // uses output
    config,
    about,
    init,
    quit,
    getfileinfo,
    infoDlg,
    isourfile,
    play,
    pause,
    unpause,
    ispaused,
    stop,

    getlength,
    getoutputtime,
    setoutputtime,

    setvolume,
    setpan,

    0,0,0,0,0,0,0,0,0, // vis stuff


    0,0, // dsp

    eq_set,

    NULL,       // setinfo

    0 // out_mod
};

__declspec( dllexport ) In_Module * winampGetInModule2()
{
    return &mod;
}
