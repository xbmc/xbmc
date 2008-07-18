/*
 * function: Main control program for aacDECdrop
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright (C) 2002 John Edwards
 *
 * last mod: aacDECdrop decoder last updated 2002-03-14
 */

#include <windows.h>
#include <shellapi.h>
#include <string.h>
#include <stdio.h>
#include <commctrl.h>

#include "resource.h"
#include "decthread.h"
#include "decode.h"
#include "misc.h"

#define LOSHORT(l)           ((SHORT)(l))
#define HISHORT(l)           ((SHORT)(((DWORD)(l) >> 16) & 0xFFFF))

#define INI_FILE "aacDECdrop.ini"

#define CREATEFONT(sz) \
	CreateFont((sz), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
		VARIABLE_PITCH | FF_SWISS, "")
   
HANDLE event = NULL;
int width = 130, height = 130;
RECT bar1, bar2, vbrBR;
int prog1 = 0, prog2 = 0;
int moving = 0;
POINT pt;
HINSTANCE hinst;
int frame = 0;
HBITMAP hbm[12], temp;
HMENU menu;
int decoding_done = 0;
int stop_decoding = 0;
double file_complete;
int totalfiles;
int numfiles;
HWND g_hwnd;
HWND qcwnd;
HFONT font2;
char *fileName;

SettingsAAC     iniSettings;            // iniSettings holds the parameters for the aacDECdrop configuration

int animate = 0;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK QCProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) ;


/*
 *  Write the .ini file using the current aacDECdrop settings
 */

int
WriteIniFile ( const char* Filename )
{
	FILE* fp;

	if ( (fp = fopen (Filename, "w")) == NULL )
		return EOF;                             // could not open file

	fprintf (fp, "[aacDECdrop]\n");
	fprintf (fp, "Window_X=%i\n"          , iniSettings.window_x     );
	fprintf (fp, "Window_Y=%i\n"          , iniSettings.window_y     );
	fprintf (fp, "Always_on_top=%i\n"     , iniSettings.always_on_top);
	fprintf (fp, "Logerr=%i\n"            , iniSettings.logerr       );
	fprintf (fp, "DecodeMode=%i\n"        , iniSettings.decode_mode  );
	fprintf (fp, "OutputFormat=%i\n"      , iniSettings.outputFormat );
	fprintf (fp, "FileType=%i\n"          , iniSettings.fileType     );
	fprintf (fp, "ObjectType=%i\n"        , iniSettings.object_type  );
	return fclose (fp);
}

/*
 * Read the .ini file and set the aacDECdrop settings
 */

int
ReadIniFile ( FILE* fp )
{
	char  buff [256];
	int   val;

	rewind ( fp );
	fgets ( buff, sizeof buff, fp );

	if ( 0 != memcmp ( buff, "[aacDECdrop]", 12 ) )
		return EOF;

	while ( fgets ( buff, sizeof buff, fp ) != NULL ) {
		if      ( 1 == sscanf ( buff, "Window_X=%d"     , &val ) )  iniSettings.window_x      = val;
		else if ( 1 == sscanf ( buff, "Window_Y=%d"     , &val ) )  iniSettings.window_y      = val;
		else if ( 1 == sscanf ( buff, "Always_on_top=%d", &val ) )  iniSettings.always_on_top = val;
		else if ( 1 == sscanf ( buff, "Logerr=%d"       , &val ) )  iniSettings.logerr        = val;
		else if ( 1 == sscanf ( buff, "DecodeMode=%d"   , &val ) )  iniSettings.decode_mode   = val;
		else if ( 1 == sscanf ( buff, "OutputFormat=%d" , &val ) )  iniSettings.outputFormat  = val;
		else if ( 1 == sscanf ( buff, "FileType=%d"     , &val ) )  iniSettings.fileType      = val;
		else if ( 1 == sscanf ( buff, "ObjectType=%d"   , &val ) )  iniSettings.object_type   = val;
	}

	return 0;
}


/*
 * Get aacDECdrop settings at startup, writes .ini file, if not present
 */

void
GetAACdecSettings ( void )
{
	FILE*  fp = NULL;
	char   PathAndName [] = {INI_FILE};

	// set default values
	iniSettings.window_x      =   64;        // default box position (x co-ord)
	iniSettings.window_y      =   64;        // default box position (y co-ord)
	iniSettings.always_on_top =    8;        // default = on
	iniSettings.logerr        =    0;        // default = off
	iniSettings.decode_mode   =    1;        // default = 1 (decode to file)
	iniSettings.outputFormat  =    1;        // default = 1 (16 bit PCM)
	iniSettings.fileType      =    1;        // default = 1 (Microsoft WAV)
	iniSettings.object_type   =    1;        // default = 1 (Low Complexity)

	// Read INI_FILE
	if ( (fp = fopen (PathAndName, "r")) == NULL ) {    // file does not exist: write it!
		WriteIniFile ( PathAndName );
	}
	else {                                              // file does exist: read it!
		ReadIniFile (fp);
		fclose (fp);
	}

	return;
}

void set_always_on_top(HWND hwnd, int v)
{
	CheckMenuItem(menu, IDM_ONTOP, v ? MF_CHECKED : MF_UNCHECKED);
	SetWindowPos(hwnd, v ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE);
	iniSettings.always_on_top = v;
}

void set_logerr(HWND hwnd, int v)
{
	CheckMenuItem(menu, IDM_LOGERR, v ? MF_CHECKED : MF_UNCHECKED);
	iniSettings.logerr = v;
	set_use_dialogs(v);
}

void set_decode_mode(int v)
{
	decthread_set_decode_mode(v);
	iniSettings.decode_mode = v;
}

void set_outputFormat(int v)
{
	decthread_set_outputFormat(v);
	iniSettings.outputFormat = v;
}

void set_fileType(int v)
{
	decthread_set_fileType(v);
	iniSettings.fileType = v;
}

void set_object_type(int v)
{
	decthread_set_object_type(v);
	iniSettings.object_type = v;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	static char szAppName[] = "aacDECdrop";
	HWND hwnd;
	MSG msg;
	WNDCLASS wndclass;
	const int width = 130;
	const int height = 130;
	int x;
	int y;

	hinst = hInstance;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(hinst, MAKEINTRESOURCE(IDI_ICON1));
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szAppName;

	RegisterClass(&wndclass);

	GetAACdecSettings();

	x = max(min(iniSettings.window_x, GetSystemMetrics(SM_CXSCREEN) - width), 0);
	y = max(min(iniSettings.window_y, GetSystemMetrics(SM_CYSCREEN) - height), 0);

	hwnd = CreateWindow(szAppName, "aacDECdrop", WS_POPUP | WS_DLGFRAME, x, y,
	width, height, NULL, NULL, hInstance, NULL);

	g_hwnd = hwnd;

	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);

	font2 = CREATEFONT(10);

	SetTimer(hwnd, 1, 80, NULL);

	set_always_on_top(hwnd, iniSettings.always_on_top);
	set_logerr(hwnd, iniSettings.logerr);
	set_decode_mode(iniSettings.decode_mode);
	set_outputFormat(iniSettings.outputFormat);
	set_fileType(iniSettings.fileType);
	set_object_type(iniSettings.object_type);
	
	for (frame = 0; frame < 8; frame++)
		hbm[frame] = LoadImage(hinst, MAKEINTRESOURCE(IDB_TF01 + frame), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	frame = 0;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	for (frame = 0; frame < 8; frame++) 
		DeleteObject(hbm[frame]);

	return msg.wParam;
}

void HandleDrag(HWND hwnd, HDROP hDrop)
{
	int cFiles, i;
	char szFile[MAX_PATH];
	char *ext;
	int flag = 0;

	cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
	for (i = 0; i < cFiles; i++)
	{
		DragQueryFile(hDrop, i, szFile, sizeof(szFile));

		if (ext = strrchr(szFile, '.'))
		{
			if (stricmp(ext, ".aac") == 0 || stricmp(ext, ".mp4") == 0 ||
                stricmp(ext, ".m4a") == 0 || stricmp(ext, ".m4p") == 0)
			{
				flag = 1;
				decthread_addfile(szFile);
				stop_decoding = 0;
			}
		}
	}

	DragFinish(hDrop);

	if (flag)
		SetEvent(event);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc, hmem;
	static HDC offscreen;
	PAINTSTRUCT ps;
	RECT rect, rect2;
	BITMAP bm;
	POINT point;
	static POINT start;
	static int dragging = 0;
	HDC desktop;
	HBITMAP hbitmap;
	HANDLE hdrop;
	HFONT dfltFont;
	int dfltBGMode;
	double percomp;

	switch (message)
	{
		case WM_CREATE:
			menu = LoadMenu(hinst, MAKEINTRESOURCE(IDR_MENU1));
			menu = GetSubMenu(menu, 0);

			offscreen = CreateCompatibleDC(NULL);
			desktop = GetDC(GetDesktopWindow());
			hbitmap = CreateCompatibleBitmap(desktop, 200, 200);
			ReleaseDC(GetDesktopWindow(), desktop);
			SelectObject(offscreen, hbitmap);

			// Start the engines
			decthread_init();

			// We accept drag&drop
			DragAcceptFiles(hwnd, TRUE);
			return 0;

		case WM_PAINT:
			hdc = BeginPaint(hwnd, &ps);
			GetClientRect(hwnd, &rect);
			width = rect.right + 1;
			height = rect.bottom + 1;

			FillRect(offscreen, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
			DrawText(offscreen, "Drop Files Here", -1, &rect, DT_SINGLELINE | DT_CENTER);
			SetRect(&rect2, 0, height - 110, width, height - 25);
			DrawText(offscreen, "For Decoding", -1, &rect2, DT_SINGLELINE | DT_CENTER);

			hmem = CreateCompatibleDC(offscreen);
			SelectObject(hmem, hbm[frame]);
			GetObject(hbm[frame], sizeof(BITMAP), &bm);
			BitBlt(offscreen, width / 2 - 33, height / 2 - 31, bm.bmWidth, bm.bmHeight, hmem, 0, 0, SRCCOPY);
			DeleteDC(hmem);

			percomp = ((double)(totalfiles - numfiles) + 1 - (1 - file_complete)) / (double)totalfiles;

			SetRect(&vbrBR, 0, height - 35, width, height - 19);

			dfltBGMode = SetBkMode(offscreen, TRANSPARENT);
			dfltFont = SelectObject(offscreen, font2);

			SetRect(&bar1, 0, height - 23, (int)(file_complete * width), height - 13);
			SetRect(&bar2, 0, height - 12, (int)(percomp * width), height - 2);

			FillRect(offscreen, &bar1, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
			FillRect(offscreen, &bar2, (HBRUSH)GetStockObject(DKGRAY_BRUSH));

			if (fileName)
			{
				char* sep;
				char  fileCaption[80];

				if ((sep = strrchr(fileName, '\\')) != 0)
					fileName = sep+1;

				(void) strcpy(fileCaption, "   ");
				(void) strcat(fileCaption, fileName);

				DrawText(offscreen, fileCaption, -1, &bar1, DT_SINGLELINE | DT_LEFT);
			}

			SelectObject(offscreen, dfltFont);
			SetBkMode(offscreen, dfltBGMode);

			BitBlt(hdc, 0, 0, width, height, offscreen, 0, 0, SRCCOPY);

			EndPaint(hwnd, &ps);

			return DefWindowProc(hwnd, message, wParam, lParam);
			//return 0;

		case WM_TIMER:
			if (animate || frame)
			{
				frame++;
				if (frame > 7) 
					frame -= 8;
			}
			else
			{
				frame = 0;
			}
			GetClientRect(hwnd, &rect);
			InvalidateRect(hwnd, &rect, FALSE);
			return 0;

		case WM_LBUTTONDOWN:
			start.x = LOWORD(lParam);
			start.y = HIWORD(lParam);
			ClientToScreen(hwnd, &start);
			GetWindowRect(hwnd, &rect);
			start.x -= rect.left;
			start.y -= rect.top;
			dragging = 1;
			SetCapture(hwnd);
			return 0;

		case WM_LBUTTONUP:
			if (dragging)
			{
				dragging = 0;
				ReleaseCapture();
			}
			return 0;

		case WM_MOUSEMOVE:
			if (dragging)
			{
				point.x = LOSHORT(lParam);
				point.y = HISHORT(lParam);

				/* lParam can contain negative coordinates !
				 * point.x = LOWORD(lParam);
				 * point.y = HIWORD(lParam);
				 */

				ClientToScreen(hwnd, &point);
				SetWindowPos(hwnd, 0, point.x - start.x, point.y - start.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
				iniSettings.window_x = point.x - start.x;
				iniSettings.window_y = point.y - start.y;
			}
			return 0;

		case WM_CAPTURECHANGED:
			if (dragging)
			{
				dragging = 0;
				ReleaseCapture();
			}
			return 0;

		case WM_RBUTTONUP:
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			ClientToScreen(hwnd, &point);
			TrackPopupMenu(menu, TPM_RIGHTBUTTON, point.x, point.y, 0, hwnd, NULL);
			return 0;

		case WM_COMMAND:
			switch (LOWORD(wParam)) 
			{
				case IDM_QUIT:
					WriteIniFile(INI_FILE);
					decoding_done = 1;
					PostQuitMessage(0);
					break;
				case IDM_ONTOP:
					set_always_on_top(hwnd, ~GetMenuState(menu, LOWORD(wParam), MF_BYCOMMAND) & MF_CHECKED);
					break;	
				case IDM_LOGERR:
					set_logerr(hwnd, ~GetMenuState(menu, LOWORD(wParam), MF_BYCOMMAND) & MF_CHECKED);
					break;
				case IDM_STOP_DEC:
				{
					int v = ~GetMenuState(menu, LOWORD(wParam), MF_BYCOMMAND) & MF_CHECKED;
					if(v == 8)
						stop_decoding = 1;
					break;
				}
				case IDM_VOLUME:
				{
					int value = 
					DialogBox(
					hinst,  
					MAKEINTRESOURCE(IDD_VOLUME),   
					hwnd, QCProc);

					if (value == -2)
						break;
					break;
				}
				case IDM_ABOUT:
				{
					int value = DialogBox(hinst, MAKEINTRESOURCE(IDD_ABOUT), hwnd, QCProc);
					if (value == -7)
						break;
					break;
				}

			} // LOWORD(wParam)
			return 0;

		case WM_DROPFILES:
			hdrop = (HANDLE)wParam;
			HandleDrag(hwnd, hdrop);
			return 0;
	
		case WM_DESTROY:
			decoding_done = 1;
			PostQuitMessage(0);
			return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

/*
 *  Encode parameters dialog procedures.
 */

BOOL CALLBACK QCProc(HWND hwndDlg, UINT message, 
                     WPARAM wParam, LPARAM lParam) 
{
	switch (message) 
	{ 
		case WM_INITDIALOG: 
 
			if(iniSettings.decode_mode == 0)
			{
				CheckDlgButton(hwndDlg,IDC_PLAYBACK,TRUE);
				CheckDlgButton(hwndDlg,IDC_WAV,TRUE);
				if(iniSettings.outputFormat != 1
					&& iniSettings.outputFormat != 5
					&& iniSettings.outputFormat != 6
					&& iniSettings.outputFormat != 7
					&& iniSettings.outputFormat != 8)
					CheckDlgButton(hwndDlg,IDC_16BIT,TRUE);
				else if(iniSettings.outputFormat == 1)
					CheckDlgButton(hwndDlg,IDC_16BIT,TRUE);
				else if(iniSettings.outputFormat == 5)
					CheckDlgButton(hwndDlg,IDC_16BIT_DITHER,TRUE);
				else if(iniSettings.outputFormat == 6)
					CheckDlgButton(hwndDlg,IDC_16BIT_L_SHAPE,TRUE);
				else if(iniSettings.outputFormat == 7)
					CheckDlgButton(hwndDlg,IDC_16BIT_M_SHAPE,TRUE);
				else if(iniSettings.outputFormat == 8)
					CheckDlgButton(hwndDlg,IDC_16BIT_H_SHAPE,TRUE);
				CheckDlgButton(hwndDlg,IDC_WAV,TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AIFF), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_SUNAU), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_DECAU), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_24BIT), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_32BIT), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_FLOATS), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_16BIT), TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_16BIT_DITHER), TRUE);
			}
			else if(iniSettings.decode_mode == 1)
			{
				CheckDlgButton(hwndDlg,IDC_PLAYBACK,FALSE);
				if(iniSettings.outputFormat == 1)
					CheckDlgButton(hwndDlg,IDC_16BIT,TRUE);
				else if(iniSettings.outputFormat == 2)
					CheckDlgButton(hwndDlg,IDC_24BIT,TRUE);
				else if(iniSettings.outputFormat == 3)
					CheckDlgButton(hwndDlg,IDC_32BIT,TRUE);
				else if(iniSettings.outputFormat == 4)
					CheckDlgButton(hwndDlg,IDC_FLOATS,TRUE);
				else if(iniSettings.outputFormat == 5)
					CheckDlgButton(hwndDlg,IDC_16BIT_DITHER,TRUE);
				else if(iniSettings.outputFormat == 6)
					CheckDlgButton(hwndDlg,IDC_16BIT_L_SHAPE,TRUE);
				else if(iniSettings.outputFormat == 7)
					CheckDlgButton(hwndDlg,IDC_16BIT_M_SHAPE,TRUE);
				else if(iniSettings.outputFormat == 8)
					CheckDlgButton(hwndDlg,IDC_16BIT_H_SHAPE,TRUE);

				if(iniSettings.fileType == 1)
					CheckDlgButton(hwndDlg,IDC_WAV,TRUE);
				else if(iniSettings.fileType == 2)
					CheckDlgButton(hwndDlg,IDC_AIFF,TRUE);
				else if(iniSettings.fileType == 3)
					CheckDlgButton(hwndDlg,IDC_SUNAU,TRUE);
				else if(iniSettings.fileType == 4)
					CheckDlgButton(hwndDlg,IDC_DECAU,TRUE);
			}

			if(iniSettings.object_type == 0)
				CheckDlgButton(hwndDlg,IDC_MAIN,TRUE);
			else if(iniSettings.object_type == 1)
				CheckDlgButton(hwndDlg,IDC_LC,TRUE);
			else if(iniSettings.object_type == 3)
				CheckDlgButton(hwndDlg,IDC_LTP,TRUE);
			else if(iniSettings.object_type == 23)
				CheckDlgButton(hwndDlg,IDC_LD,TRUE);
			break;

		case WM_CLOSE:
			EndDialog(hwndDlg, -1);
			break;

		case WM_COMMAND: 
			switch (LOWORD(wParam)) 
			{
				case IDC_BUTTON1:
				{
					if (IsDlgButtonChecked(hwndDlg, IDC_PLAYBACK) == BST_CHECKED)
						set_decode_mode(0);            // Playback
					else if (IsDlgButtonChecked(hwndDlg, IDC_DECODE) == BST_CHECKED)
						set_decode_mode(1);            // Decode to File

					if (IsDlgButtonChecked(hwndDlg, IDC_WAV) == BST_CHECKED)
						set_fileType(1);             // Microsoft WAV
					else if (IsDlgButtonChecked(hwndDlg, IDC_AIFF) == BST_CHECKED)
						set_fileType(2);             // Apple/SGI AIFF
					else if (IsDlgButtonChecked(hwndDlg, IDC_SUNAU) == BST_CHECKED)
						set_fileType(3);             // Sun/NeXT AU
					else if (IsDlgButtonChecked(hwndDlg, IDC_DECAU) == BST_CHECKED)
						set_fileType(4);             // DEC AU

					if (IsDlgButtonChecked(hwndDlg, IDC_16BIT) == BST_CHECKED)
						set_outputFormat(1);             // 16 bit PCM
					else if (IsDlgButtonChecked(hwndDlg, IDC_24BIT) == BST_CHECKED)
						set_outputFormat(2);             // 24 bit PCM
					else if (IsDlgButtonChecked(hwndDlg, IDC_32BIT) == BST_CHECKED)
						set_outputFormat(3);             // 32 bit PCM
					else if (IsDlgButtonChecked(hwndDlg, IDC_FLOATS) == BST_CHECKED)
						set_outputFormat(4);             // 32 bit floats
					else if (IsDlgButtonChecked(hwndDlg, IDC_16BIT_DITHER) == BST_CHECKED)
						set_outputFormat(5);             // 16 bit PCM dithered
					else if (IsDlgButtonChecked(hwndDlg, IDC_16BIT_L_SHAPE) == BST_CHECKED)
						set_outputFormat(6);             // dithered LIGHT noise shaping
					else if (IsDlgButtonChecked(hwndDlg, IDC_16BIT_M_SHAPE) == BST_CHECKED)
						set_outputFormat(7);             // dithered MEDIUM noise shaping
					else if (IsDlgButtonChecked(hwndDlg, IDC_16BIT_H_SHAPE) == BST_CHECKED)
						set_outputFormat(8);             // dithered HEAVY noise shaping

					if (IsDlgButtonChecked(hwndDlg, IDC_MAIN) == BST_CHECKED)
						set_object_type(0);             // Main
					else if (IsDlgButtonChecked(hwndDlg, IDC_LC) == BST_CHECKED)
						set_object_type(1);             // Low Complexity
					else if (IsDlgButtonChecked(hwndDlg, IDC_LTP) == BST_CHECKED)
						set_object_type(3);             // Long Term Prediction
					else if (IsDlgButtonChecked(hwndDlg, IDC_LD) == BST_CHECKED)
						set_object_type(23);            // Low Delay

					EndDialog(hwndDlg, -2);
					return TRUE;
				}
				case IDC_BUTTON6:
					EndDialog(hwndDlg, -7);
					return TRUE;

				case IDC_PLAYBACK:
					CheckDlgButton(hwndDlg,IDC_WAV,TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_AIFF), FALSE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_SUNAU), FALSE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_DECAU), FALSE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_24BIT), FALSE);
					CheckDlgButton(hwndDlg,IDC_24BIT,FALSE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_32BIT), FALSE);
					CheckDlgButton(hwndDlg,IDC_32BIT,FALSE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_FLOATS), FALSE);
					CheckDlgButton(hwndDlg,IDC_FLOATS,FALSE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_16BIT), TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_16BIT_DITHER), TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_16BIT_L_SHAPE), TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_16BIT_M_SHAPE), TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_16BIT_H_SHAPE), TRUE);
					if (IsDlgButtonChecked(hwndDlg, IDC_16BIT_DITHER) != BST_CHECKED
						&& IsDlgButtonChecked(hwndDlg, IDC_16BIT_L_SHAPE) != BST_CHECKED
						&& IsDlgButtonChecked(hwndDlg, IDC_16BIT_M_SHAPE) != BST_CHECKED
						&& IsDlgButtonChecked(hwndDlg, IDC_16BIT_H_SHAPE) != BST_CHECKED)
						CheckDlgButton(hwndDlg,IDC_16BIT,TRUE);
					break;
				case IDC_DECODE:
					EnableWindow(GetDlgItem(hwndDlg, IDC_AIFF), FALSE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_SUNAU), FALSE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_DECAU), FALSE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_24BIT), TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_32BIT), TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_FLOATS), TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_16BIT), TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_16BIT_DITHER), TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_16BIT_L_SHAPE), TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_16BIT_M_SHAPE), TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_16BIT_H_SHAPE), TRUE);
					break;
				default:
					break;
       	  		}
	}
	return FALSE; 
}


/******************************** end of main.c ********************************/

