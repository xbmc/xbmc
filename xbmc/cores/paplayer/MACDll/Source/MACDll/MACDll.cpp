#include "MACDll.h"
#include "resource.h"
#if !defined(_LINUX) && !defined(__APPLE__)
#include "WinFileIO.h"
#include "APEInfoDialog.h"
#include "WAVInfoDialog.h"
#else
#include "StdLibFileIO.h"
#endif
#include "APEDecompress.h"
#include "APECompressCreate.h"
#include "APECompressCore.h"
#include "APECompress.h"
#include "APEInfo.h"
#include "APETag.h"
#include "CharacterHelper.h"

int __stdcall GetVersionNumber()
{
	return MAC_VERSION_NUMBER;
}

#if !defined(_LINUX) && !defined(__APPLE__)
int __stdcall GetInterfaceCompatibility(int nVersion, BOOL bDisplayWarningsOnFailure, HWND hwndParent)
{
	int nRetVal = 0;
	if (nVersion > MAC_VERSION_NUMBER)
	{
		nRetVal = -1;
		if (bDisplayWarningsOnFailure)
		{
			TCHAR cMessage[1024];
			_stprintf(cMessage, _T("You system does not have a new enough version of Monkey's Audio installed.\n")
				_T("Please visit www.monkeysaudio.com for the latest version.\n\n(version %.2f or later required)"),
				float(nVersion) / float(1000));
			MessageBox(hwndParent, cMessage, _T("Please Update Monkey's Audio"), MB_OK | MB_ICONINFORMATION);
		}
	}
	else if (nVersion < 3940)
	{
		nRetVal = -1;
		if (bDisplayWarningsOnFailure)
		{
			TCHAR cMessage[1024];
			_stprintf(cMessage, _T("This program is trying to use an old version of Monkey's Audio.\n")
				_T("Please contact the author about updating their support for Monkey's Audio.\n\n")
				_T("Monkey's Audio currently installed: %.2f\nProgram is searching for: %.2f"),
				float(MAC_VERSION_NUMBER) / float(1000), float(nVersion) / float(1000));
			MessageBox(hwndParent, cMessage, _T("Program Requires Updating"), MB_OK | MB_ICONINFORMATION);
		}
	}

	return nRetVal;
}

int __stdcall ShowFileInfoDialog(const str_ansi * pFilename, HWND hwndWindow)
{
   	// convert the filename
	CSmartPtr<wchar_t> spFilename(GetUTF16FromANSI(pFilename), TRUE);

	// make sure the file exists
	WIN32_FIND_DATA FindData = { 0 };
	HANDLE hFind = FindFirstFile(spFilename, &FindData);
	if (hFind == INVALID_HANDLE_VALUE) 
	{
		MessageBox(hwndWindow, _T("File not found."), _T("File Info"), MB_OK);
		return 0;
	}
	else 
	{
		FindClose(hFind);
	}
    	
    // see what type the file is
	if ((_tcsicmp(&spFilename[_tcslen(spFilename) - 4], _T(".ape")) == 0) ||
		(_tcsicmp(&spFilename[_tcslen(spFilename) - 4], _T(".apl")) == 0)) 
	{
		CAPEInfoDialog APEInfoDialog;
		APEInfoDialog.ShowAPEInfoDialog(spFilename, GetModuleHandle(_T("MACDll.dll")), (LPCTSTR) IDD_APE_INFO, hwndWindow);
		return 0;
	}
	else if (_tcsicmp(&spFilename[_tcslen(spFilename) - 4], _T(".wav")) == 0) 
	{
		CWAVInfoDialog WAVInfoDialog;
		WAVInfoDialog.ShowWAVInfoDialog(spFilename, GetModuleHandle(_T("MACDll.dll")), (LPCTSTR) IDD_WAV_INFO, hwndWindow);
		return 0;
	}
	else 
	{
		MessageBox(hwndWindow, _T("File type not supported. (only .ape, .apl, and .wav files currently supported)"), _T("File Info: Unsupported File Type"), MB_OK);
		return 0;
	};
}
#endif

int __stdcall TagFileSimple(const str_ansi * pFilename, const char * pArtist, const char * pAlbum, const char * pTitle, const char * pComment, const char * pGenre, const char * pYear, const char * pTrack, BOOL bClearFirst, BOOL bUseOldID3)
{
	CSmartPtr<wchar_t> spFilename(GetUTF16FromANSI(pFilename), TRUE);

	IO_CLASS_NAME FileIO;
	if (FileIO.Open(spFilename) != 0)
		return -1;
	
	CAPETag APETag(&FileIO, TRUE);

	if (bClearFirst)
		APETag.ClearFields();	
	
	APETag.SetFieldString(APE_TAG_FIELD_ARTIST, pArtist, TRUE);
	APETag.SetFieldString(APE_TAG_FIELD_ALBUM, pAlbum, TRUE);
	APETag.SetFieldString(APE_TAG_FIELD_TITLE, pTitle, TRUE);
	APETag.SetFieldString(APE_TAG_FIELD_GENRE, pGenre, TRUE);
	APETag.SetFieldString(APE_TAG_FIELD_YEAR, pYear, TRUE);
	APETag.SetFieldString(APE_TAG_FIELD_COMMENT, pComment, TRUE);
	APETag.SetFieldString(APE_TAG_FIELD_TRACK, pTrack, TRUE);
	
	if (APETag.Save(bUseOldID3) != 0)
	{
		return -1;
	}
	
	return 0;
}

int __stdcall GetID3Tag(const str_ansi * pFilename, ID3_TAG * pID3Tag)
{
	CSmartPtr<wchar_t> spFilename(GetUTF16FromANSI(pFilename), TRUE);

	IO_CLASS_NAME FileIO;
	if (FileIO.Open(spFilename) != 0)
		return -1;
	
	CAPETag APETag(&FileIO, TRUE);
	
	return APETag.CreateID3Tag(pID3Tag);
}

int __stdcall RemoveTag(char * pFilename)
{
	CSmartPtr<wchar_t> spFilename(GetUTF16FromANSI(pFilename), TRUE);

	int nErrorCode = ERROR_SUCCESS;
	CSmartPtr<IAPEDecompress> spAPEDecompress(CreateIAPEDecompress(spFilename, &nErrorCode));
	if (spAPEDecompress == NULL) return -1;
	GET_TAG(spAPEDecompress)->Remove(FALSE);
	return 0;
}

/*****************************************************************************************
CAPEDecompress wrapper(s)
*****************************************************************************************/
APE_DECOMPRESS_HANDLE __stdcall c_APEDecompress_Create(const str_ansi * pFilename, int * pErrorCode)
{
	CSmartPtr<wchar_t> spFilename(GetUTF16FromANSI(pFilename), TRUE);
	return (APE_DECOMPRESS_HANDLE) CreateIAPEDecompress(spFilename, pErrorCode);
}

APE_DECOMPRESS_HANDLE __stdcall c_APEDecompress_CreateW(const str_utf16 * pFilename, int * pErrorCode)
{
	return (APE_DECOMPRESS_HANDLE) CreateIAPEDecompress(pFilename, pErrorCode);
}

void __stdcall c_APEDecompress_Destroy(APE_DECOMPRESS_HANDLE hAPEDecompress)
{
	IAPEDecompress * pAPEDecompress = (IAPEDecompress *) hAPEDecompress;
	if (pAPEDecompress)
		delete pAPEDecompress;
}

int __stdcall c_APEDecompress_GetData(APE_DECOMPRESS_HANDLE hAPEDecompress, char * pBuffer, int nBlocks, int * pBlocksRetrieved)
{
	return ((IAPEDecompress *) hAPEDecompress)->GetData(pBuffer, nBlocks, pBlocksRetrieved);
}

int __stdcall c_APEDecompress_Seek(APE_DECOMPRESS_HANDLE hAPEDecompress, int nBlockOffset)
{
	return ((IAPEDecompress *) hAPEDecompress)->Seek(nBlockOffset);
}

int __stdcall c_APEDecompress_GetInfo(APE_DECOMPRESS_HANDLE hAPEDecompress, APE_DECOMPRESS_FIELDS Field, int nParam1, int nParam2)
{
	return ((IAPEDecompress *) hAPEDecompress)->GetInfo(Field, nParam1, nParam2);
}

/*****************************************************************************************
CAPECompress wrapper(s)
*****************************************************************************************/
APE_COMPRESS_HANDLE __stdcall c_APECompress_Create(int * pErrorCode)
{
	return (APE_COMPRESS_HANDLE) CreateIAPECompress(pErrorCode);
}

void __stdcall c_APECompress_Destroy(APE_COMPRESS_HANDLE hAPECompress)
{
	IAPECompress * pAPECompress = (IAPECompress *) hAPECompress;
	if (pAPECompress)
		delete pAPECompress;
}

int __stdcall c_APECompress_Start(APE_COMPRESS_HANDLE hAPECompress, const char * pOutputFilename, const WAVEFORMATEX * pwfeInput, int nMaxAudioBytes, int nCompressionLevel, const void * pHeaderData, int nHeaderBytes)
{
	CSmartPtr<wchar_t> spOutputFilename(GetUTF16FromANSI(pOutputFilename), TRUE);
	return ((IAPECompress *) hAPECompress)->Start(spOutputFilename, pwfeInput, nMaxAudioBytes, nCompressionLevel, pHeaderData, nHeaderBytes);
}

int __stdcall c_APECompress_StartW(APE_COMPRESS_HANDLE hAPECompress, const str_utf16 * pOutputFilename, const WAVEFORMATEX * pwfeInput, int nMaxAudioBytes, int nCompressionLevel, const void * pHeaderData, int nHeaderBytes)
{
	return ((IAPECompress *) hAPECompress)->Start(pOutputFilename, pwfeInput, nMaxAudioBytes, nCompressionLevel, pHeaderData, nHeaderBytes);
}

int __stdcall c_APECompress_AddData(APE_COMPRESS_HANDLE hAPECompress, unsigned char * pData, int nBytes)
{
	return ((IAPECompress *) hAPECompress)->AddData(pData, nBytes);
}

int __stdcall c_APECompress_GetBufferBytesAvailable(APE_COMPRESS_HANDLE hAPECompress)
{
	return ((IAPECompress *) hAPECompress)->GetBufferBytesAvailable();
}

unsigned char * __stdcall c_APECompress_LockBuffer(APE_COMPRESS_HANDLE hAPECompress, int * pBytesAvailable)
{
	return ((IAPECompress *) hAPECompress)->LockBuffer(pBytesAvailable);
}

int __stdcall c_APECompress_UnlockBuffer(APE_COMPRESS_HANDLE hAPECompress, int nBytesAdded, BOOL bProcess)
{
	return ((IAPECompress *) hAPECompress)->UnlockBuffer(nBytesAdded, bProcess);
}

int __stdcall c_APECompress_Finish(APE_COMPRESS_HANDLE hAPECompress, unsigned char * pTerminatingData, int nTerminatingBytes, int nWAVTerminatingBytes)
{
	return ((IAPECompress *) hAPECompress)->Finish(pTerminatingData, nTerminatingBytes, nWAVTerminatingBytes);
}

int __stdcall c_APECompress_Kill(APE_COMPRESS_HANDLE hAPECompress)
{
	return ((IAPECompress *) hAPECompress)->Kill();
}

CAPETag * __stdcall c_GetAPETag(const str_ansi * pFilename, bool bCheckID3Tag)
{
	CSmartPtr<wchar_t> spFilename(GetUTF16FromANSI(pFilename), TRUE);

	IO_CLASS_NAME FileIO;
	if (FileIO.Open(spFilename) != 0)
		return NULL;

	CAPETag *pAPETag = new CAPETag(&FileIO, TRUE);

	return pAPETag;
}

__int64 __stdcall c_GetAPEDuration(const str_ansi * pFilename)
{
	CSmartPtr<wchar_t> spFilename(GetUTF16FromANSI(pFilename), TRUE);
	int error;
	IAPEDecompress *pDecompress = CreateIAPEDecompress(spFilename, &error);
	if (!pDecompress)
		return 0;
	__int64 ret = pDecompress->GetInfo(APE_INFO_LENGTH_MS, 0, 0);
	delete pDecompress;
	return ret;
}

