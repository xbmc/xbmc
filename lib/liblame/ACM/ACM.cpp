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
	\version \$Id: ACM.cpp,v 1.20.8.1 2008/11/01 20:41:47 robert Exp $
*/

#if !defined(STRICT)
#define STRICT
#endif // STRICT

#include <algorithm>

#include <windows.h>
#include <windowsx.h>
#include <intshcut.h>

#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>

#include <assert.h>

#include <lame.h>

#include "adebug.h"
#include "resource.h"
#include "ACMStream.h"

#ifdef ENABLE_DECODING
#include "DecodeStream.h"
#endif // ENABLE_DECODING

#include "ACM.h"

#ifndef IDC_HAND
#define IDC_HAND            MAKEINTRESOURCE(32649)
#endif // IDC_HAND

char ACM::VersionString[120];

const char ACM_VERSION[] = "0.9.2";

#ifdef WIN32
//
//  32-bit versions
//
#if (WINVER >= 0x0400)
 #define VERSION_ACM_DRIVER  MAKE_ACM_VERSION(4,  0, 0)
#else
#define VERSION_ACM_DRIVER  MAKE_ACM_VERSION(3, 51, 0)
#endif
#define VERSION_MSACM MAKE_ACM_VERSION(3, 50, 0)

#else
//
//  16-bit versions
//
#define VERSION_ACM_DRIVER MAKE_ACM_VERSION(1, 0, 0)
#define VERSION_MSACM MAKE_ACM_VERSION(2, 1, 0)

#endif

#define PERSONAL_FORMAT WAVE_FORMAT_MPEGLAYER3
#define SIZE_FORMAT_STRUCT sizeof(MPEGLAYER3WAVEFORMAT)
//#define SIZE_FORMAT_STRUCT 0

//static const char channel_mode[][13] = {"mono","stereo","joint stereo","dual channel"};
static const char channel_mode[][13] = {"mono","stereo"};
static const unsigned int mpeg1_freq[] = {48000,44100,32000};
static const unsigned int mpeg2_freq[] = {24000,22050,16000,12000,11025,8000};
static const unsigned int mpeg1_bitrate[] = {320, 256, 224, 192, 160, 128, 112, 96, 80, 64, 56, 48, 40, 32};
static const unsigned int mpeg2_bitrate[] = {160, 144, 128, 112,  96,  80,  64, 56, 48, 40, 32, 24, 16,  8};

#define SIZE_CHANNEL_MODE (sizeof(channel_mode)  / (sizeof(char) * 13))
#define SIZE_FREQ_MPEG1 (sizeof(mpeg1_freq)    / sizeof(unsigned int))
#define SIZE_FREQ_MPEG2 (sizeof(mpeg2_freq)    / sizeof(unsigned int))
#define SIZE_BITRATE_MPEG1 (sizeof(mpeg1_bitrate) / sizeof(unsigned int))
#define SIZE_BITRATE_MPEG2 (sizeof(mpeg2_bitrate) / sizeof(unsigned int))

static const int FORMAT_TAG_MAX_NB = 2; // PCM and PERSONAL (mandatory to have at least PCM and your format)
static const int FILTER_TAG_MAX_NB = 0; // this is a codec, not a filter

// number of supported PCM formats
static const int FORMAT_MAX_NB_PCM =
	2 *                                           // number of PCM channel mode (stereo/mono)
		(SIZE_FREQ_MPEG1 + // number of MPEG 1 sampling freq
		SIZE_FREQ_MPEG2); // number of MPEG 2 sampling freq

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
bool bitrate_item::operator<(const bitrate_item & other_bitrate) const
{
	return (other_bitrate.frequency < frequency ||
		    (other_bitrate.frequency == frequency &&
			 (other_bitrate.bitrate < bitrate ||
			  (other_bitrate.bitrate == bitrate &&
			   (other_bitrate.channels < channels)))));
}

//////////////////////////////////////////////////////////////////////
// Configuration Dialog
//////////////////////////////////////////////////////////////////////
/*
static CALLBACK ConfigProc(
  HWND hwndDlg,  // handle to dialog box
UINT uMsg,     // message
WPARAM wParam, // first message parameter
LPARAM lParam  // second message parameter
)
{
	BOOL bResult;

	switch (uMsg) {
		case WM_COMMAND:
			UINT command;
			command = GET_WM_COMMAND_ID(wParam, lParam);
            if (IDOK == command)
            {
                EndDialog(hwndDlg, (IDOK == command));
            } else if (IDCANCEL == command)
            {
                EndDialog(hwndDlg, (IDOK == command));
            }
            bResult = FALSE;
			break;
		default:
			bResult = FALSE; // will be treated by DefWindowProc
}
	return bResult;
}


inline DWORD ACM::Configure(HWND hParentWindow, LPDRVCONFIGINFO pConfig)
{
	my_debug.OutPut(DEBUG_LEVEL_FUNC_START, "ACM : Configure (Parent Window = 0x%08X)",hParentWindow);

	DialogBoxParam( my_hModule, MAKEINTRESOURCE(IDD_CONFIG), hParentWindow, ::ConfigProc , (LPARAM)this);

	return DRVCNF_OK; // Can also return
					// DRVCNF_CANCEL
					// and DRVCNF_RESTART
}
*/
//////////////////////////////////////////////////////////////////////
// About Dialog
//////////////////////////////////////////////////////////////////////

static BOOL CALLBACK AboutProc(
  HWND hwndDlg,  // handle to dialog box
UINT uMsg,     // message
WPARAM wParam, // first message parameter
LPARAM lParam  // second message parameter
)
{
	static HBRUSH hBrushStatic = NULL;
//	static LOGFONT lf;  // structure for font information  
//	static HFONT hfnt;
	static HCURSOR hcOverCursor = NULL;
	BOOL bResult;

	switch (uMsg) {
		case WM_INITDIALOG:
			char tmp[150];
			wsprintf(tmp,"LAME MP3 codec v%s", ACM::GetVersionString());
			::SetWindowText(GetDlgItem( hwndDlg, IDC_STATIC_ABOUT_TITLE), tmp);

/*
			::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf); 
			lf.lfUnderline = TRUE;

			hfnt = ::CreateFontIndirect(&lf);

			::SendMessage(::GetDlgItem(hwndDlg,IDC_STATIC_ABOUT_URL), WM_SETFONT, (WPARAM) hfnt, TRUE);
* /
			hBrushStatic = ::CreateSolidBrush(::GetSysColor (COLOR_BTNFACE));
*/			hcOverCursor = ::LoadCursor(NULL,(LPCTSTR)IDC_HAND); 
			if (hcOverCursor == NULL)
				hcOverCursor = ::LoadCursor(NULL,(LPCTSTR)IDC_CROSS); 

			bResult = TRUE;
			break;
/*
		case WM_CTLCOLORSTATIC:
			/// \todo only if there are URLs
			if ((HWND)lParam == ::GetDlgItem(hwndDlg,IDC_STATIC_ABOUT_URL))
			{
				::SetTextColor((HDC)wParam, ::GetSysColor (COLOR_HIGHLIGHT));
				::SetBkColor((HDC)wParam, ::GetSysColor (COLOR_BTNFACE));

				return (LRESULT) hBrushStatic;
			}
			else
				return (LRESULT) NULL;
*/
		case WM_MOUSEMOVE:
			{
				POINT pnt;
				::GetCursorPos(&pnt);

				RECT rect;
				::GetWindowRect( ::GetDlgItem(hwndDlg,IDC_STATIC_ABOUT_URL), &rect);

				if (  ::PtInRect(&rect,pnt)  )
				{
					::SetCursor(hcOverCursor);
				}


			}
			break;

		case WM_LBUTTONUP:
			{
				POINT pnt;
				::GetCursorPos(&pnt);

				RECT rect;
				::GetWindowRect( ::GetDlgItem(hwndDlg,IDC_STATIC_ABOUT_URL), &rect);

				TCHAR Url[200];
				bool bUrl = false;
				if (::PtInRect(&rect,pnt))
				{
					wsprintf(Url,get_lame_url());
					bUrl = true;
				}

				if (bUrl)
				{
					LPSTR tmpStr;
					HRESULT hresult = ::TranslateURL(Url, TRANSLATEURL_FL_GUESS_PROTOCOL|TRANSLATEURL_FL_GUESS_PROTOCOL, &tmpStr);
					if (hresult == S_OK)
						::ShellExecute(hwndDlg,"open",tmpStr,NULL,"",SW_SHOWMAXIMIZED );
					else if (hresult == S_FALSE)
						::ShellExecute(hwndDlg,"open",Url,NULL,"",SW_SHOWMAXIMIZED );
				}

			}
			break;

		case WM_COMMAND:
			UINT command;
			command = GET_WM_COMMAND_ID(wParam, lParam);
            if (IDOK == command)
            {
                EndDialog(hwndDlg, TRUE);
            }
            bResult = FALSE;
			break;

		case IDC_STATIC_ABOUT_URL:
			break;
		default:
			bResult = FALSE; // will be treated by DefWindowProc
}
	return bResult;
}

inline DWORD ACM::About(HWND hParentWindow)
{
	my_debug.OutPut(DEBUG_LEVEL_FUNC_START, "ACM : About (Parent Window = 0x%08X)",hParentWindow);

	DialogBoxParam( my_hModule, MAKEINTRESOURCE(IDD_ABOUT), hParentWindow, ::AboutProc , (LPARAM)this);

	return DRVCNF_OK; // Can also return
// DRVCNF_CANCEL
// and DRVCNF_RESTART
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ACM::ACM( HMODULE hModule )
 :my_hModule(hModule),
  my_hIcon(NULL),
  my_debug(ADbg(DEBUG_LEVEL_CREATION)),
  my_EncodingProperties(hModule)
{
	my_EncodingProperties.ParamsRestore();

	/// \todo get the debug level from the registry
	unsigned char DebugFileName[512];

	char tmp[128];
	wsprintf(tmp,"LAMEacm 0x%08X",this);
	my_debug.setPrefix(tmp); /// \todo get it from the registry
	my_debug.setIncludeTime(true);  /// \todo get it from the registry

	// Check in the registry if we have to Output Debug information
	DebugFileName[0] = '\0';

	HKEY OssKey;
	if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SOFTWARE\\MUKOLI", 0, KEY_READ , &OssKey ) == ERROR_SUCCESS) {
		DWORD DataType;
		DWORD DebugFileNameSize = 512;
		if (RegQueryValueEx( OssKey, "DebugFile", NULL, &DataType, DebugFileName, &DebugFileNameSize ) == ERROR_SUCCESS) {
			if (DataType == REG_SZ) {
				my_debug.setUseFile(true);
				my_debug.setDebugFile((char *)DebugFileName);
				my_debug.OutPut("Debug file is %s",(char *)DebugFileName);
			}
		}
	}
        wsprintf(VersionString,"%s - %s", ACM_VERSION, get_lame_version() );
	BuildBitrateTable();
	
	my_debug.OutPut(DEBUG_LEVEL_FUNC_START, "New ACM Creation (0x%08X)",this);
}

ACM::~ACM()
{
// not used, it's done automatically when closing the driver	if (my_hIcon != NULL)
//		CloseHandle(my_hIcon);

	bitrate_table.clear();

	my_debug.OutPut(DEBUG_LEVEL_FUNC_START, "ACM Deleted (0x%08X)",this);
}

//////////////////////////////////////////////////////////////////////
// Main message handler
//////////////////////////////////////////////////////////////////////

LONG ACM::DriverProcedure(const HDRVR hdrvr, const UINT msg, LONG lParam1, LONG lParam2)
{
    DWORD dwRes = 0L;

//my_debug.OutPut(DEBUG_LEVEL_MSG, "message 0x%08X for ThisACM 0x%08X", msg, this);

switch (msg) {
    case DRV_INSTALL:
		my_debug.OutPut(DEBUG_LEVEL_MSG, "DRV_INSTALL");
		// Sent when the driver is installed.
		dwRes = DRVCNF_OK;  // Can also return 
		break;              // DRVCNF_CANCEL
							// and DRV_RESTART

	case DRV_REMOVE:
		// Sent when the driver is removed.
		my_debug.OutPut(DEBUG_LEVEL_MSG, "DRV_REMOVE");
		dwRes = 1L;  // return value ignored
		break;

    case DRV_QUERYCONFIGURE:
		my_debug.OutPut(DEBUG_LEVEL_MSG, "DRV_QUERYCONFIGURE");
		// Sent to determine if the driver can be
		// configured.
		dwRes = 1L;  // Zero indicates configuration
		break;       // NOT supported

	case DRV_CONFIGURE:
		my_debug.OutPut(DEBUG_LEVEL_MSG, "DRV_CONFIGURE");
		// Sent to display the configuration
		// dialog box for the driver.
//		dwRes = Configure( (HWND) lParam1, (LPDRVCONFIGINFO) lParam2 );
		if (my_EncodingProperties.Config(my_hModule, (HWND) lParam1))
		{
			dwRes = DRVCNF_OK; // Can also return
					// DRVCNF_CANCEL
					// and DRVCNF_RESTART
		} else {
			dwRes = DRVCNF_CANCEL;
		}
		break;

	/**************************************
	// ACM additional messages
	***************************************/

	case ACMDM_DRIVER_ABOUT:
		my_debug.OutPut(DEBUG_LEVEL_MSG, "ACMDM_DRIVER_ABOUT");

		dwRes = About( (HWND) lParam1 );

        break;

	case ACMDM_DRIVER_DETAILS: // acmDriverDetails
		// Fill-in general informations about the driver/codec
		my_debug.OutPut(DEBUG_LEVEL_MSG, "ACMDM_DRIVER_DETAILS");

		dwRes = OnDriverDetails(hdrvr, (LPACMDRIVERDETAILS) lParam1);
        
		break;

	case ACMDM_FORMATTAG_DETAILS: // acmFormatTagDetails
		my_debug.OutPut(DEBUG_LEVEL_MSG, "ACMDM_FORMATTAG_DETAILS");

		dwRes = OnFormatTagDetails((LPACMFORMATTAGDETAILS) lParam1, lParam2);

        break;

	case ACMDM_FORMAT_DETAILS: // acmFormatDetails
		my_debug.OutPut(DEBUG_LEVEL_MSG, "ACMDM_FORMAT_DETAILS");

		dwRes = OnFormatDetails((LPACMFORMATDETAILS) lParam1, lParam2);
		
        break;           

    case ACMDM_FORMAT_SUGGEST: // acmFormatSuggest
		// Sent to determine if the driver can be
		// configured.
		my_debug.OutPut(DEBUG_LEVEL_MSG, "ACMDM_FORMAT_SUGGEST");
		dwRes = OnFormatSuggest((LPACMDRVFORMATSUGGEST) lParam1);
        break; 

	/**************************************
	// ACM stream messages
	***************************************/

	case ACMDM_STREAM_OPEN:
	// Sent to determine if the driver can be
	// configured.
		my_debug.OutPut(DEBUG_LEVEL_MSG, "ACMDM_STREAM_OPEN");
		dwRes = OnStreamOpen((LPACMDRVSTREAMINSTANCE) lParam1);
        break; 

	case ACMDM_STREAM_SIZE:
	// returns a recommended size for a source 
	// or destination buffer on an ACM stream
		my_debug.OutPut(DEBUG_LEVEL_MSG, "ACMDM_STREAM_SIZE");
		dwRes = OnStreamSize((LPACMDRVSTREAMINSTANCE)lParam1, (LPACMDRVSTREAMSIZE)lParam2);
        break; 

	case ACMDM_STREAM_PREPARE:
	// prepares an ACMSTREAMHEADER structure for
	// an ACM stream conversion
		my_debug.OutPut(DEBUG_LEVEL_MSG, "ACMDM_STREAM_PREPARE");
		dwRes = OnStreamPrepareHeader((LPACMDRVSTREAMINSTANCE)lParam1, (LPACMSTREAMHEADER) lParam2);
        break; 

	case ACMDM_STREAM_UNPREPARE:
	// cleans up the preparation performed by
	// the ACMDM_STREAM_PREPARE message for an ACM stream
		my_debug.OutPut(DEBUG_LEVEL_MSG, "ACMDM_STREAM_UNPREPARE");
		dwRes = OnStreamUnPrepareHeader((LPACMDRVSTREAMINSTANCE)lParam1, (LPACMSTREAMHEADER) lParam2);
        break; 

	case ACMDM_STREAM_CONVERT:
	// perform a conversion on the specified conversion stream
		my_debug.OutPut(DEBUG_LEVEL_MSG, "ACMDM_STREAM_CONVERT");
		dwRes = OnStreamConvert((LPACMDRVSTREAMINSTANCE)lParam1, (LPACMDRVSTREAMHEADER) lParam2);
		
        break; 

	case ACMDM_STREAM_CLOSE:
	// closes an ACM conversion stream
		my_debug.OutPut(DEBUG_LEVEL_MSG, "ACMDM_STREAM_CLOSE");
		dwRes = OnStreamClose((LPACMDRVSTREAMINSTANCE)lParam1);
        break;

	/**************************************
	// Unknown message
	***************************************/

	default:
	// Process any other messages.
		my_debug.OutPut(DEBUG_LEVEL_MSG, "ACM::DriverProc unknown message (0x%08X), lParam1 = 0x%08X, lParam2 = 0x%08X", msg, lParam1, lParam2);
		return DefDriverProc ((DWORD)this, hdrvr, msg, lParam1, lParam2);
    }

    return dwRes;
}

//////////////////////////////////////////////////////////////////////
// Special message handlers
//////////////////////////////////////////////////////////////////////
/*!
	Retreive the config details of this ACM driver
	The index represent the specified format

	\param a_FormatDetails will be filled with all the corresponding data
*/
inline DWORD ACM::OnFormatDetails(LPACMFORMATDETAILS a_FormatDetails, const LPARAM a_Query)
{
	DWORD Result = ACMERR_NOTPOSSIBLE;

	my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "ACM_FORMATDETAILS a_Query = 0x%08X",a_Query);
	switch (a_Query & ACM_FORMATDETAILSF_QUERYMASK) {

		// Fill-in the informations corresponding to the FormatDetails->dwFormatTagIndex
		case ACM_FORMATDETAILSF_INDEX :
			my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "enter ACM_FORMATDETAILSF_INDEX for index 0x%04X:%03d",a_FormatDetails->dwFormatTag,a_FormatDetails->dwFormatIndex);
			if (a_FormatDetails->dwFormatTag == PERSONAL_FORMAT) {
				if (a_FormatDetails->dwFormatIndex < GetNumberEncodingFormats()) {
					LPWAVEFORMATEX WaveExt;
					WaveExt = a_FormatDetails->pwfx;

					WaveExt->wFormatTag = PERSONAL_FORMAT;

					my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "format in  : channels %d, sample rate %d", WaveExt->nChannels, WaveExt->nSamplesPerSec);
					GetMP3FormatForIndex(a_FormatDetails->dwFormatIndex, *WaveExt, a_FormatDetails->szFormat);
					my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "format out : channels %d, sample rate %d", WaveExt->nChannels, WaveExt->nSamplesPerSec);
					Result = MMSYSERR_NOERROR;
				}
				else
				{
					my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "ACM_FORMATDETAILSF_INDEX unknown index 0x%04X:%03d",a_FormatDetails->dwFormatTag,a_FormatDetails->dwFormatIndex);
				}
			}
			else if (a_FormatDetails->dwFormatTag == WAVE_FORMAT_PCM) {
				if (a_FormatDetails->dwFormatIndex < FORMAT_MAX_NB_PCM) {
					LPWAVEFORMATEX WaveExt;
					WaveExt = a_FormatDetails->pwfx;

					WaveExt->wFormatTag = WAVE_FORMAT_PCM;

					my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "format in  : channels %d, sample rate %d", WaveExt->nChannels, WaveExt->nSamplesPerSec);
					GetPCMFormatForIndex(a_FormatDetails->dwFormatIndex, *WaveExt, a_FormatDetails->szFormat);
					my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "format out : channels %d, sample rate %d", WaveExt->nChannels, WaveExt->nSamplesPerSec);
					Result = MMSYSERR_NOERROR;
				}
				else
				{
					my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "ACM_FORMATDETAILSF_INDEX unknown index 0x%04X:%03d",a_FormatDetails->dwFormatTag,a_FormatDetails->dwFormatIndex);
				}
			}
			else
			{
				my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Unknown a_FormatDetails->dwFormatTag = 0x%08X",a_FormatDetails->dwFormatTag);
			}

		case ACM_FORMATDETAILSF_FORMAT :
			/// \todo we may output the corresponding strong (only for personal format)
			LPWAVEFORMATEX WaveExt;
			WaveExt = a_FormatDetails->pwfx;

			my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "enter ACM_FORMATDETAILSF_FORMAT : 0x%04X:%03d, format in : channels %d, sample rate %d",a_FormatDetails->dwFormatTag,a_FormatDetails->dwFormatIndex, WaveExt->nChannels, WaveExt->nSamplesPerSec);

			Result = MMSYSERR_NOERROR;
			break;
		
		default:
			Result = ACMERR_NOTPOSSIBLE;
			break;
	}

	a_FormatDetails->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;

	return Result;
}

/*!
	Retreive the details of each known format by this ACM driver
	The index represent the specified format (0 = MP3 / 1 = PCM)

	\param a_FormatTagDetails will be filled with all the corresponding data
*/
inline DWORD ACM::OnFormatTagDetails(LPACMFORMATTAGDETAILS a_FormatTagDetails, const LPARAM a_Query)
{
	DWORD Result;
	DWORD the_format = WAVE_FORMAT_UNKNOWN; // the format to give details

	if (a_FormatTagDetails->cbStruct >= sizeof(*a_FormatTagDetails)) {

		my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "ACMDM_FORMATTAG_DETAILS, a_Query = 0x%08X",a_Query);
		switch(a_Query & ACM_FORMATTAGDETAILSF_QUERYMASK) {

			case ACM_FORMATTAGDETAILSF_INDEX:
			// Fill-in the informations corresponding to the a_FormatDetails->dwFormatTagIndex
				my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "get ACM_FORMATTAGDETAILSF_INDEX for index %03d",a_FormatTagDetails->dwFormatTagIndex);

				if (a_FormatTagDetails->dwFormatTagIndex < FORMAT_TAG_MAX_NB) {
					switch (a_FormatTagDetails->dwFormatTagIndex)
					{
					case 0:
						the_format = PERSONAL_FORMAT;
						break;
					default :
						the_format = WAVE_FORMAT_PCM;
						break;
					}
				}
				else
				{
					my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "ACM_FORMATTAGDETAILSF_INDEX for unsupported index %03d",a_FormatTagDetails->dwFormatTagIndex);
					Result = ACMERR_NOTPOSSIBLE;
				}
				break;

			case ACM_FORMATTAGDETAILSF_FORMATTAG:
			// Fill-in the informations corresponding to the a_FormatDetails->dwFormatTagIndex and hdrvr given
				switch (a_FormatTagDetails->dwFormatTag)
				{
				case WAVE_FORMAT_PCM:
					the_format = WAVE_FORMAT_PCM;
					break;
				case PERSONAL_FORMAT:
					the_format = PERSONAL_FORMAT;
					break;
				default:
                    return (ACMERR_NOTPOSSIBLE);
				}
				my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "get ACM_FORMATTAGDETAILSF_FORMATTAG for index 0x%02X, cStandardFormats = %d",a_FormatTagDetails->dwFormatTagIndex,a_FormatTagDetails->cStandardFormats);
				break;
			case ACM_FORMATTAGDETAILSF_LARGESTSIZE:
				my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "ACM_FORMATTAGDETAILSF_LARGESTSIZE not used");
				Result = 0L;
				break;
			default:
				my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "OnFormatTagDetails Unknown Format tag query");
				Result = MMSYSERR_NOTSUPPORTED;
				break;
		}

		my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "OnFormatTagDetails the_format = 0x%08X",the_format);
		switch(the_format)
		{
			case WAVE_FORMAT_PCM:
				a_FormatTagDetails->dwFormatTag      = WAVE_FORMAT_PCM;
				a_FormatTagDetails->dwFormatTagIndex = 0;
				a_FormatTagDetails->cbFormatSize     = sizeof(PCMWAVEFORMAT);
				/// \note 0 may mean we don't know how to decode
				a_FormatTagDetails->fdwSupport       = ACMDRIVERDETAILS_SUPPORTF_CODEC;
				a_FormatTagDetails->cStandardFormats = FORMAT_MAX_NB_PCM;
				// should be filled by Windows				a_FormatTagDetails->szFormatTag[0] = '\0';
				Result = MMSYSERR_NOERROR;
				break;
			case PERSONAL_FORMAT:
				a_FormatTagDetails->dwFormatTag      = PERSONAL_FORMAT;
				a_FormatTagDetails->dwFormatTagIndex = 1;
				a_FormatTagDetails->cbFormatSize     = SIZE_FORMAT_STRUCT;
				a_FormatTagDetails->fdwSupport       = ACMDRIVERDETAILS_SUPPORTF_CODEC;
				a_FormatTagDetails->cStandardFormats = GetNumberEncodingFormats();
				lstrcpyW( a_FormatTagDetails->szFormatTag, L"Lame MP3" );
				Result = MMSYSERR_NOERROR;
				break;
			default:
				my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "OnFormatTagDetails Unknown format 0x%08X",the_format);
				return (ACMERR_NOTPOSSIBLE);
		}
		my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "OnFormatTagDetails %d possibilities for format 0x%08X",a_FormatTagDetails->cStandardFormats,the_format);
	}
	else
	{
		my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "a_FormatTagDetails->cbStruct < sizeof(*a_FormatDetails)");
		Result = ACMERR_NOTPOSSIBLE;
	}

	return Result;
}

/*!
	Retreive the global details of this ACM driver

	\param a_DriverDetail will be filled with all the corresponding data
*/
inline DWORD ACM::OnDriverDetails(const HDRVR hdrvr, LPACMDRIVERDETAILS a_DriverDetail)
{
	if (my_hIcon == NULL)
		my_hIcon = LoadIcon(GetDriverModuleHandle(hdrvr), MAKEINTRESOURCE(IDI_ICON));
	a_DriverDetail->hicon       = my_hIcon;

	a_DriverDetail->fccType     = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
	a_DriverDetail->fccComp     = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;

	/// \note this is an explicit hack of the FhG values
	/// \note later it could be a new value when the decoding is done
	a_DriverDetail->wMid        = MM_FRAUNHOFER_IIS;
	a_DriverDetail->wPid        = MM_FHGIIS_MPEGLAYER3;

	a_DriverDetail->vdwACM      = VERSION_MSACM;
	a_DriverDetail->vdwDriver   = VERSION_ACM_DRIVER;
	a_DriverDetail->fdwSupport  = ACMDRIVERDETAILS_SUPPORTF_CODEC;
	a_DriverDetail->cFormatTags = FORMAT_TAG_MAX_NB; // 2 : MP3 and PCM
//	a_DriverDetail->cFormatTags = 1; // 2 : MP3 and PCM
	a_DriverDetail->cFilterTags = FILTER_TAG_MAX_NB;

	lstrcpyW( a_DriverDetail->szShortName, L"LAME MP3" );
	char tmpStr[128];
	wsprintf(tmpStr, "LAME MP3 Codec v%s", GetVersionString());
	int u = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, tmpStr, -1, a_DriverDetail->szLongName, 0);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, tmpStr, -1, a_DriverDetail->szLongName, u);
	lstrcpyW( a_DriverDetail->szCopyright, L"2002 Steve Lhomme" );
	lstrcpyW( a_DriverDetail->szLicensing, L"LGPL (see gnu.org)" );
	/// \todo update this part when the code changes
	lstrcpyW( a_DriverDetail->szFeatures , L"only CBR implementation" );

    return MMSYSERR_NOERROR;  // Can also return DRVCNF_CANCEL
}

/*!
	Suggest an output format for the specified input format

	\param a_FormatSuggest will be filled with all the corresponding data
*/
inline DWORD ACM::OnFormatSuggest(LPACMDRVFORMATSUGGEST a_FormatSuggest)
{
	DWORD Result = MMSYSERR_NOTSUPPORTED;
    DWORD fdwSuggest = (ACM_FORMATSUGGESTF_TYPEMASK & a_FormatSuggest->fdwSuggest);

my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Suggest %s%s%s%s (0x%08X)",
				 (fdwSuggest & ACM_FORMATSUGGESTF_NCHANNELS) ? "channels, ":"",
				 (fdwSuggest & ACM_FORMATSUGGESTF_NSAMPLESPERSEC) ? "samples/sec, ":"",
				 (fdwSuggest & ACM_FORMATSUGGESTF_WBITSPERSAMPLE) ? "bits/sample, ":"",
				 (fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG) ? "format, ":"",
				 fdwSuggest);

my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Suggest for source format = 0x%04X, channels = %d, Samples/s = %d, AvgB/s = %d, BlockAlign = %d, b/sample = %d",
				 a_FormatSuggest->pwfxSrc->wFormatTag,
				 a_FormatSuggest->pwfxSrc->nChannels,
				 a_FormatSuggest->pwfxSrc->nSamplesPerSec,
				 a_FormatSuggest->pwfxSrc->nAvgBytesPerSec,
				 a_FormatSuggest->pwfxSrc->nBlockAlign,
				 a_FormatSuggest->pwfxSrc->wBitsPerSample);

my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Suggested destination format = 0x%04X, channels = %d, Samples/s = %d, AvgB/s = %d, BlockAlign = %d, b/sample = %d",
			 a_FormatSuggest->pwfxDst->wFormatTag,
			 a_FormatSuggest->pwfxDst->nChannels,
			 a_FormatSuggest->pwfxDst->nSamplesPerSec,
			 a_FormatSuggest->pwfxDst->nAvgBytesPerSec,
			 a_FormatSuggest->pwfxDst->nBlockAlign,
			 a_FormatSuggest->pwfxDst->wBitsPerSample);

	switch (a_FormatSuggest->pwfxSrc->wFormatTag)
	{
        case WAVE_FORMAT_PCM:
			/// \todo handle here the decoding ?
			my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Suggest for PCM source");
            //
			//  if the destination format tag is restricted, verify that
			//  it is within our capabilities...
			//
			//  this driver is able to decode to PCM
			//
			if (ACM_FORMATSUGGESTF_WFORMATTAG & fdwSuggest)
            {
                if (PERSONAL_FORMAT != a_FormatSuggest->pwfxDst->wFormatTag)
                    return (ACMERR_NOTPOSSIBLE);
            }
            else
			{
                a_FormatSuggest->pwfxDst->wFormatTag = PERSONAL_FORMAT;
            }


my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Suggest succeed A");
            //
			//  if the destination channel count is restricted, verify that
			//  it is within our capabilities...
			//
			//  this driver is not able to change the number of channels
			//
			if (ACM_FORMATSUGGESTF_NCHANNELS & fdwSuggest)
            {
                if (a_FormatSuggest->pwfxSrc->nChannels != a_FormatSuggest->pwfxDst->nChannels)
                    return (ACMERR_NOTPOSSIBLE);
            }
            else
			{
                a_FormatSuggest->pwfxDst->nChannels = a_FormatSuggest->pwfxSrc->nChannels;
            }

			if (a_FormatSuggest->pwfxSrc->nChannels != 1 && a_FormatSuggest->pwfxSrc->nChannels != 2)
				return MMSYSERR_INVALPARAM;


my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Suggest succeed B");
            //
			//  if the destination samples per second is restricted, verify
			//  that it is within our capabilities...
			//
			//  this driver is not able to change the sample rate
			//
			if (ACM_FORMATSUGGESTF_NSAMPLESPERSEC & fdwSuggest)
            {
                if (a_FormatSuggest->pwfxSrc->nSamplesPerSec != a_FormatSuggest->pwfxDst->nSamplesPerSec)
                    return (ACMERR_NOTPOSSIBLE);
            }
            else
			{
                a_FormatSuggest->pwfxDst->nSamplesPerSec = a_FormatSuggest->pwfxSrc->nSamplesPerSec;
            }


my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Suggest succeed C");
            //
			//  if the destination bits per sample is restricted, verify
			//  that it is within our capabilities...
			//
			//  We prefer decoding to 16-bit PCM.
			//
			if (ACM_FORMATSUGGESTF_WBITSPERSAMPLE & fdwSuggest)
            {
                if ( (16 != a_FormatSuggest->pwfxDst->wBitsPerSample) && (8 != a_FormatSuggest->pwfxDst->wBitsPerSample) )
                    return (ACMERR_NOTPOSSIBLE);
            }
            else
			{
                a_FormatSuggest->pwfxDst->wBitsPerSample = 16;
            }

			//			a_FormatSuggest->pwfxDst->nBlockAlign = FORMAT_BLOCK_ALIGN;
			a_FormatSuggest->pwfxDst->nBlockAlign = a_FormatSuggest->pwfxDst->nChannels * a_FormatSuggest->pwfxDst->wBitsPerSample / 8;
			
			a_FormatSuggest->pwfxDst->nAvgBytesPerSec = a_FormatSuggest->pwfxDst->nChannels * 64000 / 8;

			my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Suggest succeed");
			Result = MMSYSERR_NOERROR;


			break;
		case PERSONAL_FORMAT:
			my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Suggest for PERSONAL source");
            //
			//  if the destination format tag is restricted, verify that
			//  it is within our capabilities...
			//
			//  this driver is able to decode to PCM
			//
			if (ACM_FORMATSUGGESTF_WFORMATTAG & fdwSuggest)
            {
                if (WAVE_FORMAT_PCM != a_FormatSuggest->pwfxDst->wFormatTag)
                    return (ACMERR_NOTPOSSIBLE);
            }
            else
			{
                a_FormatSuggest->pwfxDst->wFormatTag = WAVE_FORMAT_PCM;
            }


            //
			//  if the destination channel count is restricted, verify that
			//  it is within our capabilities...
			//
			//  this driver is not able to change the number of channels
			//
			if (ACM_FORMATSUGGESTF_NCHANNELS & fdwSuggest)
            {
                if (a_FormatSuggest->pwfxSrc->nChannels != a_FormatSuggest->pwfxDst->nChannels)
                    return (ACMERR_NOTPOSSIBLE);
            }
            else
			{
                a_FormatSuggest->pwfxDst->nChannels = a_FormatSuggest->pwfxSrc->nChannels;
            }


            //
			//  if the destination samples per second is restricted, verify
			//  that it is within our capabilities...
			//
			//  this driver is not able to change the sample rate
			//
			if (ACM_FORMATSUGGESTF_NSAMPLESPERSEC & fdwSuggest)
            {
                if (a_FormatSuggest->pwfxSrc->nSamplesPerSec != a_FormatSuggest->pwfxDst->nSamplesPerSec)
                    return (ACMERR_NOTPOSSIBLE);
            }
            else
			{
                a_FormatSuggest->pwfxDst->nSamplesPerSec = a_FormatSuggest->pwfxSrc->nSamplesPerSec;
            }


            //
			//  if the destination bits per sample is restricted, verify
			//  that it is within our capabilities...
			//
			//  We prefer decoding to 16-bit PCM.
			//
			if (ACM_FORMATSUGGESTF_WBITSPERSAMPLE & fdwSuggest)
            {
                if ( (16 != a_FormatSuggest->pwfxDst->wBitsPerSample) && (8 != a_FormatSuggest->pwfxDst->wBitsPerSample) )
                    return (ACMERR_NOTPOSSIBLE);
            }
            else
			{
                a_FormatSuggest->pwfxDst->wBitsPerSample = 16;
            }

			//			a_FormatSuggest->pwfxDst->nBlockAlign = FORMAT_BLOCK_ALIGN;
			a_FormatSuggest->pwfxDst->nBlockAlign = a_FormatSuggest->pwfxDst->nChannels * a_FormatSuggest->pwfxDst->wBitsPerSample / 8;
			
			/// \todo this value must be a correct one !
			a_FormatSuggest->pwfxDst->nAvgBytesPerSec = a_FormatSuggest->pwfxDst->nSamplesPerSec * a_FormatSuggest->pwfxDst->nChannels * a_FormatSuggest->pwfxDst->wBitsPerSample / 8;

			my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Suggest succeed");
			Result = MMSYSERR_NOERROR;


			break;
	}

	my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Suggested destination format = 0x%04X, channels = %d, Samples/s = %d, AvgB/s = %d, BlockAlign = %d, b/sample = %d",
				 a_FormatSuggest->pwfxDst->wFormatTag,
				 a_FormatSuggest->pwfxDst->nChannels,
				 a_FormatSuggest->pwfxDst->nSamplesPerSec,
				 a_FormatSuggest->pwfxDst->nAvgBytesPerSec,
				 a_FormatSuggest->pwfxDst->nBlockAlign,
				 a_FormatSuggest->pwfxDst->wBitsPerSample);

	return Result;
}

/*!
	Create a stream instance for decoding/encoding

	\param a_StreamInstance contain information about the stream desired
*/
inline DWORD ACM::OnStreamOpen(LPACMDRVSTREAMINSTANCE a_StreamInstance)
{
	DWORD Result = ACMERR_NOTPOSSIBLE;

	//
	//  the most important condition to check before doing anything else
	//  is that this ACM driver can actually perform the conversion we are
	//  being opened for. this check should fail as quickly as possible
	//  if the conversion is not possible by this driver.
	//
	//  it is VERY important to fail quickly so the ACM can attempt to
	//  find a driver that is suitable for the conversion. also note that
	//  the ACM may call this driver several times with slightly different
	//  format specifications before giving up.
	//
	//  this driver first verifies that the source and destination formats
	//  are acceptable...
	//
	switch (a_StreamInstance->pwfxSrc->wFormatTag)
	{
        case WAVE_FORMAT_PCM:
			my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Open stream for PCM source (%05d samples %d channels %d bits/sample)",a_StreamInstance->pwfxSrc->nSamplesPerSec,a_StreamInstance->pwfxSrc->nChannels,a_StreamInstance->pwfxSrc->wBitsPerSample);
			if (a_StreamInstance->pwfxDst->wFormatTag == PERSONAL_FORMAT)
			{
				unsigned int OutputFrequency;

				/// \todo Smart mode
				if (my_EncodingProperties.GetSmartOutputMode())
					OutputFrequency = ACMStream::GetOutputSampleRate(a_StreamInstance->pwfxSrc->nSamplesPerSec,a_StreamInstance->pwfxDst->nAvgBytesPerSec,a_StreamInstance->pwfxDst->nChannels);
				else
					OutputFrequency = a_StreamInstance->pwfxSrc->nSamplesPerSec;

				my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Open stream for PERSONAL output (%05d samples %d channels %d bits/sample %d kbps)",a_StreamInstance->pwfxDst->nSamplesPerSec,a_StreamInstance->pwfxDst->nChannels,a_StreamInstance->pwfxDst->wBitsPerSample,8 * a_StreamInstance->pwfxDst->nAvgBytesPerSec);

				/// \todo add the possibility to have channel resampling (mono to stereo / stereo to mono)
				/// \todo support resampling ?
				/// \todo only do the test on OutputFrequency in "Smart Output" mode
				if (a_StreamInstance->pwfxDst->nSamplesPerSec != OutputFrequency ||
//					a_StreamInstance->pwfxSrc->nSamplesPerSec != a_StreamInstance->pwfxDst->nSamplesPerSec ||
					a_StreamInstance->pwfxSrc->nChannels != a_StreamInstance->pwfxDst->nChannels ||
					a_StreamInstance->pwfxSrc->wBitsPerSample != 16)
				{
					Result = ACMERR_NOTPOSSIBLE;
				} else {
					if ((a_StreamInstance->fdwOpen & ACM_STREAMOPENF_QUERY) == 0)
					{
						ACMStream * the_stream = ACMStream::Create();
						a_StreamInstance->dwInstance = (DWORD) the_stream;

						if (the_stream != NULL)
						{
							MPEGLAYER3WAVEFORMAT * casted = (MPEGLAYER3WAVEFORMAT *) a_StreamInstance->pwfxDst;
							vbr_mode a_mode = (casted->fdwFlags-2 == 0)?vbr_abr:vbr_off;
							if (the_stream->init(a_StreamInstance->pwfxDst->nSamplesPerSec,
												 OutputFrequency,
												 a_StreamInstance->pwfxDst->nChannels,
												 a_StreamInstance->pwfxDst->nAvgBytesPerSec,
												 a_mode))
								Result = MMSYSERR_NOERROR;
							else
								ACMStream::Erase( the_stream );
						}
					}
					else
					{
						Result = MMSYSERR_NOERROR;
					}
				}
			}
			break;
		case PERSONAL_FORMAT:
			my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Open stream for PERSONAL source (%05d samples %d channels %d bits/sample %d kbps)",a_StreamInstance->pwfxSrc->nSamplesPerSec,a_StreamInstance->pwfxSrc->nChannels,a_StreamInstance->pwfxSrc->wBitsPerSample,8 * a_StreamInstance->pwfxSrc->nAvgBytesPerSec);
			if (a_StreamInstance->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
			{
#ifdef ENABLE_DECODING
				if ((a_StreamInstance->fdwOpen & ACM_STREAMOPENF_QUERY) == 0)
				{
					/// \todo create the decoding stream
					my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Open stream for PCM output (%05d samples %d channels %d bits/sample %d B/s)",a_StreamInstance->pwfxDst->nSamplesPerSec,a_StreamInstance->pwfxDst->nChannels,a_StreamInstance->pwfxDst->wBitsPerSample,a_StreamInstance->pwfxDst->nAvgBytesPerSec);

					DecodeStream * the_stream = DecodeStream::Create();
					a_StreamInstance->dwInstance = (DWORD) the_stream;

					if (the_stream != NULL)
					{
						if (the_stream->init(a_StreamInstance->pwfxDst->nSamplesPerSec,
											 a_StreamInstance->pwfxDst->nChannels,
											 a_StreamInstance->pwfxDst->nAvgBytesPerSec,
											 a_StreamInstance->pwfxSrc->nAvgBytesPerSec))
							Result = MMSYSERR_NOERROR;
						else
							DecodeStream::Erase( the_stream );
					}
				}
				else
				{
					/// \todo decoding verification
					my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Open stream is valid");
					Result = MMSYSERR_NOERROR;
				}
#endif // ENABLE_DECODING
			}
			break;
	}

	my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Open stream Result = %d",Result);
	return Result;
}

inline DWORD ACM::OnStreamSize(LPACMDRVSTREAMINSTANCE a_StreamInstance, LPACMDRVSTREAMSIZE the_StreamSize)
{
	DWORD Result = ACMERR_NOTPOSSIBLE;

    switch (ACM_STREAMSIZEF_QUERYMASK & the_StreamSize->fdwSize)
    {
	case ACM_STREAMSIZEF_DESTINATION:
		my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Get source buffer size for destination size = %d",the_StreamSize->cbDstLength);
		break;
	case ACM_STREAMSIZEF_SOURCE:
		my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "Get destination buffer size for source size = %d",the_StreamSize->cbSrcLength);
        if (WAVE_FORMAT_PCM == a_StreamInstance->pwfxSrc->wFormatTag &&
			PERSONAL_FORMAT == a_StreamInstance->pwfxDst->wFormatTag)
        {
			ACMStream * the_stream = (ACMStream *) a_StreamInstance->dwInstance;
			if (the_stream != NULL)
			{
				the_StreamSize->cbDstLength = the_stream->GetOutputSizeForInput(the_StreamSize->cbSrcLength);
				Result = MMSYSERR_NOERROR;
			}
		}
        else if (PERSONAL_FORMAT == a_StreamInstance->pwfxSrc->wFormatTag &&
			 WAVE_FORMAT_PCM== a_StreamInstance->pwfxDst->wFormatTag)
		{
#ifdef ENABLE_DECODING
			DecodeStream * the_stream = (DecodeStream *) a_StreamInstance->dwInstance;
			if (the_stream != NULL)
			{
				the_StreamSize->cbDstLength = the_stream->GetOutputSizeForInput(the_StreamSize->cbSrcLength);
				Result = MMSYSERR_NOERROR;
			}
#endif // ENABLE_DECODING
		}
		break;
	default:
		Result = MMSYSERR_INVALFLAG;
		break;
	}

	return Result;
}

inline DWORD ACM::OnStreamClose(LPACMDRVSTREAMINSTANCE a_StreamInstance)
{
	DWORD Result = ACMERR_NOTPOSSIBLE;

	my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "OnStreamClose the stream 0x%X",a_StreamInstance->dwInstance);
    if (WAVE_FORMAT_PCM == a_StreamInstance->pwfxSrc->wFormatTag &&
		PERSONAL_FORMAT == a_StreamInstance->pwfxDst->wFormatTag)
    {
	ACMStream::Erase( (ACMStream *) a_StreamInstance->dwInstance );
	}
    else if (PERSONAL_FORMAT == a_StreamInstance->pwfxSrc->wFormatTag &&
		 WAVE_FORMAT_PCM== a_StreamInstance->pwfxDst->wFormatTag)
    {
#ifdef ENABLE_DECODING
		DecodeStream::Erase( (DecodeStream *) a_StreamInstance->dwInstance );
#endif // ENABLE_DECODING
	}

	// nothing to do yet
	Result = MMSYSERR_NOERROR;

	return Result;
}

inline DWORD ACM::OnStreamPrepareHeader(LPACMDRVSTREAMINSTANCE a_StreamInstance, LPACMSTREAMHEADER a_StreamHeader)
{
	DWORD Result = ACMERR_NOTPOSSIBLE;

	my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "  prepare : Src : %d (0x%08X) / %d - Dst : %d (0x%08X) / %d"
												, a_StreamHeader->cbSrcLength
												, a_StreamHeader->pbSrc
												, a_StreamHeader->cbSrcLengthUsed
												, a_StreamHeader->cbDstLength
												, a_StreamHeader->pbDst
												, a_StreamHeader->cbDstLengthUsed
											  );

	if (WAVE_FORMAT_PCM == a_StreamInstance->pwfxSrc->wFormatTag &&
		PERSONAL_FORMAT == a_StreamInstance->pwfxDst->wFormatTag)
	{
		ACMStream * the_stream = (ACMStream *)a_StreamInstance->dwInstance;
		
		if (the_stream->open(my_EncodingProperties))
			Result = MMSYSERR_NOERROR;
	}
	else if (PERSONAL_FORMAT == a_StreamInstance->pwfxSrc->wFormatTag &&
		     WAVE_FORMAT_PCM == a_StreamInstance->pwfxDst->wFormatTag)
	{
#ifdef ENABLE_DECODING
		DecodeStream * the_stream = (DecodeStream *)a_StreamInstance->dwInstance;
		
		if (the_stream->open())
			Result = MMSYSERR_NOERROR;
#endif // ENABLE_DECODING
	}

	return Result;
}

inline DWORD ACM::OnStreamUnPrepareHeader(LPACMDRVSTREAMINSTANCE a_StreamInstance, LPACMSTREAMHEADER a_StreamHeader)
{
	DWORD Result = ACMERR_NOTPOSSIBLE;

	my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "unprepare : Src : %d / %d - Dst : %d / %d"
											, a_StreamHeader->cbSrcLength
											, a_StreamHeader->cbSrcLengthUsed
											, a_StreamHeader->cbDstLength
											, a_StreamHeader->cbDstLengthUsed
											);
    if (WAVE_FORMAT_PCM == a_StreamInstance->pwfxSrc->wFormatTag &&
		PERSONAL_FORMAT == a_StreamInstance->pwfxDst->wFormatTag)
    {
	ACMStream * the_stream = (ACMStream *)a_StreamInstance->dwInstance;
	DWORD OutputSize = a_StreamHeader->cbDstLength;
	
	if (the_stream->close(a_StreamHeader->pbDst, &OutputSize) && (OutputSize <= a_StreamHeader->cbDstLength))
	{
		a_StreamHeader->cbDstLengthUsed = OutputSize;
			Result = MMSYSERR_NOERROR;
		}
	}
    else if (PERSONAL_FORMAT == a_StreamInstance->pwfxSrc->wFormatTag &&
		 WAVE_FORMAT_PCM== a_StreamInstance->pwfxDst->wFormatTag)
    {
#ifdef ENABLE_DECODING
		DecodeStream * the_stream = (DecodeStream *)a_StreamInstance->dwInstance;
		DWORD OutputSize = a_StreamHeader->cbDstLength;
		
		if (the_stream->close(a_StreamHeader->pbDst, &OutputSize) && (OutputSize <= a_StreamHeader->cbDstLength))
		{
			a_StreamHeader->cbDstLengthUsed = OutputSize;
			Result = MMSYSERR_NOERROR;
	}
#endif // ENABLE_DECODING
	}

	return Result;
}

inline DWORD ACM::OnStreamConvert(LPACMDRVSTREAMINSTANCE a_StreamInstance, LPACMDRVSTREAMHEADER a_StreamHeader)
{
	DWORD Result = ACMERR_NOTPOSSIBLE;

	if (WAVE_FORMAT_PCM == a_StreamInstance->pwfxSrc->wFormatTag &&
		PERSONAL_FORMAT == a_StreamInstance->pwfxDst->wFormatTag)
	{
		my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "OnStreamConvert SRC = PCM (encode)");

		ACMStream * the_stream = (ACMStream *) a_StreamInstance->dwInstance;
		if (the_stream != NULL)
		{
			if (the_stream->ConvertBuffer( a_StreamHeader ))
				Result = MMSYSERR_NOERROR;
		}
	}
	else if (PERSONAL_FORMAT == a_StreamInstance->pwfxSrc->wFormatTag &&
		     WAVE_FORMAT_PCM == a_StreamInstance->pwfxDst->wFormatTag)
	{
		my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "OnStreamConvert SRC = MP3 (decode)");

#ifdef ENABLE_DECODING
		DecodeStream * the_stream = (DecodeStream *) a_StreamInstance->dwInstance;
		if (the_stream != NULL)
		{
			if (the_stream->ConvertBuffer( a_StreamHeader ))
				Result = MMSYSERR_NOERROR;
		}
#endif // ENABLE_DECODING
	}
	else
		my_debug.OutPut(DEBUG_LEVEL_FUNC_CODE, "OnStreamConvert unsupported conversion");

	return Result;
}


void ACM::GetMP3FormatForIndex(const DWORD the_Index, WAVEFORMATEX & the_Format, unsigned short the_String[ACMFORMATDETAILS_FORMAT_CHARS]) const
{
	int Block_size;
    char temp[ACMFORMATDETAILS_FORMAT_CHARS];


	if (the_Index < bitrate_table.size())
	{
	//	the_Format.wBitsPerSample = 16;
		the_Format.wBitsPerSample = 0;
	
		/// \todo handle more channel modes (mono, stereo, joint-stereo, dual-channel)
	//	the_Format.nChannels = SIZE_CHANNEL_MODE - int(the_Index % SIZE_CHANNEL_MODE);
	
		the_Format.nBlockAlign = 1;

		the_Format.nSamplesPerSec = bitrate_table[the_Index].frequency;
		the_Format.nAvgBytesPerSec = bitrate_table[the_Index].bitrate * 1000 / 8;
		if (bitrate_table[the_Index].frequency >= mpeg1_freq[SIZE_FREQ_MPEG1-1])
			Block_size = 1152;
		else
			Block_size = 576;
	
		the_Format.nChannels = bitrate_table[the_Index].channels;

		the_Format.cbSize = sizeof(MPEGLAYER3WAVEFORMAT) - sizeof(WAVEFORMATEX);
		MPEGLAYER3WAVEFORMAT * tmpFormat = (MPEGLAYER3WAVEFORMAT *) &the_Format;
		tmpFormat->wID             = 1;
		// this is the only way I found to know if we do CBR or ABR
		tmpFormat->fdwFlags        = 2 + ((bitrate_table[the_Index].mode == vbr_abr)?0:2);
		tmpFormat->nBlockSize      = WORD(Block_size * the_Format.nAvgBytesPerSec / the_Format.nSamplesPerSec);
		tmpFormat->nFramesPerBlock = 1;
		tmpFormat->nCodecDelay     = 0; // 0x0571 on FHG
	
         /// \todo : generate the string with the appropriate stereo mode
         if (bitrate_table[the_Index].mode == vbr_abr)
             wsprintfA( temp, "%d Hz, %d kbps ABR, %s", the_Format.nSamplesPerSec, the_Format.nAvgBytesPerSec * 8 / 1000, (the_Format.nChannels == 1)?"Mono":"Stereo");
         else
             wsprintfA( temp, "%d Hz, %d kbps CBR, %s", the_Format.nSamplesPerSec, the_Format.nAvgBytesPerSec * 8 / 1000, (the_Format.nChannels == 1)?"Mono":"Stereo");

         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, temp, -1, the_String, ACMFORMATDETAILS_FORMAT_CHARS);
     }
 }

void ACM::GetPCMFormatForIndex(const DWORD the_Index, WAVEFORMATEX & the_Format, unsigned short the_String[ACMFORMATDETAILS_FORMAT_CHARS]) const
{
	the_Format.nChannels = SIZE_CHANNEL_MODE - int(the_Index % SIZE_CHANNEL_MODE);
	the_Format.wBitsPerSample = 16;
	the_Format.nBlockAlign = the_Format.nChannels * the_Format.wBitsPerSample / 8;


	DWORD a_Channel_Independent = the_Index / SIZE_CHANNEL_MODE;

	// first MPEG1 frequencies
	if (a_Channel_Independent < SIZE_FREQ_MPEG1)
	{
		the_Format.nSamplesPerSec = mpeg1_freq[a_Channel_Independent];
	}
	else
	{
		a_Channel_Independent -= SIZE_FREQ_MPEG1;
		the_Format.nSamplesPerSec = mpeg2_freq[a_Channel_Independent];
	}

	the_Format.nAvgBytesPerSec = the_Format.nSamplesPerSec * the_Format.nChannels * the_Format.wBitsPerSample / 8;
}

DWORD ACM::GetNumberEncodingFormats() const
{
	return bitrate_table.size();
}

bool ACM::IsSmartOutput(const int frequency, const int bitrate, const int channels) const
{
	double compression_ratio = double(frequency * 2 * channels) / double(bitrate * 100);

//my_debug.OutPut(DEBUG_LEVEL_FUNC_DEBUG, "compression_ratio %f, freq %d, bitrate %d, channels %d", compression_ratio, frequency, bitrate, channels);

	if(my_EncodingProperties.GetSmartOutputMode())
		return (compression_ratio <= my_EncodingProperties.GetSmartRatio());
	else return true;
}

void ACM::BuildBitrateTable()
{
	my_debug.OutPut("entering BuildBitrateTable");

	// fill the table
	unsigned int channel,bitrate,freq;
	
	bitrate_table.clear();

	// CBR bitrates
	for (channel = 0;channel < SIZE_CHANNEL_MODE;channel++)
	{
		// MPEG I
		for (freq = 0;freq < SIZE_FREQ_MPEG1;freq++)
		{
			for (bitrate = 0;bitrate < SIZE_BITRATE_MPEG1;bitrate++)
			{

				if (!my_EncodingProperties.GetSmartOutputMode() || IsSmartOutput(mpeg1_freq[freq], mpeg1_bitrate[bitrate], channel+1))
				{
					bitrate_item bitrate_table_tmp;
					
					bitrate_table_tmp.frequency = mpeg1_freq[freq];
					bitrate_table_tmp.bitrate = mpeg1_bitrate[bitrate];
					bitrate_table_tmp.channels = channel+1;
					bitrate_table_tmp.mode = vbr_off;
					bitrate_table.push_back(bitrate_table_tmp);
				}
			}
		}
		// MPEG II / II.5
		for (freq = 0;freq < SIZE_FREQ_MPEG2;freq++)
		{
			for (bitrate = 0;bitrate < SIZE_BITRATE_MPEG2;bitrate++)
			{
				if (!my_EncodingProperties.GetSmartOutputMode() || IsSmartOutput(mpeg2_freq[freq], mpeg2_bitrate[bitrate], channel+1))
				{
					bitrate_item bitrate_table_tmp;

                                        bitrate_table_tmp.frequency = mpeg2_freq[freq];
					bitrate_table_tmp.bitrate = mpeg2_bitrate[bitrate];
					bitrate_table_tmp.channels = channel+1;
					bitrate_table_tmp.mode = vbr_abr;
					bitrate_table.push_back(bitrate_table_tmp);
				}
			}
		}
	}

	if (my_EncodingProperties.GetAbrOutputMode())
	// ABR bitrates
	{
		for (channel = 0;channel < SIZE_CHANNEL_MODE;channel++)
		{
			// MPEG I
			for (freq = 0;freq < SIZE_FREQ_MPEG1;freq++)
			{
				for (bitrate = my_EncodingProperties.GetAbrBitrateMax();
					   bitrate >= my_EncodingProperties.GetAbrBitrateMin(); 
				     bitrate -= my_EncodingProperties.GetAbrBitrateStep())
				{
					if (bitrate >= mpeg1_bitrate[SIZE_BITRATE_MPEG1-1] && (!my_EncodingProperties.GetSmartOutputMode() || IsSmartOutput(mpeg1_freq[freq], bitrate, channel+1)))
					{
						bitrate_item bitrate_table_tmp;
						
						bitrate_table_tmp.frequency = mpeg1_freq[freq];
						bitrate_table_tmp.bitrate = bitrate;
						bitrate_table_tmp.channels = channel+1;
						bitrate_table_tmp.mode = vbr_abr;
						bitrate_table.push_back(bitrate_table_tmp);
					}
				}
			}
			// MPEG II / II.5
			for (freq = 0;freq < SIZE_FREQ_MPEG2;freq++)
			{
				for (bitrate = my_EncodingProperties.GetAbrBitrateMax();
					   bitrate >= my_EncodingProperties.GetAbrBitrateMin(); 
				     bitrate -= my_EncodingProperties.GetAbrBitrateStep())
				{
					if (bitrate >= mpeg2_bitrate[SIZE_BITRATE_MPEG2-1] && (!my_EncodingProperties.GetSmartOutputMode() || IsSmartOutput(mpeg2_freq[freq], bitrate, channel+1)))
					{
						bitrate_item bitrate_table_tmp;
						
						bitrate_table_tmp.frequency = mpeg2_freq[freq];
						bitrate_table_tmp.bitrate = bitrate;
						bitrate_table_tmp.channels = channel+1;
						bitrate_table_tmp.mode = vbr_abr;
						bitrate_table.push_back(bitrate_table_tmp);
					}
				}
			}
		}
	}

	// sorting by frequency/bitrate/channel
	std::sort(bitrate_table.begin(), bitrate_table.end());

/*	{
		// display test
		int i=0;
		for (i=0; i<bitrate_table.size();i++)
		{
			my_debug.OutPut("bitrate_table[%d].frequency = %d",i,bitrate_table[i].frequency);
			my_debug.OutPut("bitrate_table[%d].bitrate = %d",i,bitrate_table[i].bitrate);
			my_debug.OutPut("bitrate_table[%d].channel = %d",i,bitrate_table[i].channels);
			my_debug.OutPut("bitrate_table[%d].ABR = %s\n",i,(bitrate_table[i].mode == vbr_abr)?"ABR":"CBR");
		}
	}*/

	my_debug.OutPut("leaving BuildBitrateTable");
}
