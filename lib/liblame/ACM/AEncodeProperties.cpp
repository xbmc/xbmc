/**
 *
 * Lame ACM wrapper, encode/decode MP3 based RIFF/AVI files in MS Windows
 *
 *  Copyright (c) 2002 Steve Lhomme <steve.lhomme at free.fr>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 
/*!
	\author Steve Lhomme
	\version \$Id: AEncodeProperties.cpp,v 1.9.8.1 2008/11/01 20:41:47 robert Exp $
*/

#if !defined(STRICT)
#define STRICT
#endif // !defined(STRICT)

#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <assert.h>

#ifdef _MSC_VER
// no problem with unknown pragmas
#pragma warning(disable: 4068)
#endif

#include "resource.h"
#include <lame.h>
#include "adebug.h"
#include "AEncodeProperties.h"
#include "ACM.h"
//#include "AParameters/AParameters.h"

#ifndef TTS_BALLOON
#define TTS_BALLOON            0x40
#endif // TTS_BALLOON

const unsigned int AEncodeProperties::the_Bitrates[18] = {320, 256, 224, 192, 160, 144, 128, 112, 96, 80, 64, 56, 48, 40, 32, 24, 16, 8 };
const unsigned int AEncodeProperties::the_MPEG1_Bitrates[14] = {320, 256, 224, 192, 160, 128, 112, 96, 80, 64, 56, 48, 40, 32 };
const unsigned int AEncodeProperties::the_MPEG2_Bitrates[14] = {160, 144, 128, 112, 96, 80, 64, 56, 48, 40, 32, 24, 16, 8};
const unsigned int AEncodeProperties::the_ChannelModes[3] = { STEREO, JOINT_STEREO, DUAL_CHANNEL };
//const char         AEncodeProperties::the_Presets[][13] = {"None", "CD", "Studio", "Hi-Fi", "Phone", "Voice", "Radio", "Tape", "FM", "AM", "SW"};
//const LAME_QUALTIY_PRESET AEncodeProperties::the_Presets[] = {LQP_NOPRESET, LQP_R3MIX_QUALITY, LQP_NORMAL_QUALITY, LQP_LOW_QUALITY, LQP_HIGH_QUALITY, LQP_VERYHIGH_QUALITY, LQP_VOICE_QUALITY, LQP_PHONE, LQP_SW, LQP_AM, LQP_FM, LQP_VOICE, LQP_RADIO, LQP_TAPE, LQP_HIFI, LQP_CD, LQP_STUDIO};
//const unsigned int AEncodeProperties::the_SamplingFreqs[9] = { 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000 };

ToolTipItem AEncodeProperties::Tooltips[13]={
	{ IDC_CHECK_ENC_ABR, "Allow encoding with an average bitrate\r\ninstead of a constant one.\r\n\r\nIt can improve the quality for the same bitrate." },
	{ IDC_CHECK_COPYRIGHT, "Mark the encoded data as copyrighted." },
	{ IDC_CHECK_CHECKSUM, "Put a checksum in the encoded data.\r\n\r\nThis can make the file less sensitive to data loss." },
	{ IDC_CHECK_ORIGINAL, "Mark the encoded data as an original file." },
	{ IDC_CHECK_PRIVATE, "Mark the encoded data as private." },
	{ IDC_COMBO_ENC_STEREO, "Select the type of stereo mode used for encoding:\r\n\r\n- Stereo : the usual one\r\n- Joint-Stereo : mix both channel to achieve better compression\r\n- Dual Channel : treat both channel as separate" },
	{ IDC_STATIC_DECODING, "Decoding not supported for the moment by the codec." },
	{ IDC_CHECK_ENC_SMART, "Disable bitrate when there is too much compression.\r\n(default 1:15 ratio)" },
	{ IDC_STATIC_CONFIG_VERSION, "Version of this codec.\r\n\r\nvX.X.X is the version of the codec interface.\r\nX.XX is the version of the encoding engine." },
	{ IDC_SLIDER_AVERAGE_MIN, "Select the minimum Average Bitrate allowed." },
	{ IDC_SLIDER_AVERAGE_MAX, "Select the maximum Average Bitrate allowed." },
	{ IDC_SLIDER_AVERAGE_STEP, "Select the step of Average Bitrate between the min and max.\r\n\r\nA step of 5 between 152 and 165 means you have :\r\n165, 160 and 155" },
	{ IDC_SLIDER_AVERAGE_SAMPLE, "Check the resulting values of the (min,max,step) combination.\r\n\r\nUse the keyboard to navigate (right -> left)." },
};
//int AEncodeProperties::tst = 0;

/*
#pragma argsused
static UINT CALLBACK DLLFindCallback(
  HWND hdlg,      // handle to child dialog box
  UINT uiMsg,     // message identifier
  WPARAM wParam,  // message parameter
  LPARAM lParam   // message parameter
  )
{
	UINT result = 0;

	switch (uiMsg)
	{
		case WM_NOTIFY:
			OFNOTIFY * info = (OFNOTIFY *)lParam;
			if (info->hdr.code == CDN_FILEOK)
			{
				result = 1; // by default we don't accept the file

				// Check if the selected file is a valid DLL with all the required functions
				ALameDLL * tstFile = new ALameDLL;
				if (tstFile != NULL)
				{
					if (tstFile->Load(info->lpOFN->lpstrFile))
					{
						result = 0;
					}

					delete tstFile;
				}

				if (result == 1)
				{
					TCHAR output[250];
					::LoadString(AOut::GetInstance(),IDS_STRING_DLL_UNRECOGNIZED,output,250);
					AOut::MyMessageBox( output, MB_OK|MB_ICONEXCLAMATION, hdlg);
					SetWindowLong(hdlg, DWL_MSGRESULT , -100);
				}
			}
	}

	return result;
}

#pragma argsused
static int CALLBACK BrowseFolderCallbackroc(
    HWND hwnd,
    UINT uMsg,
    LPARAM lParam,
    LPARAM lpData
    )
{
	AEncodeProperties * the_prop;
	the_prop = (AEncodeProperties *) lpData;


	if (uMsg == BFFM_INITIALIZED)
	{
//		char FolderName[MAX_PATH];
//		SHGetPathFromIDList((LPITEMIDLIST) lParam,FolderName);
//ADbg tst;
//tst.OutPut("init folder to %s ",the_prop->GetOutputDirectory());
//		CreateFile();
		::SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM)TRUE, (LPARAM)the_prop->GetOutputDirectory());
	}/* else if (uMsg == BFFM_SELCHANGED)
	{
		// verify that the folder is writable
//		::SendMessage(hwnd, BFFM_ENABLEOK, 0, (LPARAM)0); // disable
		char FolderName[MAX_PATH];
		SHGetPathFromIDList((LPITEMIDLIST) lParam, FolderName);
		
//		if (CreateFile(FolderName,STANDARD_RIGHTS_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL) == INVALID_HANDLE_VALUE)
		if ((GetFileAttributes(FolderName) & FILE_ATTRIBUTE_DIRECTORY) != 0)
			::SendMessage(hwnd, BFFM_ENABLEOK, 0, (LPARAM)1); // enable
		else
			::SendMessage(hwnd, BFFM_ENABLEOK, 0, (LPARAM)0); // disable
//ADbg tst;
//tst.OutPut("change folder to %s ",FolderName);
	}* /

	return 0;
}
*/
#pragma argsused
static BOOL CALLBACK ConfigProc(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
  )
{
	BOOL bResult;
	AEncodeProperties * the_prop;
	the_prop = (AEncodeProperties *) GetProp(hwndDlg, "AEncodeProperties-Config");

	switch (uMsg) {
		case WM_COMMAND:
			if (the_prop != NULL)
			{
				bResult = the_prop->HandleDialogCommand( hwndDlg, wParam, lParam);
			}
			break;
		case WM_INITDIALOG:
			assert(the_prop == NULL);

			the_prop = (AEncodeProperties *) lParam;
			the_prop->my_debug.OutPut("there hwnd = 0x%08X",hwndDlg);

			assert(the_prop != NULL);

			SetProp(hwndDlg, "AEncodeProperties-Config", the_prop);

			the_prop->InitConfigDlg(hwndDlg);

			bResult = TRUE;
			break;

		case WM_HSCROLL:
			// check if it's the ABR sliders
			if ((HWND)lParam == GetDlgItem(hwndDlg,IDC_SLIDER_AVERAGE_MIN))
			{
				the_prop->UpdateDlgFromSlides(hwndDlg);
			}
			else if ((HWND)lParam == GetDlgItem(hwndDlg,IDC_SLIDER_AVERAGE_MAX))
			{
				the_prop->UpdateDlgFromSlides(hwndDlg);
			}
			else if ((HWND)lParam == GetDlgItem(hwndDlg,IDC_SLIDER_AVERAGE_STEP))
			{
				the_prop->UpdateDlgFromSlides(hwndDlg);
			}
			else if ((HWND)lParam == GetDlgItem(hwndDlg,IDC_SLIDER_AVERAGE_SAMPLE))
			{
				the_prop->UpdateDlgFromSlides(hwndDlg);
			}
			break;

		case WM_NOTIFY:
			if (TTN_GETDISPINFO == ((LPNMHDR)lParam)->code) {
				NMTTDISPINFO *lphdr = (NMTTDISPINFO *)lParam;
				UINT id = (lphdr->uFlags & TTF_IDISHWND) ? GetWindowLong((HWND)lphdr->hdr.idFrom, GWL_ID) : lphdr->hdr.idFrom;

				*lphdr->lpszText = 0;

				SendMessage(lphdr->hdr.hwndFrom, TTM_SETMAXTIPWIDTH, 0, 5000);

				for(int i=0; i<sizeof AEncodeProperties::Tooltips/sizeof AEncodeProperties::Tooltips[0]; ++i) {
					if (id == AEncodeProperties::Tooltips[i].id)
						lphdr->lpszText = const_cast<char *>(AEncodeProperties::Tooltips[i].tip);
				}

				return TRUE;
			}
			break;

		default:
			bResult = FALSE; // will be treated by DefWindowProc
	}
	return bResult;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
/**
	\class AEncodeProperties
*/


const char * AEncodeProperties::GetChannelModeString(int a_channelID) const
{
	assert(a_channelID < sizeof(the_ChannelModes));

	switch (a_channelID) {
		case 0:
			return "Stereo";
		case 1:
			return "Joint-stereo";
		case 2:
			return "Dual Channel";
		default:
			assert(a_channelID);
			return NULL;
	}
}

const int AEncodeProperties::GetBitrateString(char * string, int string_size, int a_bitrateID) const
{
	assert(a_bitrateID < sizeof(the_Bitrates));
	assert(string != NULL);

	if (string_size >= 4)
		return wsprintf(string,"%d",the_Bitrates[a_bitrateID]);
	else
		return -1;
}

const unsigned int AEncodeProperties::GetChannelModeValue() const
{
	assert(nChannelIndex < sizeof(the_ChannelModes));

	return the_ChannelModes[nChannelIndex];
}

const unsigned int AEncodeProperties::GetBitrateValue() const
{
	assert(nMinBitrateIndex < sizeof(the_Bitrates));

	return the_Bitrates[nMinBitrateIndex];
}

inline const int AEncodeProperties::GetBitrateValueMPEG2(DWORD & bitrate) const
{
	int i;

	for (i=0;i<sizeof(the_MPEG2_Bitrates)/sizeof(unsigned int);i++)
	{
		if (the_MPEG2_Bitrates[i] == the_Bitrates[nMinBitrateIndex])
		{
			bitrate = the_MPEG2_Bitrates[i];
			return 0;
		}
		else if (the_MPEG2_Bitrates[i] < the_Bitrates[nMinBitrateIndex])
		{
			bitrate = the_MPEG2_Bitrates[i];
			return -1;
		}
	}
	
	bitrate = 160;
	return -1;
}

inline const int AEncodeProperties::GetBitrateValueMPEG1(DWORD & bitrate) const
{
	int i;

	for (i=sizeof(the_MPEG1_Bitrates)/sizeof(unsigned int)-1;i>=0;i--)
	{
		if (the_MPEG1_Bitrates[i] == the_Bitrates[nMinBitrateIndex])
		{
			bitrate = the_MPEG1_Bitrates[i];
			return 0;
		}
		else if (the_MPEG1_Bitrates[i] > the_Bitrates[nMinBitrateIndex])
		{
			bitrate = the_MPEG1_Bitrates[i];
			return 1;
		}
	}
	
	bitrate = 32;
	return 1;
}
/*
const int AEncodeProperties::GetBitrateValue(DWORD & bitrate, const DWORD MPEG_Version) const
{
	assert((MPEG_Version == MPEG1) || (MPEG_Version == MPEG2));
	assert(nMinBitrateIndex < sizeof(the_Bitrates));

	if (MPEG_Version == MPEG2)
		return GetBitrateValueMPEG2(bitrate);
	else
		return GetBitrateValueMPEG1(bitrate);
}
/*
const char * AEncodeProperties::GetPresetModeString(const int a_presetID) const
{
	assert(a_presetID < sizeof(the_Presets));

	switch (a_presetID) {
		case 1:
			return "r3mix";
		case 2:
			return "Normal";
		case 3:
			return "Low";
		case 4:
			return "High";
		case 5:
			return "Very High";
		case 6:
			return "Voice";
		case 7:
			return "Phone";
		case 8:
			return "SW";
		case 9:
			return "AM";
		case 10:
			return "FM";
		case 11:
			return "Voice";
		case 12:
			return "Radio";
		case 13:
			return "Tape";
		case 14:
			return "Hi-Fi";
		case 15:
			return "CD";
		case 16:
			return "Studio";
		default:
			return "None";
	}
}

const LAME_QUALTIY_PRESET AEncodeProperties::GetPresetModeValue() const
{
	assert(nPresetIndex < sizeof(the_Presets));

	return the_Presets[nPresetIndex];
}
*/
bool AEncodeProperties::Config(const HINSTANCE Hinstance, const HWND HwndParent)
{
	//WM_INITDIALOG ?

	// remember the instance to retreive strings
//	hDllInstance = Hinstance;

	my_debug.OutPut("here");
	int ret = ::DialogBoxParam(Hinstance, MAKEINTRESOURCE(IDD_CONFIG), HwndParent, ::ConfigProc, (LPARAM) this);
/*	if (ret == -1)
	{
		LPVOID lpMsgBuf;
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);
		// Process any inserts in lpMsgBuf.
		// ...
		// Display the string.
		AOut::MyMessageBox( (LPCTSTR)lpMsgBuf, MB_OK | MB_ICONINFORMATION );
		// Free the buffer.
		LocalFree( lpMsgBuf );	
		return false;
	}
*/	
	return true;
}

bool AEncodeProperties::InitConfigDlg(HWND HwndDlg)
{
	// get all the required strings
//	TCHAR Version[5];
//	LoadString(hDllInstance, IDS_STRING_VERSION, Version, 5);

	int i;

	// Add required channel modes
	SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_ENC_STEREO), CB_RESETCONTENT , NULL, NULL);
	for (i=0;i<GetChannelLentgh();i++)
		SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_ENC_STEREO), CB_ADDSTRING, NULL, (LPARAM) GetChannelModeString(i));

	char tmp[20];
	wsprintf(tmp, "v%s",ACM::GetVersionString());
	SetWindowText( GetDlgItem( HwndDlg, IDC_STATIC_CONFIG_VERSION), tmp);

	// Add all possible re-sampling freq
/*	SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_SAMPLEFREQ), CB_RESETCONTENT , NULL, NULL);
	char tmp[10];
	for (i=0;i<sizeof(the_SamplingFreqs)/sizeof(unsigned int);i++)
	{
		wsprintf(tmp, "%d", the_SamplingFreqs[i]);
		SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_SAMPLEFREQ), CB_ADDSTRING, NULL, (LPARAM) tmp );
	}
*/	

	// Add required bitrates
/*	SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_BITRATE), CB_RESETCONTENT , NULL, NULL);
	for (i=0;i<GetBitrateLentgh();i++)
	{
		GetBitrateString(tmp, 5, i);
		SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_BITRATE), CB_ADDSTRING, NULL, (LPARAM) tmp );
	}

	// Add bitrates to the VBR combo box too
	SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_MAXBITRATE), CB_RESETCONTENT , NULL, NULL);
	for (i=0;i<GetBitrateLentgh();i++)
	{
		GetBitrateString(tmp, 5, i);
		SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_MAXBITRATE), CB_ADDSTRING, NULL, (LPARAM) tmp );
	}

	// Add VBR Quality Slider
	SendMessage(GetDlgItem( HwndDlg, IDC_SLIDER_QUALITY), TBM_SETRANGE, TRUE, MAKELONG(0,9));

	// Add presets
	SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_PRESET), CB_RESETCONTENT , NULL, NULL);
	for (i=0;i<GetPresetLentgh();i++)
		SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_PRESET), CB_ADDSTRING, NULL, (LPARAM) GetPresetModeString(i));
*/

	// Add ABR Sliders
	SendMessage(GetDlgItem( HwndDlg, IDC_SLIDER_AVERAGE_MIN), TBM_SETRANGE, TRUE, MAKELONG(8,320));
	SendMessage(GetDlgItem( HwndDlg, IDC_SLIDER_AVERAGE_MAX), TBM_SETRANGE, TRUE, MAKELONG(8,320));
	SendMessage(GetDlgItem( HwndDlg, IDC_SLIDER_AVERAGE_STEP), TBM_SETRANGE, TRUE, MAKELONG(1,16));

	// Tool-Tip initialiasiation
	TOOLINFO ti;
	HWND ToolTipWnd;
	char DisplayStr[30] = "test tooltip";

	ToolTipWnd = CreateWindowEx(WS_EX_TOPMOST,
        TOOLTIPS_CLASS,
        NULL,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP|TTS_BALLOON ,		
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        HwndDlg,
        NULL,
        NULL,
        NULL
        );

	SetWindowPos(ToolTipWnd,
        HWND_TOPMOST,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    /* INITIALIZE MEMBERS OF THE TOOLINFO STRUCTURE */
	ti.cbSize		= sizeof(TOOLINFO);
	ti.uFlags		= TTF_SUBCLASS | TTF_IDISHWND;
	ti.hwnd			= HwndDlg;
	ti.lpszText		= LPSTR_TEXTCALLBACK;
    
    /* SEND AN ADDTOOL MESSAGE TO THE TOOLTIP CONTROL WINDOW */
	for(i=0; i<sizeof Tooltips/sizeof Tooltips[0]; ++i) {
		ti.uId			= (WPARAM)GetDlgItem(HwndDlg, Tooltips[i].id);

		if (ti.uId)
			SendMessage(ToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&ti);
	}

my_debug.OutPut("call UpdateConfigs");

	UpdateConfigs(HwndDlg);

my_debug.OutPut("call UpdateDlgFromValue");

	UpdateDlgFromValue(HwndDlg);


	my_debug.OutPut("finished InitConfigDlg");


	return true;
}

bool AEncodeProperties::UpdateDlgFromValue(HWND HwndDlg)
{
	// get all the required strings
//	TCHAR Version[5];
//	LoadString(hDllInstance, IDS_STRING_VERSION, Version, 5);

	int i;

	// Check boxes if required
	::CheckDlgButton( HwndDlg, IDC_CHECK_CHECKSUM,     GetCRCMode()        ?BST_CHECKED:BST_UNCHECKED );
	::CheckDlgButton( HwndDlg, IDC_CHECK_ORIGINAL,     GetOriginalMode()   ?BST_CHECKED:BST_UNCHECKED );
	::CheckDlgButton( HwndDlg, IDC_CHECK_PRIVATE,      GetPrivateMode()    ?BST_CHECKED:BST_UNCHECKED );
	::CheckDlgButton( HwndDlg, IDC_CHECK_COPYRIGHT,    GetCopyrightMode()  ?BST_CHECKED:BST_UNCHECKED );
	::CheckDlgButton( HwndDlg, IDC_CHECK_ENC_SMART,    GetSmartOutputMode()?BST_CHECKED:BST_UNCHECKED );
	::CheckDlgButton( HwndDlg, IDC_CHECK_ENC_ABR,      GetAbrOutputMode()  ?BST_CHECKED:BST_UNCHECKED );
//	::CheckDlgButton( HwndDlg, IDC_CHECK_RESERVOIR,    !GetNoBiResMode() ?BST_CHECKED:BST_UNCHECKED );
//	::CheckDlgButton( HwndDlg, IDC_CHECK_XINGVBR,      GetXingFrameMode()?BST_CHECKED:BST_UNCHECKED );
//	::CheckDlgButton( HwndDlg, IDC_CHECK_RESAMPLE,     GetResampleMode() ?BST_CHECKED:BST_UNCHECKED );
//	::CheckDlgButton( HwndDlg, IDC_CHECK_CHANNELFORCE, bForceChannel     ?BST_CHECKED:BST_UNCHECKED );
	
	// Add required channel modes
	for (i=0;i<GetChannelLentgh();i++)
	{
		if (i == nChannelIndex)
		{
			SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_ENC_STEREO), CB_SETCURSEL, i, NULL);
			break;
		}
	}

	// Add VBR Quality
	SendMessage(GetDlgItem( HwndDlg, IDC_SLIDER_AVERAGE_MIN), TBM_SETPOS, TRUE, AverageBitrate_Min);
	SendMessage(GetDlgItem( HwndDlg, IDC_SLIDER_AVERAGE_MAX), TBM_SETPOS, TRUE, AverageBitrate_Max);
	SendMessage(GetDlgItem( HwndDlg, IDC_SLIDER_AVERAGE_STEP), TBM_SETPOS, TRUE, AverageBitrate_Step);
	SendMessage(GetDlgItem( HwndDlg, IDC_SLIDER_AVERAGE_SAMPLE), TBM_SETPOS, TRUE, AverageBitrate_Max);

	UpdateDlgFromSlides(HwndDlg);

	EnableAbrOptions(HwndDlg, GetAbrOutputMode());
//	UpdateAbrSteps(AverageBitrate_Min, AverageBitrate_Max, AverageBitrate_Step);
	// Add all possible re-sampling freq
/*	SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_SAMPLEFREQ), CB_SETCURSEL, nSamplingFreqIndex, NULL);
	

	// Add required bitrates
	for (i=0;i<GetBitrateLentgh();i++)
	{
		if (i == nMinBitrateIndex)
		{
			SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_BITRATE), CB_SETCURSEL, i, NULL);
			break;
		}
	}

	// Add bitrates to the VBR combo box too
	for (i=0;i<GetBitrateLentgh();i++)
	{
		if (i == nMaxBitrateIndex)
		{
			SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_MAXBITRATE), CB_SETCURSEL, i, NULL);
			break;
		}
	}

//	SendMessage(GetDlgItem( HwndDlg, IDC_SLIDER_QUALITY), TBM_SETRANGE, TRUE, MAKELONG(0,9));

	char tmp[3];
	wsprintf(tmp,"%d",VbrQuality);
	SetWindowText(GetDlgItem( HwndDlg, IDC_CONFIG_QUALITY), tmp);
	SendMessage(GetDlgItem( HwndDlg, IDC_SLIDER_QUALITY), TBM_SETPOS, TRUE, VbrQuality);
	
	wsprintf(tmp,"%d",AverageBitrate);
	SetWindowText(GetDlgItem( HwndDlg, IDC_EDIT_AVERAGE), tmp);
	
	// Display VBR settings if needed
	AEncodeProperties::DisplayVbrOptions(HwndDlg, mBRmode);

	// Display Resample settings if needed
	if (GetResampleMode())
	{
		::EnableWindow(::GetDlgItem(HwndDlg,IDC_COMBO_SAMPLEFREQ), TRUE);
	}
	else
	{
		::EnableWindow(::GetDlgItem(HwndDlg,IDC_COMBO_SAMPLEFREQ), FALSE);
	}


	// Add presets
	for (i=0;i<GetPresetLentgh();i++)
	{
		if (i == nPresetIndex)
		{
			SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_PRESET), CB_SETCURSEL, i, NULL);
			break;
		}
	}

	// Add User configs
//	SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_SETTINGS), CB_RESETCONTENT , NULL, NULL);
	::SetWindowText(::GetDlgItem( HwndDlg, IDC_EDIT_OUTPUTDIR), OutputDir.c_str());
*/
	/**
		\todo Select the right saved config
	*/

	return true;
}

bool AEncodeProperties::UpdateValueFromDlg(HWND HwndDlg)
{
	nChannelIndex      = SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_ENC_STEREO),   CB_GETCURSEL, NULL, NULL);
//	nMinBitrateIndex   = SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_BITRATE),    CB_GETCURSEL, NULL, NULL);
//	nMaxBitrateIndex   = SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_MAXBITRATE), CB_GETCURSEL, NULL, NULL);
//	nPresetIndex       = SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_PRESET),     CB_GETCURSEL, NULL, NULL);
//	VbrQuality         = SendMessage(GetDlgItem( HwndDlg, IDC_SLIDER_QUALITY), TBM_GETPOS , NULL, NULL);
//	nSamplingFreqIndex = SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_SAMPLEFREQ), CB_GETCURSEL, NULL, NULL);

	bCRC          = (::IsDlgButtonChecked( HwndDlg, IDC_CHECK_CHECKSUM)     == BST_CHECKED);
	bCopyright    = (::IsDlgButtonChecked( HwndDlg, IDC_CHECK_COPYRIGHT)    == BST_CHECKED);
	bOriginal     = (::IsDlgButtonChecked( HwndDlg, IDC_CHECK_ORIGINAL)     == BST_CHECKED);
	bPrivate      = (::IsDlgButtonChecked( HwndDlg, IDC_CHECK_PRIVATE)      == BST_CHECKED);
	bSmartOutput  = (::IsDlgButtonChecked( HwndDlg, IDC_CHECK_ENC_SMART)    == BST_CHECKED);
	bAbrOutput    = (::IsDlgButtonChecked( HwndDlg, IDC_CHECK_ENC_ABR)      == BST_CHECKED);
//	bNoBitRes     =!(::IsDlgButtonChecked( HwndDlg, IDC_CHECK_RESERVOIR)    == BST_CHECKED);
//	bXingFrame    = (::IsDlgButtonChecked( HwndDlg, IDC_CHECK_XINGVBR)      == BST_CHECKED);
//	bResample     = (::IsDlgButtonChecked( HwndDlg, IDC_CHECK_RESAMPLE)     == BST_CHECKED);
//	bForceChannel = (::IsDlgButtonChecked( HwndDlg, IDC_CHECK_CHANNELFORCE) == BST_CHECKED);

	AverageBitrate_Min  = SendMessage(GetDlgItem( HwndDlg, IDC_SLIDER_AVERAGE_MIN), TBM_GETPOS , NULL, NULL);
	AverageBitrate_Max  = SendMessage(GetDlgItem( HwndDlg, IDC_SLIDER_AVERAGE_MAX), TBM_GETPOS , NULL, NULL);
	AverageBitrate_Step = SendMessage(GetDlgItem( HwndDlg, IDC_SLIDER_AVERAGE_STEP), TBM_GETPOS , NULL, NULL);

	EnableAbrOptions(HwndDlg, bAbrOutput);

my_debug.OutPut("nChannelIndex %d, bCRC %d, bCopyright %d, bOriginal %d, bPrivate %d",nChannelIndex, bCRC, bCopyright, bOriginal, bPrivate);

/*	char tmpPath[MAX_PATH];
	::GetWindowText( ::GetDlgItem( HwndDlg, IDC_EDIT_OUTPUTDIR), tmpPath, MAX_PATH);
	OutputDir = tmpPath;

	if (::IsDlgButtonChecked(HwndDlg, IDC_RADIO_BITRATE_CBR) == BST_CHECKED)
		mBRmode = BR_CBR;
	else if (::IsDlgButtonChecked(HwndDlg, IDC_RADIO_BITRATE_VBR) == BST_CHECKED)
		mBRmode = BR_VBR;
	else
		mBRmode = BR_ABR;
	
	::GetWindowText( ::GetDlgItem( HwndDlg, IDC_EDIT_AVERAGE), tmpPath, MAX_PATH);
	AverageBitrate = atoi(tmpPath);
	if (AverageBitrate < 8)
		AverageBitrate = 8;
	if (AverageBitrate > 320)
		AverageBitrate = 320;
*/
	return true;
}
/*
VBRMETHOD AEncodeProperties::GetVBRValue(DWORD & MaxBitrate, int & Quality, DWORD & AbrBitrate, BOOL & VBRHeader, const DWORD MPEG_Version) const
{
	assert((MPEG_Version == MPEG1) || (MPEG_Version == MPEG2));
	assert(nMaxBitrateIndex < sizeof(the_Bitrates));

	if (mBRmode == BR_VBR)
	{
		MaxBitrate = the_Bitrates[nMaxBitrateIndex];

		if (MPEG_Version == MPEG1)
			MaxBitrate = MaxBitrate>the_MPEG1_Bitrates[sizeof(the_MPEG1_Bitrates)/sizeof(unsigned int)-1]?MaxBitrate:the_MPEG1_Bitrates[sizeof(the_MPEG1_Bitrates)/sizeof(unsigned int)-1];
		else
			MaxBitrate = MaxBitrate<the_MPEG2_Bitrates[0]?MaxBitrate:the_MPEG2_Bitrates[0];

		VBRHeader = bXingFrame;
		Quality = VbrQuality;
		AbrBitrate = 0;

		return VBR_METHOD_DEFAULT; // for the moment
	} 
	else if (mBRmode == BR_ABR)
	{
		MaxBitrate = the_Bitrates[nMaxBitrateIndex];

		if (MPEG_Version == MPEG1)
			MaxBitrate = MaxBitrate>the_MPEG1_Bitrates[sizeof(the_MPEG1_Bitrates)/sizeof(unsigned int)-1]?MaxBitrate:the_MPEG1_Bitrates[sizeof(the_MPEG1_Bitrates)/sizeof(unsigned int)-1];
		else
			MaxBitrate = MaxBitrate<the_MPEG2_Bitrates[0]?MaxBitrate:the_MPEG2_Bitrates[0];

		VBRHeader = bXingFrame;
		Quality = 0;
		AbrBitrate = AverageBitrate*1000;
		return VBR_METHOD_ABR;
	}
	else
	{
		return VBR_METHOD_NONE;
	}
}
*/
void AEncodeProperties::ParamsRestore()
{
	// use these default parameters in case one is not found
	bCopyright    = true;
	bCRC          = true;
	bOriginal     = true;
	bPrivate      = true;
	bNoBitRes     = false; // enable bit reservoir
	bXingFrame    = true;
	bResample     = false;
	bForceChannel = false;
	bSmartOutput  = true;
	bAbrOutput    = true;
	
	AverageBitrate_Min = 80; // a bit lame
	AverageBitrate_Max = 160; // a bit lame
	AverageBitrate_Step = 8; // a bit lame
	SmartRatioMax = 15.0;

	nChannelIndex = 2; // joint-stereo
	mBRmode       = BR_CBR;
	nMinBitrateIndex = 6; // 128 kbps (works for both MPEGI and II)
	nMaxBitrateIndex = 4; // 160 kbps (works for both MPEGI and II)
	nPresetIndex = 0; // None
	VbrQuality = 1; // Quite High
//	AverageBitrate = 128; // a bit lame
	nSamplingFreqIndex = 1; // 44100

//	OutputDir = "c:\\";

//	DllLocation = "plugins\\lame_enc.dll";

	// get the values from the saved file if possible
	if (my_stored_data.LoadFile(my_store_location))
	{
		TiXmlNode* node;

		node = my_stored_data.FirstChild("lame_acm");

		TiXmlElement* CurrentNode = node->FirstChildElement("encodings");

		std::string CurrentConfig = "";

		if (CurrentNode->Attribute("default") != NULL)
		{
			CurrentConfig = *CurrentNode->Attribute("default");
		}

/*		// output parameters
		TiXmlElement* iterateElmt = node->FirstChildElement("DLL");
		if (iterateElmt != NULL)
		{
			const std::string * tmpname = iterateElmt->Attribute("location");
			if (tmpname != NULL)
			{
				DllLocation = *tmpname;
			}
		}
*/
		GetValuesFromKey(CurrentConfig, *CurrentNode);
	}
	else
	{
		/**
			\todo save the data in the file !
		*/
	}
}

void AEncodeProperties::ParamsSave()
{
/*


	save the current parameters in the corresponding subkey
	



	HKEY OssKey;

	if (RegCreateKeyEx ( HKEY_LOCAL_MACHINE, "SOFTWARE\\MUKOLI\\out_lame", 0, "", REG_OPTION_NON_VOLATILE, KEY_WRITE , NULL, &OssKey, NULL ) == ERROR_SUCCESS) {

		if (RegSetValueEx(OssKey, "DLL Location", 0, REG_EXPAND_SZ, (CONST BYTE *)DllLocation, strlen(DllLocation)+1 ) != ERROR_SUCCESS)
			return;
		
		RegCloseKey(OssKey); 
	}
*/
}
/*
void AEncodeProperties::DisplayVbrOptions(const HWND hDialog, const BRMode the_mode)
{
	bool bVBR = false;
	bool bABR = false;

	switch ( the_mode )
	{
		case BR_CBR:
			::CheckRadioButton(hDialog, IDC_RADIO_BITRATE_CBR, IDC_RADIO_BITRATE_ABR, IDC_RADIO_BITRATE_CBR);
			break;
		case BR_VBR:
			::CheckRadioButton(hDialog, IDC_RADIO_BITRATE_CBR, IDC_RADIO_BITRATE_ABR, IDC_RADIO_BITRATE_VBR);
			bVBR = true;
			break;
		case BR_ABR:
			::CheckRadioButton(hDialog, IDC_RADIO_BITRATE_CBR, IDC_RADIO_BITRATE_ABR, IDC_RADIO_BITRATE_ABR);
			bABR = true;
			break;

	}

	if(bVBR|bABR)
	{
		::SetWindowText(::GetDlgItem(hDialog,IDC_STATIC_MINBITRATE), "Min Bitrate");
	}
	else
	{
		::SetWindowText(::GetDlgItem(hDialog,IDC_STATIC_MINBITRATE), "Bitrate");
	}

	::EnableWindow(::GetDlgItem( hDialog, IDC_CHECK_XINGVBR), bVBR|bABR);

	::EnableWindow(::GetDlgItem( hDialog, IDC_COMBO_MAXBITRATE), bVBR|bABR);

	::EnableWindow(::GetDlgItem( hDialog, IDC_STATIC_MAXBITRATE), bVBR|bABR);

	::EnableWindow(::GetDlgItem( hDialog, IDC_SLIDER_QUALITY), bVBR);

	::EnableWindow(::GetDlgItem( hDialog, IDC_CONFIG_QUALITY), bVBR);

	::EnableWindow(::GetDlgItem( hDialog, IDC_STATIC_VBRQUALITY), bVBR);

	::EnableWindow(::GetDlgItem( hDialog, IDC_STATIC_VBRQUALITY_LOW), bVBR);

	::EnableWindow(::GetDlgItem( hDialog, IDC_STATIC_VBRQUALITY_HIGH), bVBR);

	::EnableWindow(::GetDlgItem( hDialog, IDC_STATIC_ABR), bABR);

	::EnableWindow(::GetDlgItem( hDialog, IDC_EDIT_AVERAGE), bABR);
}
*/
AEncodeProperties::AEncodeProperties(HMODULE hModule)
 :my_debug(ADbg(DEBUG_LEVEL_CREATION)),
 my_hModule(hModule)
{
	std::string path = "";
//	HMODULE htmp = LoadLibrary("out_lame.dll");
	if (hModule != NULL)
	{
		char output[MAX_PATH];
		::GetModuleFileName(hModule, output, MAX_PATH);
//		::FreeLibrary(htmp);

		path = output;
	}
	my_store_location = path.substr(0,path.find_last_of('\\')+1);
	my_store_location += "lame_acm.xml";

	my_debug.OutPut("store path = %s",my_store_location.c_str());
//#ifdef OLD
//	::OutputDebugString(my_store_location.c_str());

	// make sure the XML file is present
	HANDLE hFile = ::CreateFile(my_store_location.c_str(), 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL );
	::CloseHandle(hFile);
//#endif // OLD
	my_debug.OutPut("AEncodeProperties creation completed (0x%08X)",this);
}

// Save the values to the right XML saved config
void AEncodeProperties::SaveValuesToStringKey(const std::string & config_name)
{
	// get the current data in the file to keep them
	if (my_stored_data.LoadFile(my_store_location))
	{
		// check if the Node corresponding to the config_name already exist.
		TiXmlNode* node = my_stored_data.FirstChild("lame_acm");

		if (node != NULL)
		{
			TiXmlElement* ConfigNode = node->FirstChildElement("encodings");

			if (ConfigNode != NULL)
			{
				// look all the <config> tags
				TiXmlElement* tmpNode = ConfigNode->FirstChildElement("config");
				while (tmpNode != NULL)
				{
					const std::string * tmpname = tmpNode->Attribute("name");
					if (tmpname->compare(config_name) == 0)
					{
						break;
					}
					tmpNode = tmpNode->NextSiblingElement("config");
				}

				if (tmpNode == NULL)
				{
					// Create the node
					tmpNode = new TiXmlElement("config");
					tmpNode->SetAttribute("name",config_name);

					// save data in the node
					SaveValuesToElement(tmpNode);

					ConfigNode->InsertEndChild(*tmpNode);
				}
				else
				{
					// save data in the node
					SaveValuesToElement(tmpNode);
				}


				// and save the file
				my_stored_data.SaveFile(my_store_location);
			}
		}
	}
}

void AEncodeProperties::GetValuesFromKey(const std::string & config_name, const TiXmlNode & parentNode)
{
	TiXmlElement* tmpElt;
	TiXmlElement* iterateElmt;

	// find the config that correspond to CurrentConfig
	iterateElmt = parentNode.FirstChildElement("config");
	while (iterateElmt != NULL)
	{
		const std::string * tmpname = iterateElmt->Attribute("name");
		if ((tmpname != NULL) && (tmpname->compare(config_name) == 0))
		{
			break;
		}
		iterateElmt = iterateElmt->NextSiblingElement("config");
	}

	if (iterateElmt != NULL)
	{
		// get all the parameters saved in this Element
		const std::string * tmpname;

		// Smart output parameter
		tmpElt = iterateElmt->FirstChildElement("Smart");
		if (tmpElt != NULL)
		{
			tmpname = tmpElt->Attribute("use");
			if (tmpname != NULL)
				bSmartOutput = (tmpname->compare("true") == 0);
			
			tmpname = tmpElt->Attribute("ratio");
			if (tmpname != NULL)
				SmartRatioMax = atof(tmpname->c_str());
		}

		// Smart output parameter
		tmpElt = iterateElmt->FirstChildElement("ABR");
		if (tmpElt != NULL)
		{
			tmpname = tmpElt->Attribute("use");
			if (tmpname != NULL)
				bAbrOutput = (tmpname->compare("true") == 0);
			
			tmpname = tmpElt->Attribute("min");
			if (tmpname != NULL)
				AverageBitrate_Min = atoi(tmpname->c_str());

			tmpname = tmpElt->Attribute("max");
			if (tmpname != NULL)
				AverageBitrate_Max = atoi(tmpname->c_str());

			tmpname = tmpElt->Attribute("step");
			if (tmpname != NULL)
				AverageBitrate_Step = atoi(tmpname->c_str());
		}

		// Copyright parameter
		tmpElt = iterateElmt->FirstChildElement("Copyright");
		if (tmpElt != NULL)
		{
			tmpname = tmpElt->Attribute("use");
			if (tmpname != NULL)
				bCopyright = (tmpname->compare("true") == 0);
		}

		// Copyright parameter
		tmpElt = iterateElmt->FirstChildElement("CRC");
		if (tmpElt != NULL)
		{
			tmpname = tmpElt->Attribute("use");
			if (tmpname != NULL)
				bCRC = (tmpname->compare("true") == 0);
		}

		// Copyright parameter
		tmpElt = iterateElmt->FirstChildElement("Original");
		if (tmpElt != NULL)
		{
			tmpname = tmpElt->Attribute("use");
			if (tmpname != NULL)
				bOriginal = (tmpname->compare("true") == 0);
		}

		// Copyright parameter
		tmpElt = iterateElmt->FirstChildElement("Private");
		if (tmpElt != NULL)
		{
			tmpname = tmpElt->Attribute("use");
			if (tmpname != NULL)
				bPrivate = (tmpname->compare("true") == 0);
		}
/*
		// Copyright parameter
		tmpElt = iterateElmt->FirstChildElement("Bit_reservoir");
		if (tmpElt != NULL)
		{
			tmpname = tmpElt->Attribute("use");
			if (tmpname != NULL)
				bNoBitRes = !(tmpname->compare("true") == 0);
		}

		// bitrates
		tmpElt = iterateElmt->FirstChildElement("bitrate");
		tmpname = tmpElt->Attribute("min");
		if (tmpname != NULL)
		{
			unsigned int uitmp = atoi(tmpname->c_str());
			for (int i=0;i<sizeof(the_Bitrates)/sizeof(unsigned int);i++)
			{
				if (the_Bitrates[i] == uitmp)
				{
					nMinBitrateIndex = i;
					break;
				}
			}
		}

		tmpname = tmpElt->Attribute("max");
		if (tmpname != NULL)
		{
			unsigned int uitmp = atoi(tmpname->c_str());
			for (int i=0;i<sizeof(the_Bitrates)/sizeof(unsigned int);i++)
			{
				if (the_Bitrates[i] == uitmp)
				{
					nMaxBitrateIndex = i;
					break;
				}
			}
		}
*/
/*
		// resampling parameters
		tmpElt = iterateElmt->FirstChildElement("resampling");
		if (tmpElt != NULL)
		{
			tmpname = tmpElt->Attribute("use");
			if (tmpname != NULL)
				bResample = (tmpname->compare("true") == 0);

			unsigned int uitmp = atoi(tmpElt->Attribute("freq")->c_str());
			for (int i=0;i<sizeof(the_SamplingFreqs)/sizeof(unsigned int);i++)
			{
				if (the_SamplingFreqs[i] == uitmp)
				{
					nSamplingFreqIndex = i;
					break;
				}
			}
		}

		// VBR parameters
		tmpElt = iterateElmt->FirstChildElement("VBR");
		if (tmpElt != NULL)
		{
			tmpname = tmpElt->Attribute("use");
			if (tmpname != NULL)
			{
				if (tmpname->compare("ABR") == 0)
					mBRmode = BR_ABR;
				else if (tmpname->compare("true") == 0)
					mBRmode = BR_VBR;
				else
					mBRmode = BR_CBR;
			}

			tmpname = tmpElt->Attribute("header");
			if (tmpname != NULL)
				bXingFrame = (tmpname->compare("true") == 0);

			tmpname = tmpElt->Attribute("quality");
			if (tmpname != NULL)
			{
				VbrQuality = atoi(tmpname->c_str());
			}

			tmpname = tmpElt->Attribute("average");
			if (tmpname != NULL)
			{
				AverageBitrate = atoi(tmpname->c_str());
			}
			else
			{
			}
		}

		// output parameters
		tmpElt = iterateElmt->FirstChildElement("output");
		if (tmpElt != NULL)
		{
			OutputDir = *tmpElt->Attribute("path");
		}
*/
//#ifdef OLD
		// Channel mode parameter
		tmpElt = iterateElmt->FirstChildElement("Channel");
		if (tmpElt != NULL)
		{
			const std::string * tmpStr = tmpElt->Attribute("mode");
			if (tmpStr != NULL)
			{
				for (int i=0;i<GetChannelLentgh();i++)
				{
					if (tmpStr->compare(GetChannelModeString(i)) == 0)
					{
						nChannelIndex = i;
						break;
					}
				}
			}
/*
			tmpname = tmpElt->Attribute("force");
			if (tmpname != NULL)
				bForceChannel = (tmpname->compare("true") == 0);
*/
		}
//#endif // OLD

		// Preset parameter
/*
		tmpElt = iterateElmt->FirstChildElement("Preset");
		if (tmpElt != NULL)
		{
			const std::string * tmpStr = tmpElt->Attribute("type");
			for (int i=0;i<GetPresetLentgh();i++)
			{
				if (tmpStr->compare(GetPresetModeString(i)) == 0)
				{
					nPresetIndex = i;
					break;
				}
			}

		}
*/
	}
}

/**
	\todo save the parameters
* /
void AEncodeProperties::SaveParams(const HWND hParentWnd)
{
	char string[MAX_PATH];
/*	int nIdx = SendMessage(::GetDlgItem( hParentWnd ,IDC_COMBO_SETTINGS ), CB_GETCURSEL, NULL, NULL);
	::SendMessage(::GetDlgItem( hParentWnd ,IDC_COMBO_SETTINGS ), CB_GETLBTEXT , nIdx, (LPARAM) string);
* /
}*/

bool AEncodeProperties::operator !=(const AEncodeProperties & the_instance) const
{
/*
	::OutputDebugString(bCopyright != the_instance.bCopyright?"1":"-");
	::OutputDebugString(bCRC != the_instance.bCRC            ?"2":"-");
	::OutputDebugString(bOriginal != the_instance.bOriginal  ?"3":"-");
	::OutputDebugString(bPrivate != the_instance.bPrivate    ?"4":"-");
	::OutputDebugString(bNoBitRes != the_instance.bNoBitRes  ?"5":"-");
	::OutputDebugString(mBRmode != the_instance.mBRmode      ?"6":"-");
	::OutputDebugString(bXingFrame != the_instance.bXingFrame?"7":"-");
	::OutputDebugString(bForceChannel != the_instance.bForceChannel?"8":"-");
	::OutputDebugString(bResample != the_instance.bResample  ?"9":"-");
	::OutputDebugString(nChannelIndex != the_instance.nChannelIndex?"10":"-");
	::OutputDebugString(nMinBitrateIndex != the_instance.nMinBitrateIndex?"11":"-");
	::OutputDebugString(nMaxBitrateIndex != the_instance.nMaxBitrateIndex?"12":"-");
	::OutputDebugString(nPresetIndex != the_instance.nPresetIndex?"13":"-");
	::OutputDebugString(VbrQuality != the_instance.VbrQuality?"14":"-");
	::OutputDebugString(AverageBitrate != the_instance.AverageBitrate?"15":"-");
	::OutputDebugString(nSamplingFreqIndex != the_instance.nSamplingFreqIndex?"16":"-");
	::OutputDebugString(OutputDir.compare(the_instance.OutputDir) != 0?"17":"-");

	std::string tmp = "";
	char tmpI[10];
	_itoa(AverageBitrate,tmpI,10);
	tmp += tmpI;
	tmp += " != ";
	_itoa(the_instance.AverageBitrate,tmpI,10);
	tmp += tmpI;
	::OutputDebugString(tmp.c_str());
*/
	return ((bCopyright != the_instance.bCopyright)
		 || (bCRC != the_instance.bCRC)
		 || (bOriginal != the_instance.bOriginal)
		 || (bPrivate != the_instance.bPrivate)
		 || (bSmartOutput != the_instance.bSmartOutput)
		 || (SmartRatioMax != the_instance.SmartRatioMax)
		 || (bAbrOutput != the_instance.bAbrOutput)
		 || (AverageBitrate_Min != the_instance.AverageBitrate_Min)
		 || (AverageBitrate_Max != the_instance.AverageBitrate_Max)
		 || (AverageBitrate_Step != the_instance.AverageBitrate_Step)
		 || (bNoBitRes != the_instance.bNoBitRes)
		 || (mBRmode != the_instance.mBRmode)
		 || (bXingFrame != the_instance.bXingFrame)
		 || (bForceChannel != the_instance.bForceChannel)
		 || (bResample != the_instance.bResample)
		 || (nChannelIndex != the_instance.nChannelIndex)
		 || (nMinBitrateIndex != the_instance.nMinBitrateIndex)
		 || (nMaxBitrateIndex != the_instance.nMaxBitrateIndex)
		 || (nPresetIndex != the_instance.nPresetIndex)
		 || (VbrQuality != the_instance.VbrQuality)
//		 || (AverageBitrate != the_instance.AverageBitrate)
		 || (nSamplingFreqIndex != the_instance.nSamplingFreqIndex)
//		 || (OutputDir.compare(the_instance.OutputDir) != 0)
		);
}

void AEncodeProperties::SelectSavedParams(const std::string the_string)
{
	// get the values from the saved file if possible
	if (my_stored_data.LoadFile(my_store_location))
	{
		TiXmlNode* node;

		node = my_stored_data.FirstChild("lame_acm");

		TiXmlElement* CurrentNode = node->FirstChildElement("encodings");

		if (CurrentNode != NULL)
		{
			CurrentNode->SetAttribute("default",the_string);
			GetValuesFromKey(the_string, *CurrentNode);
			my_stored_data.SaveFile(my_store_location);
		}
	}
}

inline void AEncodeProperties::SetAttributeBool(TiXmlElement * the_elt,const std::string & the_string, const bool the_value) const
{
	if (the_value == false)
		the_elt->SetAttribute(the_string, "false");
	else
		the_elt->SetAttribute(the_string, "true");
}

void AEncodeProperties::SaveValuesToElement(TiXmlElement * the_element) const
{
	// get all the parameters saved in this Element
	TiXmlElement * tmpElt;

	// Bit Reservoir parameter
/*
	tmpElt = the_element->FirstChildElement("Bit_reservoir");
	if (tmpElt == NULL)
	{
		tmpElt = new TiXmlElement("Bit_reservoir");
		SetAttributeBool(tmpElt, "use", !bNoBitRes);
		the_element->InsertEndChild(*tmpElt);
	}
	else
	{
		SetAttributeBool(tmpElt, "use", !bNoBitRes);
	}
*/
	// Copyright parameter
	tmpElt = the_element->FirstChildElement("Copyright");
	if (tmpElt == NULL)
	{
		tmpElt = new TiXmlElement("Copyright");
		SetAttributeBool( tmpElt, "use", bCopyright);
		the_element->InsertEndChild(*tmpElt);
	}
	else
	{
		SetAttributeBool( tmpElt, "use", bCopyright);
	}

	// Smart Output parameter
	tmpElt = the_element->FirstChildElement("Smart");
	if (tmpElt == NULL)
	{
		tmpElt = new TiXmlElement("Smart");
		SetAttributeBool( tmpElt, "use", bSmartOutput);
		tmpElt->SetAttribute("ratio", int(SmartRatioMax));
		the_element->InsertEndChild(*tmpElt);
	}
	else
	{
		SetAttributeBool( tmpElt, "use", bSmartOutput);
		tmpElt->SetAttribute("ratio", int(SmartRatioMax));
	}

	// Smart Output parameter
	tmpElt = the_element->FirstChildElement("ABR");
	if (tmpElt == NULL)
	{
		tmpElt = new TiXmlElement("ABR");
		SetAttributeBool( tmpElt, "use", bAbrOutput);
		tmpElt->SetAttribute("min", AverageBitrate_Min);
		tmpElt->SetAttribute("max", AverageBitrate_Max);
		tmpElt->SetAttribute("step", AverageBitrate_Step);
		the_element->InsertEndChild(*tmpElt);
	}
	else
	{
		SetAttributeBool( tmpElt, "use", bAbrOutput);
		tmpElt->SetAttribute("min", AverageBitrate_Min);
		tmpElt->SetAttribute("max", AverageBitrate_Max);
		tmpElt->SetAttribute("step", AverageBitrate_Step);
	}

	// CRC parameter
	tmpElt = the_element->FirstChildElement("CRC");
	if (tmpElt == NULL)
	{
		tmpElt = new TiXmlElement("CRC");
		SetAttributeBool( tmpElt, "use", bCRC);
		the_element->InsertEndChild(*tmpElt);
	}
	else
	{
		SetAttributeBool( tmpElt, "use", bCRC);
	}

	// Original parameter
	tmpElt = the_element->FirstChildElement("Original");
	if (tmpElt == NULL)
	{
		tmpElt = new TiXmlElement("Original");
		SetAttributeBool( tmpElt, "use", bOriginal);
		the_element->InsertEndChild(*tmpElt);
	}
	else
	{
		SetAttributeBool( tmpElt, "use", bOriginal);
	}

	// Private parameter
	tmpElt = the_element->FirstChildElement("Private");
	if (tmpElt == NULL)
	{
		tmpElt = new TiXmlElement("Private");
		SetAttributeBool( tmpElt, "use", bPrivate);
		the_element->InsertEndChild(*tmpElt);
	}
	else
	{
		SetAttributeBool( tmpElt, "use", bPrivate);
	}

	// Channel Mode parameter
	tmpElt = the_element->FirstChildElement("Channel");
	if (tmpElt == NULL)
	{
		tmpElt = new TiXmlElement("Channel");
		tmpElt->SetAttribute("mode", GetChannelModeString(nChannelIndex));
//		SetAttributeBool( tmpElt, "force", bForceChannel);
		the_element->InsertEndChild(*tmpElt);
	}
	else
	{
		tmpElt->SetAttribute("mode", GetChannelModeString(nChannelIndex));
//		SetAttributeBool( tmpElt, "force", bForceChannel);
	}
/*
	// Preset parameter
	tmpElt = the_element->FirstChildElement("Preset");
	if (tmpElt == NULL)
	{
		tmpElt = new TiXmlElement("Preset");
		tmpElt->SetAttribute("type", GetPresetModeString(nPresetIndex));
		the_element->InsertEndChild(*tmpElt);
	}
	else
	{
		tmpElt->SetAttribute("type", GetPresetModeString(nPresetIndex));
	}

	// Bitrate parameter
	tmpElt = the_element->FirstChildElement("bitrate");
	if (tmpElt == NULL)
	{
		tmpElt = new TiXmlElement("bitrate");
		tmpElt->SetAttribute("min", the_Bitrates[nMinBitrateIndex]);
		tmpElt->SetAttribute("max", the_Bitrates[nMaxBitrateIndex]);
		the_element->InsertEndChild(*tmpElt);
	}
	else
	{
		tmpElt->SetAttribute("min", the_Bitrates[nMinBitrateIndex]);
		tmpElt->SetAttribute("max", the_Bitrates[nMaxBitrateIndex]);
	}

	// Output Directory parameter
	tmpElt = the_element->FirstChildElement("output");
	if (tmpElt == NULL)
	{
		tmpElt = new TiXmlElement("output");
		tmpElt->SetAttribute("path", OutputDir);
		the_element->InsertEndChild(*tmpElt);
	}
	else
	{
		tmpElt->SetAttribute("path", OutputDir);
	}
*/
/*
	// Resampling parameter
	tmpElt = the_element->FirstChildElement("resampling");
	if (tmpElt == NULL)
	{
		tmpElt = new TiXmlElement("resampling");
		SetAttributeBool( tmpElt, "use", bResample);
		tmpElt->SetAttribute("freq", the_SamplingFreqs[nSamplingFreqIndex]);
		the_element->InsertEndChild(*tmpElt);
	}
	else
	{
		SetAttributeBool( tmpElt, "use", bResample);
		tmpElt->SetAttribute("freq", the_SamplingFreqs[nSamplingFreqIndex]);
	}

	// VBR parameter
	tmpElt = the_element->FirstChildElement("VBR");
	if (tmpElt == NULL)
	{
		tmpElt = new TiXmlElement("VBR");
		
		if (mBRmode == BR_ABR)
			tmpElt->SetAttribute("use", "ABR");
		else
			SetAttributeBool( tmpElt, "use", (mBRmode != BR_CBR));

		SetAttributeBool( tmpElt, "header", bXingFrame);
		tmpElt->SetAttribute("quality", VbrQuality);
		tmpElt->SetAttribute("average", AverageBitrate);
		the_element->InsertEndChild(*tmpElt);
	}
	else
	{
		if (mBRmode == BR_ABR)
			tmpElt->SetAttribute("use", "ABR");
		else
			SetAttributeBool( tmpElt, "use", (mBRmode != BR_CBR));

		SetAttributeBool( tmpElt, "header", bXingFrame);
		tmpElt->SetAttribute("quality", VbrQuality);
		tmpElt->SetAttribute("average", AverageBitrate);
	}
*/
}

bool AEncodeProperties::HandleDialogCommand(const HWND parentWnd, const WPARAM wParam, const LPARAM lParam)
{
	UINT command;
	command = GET_WM_COMMAND_ID(wParam, lParam);

	switch (command)
	{
	case IDOK :
	{
		bool bShouldEnd = true;

		// save parameters
		char string[MAX_PATH];
//		::GetWindowText(::GetDlgItem( parentWnd, IDC_COMBO_SETTINGS), string, MAX_PATH);

		wsprintf(string,"Current"); // only the Current config is supported at the moment
		
		my_debug.OutPut("my_hModule = 0x%08X",my_hModule);
/*
		AEncodeProperties tmpDlgProps(my_hModule);
		AEncodeProperties tmpSavedProps(my_hModule);
//#ifdef OLD
		tmpDlgProps.UpdateValueFromDlg(parentWnd);
		tmpSavedProps.SelectSavedParams(string);
		tmpSavedProps.ParamsRestore();
		// check if the values from the DLG are the same as the one saved in the config file
		// if yes, just do nothing
/*
		if (tmpDlgProps != tmpSavedProps)
		{
			int save;

			if (strcmp(string,"Current") == 0)
			{
				// otherwise, prompt the user if he wants to overwrite the settings
				TCHAR tmpStr[250];
				::LoadString(AOut::GetInstance(),IDS_STRING_PROMPT_REPLACE_CURRENT,tmpStr,250);

				save = AOut::MyMessageBox( tmpStr, MB_OKCANCEL|MB_ICONQUESTION, parentWnd);
			}
			else
			{
				// otherwise, prompt the user if he wants to overwrite the settings
				TCHAR tmpStr[250];
				::LoadString(AOut::GetInstance(),IDS_STRING_PROMPT_REPLACE_SETING,tmpStr,250);
				TCHAR tmpDsp[500];
				wsprintf(tmpDsp,tmpStr,string);

				save = AOut::MyMessageBox( tmpDsp, MB_YESNOCANCEL|MB_ICONQUESTION, parentWnd);
			}

			if (save == IDCANCEL)
				bShouldEnd = false;
			else if (save == IDNO)
			{
				// save the values in 'current'
				UpdateValueFromDlg(parentWnd);
				SaveValuesToStringKey("Current");
				SelectSavedParams("Current");
			}
			else
			{
				// do so and save in XML
				UpdateValueFromDlg(parentWnd);
				SaveValuesToStringKey(string);
			}
		}
*/
//#endif // OLD
my_debug.OutPut("before : nChannelIndex %d, bCRC %d, bCopyright %d, bOriginal %d, bPrivate %d",nChannelIndex, bCRC, bCopyright, bOriginal, bPrivate);

my_debug.OutPut("call UpdateValueFromDlg");

		UpdateValueFromDlg(parentWnd);

my_debug.OutPut("call SaveValuesToStringKey");

		SaveValuesToStringKey("Current"); // only Current config is supported now

//		SaveParams(parentWnd);

//my_debug.OutPut("call SelectSavedParams");

//		SelectSavedParams(string);
//		UpdateDlgFromValue(parentWnd);

my_debug.OutPut("finished saving");

		if (bShouldEnd)
		{
			RemoveProp(parentWnd, "AEncodeProperties-Config");
		
			EndDialog(parentWnd, true);
		}
	}
	break;

	case IDCANCEL:
		RemoveProp(parentWnd, "AEncodeProperties-Config");
        EndDialog(parentWnd, false);
		break;

/*	case IDC_FIND_DLL:
	{
		OPENFILENAME file;
		char DllLocation[512];
		wsprintf(DllLocation,"%s",GetDllLocation());

		memset(&file, 0, sizeof(file));
		file.lStructSize = sizeof(file); 
		file.hwndOwner  = parentWnd;
		file.Flags = OFN_FILEMUSTEXIST | OFN_NODEREFERENCELINKS | OFN_ENABLEHOOK | OFN_EXPLORER ;
///				file.lpstrFile = AOut::the_AOut->DllLocation;
		file.lpstrFile = DllLocation;
		file.lpstrFilter = "Lame DLL (lame_enc.dll)\0LAME_ENC.DLL\0DLL (*.dll)\0*.DLL\0All (*.*)\0*.*\0";
		file.nFilterIndex = 1;
		file.nMaxFile  = sizeof(DllLocation);
		file.lpfnHook  = DLLFindCallback; // use to validate the DLL chosen

		GetOpenFileName(&file);

		SetDllLocation(DllLocation);
		// use this filename if necessary
	}
	break;
*/
/*	case IDC_BUTTON_OUTPUT:
	{
#ifndef SIMPLE_FOLDER
		BROWSEINFO info;
		memset(&info,0,sizeof(info));

		char FolderName[MAX_PATH];

		info.hwndOwner = parentWnd;
		info.pszDisplayName  = FolderName;
		info.lpfn = BrowseFolderCallbackroc;
		info.lParam = (LPARAM) this;

		// get the localised window title
		TCHAR output[250];
		::LoadString(AOut::GetInstance(),IDS_STRING_DIR_SELECT,output,250);
		info.lpszTitle = output;

#ifdef BIF_EDITBOX
		info.ulFlags |= BIF_EDITBOX;
#else // BIF_EDITBOX
		info.ulFlags |= 0x0010;
#endif // BIF_EDITBOX

#ifdef BIF_VALIDATE
		info.ulFlags |= BIF_VALIDATE;
#else // BIF_VALIDATE
		info.ulFlags |= 0x0020;
#endif // BIF_VALIDATE

#ifdef BIF_NEWDIALOGSTYLE
		info.ulFlags |= BIF_NEWDIALOGSTYLE;
#else // BIF_NEWDIALOGSTYLE
		info.ulFlags |= 0x0040;
#endif // BIF_NEWDIALOGSTYLE

		ITEMIDLIST *item = SHBrowseForFolder(&info);

    	if (item != NULL)
		{
			char tmpOutputDir[MAX_PATH];
			wsprintf(tmpOutputDir,"%s",GetOutputDirectory());

			SHGetPathFromIDList( item,tmpOutputDir );
			SetOutputDirectory( tmpOutputDir );
			::SetWindowText(GetDlgItem( parentWnd, IDC_EDIT_OUTPUTDIR), tmpOutputDir);
//					wsprintf(OutputDir,FolderName);
		}
#else // SIMPLE_FOLDER
		OPENFILENAME file;

		memset(&file, 0, sizeof(file));
		file.lStructSize = sizeof(file); 
		file.hwndOwner  = parentWnd;
		file.Flags = OFN_FILEMUSTEXIST | OFN_NODEREFERENCELINKS | OFN_ENABLEHOOK | OFN_EXPLORER ;
//				file.lpstrFile = GetDllLocation();
//				file.lpstrFile = GetOutputDirectory();
		file.lpstrInitialDir = GetOutputDirectory();
		file.lpstrFilter = "A Directory\0.*\0";
//				file.nFilterIndex = 1;
		file.nMaxFile  = MAX_PATH;
//				file.lpfnHook  = DLLFindCallback; // use to validate the DLL chosen
//				file.Flags = OFN_ENABLESIZING | OFN_NOREADONLYRETURN | OFN_HIDEREADONLY;
		file.Flags = OFN_NOREADONLYRETURN | OFN_HIDEREADONLY | OFN_EXPLORER;

		TCHAR output[250];
		::LoadString(AOut::GetInstance(),IDS_STRING_DIR_SELECT,output,250);
		file.lpstrTitle = output;

		GetSaveFileName(&file);
#endif // SIMPLE_FOLDER
	}
	break;
*/
		case IDC_CHECK_ENC_ABR:
			EnableAbrOptions(parentWnd, ::IsDlgButtonChecked( parentWnd, IDC_CHECK_ENC_ABR) == BST_CHECKED);
			break;
/*	case IDC_RADIO_BITRATE_CBR:
		AEncodeProperties::DisplayVbrOptions(parentWnd, AEncodeProperties::BR_CBR);
		break;

	case IDC_RADIO_BITRATE_VBR:
		AEncodeProperties::DisplayVbrOptions(parentWnd, AEncodeProperties::BR_VBR);
		break;

	case IDC_RADIO_BITRATE_ABR:
		AEncodeProperties::DisplayVbrOptions(parentWnd, AEncodeProperties::BR_ABR);
		break;

	case IDC_CHECK_RESAMPLE:
	{
		bool tmp_bResampleUsed = (::IsDlgButtonChecked( parentWnd, IDC_CHECK_RESAMPLE) == BST_CHECKED);
		if (tmp_bResampleUsed)
		{
			::EnableWindow(::GetDlgItem(parentWnd,IDC_COMBO_SAMPLEFREQ), TRUE);
		}
		else
		{
			::EnableWindow(::GetDlgItem(parentWnd,IDC_COMBO_SAMPLEFREQ), FALSE);
		}
	}
	break;
*/
/*	case IDC_COMBO_SETTINGS:
//				if (CBN_SELCHANGE == GET_WM_COMMAND_CMD(wParam, lParam))
		if (CBN_SELENDOK == GET_WM_COMMAND_CMD(wParam, lParam))
		{
			char string[MAX_PATH];
			int nIdx = SendMessage(HWND(lParam), CB_GETCURSEL, NULL, NULL);
			SendMessage(HWND(lParam), CB_GETLBTEXT , nIdx, (LPARAM) string);

			// get the info corresponding to the new selected item
			SelectSavedParams(string);
			UpdateDlgFromValue(parentWnd);
		}
		break;
*/
/*	case IDC_BUTTON_CONFIG_SAVE:
	{
		// save the data in the current config
		char string[MAX_PATH];
		::GetWindowText(::GetDlgItem( parentWnd, IDC_COMBO_SETTINGS), string, MAX_PATH);

		UpdateValueFromDlg(parentWnd);
		SaveValuesToStringKey(string);
		SelectSavedParams(string);
		UpdateConfigs(parentWnd);
		UpdateDlgFromValue(parentWnd);
	}
	break;

	case IDC_BUTTON_CONFIG_RENAME:
	{
		char string[MAX_PATH];
		::GetWindowText(::GetDlgItem( parentWnd, IDC_COMBO_SETTINGS), string, MAX_PATH);

		if (RenameCurrentTo(string))
		{
			// Update the names displayed
			UpdateConfigs(parentWnd);
		}

	}
	break;

	case IDC_BUTTON_CONFIG_DELETE:
	{
		char string[MAX_PATH];
		::GetWindowText(::GetDlgItem( parentWnd, IDC_COMBO_SETTINGS), string, MAX_PATH);
		
		if (DeleteConfig(string))
		{
			// Update the names displayed
			UpdateConfigs(parentWnd);
			UpdateDlgFromValue(parentWnd);
		}
	}
	break;
*/
	}
	
    return FALSE;
}

bool AEncodeProperties::RenameCurrentTo(const std::string & new_config_name)
{
	bool bResult = false;

	// display all the names of the saved configs
	// get the values from the saved file if possible
	if (my_stored_data.LoadFile(my_store_location))
	{
		TiXmlNode* node;

		node = my_stored_data.FirstChild("lame_acm");

		TiXmlElement* CurrentNode = node->FirstChildElement("encodings");

		if (CurrentNode->Attribute("default") != NULL)
		{
			std::string CurrentConfigName = *CurrentNode->Attribute("default");

			// no rename possible for Current
			if (CurrentConfigName == "")
			{
				bResult = true;
			}
			else if (CurrentConfigName != "Current")
			{
				// find the config that correspond to CurrentConfig
				TiXmlElement* iterateElmt = CurrentNode->FirstChildElement("config");
//				int Idx = 0;
				while (iterateElmt != NULL)
				{
					const std::string * tmpname = iterateElmt->Attribute("name");
					/**
						\todo support language names
					*/
					if (tmpname != NULL)
					{
						if (tmpname->compare(CurrentConfigName) == 0)
						{
							iterateElmt->SetAttribute("name",new_config_name);	
							bResult = true;
							break;
						}
					}
//					Idx++;
					iterateElmt = iterateElmt->NextSiblingElement("config");
				}
			}

			if (bResult)
			{
				CurrentNode->SetAttribute("default",new_config_name);

				my_stored_data.SaveFile(my_store_location);
			}
		}
	}

	return bResult;
}

bool AEncodeProperties::DeleteConfig(const std::string & config_name)
{
	bool bResult = false;

	if (config_name != "Current")
	{
		// display all the names of the saved configs
		// get the values from the saved file if possible
		if (my_stored_data.LoadFile(my_store_location))
		{
			TiXmlNode* node;

			node = my_stored_data.FirstChild("lame_acm");

			TiXmlElement* CurrentNode = node->FirstChildElement("encodings");

			TiXmlElement* iterateElmt = CurrentNode->FirstChildElement("config");
//			int Idx = 0;
			while (iterateElmt != NULL)
			{
				const std::string * tmpname = iterateElmt->Attribute("name");
				/**
					\todo support language names
				*/
				if (tmpname != NULL)
				{
					if (tmpname->compare(config_name) == 0)
					{
						CurrentNode->RemoveChild(iterateElmt);
						bResult = true;
						break;
					}
				}
//				Idx++;
				iterateElmt = iterateElmt->NextSiblingElement("config");
			}
		}

		if (bResult)
		{
			my_stored_data.SaveFile(my_store_location);

			// select a new default config : "Current"
			SelectSavedParams("Current");

		}
	}

	return bResult;
}

void AEncodeProperties::UpdateConfigs(const HWND HwndDlg)
{
	// Add User configs
//	SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_SETTINGS), CB_RESETCONTENT , NULL, NULL);

	// display all the names of the saved configs
	// get the values from the saved file if possible
	if (my_stored_data.LoadFile(my_store_location))
	{
		TiXmlNode* node;

		node = my_stored_data.FirstChild("lame_acm");

		TiXmlElement* CurrentNode = node->FirstChildElement("encodings");

		std::string CurrentConfig = "";

		if (CurrentNode->Attribute("default") != NULL)
		{
			CurrentConfig = *CurrentNode->Attribute("default");
		}

		TiXmlElement* iterateElmt;

my_debug.OutPut("are we here ?");

		// find the config that correspond to CurrentConfig
		iterateElmt = CurrentNode->FirstChildElement("config");
		int Idx = 0;
		while (iterateElmt != NULL)
		{
			const std::string * tmpname = iterateElmt->Attribute("name");
			/**
				\todo support language names
			*/
			if (tmpname != NULL)
			{
//				SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_SETTINGS), CB_ADDSTRING, NULL, (LPARAM) tmpname->c_str());
				if (tmpname->compare(CurrentConfig) == 0)
				{
//					SendMessage(GetDlgItem( HwndDlg, IDC_COMBO_SETTINGS), CB_SETCURSEL, Idx, NULL);
					SelectSavedParams(*tmpname);
					UpdateDlgFromValue(HwndDlg);
				}
			}
my_debug.OutPut("Idx = %d",Idx);

			Idx++;
			// only Current config supported now
//			iterateElmt = iterateElmt->NextSiblingElement("config");
			iterateElmt = NULL;
my_debug.OutPut("iterateElmt = 0x%08X",iterateElmt);

		}
	}
}
/*
void AEncodeProperties::UpdateAbrSteps(unsigned int min, unsigned int max, unsigned int step) const
{
}
*/
void AEncodeProperties::UpdateDlgFromSlides(HWND hwndDlg) const
{
	UINT value_min, value_max, value_step, value;
	char tmp[4];

	value_min = SendMessage(GetDlgItem( hwndDlg, IDC_SLIDER_AVERAGE_MIN), TBM_GETPOS, NULL, NULL);
	value_max = SendMessage(GetDlgItem( hwndDlg, IDC_SLIDER_AVERAGE_MAX), TBM_GETPOS, NULL, NULL);

	if (value_min>value_max)
	{
		SendMessage(GetDlgItem( hwndDlg, IDC_SLIDER_AVERAGE_MIN), TBM_SETPOS, TRUE, value_max);
		UpdateDlgFromSlides(hwndDlg);
		return;
	}

	if (value_max<value_min)
	{
		SendMessage(GetDlgItem( hwndDlg, IDC_SLIDER_AVERAGE_MAX), TBM_SETPOS, TRUE, value_min);
		UpdateDlgFromSlides(hwndDlg);
		return;
	}

	wsprintf(tmp,"%3d",value_min);
	::SetWindowText(GetDlgItem( hwndDlg, IDC_STATIC_AVERAGE_MIN_VALUE), tmp);
	
	SendMessage(GetDlgItem( hwndDlg, IDC_SLIDER_AVERAGE_SAMPLE), TBM_SETRANGEMIN, TRUE, value_min);

	wsprintf(tmp,"%3d",value_max);
	::SetWindowText(GetDlgItem( hwndDlg, IDC_STATIC_AVERAGE_MAX_VALUE), tmp);
	
	SendMessage(GetDlgItem( hwndDlg, IDC_SLIDER_AVERAGE_SAMPLE), TBM_SETRANGEMAX, TRUE, value_max);
	
	value_step = SendMessage(GetDlgItem( hwndDlg, IDC_SLIDER_AVERAGE_STEP), TBM_GETPOS, NULL, NULL);
	wsprintf(tmp,"%3d",value_step);
	::SetWindowText(GetDlgItem( hwndDlg, IDC_STATIC_AVERAGE_STEP_VALUE), tmp);

	SendMessage(GetDlgItem( hwndDlg, IDC_SLIDER_AVERAGE_SAMPLE), TBM_CLEARTICS, TRUE, 0);
	for(UINT i=value_max; i>=value_min;i-=value_step)
	{
		SendMessage(GetDlgItem( hwndDlg, IDC_SLIDER_AVERAGE_SAMPLE), TBM_SETTIC, 0, i);
	}
	SendMessage(GetDlgItem( hwndDlg, IDC_SLIDER_AVERAGE_SAMPLE), TBM_SETLINESIZE, 0, value_step);
	SendMessage(GetDlgItem( hwndDlg, IDC_SLIDER_AVERAGE_SAMPLE), TBM_SETPAGESIZE, 0, value_step);
	
	value = SendMessage(GetDlgItem( hwndDlg, IDC_SLIDER_AVERAGE_SAMPLE), TBM_GETPOS, NULL, NULL);
	wsprintf(tmp,"%3d",value);
	::SetWindowText(GetDlgItem( hwndDlg, IDC_STATIC_AVERAGE_SAMPLE_VALUE), tmp);
}

void AEncodeProperties::EnableAbrOptions(HWND hDialog, bool enable)
{
	::EnableWindow(::GetDlgItem( hDialog, IDC_SLIDER_AVERAGE_MIN), enable);
	::EnableWindow(::GetDlgItem( hDialog, IDC_SLIDER_AVERAGE_MAX), enable);
	::EnableWindow(::GetDlgItem( hDialog, IDC_SLIDER_AVERAGE_STEP), enable);
	::EnableWindow(::GetDlgItem( hDialog, IDC_SLIDER_AVERAGE_SAMPLE), enable);
	::EnableWindow(::GetDlgItem( hDialog, IDC_STATIC_AVERAGE_MIN), enable);
	::EnableWindow(::GetDlgItem( hDialog, IDC_STATIC_AVERAGE_MAX), enable);
	::EnableWindow(::GetDlgItem( hDialog, IDC_STATIC_AVERAGE_STEP), enable);
	::EnableWindow(::GetDlgItem( hDialog, IDC_STATIC_AVERAGE_SAMPLE), enable);
	::EnableWindow(::GetDlgItem( hDialog, IDC_STATIC_AVERAGE_MIN_VALUE), enable);
	::EnableWindow(::GetDlgItem( hDialog, IDC_STATIC_AVERAGE_MAX_VALUE), enable);
	::EnableWindow(::GetDlgItem( hDialog, IDC_STATIC_AVERAGE_STEP_VALUE), enable);
	::EnableWindow(::GetDlgItem( hDialog, IDC_STATIC_AVERAGE_SAMPLE_VALUE), enable);
}

