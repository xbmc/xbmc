/*****************************************************************************************
Monkey's Audio MACDll.h (include for using MACDll.dll in your projects)
Copyright (C) 2000-2004 by Matthew T. Ashland   All Rights Reserved.

Overview:

Basically all this dll does is wrap MACLib.lib, so browse through MACLib.h for documentation
on how to use the interfaces.

Questions / Suggestions:

Please direct questions or comments to the Monkey's Audio developers board:
	http://www.monkeysaudio.com/cgi-bin/YaBB/YaBB.cgi -> Developers
or, if necessary, matt @ monkeysaudio.com
*****************************************************************************************/

#ifndef APE_MACDLL_H
#define APE_MACDLL_H

/*****************************************************************************************
Includes
*****************************************************************************************/
#include "All.h"
#include "MACLib.h"

/*****************************************************************************************
Defines (implemented elsewhere)
*****************************************************************************************/
struct ID3_TAG;

/*****************************************************************************************
Helper functions
*****************************************************************************************/
extern "C"
{
	__declspec( dllexport ) int __stdcall GetVersionNumber();
	__declspec( dllexport ) int __stdcall GetInterfaceCompatibility(int nVersion, BOOL bDisplayWarningsOnFailure = TRUE, HWND hwndParent = NULL);
	__declspec( dllexport ) int __stdcall ShowFileInfoDialog(const str_ansi * pFilename, HWND hwndWindow);
	__declspec( dllexport ) int __stdcall TagFileSimple(const str_ansi * pFilename, const char * pArtist, const char * pAlbum, const char * pTitle, const char * pComment, const char * pGenre, const char * pYear, const char * pTrack, BOOL bClearFirst, BOOL bUseOldID3);
	__declspec( dllexport ) int __stdcall GetID3Tag(const str_ansi * pFilename, ID3_TAG * pID3Tag);
	__declspec( dllexport ) int __stdcall RemoveTag(const str_ansi * pFilename);
}

typedef int (__stdcall * proc_GetVersionNumber)();
typedef int (__stdcall * proc_GetInterfaceCompatibility)(int, BOOL, HWND);

/*****************************************************************************************
IAPECompress wrapper(s)
*****************************************************************************************/
typedef void * APE_COMPRESS_HANDLE;

typedef APE_COMPRESS_HANDLE (__stdcall * proc_APECompress_Create)(int *); 
typedef void (__stdcall * proc_APECompress_Destroy)(APE_COMPRESS_HANDLE); 
typedef int (__stdcall * proc_APECompress_Start)(APE_COMPRESS_HANDLE, const char *, const WAVEFORMATEX *, int, int, const void *, int);
typedef int (__stdcall * proc_APECompress_StartW)(APE_COMPRESS_HANDLE, const char *, const WAVEFORMATEX *, int, int, const void *, int);
typedef int (__stdcall * proc_APECompress_AddData)(APE_COMPRESS_HANDLE, unsigned char *, int);
typedef int (__stdcall * proc_APECompress_GetBufferBytesAvailable)(APE_COMPRESS_HANDLE);
typedef unsigned char * (__stdcall * proc_APECompress_LockBuffer)(APE_COMPRESS_HANDLE, int *);
typedef int (__stdcall * proc_APECompress_UnlockBuffer)(APE_COMPRESS_HANDLE, int, BOOL);
typedef int (__stdcall * proc_APECompress_Finish)(APE_COMPRESS_HANDLE, unsigned char *, int, int);
typedef int (__stdcall * proc_APECompress_Kill)(APE_COMPRESS_HANDLE);

extern "C"
{
	__declspec( dllexport ) APE_COMPRESS_HANDLE __stdcall c_APECompress_Create(int * pErrorCode = NULL);
	__declspec( dllexport ) void __stdcall c_APECompress_Destroy(APE_COMPRESS_HANDLE hAPECompress);
	__declspec( dllexport ) int __stdcall c_APECompress_Start(APE_COMPRESS_HANDLE hAPECompress, const char * pOutputFilename, const WAVEFORMATEX * pwfeInput, int nMaxAudioBytes = MAX_AUDIO_BYTES_UNKNOWN, int nCompressionLevel = COMPRESSION_LEVEL_NORMAL, const unsigned char * pHeaderData = NULL, int nHeaderBytes = CREATE_WAV_HEADER_ON_DECOMPRESSION);
	__declspec( dllexport ) int __stdcall c_APECompress_StartW(APE_COMPRESS_HANDLE hAPECompress, const str_utf16 * pOutputFilename, const WAVEFORMATEX * pwfeInput, int nMaxAudioBytes = MAX_AUDIO_BYTES_UNKNOWN, int nCompressionLevel = COMPRESSION_LEVEL_NORMAL, const unsigned char * pHeaderData = NULL, int nHeaderBytes = CREATE_WAV_HEADER_ON_DECOMPRESSION);
	__declspec( dllexport ) int __stdcall c_APECompress_AddData(APE_COMPRESS_HANDLE hAPECompress, unsigned char * pData, int nBytes);
	__declspec( dllexport ) int __stdcall c_APECompress_GetBufferBytesAvailable(APE_COMPRESS_HANDLE hAPECompress);
	__declspec( dllexport ) unsigned char * __stdcall c_APECompress_LockBuffer(APE_COMPRESS_HANDLE hAPECompress, int * pBytesAvailable);
	__declspec( dllexport )	int __stdcall c_APECompress_UnlockBuffer(APE_COMPRESS_HANDLE hAPECompress, int nBytesAdded, BOOL bProcess = TRUE);
	__declspec( dllexport )	int __stdcall c_APECompress_Finish(APE_COMPRESS_HANDLE hAPECompress, unsigned char * pTerminatingData, int nTerminatingBytes, int nWAVTerminatingBytes);
	__declspec( dllexport )	int __stdcall c_APECompress_Kill(APE_COMPRESS_HANDLE hAPECompress);
}

/*****************************************************************************************
IAPEDecompress wrapper(s)
*****************************************************************************************/
typedef void * APE_DECOMPRESS_HANDLE;

typedef APE_DECOMPRESS_HANDLE (__stdcall * proc_APEDecompress_Create)(const char *, int *); 
typedef APE_DECOMPRESS_HANDLE (__stdcall * proc_APEDecompress_CreateW)(const char *, int *); 
typedef void (__stdcall * proc_APEDecompress_Destroy)(APE_DECOMPRESS_HANDLE); 
typedef int (__stdcall * proc_APEDecompress_GetData)(APE_DECOMPRESS_HANDLE, char *, int, int *); 
typedef int (__stdcall * proc_APEDecompress_Seek)(APE_DECOMPRESS_HANDLE, int); 
typedef int (__stdcall * proc_APEDecompress_GetInfo)(APE_DECOMPRESS_HANDLE, APE_DECOMPRESS_FIELDS, int, int); 

extern "C"
{
	__declspec( dllexport ) APE_DECOMPRESS_HANDLE __stdcall c_APEDecompress_Create(const str_ansi * pFilename, int * pErrorCode = NULL);
	__declspec( dllexport ) APE_DECOMPRESS_HANDLE __stdcall c_APEDecompress_CreateW(const str_utf16 * pFilename, int * pErrorCode = NULL);
	__declspec( dllexport ) void __stdcall c_APEDecompress_Destroy(APE_DECOMPRESS_HANDLE hAPEDecompress);
	__declspec( dllexport ) int __stdcall c_APEDecompress_GetData(APE_DECOMPRESS_HANDLE hAPEDecompress, char * pBuffer, int nBlocks, int * pBlocksRetrieved);
	__declspec( dllexport ) int __stdcall c_APEDecompress_Seek(APE_DECOMPRESS_HANDLE hAPEDecompress, int nBlockOffset);
	__declspec( dllexport ) int __stdcall c_APEDecompress_GetInfo(APE_DECOMPRESS_HANDLE hAPEDecompress, APE_DECOMPRESS_FIELDS Field, int nParam1 = 0, int nParam2 = 0);
}

#endif // #ifndef APE_MACDLL_H
