#include "All.h"
#include "APEInfo.h"
#include "APEInfoDialog.h"
#include "ID3Genres.h"
#include "APECompress.h"
#include "CharacterHelper.h"

/***************************************************************************************
The dialog component ID's
***************************************************************************************/
#define FILE_NAME_EDIT                  1000

#define ENCODER_VERSION_STATIC          2000
#define COMPRESSION_LEVEL_STATIC        2001
#define FORMAT_FLAGS_STATIC             2002
#define HAS_TAG_STATIC                  2003

#define SAMPLE_RATE_STATIC              3000
#define CHANNELS_STATIC                 3001
#define BITS_PER_SAMPLE_STATIC          3002
#define PEAK_LEVEL_STATIC               3003

#define TRACK_LENGTH_STATIC             4000
#define WAV_SIZE_STATIC			        4001
#define APE_SIZE_STATIC					4002
#define COMPRESSION_RATIO_STATIC        4003

#define TITLE_EDIT						5000
#define ARTIST_EDIT						5001
#define ALBUM_EDIT						5002
#define COMMENT_EDIT					5003
#define YEAR_EDIT						5004
#define GENRE_COMBOBOX                  5005
#define TRACK_EDIT                      5006

#define SAVE_TAG_BUTTON                 6000
#define REMOVE_TAG_BUTTON               6001
#define CANCEL_BUTTON                   6002

/***************************************************************************************
Global pointer to this instance
***************************************************************************************/
CAPEInfoDialog * g_pAPEDecompressDialog = NULL;

/***************************************************************************************
Construction / destruction
***************************************************************************************/
CAPEInfoDialog::CAPEInfoDialog()
{
	g_pAPEDecompressDialog = NULL;
}

CAPEInfoDialog::~CAPEInfoDialog()
{

}

/***************************************************************************************
Display the file info dialog
***************************************************************************************/
int CAPEInfoDialog::ShowAPEInfoDialog(const str_utf16 * pFilename, HINSTANCE hInstance, const str_utf16 * lpszTemplateName, HWND hWndParent)
{
	// only allow one instance at a time
	if (g_pAPEDecompressDialog != NULL) { return -1; }

	// open the file
	int nErrorCode = ERROR_SUCCESS;
	m_pAPEDecompress = CreateIAPEDecompress(pFilename, &nErrorCode);
	if (m_pAPEDecompress == NULL || nErrorCode != ERROR_SUCCESS)
		return nErrorCode;
	
	g_pAPEDecompressDialog = this;

	DialogBoxParam(hInstance, lpszTemplateName, hWndParent, (DLGPROC) DialogProc, 0);

	// clean up
	SAFE_DELETE(m_pAPEDecompress);
	g_pAPEDecompressDialog = NULL;
	
	return 0;
}

/***************************************************************************************
Fill the genre combobox
***************************************************************************************/
int CAPEInfoDialog::FillGenreComboBox(HWND hDlg, int nComboBoxID, char *pSelectedGenre)
{
	// declare the variables	
	int nRetVal;
	HWND hGenreComboBox = GetDlgItem(hDlg, nComboBoxID);
	
	// reset the contents of the combobox
	SendMessage(hGenreComboBox, CB_RESETCONTENT, 0, 0); 
		
	// propagate the combobox (0 to 126 so "Undefined" isn't repeated)
	for (int z = 0; z < GENRE_COUNT; z++) 
	{
		//add the genre string
		nRetVal = SendMessage(hGenreComboBox, CB_ADDSTRING, 0, (LPARAM) g_ID3Genre[z]);
		if (nRetVal == CB_ERR) { return -1; }
	}

	// add the 'Undefined' genre
	nRetVal = SendMessage(hGenreComboBox, CB_ADDSTRING, 0, (LPARAM) "Undefined");
	if (nRetVal == CB_ERR) { return -1; }
	
	// set the genre id (if it's specified)
	if (pSelectedGenre)
	{
		if (strlen(pSelectedGenre) > 0)
			SendMessage(hGenreComboBox, CB_SELECTSTRING, -1, (LPARAM) pSelectedGenre);
	}

	return 0;
}

/***************************************************************************************
The dialog procedure
***************************************************************************************/
LRESULT CALLBACK CAPEInfoDialog::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	// get the class
	IAPEDecompress * pAPEDecompress = g_pAPEDecompressDialog->m_pAPEDecompress;

	int wmID, wmEvent;

	switch (message)
	{
		case WM_INITDIALOG:
		{
			// variable declares
            TCHAR cTemp[1024] = { 0 };

			// set info
			wchar_t cFilename[MAX_PATH + 1] = {0};
			GET_IO(pAPEDecompress)->GetName(&cFilename[0]);

			SetDlgItemText(hDlg, FILE_NAME_EDIT, cFilename);
			
			switch (pAPEDecompress->GetInfo(APE_INFO_COMPRESSION_LEVEL))
			{
			    case COMPRESSION_LEVEL_FAST: _stprintf(cTemp, _T("Mode: Fast")); break;
			    case COMPRESSION_LEVEL_NORMAL: _stprintf(cTemp, _T("Mode: Normal")); break;
			    case COMPRESSION_LEVEL_HIGH: _stprintf(cTemp, _T("Mode: High")); break;
			    case COMPRESSION_LEVEL_EXTRA_HIGH: _stprintf(cTemp, _T("Mode: Extra High")); break;
			    case COMPRESSION_LEVEL_INSANE: _stprintf(cTemp, _T("Mode: Insane")); break;
			    default: _stprintf(cTemp, _T("Mode: Unknown")); break;
			}
			SetDlgItemText(hDlg, COMPRESSION_LEVEL_STATIC, cTemp);
			
			_stprintf(cTemp, _T("Version: %.2f"), float(pAPEDecompress->GetInfo(APE_INFO_FILE_VERSION)) / float(1000));
			SetDlgItemText(hDlg, ENCODER_VERSION_STATIC, cTemp);

			_stprintf(cTemp, _T("Format Flags: %d"), pAPEDecompress->GetInfo(APE_INFO_FORMAT_FLAGS));
			SetDlgItemText(hDlg, FORMAT_FLAGS_STATIC, cTemp);
			
			_stprintf(cTemp, _T("Sample Rate: %d"), pAPEDecompress->GetInfo(APE_INFO_SAMPLE_RATE));
			SetDlgItemText(hDlg, SAMPLE_RATE_STATIC, cTemp);
			
			_stprintf(cTemp, _T("Channels: %d"), pAPEDecompress->GetInfo(APE_INFO_CHANNELS));
			SetDlgItemText(hDlg, CHANNELS_STATIC, cTemp);

			_stprintf(cTemp, _T("Bits Per Sample: %d"), pAPEDecompress->GetInfo(APE_INFO_BITS_PER_SAMPLE));
			SetDlgItemText(hDlg, BITS_PER_SAMPLE_STATIC, cTemp);

			int nSeconds = pAPEDecompress->GetInfo(APE_INFO_LENGTH_MS) / 1000; int nMinutes = nSeconds / 60; nSeconds = nSeconds % 60; int nHours = nMinutes / 60; nMinutes = nMinutes % 60;
			if (nHours > 0)	_stprintf(cTemp, _T("Length: %d:%02d:%02d"), nHours, nMinutes, nSeconds);
			else if (nMinutes > 0) _stprintf(cTemp, _T("Length: %d:%02d"), nMinutes, nSeconds);
			else _stprintf(cTemp, _T("Length: 0:%02d"), nSeconds);
			SetDlgItemText(hDlg, TRACK_LENGTH_STATIC, cTemp);

			int nPeakLevel = pAPEDecompress->GetInfo(APE_INFO_PEAK_LEVEL);
			if (nPeakLevel >= 0) _stprintf(cTemp, _T("Peak Level: %d"), nPeakLevel);
			else _stprintf(cTemp, _T("Peak Level: ?"));
			SetDlgItemText(hDlg, PEAK_LEVEL_STATIC, cTemp);

			// the file size
			_stprintf(cTemp, _T("APE: %.2f MB"), float(pAPEDecompress->GetInfo(APE_INFO_APE_TOTAL_BYTES)) / float(1048576));
			SetDlgItemText(hDlg, APE_SIZE_STATIC, cTemp);
			
			_stprintf(cTemp, _T("WAV: %.2f MB"), float(pAPEDecompress->GetInfo(APE_INFO_WAV_TOTAL_BYTES)) / float(1048576));
			SetDlgItemText(hDlg, WAV_SIZE_STATIC, cTemp);
			
			// the compression ratio
			_stprintf(cTemp, _T("Compression: %.2f%%"), float(pAPEDecompress->GetInfo(APE_INFO_AVERAGE_BITRATE) * 100) / float(pAPEDecompress->GetInfo(APE_INFO_DECOMPRESSED_BITRATE)));
			SetDlgItemText(hDlg, COMPRESSION_RATIO_STATIC, cTemp);

			// the has tag
			BOOL bHasID3Tag = GET_TAG(pAPEDecompress)->GetHasID3Tag();
			BOOL bHasAPETag = GET_TAG(pAPEDecompress)->GetHasAPETag();
						
			if (!bHasID3Tag && !bHasAPETag)
				_stprintf(cTemp, _T("Tag: None"));
			else if (bHasID3Tag && !bHasAPETag)
				_stprintf(cTemp, _T("Tag: ID3v1.1"));
			else if (!bHasID3Tag && bHasAPETag)
				_stprintf(cTemp, _T("Tag: APE Tag"));
			else
				_stprintf(cTemp, _T("Tag: Corrupt"));
			SetDlgItemText(hDlg, HAS_TAG_STATIC, cTemp);
				
			wchar_t cBuffer[256];
			int nBufferBytes = 256;

			nBufferBytes = 256;
			GET_TAG(pAPEDecompress)->GetFieldString(APE_TAG_FIELD_TITLE, cBuffer, &nBufferBytes);
			SetDlgItemText(hDlg, TITLE_EDIT, cBuffer);
			
			nBufferBytes = 256;
			GET_TAG(pAPEDecompress)->GetFieldString(APE_TAG_FIELD_ARTIST, cBuffer, &nBufferBytes);
			SetDlgItemText(hDlg, ARTIST_EDIT, cBuffer);
			
			nBufferBytes = 256;
			GET_TAG(pAPEDecompress)->GetFieldString(APE_TAG_FIELD_ALBUM, cBuffer, &nBufferBytes);
			SetDlgItemText(hDlg, ALBUM_EDIT, cBuffer);
			
			nBufferBytes = 256;
			GET_TAG(pAPEDecompress)->GetFieldString(APE_TAG_FIELD_COMMENT, cBuffer, &nBufferBytes);
			SetDlgItemText(hDlg, COMMENT_EDIT, cBuffer);
			
			nBufferBytes = 256;
			GET_TAG(pAPEDecompress)->GetFieldString(APE_TAG_FIELD_YEAR, cBuffer, &nBufferBytes);
			SetDlgItemText(hDlg, YEAR_EDIT, cBuffer);
			
			nBufferBytes = 256;
			GET_TAG(pAPEDecompress)->GetFieldString(APE_TAG_FIELD_TRACK, cBuffer, &nBufferBytes);
			SetDlgItemText(hDlg, TRACK_EDIT, cBuffer);
			
			g_pAPEDecompressDialog->FillGenreComboBox(hDlg, GENRE_COMBOBOX, NULL);

			nBufferBytes = 256;
			GET_TAG(pAPEDecompress)->GetFieldString(APE_TAG_FIELD_GENRE, cBuffer, &nBufferBytes);
			SetDlgItemText(hDlg, GENRE_COMBOBOX, cBuffer);
			
			return TRUE;
			break;
		}
		case WM_COMMAND:
			wmID = LOWORD(wParam);
			wmEvent = HIWORD(wParam);
			
			switch (wmID)
			{
				case IDCANCEL: 
					// traps the [esc] key
					EndDialog(hDlg, 0);
					return TRUE;
					break;
				case CANCEL_BUTTON:
					EndDialog(hDlg, 0);
				 	return TRUE;
					break;
				case REMOVE_TAG_BUTTON:
				{
					// make sure you really wanted to
					int nRetVal = ::MessageBox(hDlg, _T("Are you sure you want to permanently remove the tag?"), _T("Are You Sure?"), MB_YESNO | MB_ICONQUESTION);
					if (nRetVal == IDYES)
					{
						// remove the ID3 tag...
						if (GET_TAG(pAPEDecompress)->Remove() != 0) 
						{
							MessageBox(hDlg, _T("Error removing tag. (could the file be read-only?)"), _T("Error Removing Tag"), MB_OK | MB_ICONEXCLAMATION);
							return TRUE;
						}
						else 
                        {
							EndDialog(hDlg, 0);
					 		return TRUE;
						}
					}
					break;
				}
				case SAVE_TAG_BUTTON:
					
					// make the id3 tag
					TCHAR cBuffer[256]; int z;

					#define SAVE_FIELD(ID, Field)							\
						for (z = 0; z < 256; z++) { cBuffer[z] = 0; }		\
						GetDlgItemText(hDlg, ID, cBuffer, 256);				\
						GET_TAG(pAPEDecompress)->SetFieldString(Field, cBuffer);

					SAVE_FIELD(TITLE_EDIT, APE_TAG_FIELD_TITLE)
					SAVE_FIELD(ARTIST_EDIT, APE_TAG_FIELD_ARTIST)
					SAVE_FIELD(ALBUM_EDIT, APE_TAG_FIELD_ALBUM)
					SAVE_FIELD(COMMENT_EDIT, APE_TAG_FIELD_COMMENT)
					SAVE_FIELD(TRACK_EDIT, APE_TAG_FIELD_TRACK)
					SAVE_FIELD(YEAR_EDIT, APE_TAG_FIELD_YEAR)
					SAVE_FIELD(GENRE_COMBOBOX, APE_TAG_FIELD_GENRE)
					
					if (GET_TAG(pAPEDecompress)->Save() != 0) 
					{
						MessageBox(hDlg, _T("Error saving tag. (could the file be read-only?)"), _T("Error Saving Tag"), MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}

					EndDialog(hDlg, 0);
					break;
			}
			break;
		case WM_CLOSE:
			EndDialog(hDlg, 0);
			return TRUE;
			break;
	}
	
	return FALSE;
}