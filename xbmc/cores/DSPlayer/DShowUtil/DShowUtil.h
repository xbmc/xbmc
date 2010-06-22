#pragma once

/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *	    Copyright (C) 2003-2006 Gabest
 *	    http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include <streams.h>

#include <strsafe.h>
#include <assert.h>
#include "ObjBase.h"
#include "H264Nalu.h"

#include "MediaTypeEx.h"
#include "moreuuids.h"
#include "vd.h"
#include <vector>
#include <list>
#include "DShowUtil/SmartPtr.h"
#include "DSGeometry.h"

#ifndef ASSERT
#define ASSERT assert
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

#ifndef CHECK_HR
#define CHECK_HR(hr) if (FAILED(hr)) { goto done; }
#endif

#ifndef CHECK_BOOL_RETURN
#define CHECK_BOOL_RETURN(hr,theerrmsg) if (FAILED(hr)) { CLog::Log(LOGERROR,"%s %s",__FUNCTION__,theerrmsg); return hr;}
#endif

#ifndef CHECK_HR_RETURN
#define CHECK_HR_RETURN(hr,theerrmsg) if (FAILED(hr)) { CLog::Log(LOGERROR,"%s %s",__FUNCTION__,theerrmsg); return hr;}
#endif

#include "DshowCommon.h"
#include "Mpeg2Def.h"

#if _WIN32_WINNT < 0x0600

#define MAXUINT8    ((UINT8)~((UINT8)0))
#define MAXINT8     ((INT8)(MAXUINT8 >> 1))
#define MININT8     ((INT8)~MAXINT8)

#define MAXUINT16   ((UINT16)~((UINT16)0))
#define MAXINT16    ((INT16)(MAXUINT16 >> 1))
#define MININT16    ((INT16)~MAXINT16)

#define MAXUINT32   ((UINT32)~((UINT32)0))
#define MAXINT32    ((INT32)(MAXUINT32 >> 1))
#define MININT32    ((INT32)~MAXINT32)

#define MAXUINT64   ((UINT64)~((UINT64)0))
#define MAXINT64    ((INT64)(MAXUINT64 >> 1))
#define MININT64    ((INT64)~MAXINT64)

#define MAXULONG32  ((ULONG32)~((ULONG32)0))
#define MAXLONG32   ((LONG32)(MAXULONG32 >> 1))
#define MINLONG32   ((LONG32)~MAXLONG32)

#define MAXULONG64  ((ULONG64)~((ULONG64)0))
#define MAXLONG64   ((LONG64)(MAXULONG64 >> 1))
#define MINLONG64   ((LONG64)~MAXLONG64)

#define MAXULONGLONG ((ULONGLONG)~((ULONGLONG)0))
#define MINLONGLONG ((LONGLONG)~MAXLONGLONG)

#define MAXSIZE_T   ((SIZE_T)~((SIZE_T)0))
#define MAXSSIZE_T  ((SSIZE_T)(MAXSIZE_T >> 1))
#define MINSSIZE_T  ((SSIZE_T)~MAXSSIZE_T)

#define MAXUINT     ((UINT)~((UINT)0))
#define MAXINT      ((INT)(MAXUINT >> 1))
#define MININT      ((INT)~MAXINT)

#define MAXDWORD32  ((DWORD32)~((DWORD32)0))
#define MAXDWORD64  ((DWORD64)~((DWORD64)0))

#endif // _WIN32_WINNT < 0x0600

#define DNew new

#define QI(i) (riid == __uuidof(i)) ? GetInterface((i*)this, ppv) :
#define QI2(i) (riid == IID_##i) ? GetInterface((i*)this, ppv) :

#define LCID_NOSUBTITLES      -1

typedef enum {CDROM_NotFound, CDROM_Audio, CDROM_VideoCD, CDROM_DVDVideo, CDROM_Unknown} cdrom_t;

static const GUID CLSID_NullRenderer =
  { 0xC1F400A4, 0x3F08, 0x11D3, { 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };

DEFINE_GUID(CLSID_ReClock, 
            0x9dc15360, 0x914c, 0x46b8, 0xb9, 0xdf, 0xbf, 0xe6, 0x7f, 0xd3, 0x6c, 0x6a);


typedef struct
{
	ULONGLONG DvdGuid;
	ULONG DvdTitleId;
  ULONG DvdChapterId;
	DVD_HMSF_TIMECODE DvdTimecode;
  DVD_DOMAIN DvdDomain;
} DVD_STATUS;

typedef struct
{
  ULONG titleIndex;
  DVD_MenuAttributes menuInfo;
  DVD_TitleAttributes titleInfo;
} DvdTitle;

class  DShowUtil
{
public:
  static const wchar_t *StreamTypeToName(PES_STREAM_TYPE _Type);
  static bool GuidVectItterCompare(std::list<GUID>::iterator it, std::vector<GUID>::const_reference vect);
  static bool GuidItteratorIsNull(std::list<GUID>::iterator it);
  static bool GuidVectIsNull(std::vector<GUID>::const_reference vect);
  static bool IsPinConnected(IPin* pPin);
  static long MFTimeToMsec(const LONGLONG& time);
  static CStdString GetFilterPath(CStdString pClsid);
  static CStdString GetFilterPath(CLSID pClsid);
  static CStdStringW AnsiToUTF16(const CStdString strFrom);
  static int CountPins(IBaseFilter* pBF, int& nIn, int& nOut, int& nInC, int& nOutC);
  static bool IsSplitter(IBaseFilter* pBF, bool fCountConnectedOnly = false);
  static bool IsMultiplexer(IBaseFilter* pBF, bool fCountConnectedOnly = false);
  static bool IsStreamStart(IBaseFilter* pBF);
  static bool IsStreamEnd(IBaseFilter* pBF);
  static bool IsVideoRenderer(IBaseFilter* pBF);
  static bool IsAudioWaveRenderer(IBaseFilter* pBF);
  static std::vector<IMoniker*> GetAudioRenderersGuid();
  static HRESULT RemoveUnconnectedFilters(IFilterGraph2 *pGraph);
  static IBaseFilter* GetUpStreamFilter(IBaseFilter* pBF, IPin* pInputPin = NULL);
  static IPin* GetUpStreamPin(IBaseFilter* pBF, IPin* pInputPin = NULL);
  static IPin* GetFirstPin(IBaseFilter* pBF, PIN_DIRECTION dir = PINDIR_INPUT);
  static IPin* GetFirstDisconnectedPin(IBaseFilter* pBF, PIN_DIRECTION dir);
  static void NukeDownstream(IBaseFilter* pBF, IFilterGraph* pFG);
  static void NukeDownstream(IPin* pPin, IFilterGraph* pFG);
  static IBaseFilter* FindFilter(LPCWSTR clsid, IFilterGraph* pFG);
  static IBaseFilter* FindFilter(const CLSID& clsid, IFilterGraph* pFG);
  static IPin* FindPin(IBaseFilter* pBF, PIN_DIRECTION direction, const AM_MEDIA_TYPE* pRequestedMT);
  static IPin* FindPinMajor(IBaseFilter* pBF, PIN_DIRECTION direction, const AM_MEDIA_TYPE* pRequestedMT);
  static CStdStringW GetFilterName(IBaseFilter* pBF);
  static CStdString GetPinMainTypeString(IPin* pPin);
  static CStdStringW GetPinName(IPin* pPin);
  static IFilterGraph* GetGraphFromFilter(IBaseFilter* pBF);
  static IBaseFilter* GetFilterFromPin(IPin* pPin);
  static IPin* AppendFilter(IPin* pPin, CStdString DisplayName, IGraphBuilder* pGB);
  static IPin* InsertFilter(IPin* pPin, CStdString DisplayName, IGraphBuilder* pGB);
  static void ExtractMediaTypes(IPin* pPin, std::vector<GUID>& types);
  static void ExtractMediaTypes(IPin* pPin, std::list<CMediaType>& mts);
  static CLSID GetCLSID(IBaseFilter* pBF);
  static CLSID GetCLSID(IPin* pPin);
  static bool IsCLSIDRegistered(LPCTSTR clsid);
  static bool IsCLSIDRegistered(const CLSID& clsid);
  static void CStringToBin(CStdString str, std::vector<BYTE>& data);
  static CStdString BinToCString(BYTE* ptr, int len);
  typedef enum {CDROM_NotFound, CDROM_Audio, CDROM_VideoCD, CDROM_DVDVideo, CDROM_Unknown} cdrom_t;
  static CStdString GetDriveLabel(TCHAR drive);
  static DVD_HMSF_TIMECODE RT2HMSF(REFERENCE_TIME rt, double fps = 0);
  static REFERENCE_TIME HMSF2RT(DVD_HMSF_TIMECODE hmsf, double fps = 0);
  static void memsetd(void* dst, unsigned int c, int nbytes);
  static bool ExtractBIH(const AM_MEDIA_TYPE* pmt, BITMAPINFOHEADER* bih);
  static bool ExtractBIH(IMediaSample* pMS, BITMAPINFOHEADER* bih);
  static bool ExtractAvgTimePerFrame(const AM_MEDIA_TYPE* pmt, REFERENCE_TIME& rtAvgTimePerFrame);
  static bool ExtractInterlaced(const AM_MEDIA_TYPE* pmt);
  static bool ExtractDim(const AM_MEDIA_TYPE* pmt, int& w, int& h, int& arx, int& ary);
  static bool MakeMPEG2MediaType(CMediaType& mt, BYTE* seqhdr, DWORD len, int w, int h);
  static unsigned __int64 GetFileVersion(LPCTSTR fn);
  static bool CreateFilter(CStdStringW DisplayName, IBaseFilter** ppBF, CStdStringW& FriendlyName);
  static IBaseFilter* AppendFilter(IPin* pPin, IMoniker* pMoniker, IGraphBuilder* pGB);
  static CStdStringW GetFriendlyName(CStdStringW DisplayName);
  static HRESULT LoadExternalObject(LPCTSTR path, REFCLSID clsid, REFIID iid, void** ppv);
  static HRESULT LoadExternalFilter(LPCTSTR path, REFCLSID clsid, IBaseFilter** ppBF);
  static HRESULT LoadExternalPropertyPage(IPersist* pP, REFCLSID clsid, IPropertyPage** ppPP);
  static void UnloadExternalObjects();
  //Useless anyway
  //static CStdString MakeFullPath(LPCTSTR path);
  static CStdString GetMediaTypeName(const GUID& guid);
  static GUID GUIDFromCString(CStdString str);
  static HRESULT GUIDFromCString(CStdString str, GUID& guid);
  static CStdString CStringFromGUID(const GUID& guid);
  static CStdStringW UTF8To16(LPCSTR utf8);
  static CStdStringA UTF16To8(LPCWSTR utf16);
  static CStdString ISO6391ToLanguage(LPCSTR code);
  static CStdString ISO6392ToLanguage(LPCSTR code);
  static LCID    ISO6391ToLcid(LPCSTR code);
  static LCID    ISO6392ToLcid(LPCSTR code);
  static CStdString ISO6391To6392(LPCSTR code);
  static CStdString ISO6392To6391(LPCSTR code);
  static CStdString LanguageToISO6392(LPCTSTR lang);
  static int MakeAACInitData(BYTE* pData, int profile, int freq, int channels);
  //Need afx for the CFileStatus
  //static BOOL CFileGetStatus(LPCTSTR lpszFileName, CFileStatus& status);
  static bool DeleteRegKey(LPCTSTR pszKey, LPCTSTR pszSubkey);
  static bool SetRegKeyValue(LPCTSTR pszKey, LPCTSTR pszSubkey, LPCTSTR pszValueName, LPCTSTR pszValue);
  static bool SetRegKeyValue(LPCTSTR pszKey, LPCTSTR pszSubkey, LPCTSTR pszValue);
  static void RegisterSourceFilter(const CLSID& clsid, const GUID& subtype2, LPCTSTR chkbytes, LPCTSTR ext = NULL, ...);
  static void RegisterSourceFilter(const CLSID& clsid, const GUID& subtype2, const std::list<CStdString>& chkbytes, LPCTSTR ext = NULL, ...);
  static void UnRegisterSourceFilter(const GUID& subtype);
  static LPCTSTR GetDXVAMode(const GUID* guidDecoder);
  static void DumpBuffer(BYTE* pBuffer, int nSize);
  static CStdString ReftimeToString(const REFERENCE_TIME& rtVal);
  static REFERENCE_TIME StringToReftime(LPCTSTR strVal);
  static COLORREF YCrCbToRGB_Rec601(BYTE Y, BYTE Cr, BYTE Cb);
  static COLORREF YCrCbToRGB_Rec709(BYTE Y, BYTE Cr, BYTE Cb);
  static DWORD  YCrCbToRGB_Rec601(BYTE A, BYTE Y, BYTE Cr, BYTE Cb);
  static DWORD  YCrCbToRGB_Rec709(BYTE A, BYTE Y, BYTE Cr, BYTE Cb);
  static CStdStringW AToW(CStdStringA str);
  static CStdStringA WToA(CStdStringW str);
  static CStdString AToT(CStdStringA str);
  static CStdString WToT(CStdStringW str);
  static CStdStringA TToA(CStdString str);
  static CStdStringW TToW(CStdString str);
};

class DShowVideoInfo
{
public:
  static CStdString GetInfoOnMajorType(IGraphBuilder *pGraphBuilder, const GUID& clsidMajorType);
};

class CPinInfo : public PIN_INFO
{
public:
  CPinInfo() {pFilter = NULL;}
  ~CPinInfo() {if(pFilter) pFilter->Release();}
};

class CFilterInfo : public FILTER_INFO
{
public:
  CFilterInfo() {pGraph = NULL;}
  ~CFilterInfo() {if(pGraph) pGraph->Release();}
};

/* BeginEnumFilters */
#define BeginEnumFilters(pFilterGraph, pEnumFilters, pBaseFilter) \
{Com::SmartPtr<IEnumFilters> pEnumFilters; \
  if(pFilterGraph && SUCCEEDED(pFilterGraph->EnumFilters(&pEnumFilters))) \
{ \
  for(Com::SmartPtr<IBaseFilter> pBaseFilter; S_OK == pEnumFilters->Next(1, &pBaseFilter, 0); pBaseFilter = NULL) \
{ \

#define EndEnumFilters }}}

#define BeginEnumCachedFilters(pGraphConfig, pEnumFilters, pBaseFilter) \
{Com::SmartPtr<IEnumFilters> pEnumFilters; \
  if(pGraphConfig && SUCCEEDED(pGraphConfig->EnumCacheFilter(&pEnumFilters))) \
{ \
  for(Com::SmartPtr<IBaseFilter> pBaseFilter; S_OK == pEnumFilters->Next(1, &pBaseFilter, 0); pBaseFilter = NULL) \
{ \

#define EndEnumCachedFilters }}}

#define BeginEnumPins(pBaseFilter, pEnumPins, pPin) \
{Com::SmartPtr<IEnumPins> pEnumPins; \
  if(pBaseFilter && SUCCEEDED(pBaseFilter->EnumPins(&pEnumPins))) \
{ \
  for(Com::SmartPtr<IPin> pPin; S_OK == pEnumPins->Next(1, &pPin, 0); pPin = NULL) \
{ \

#define EndEnumPins }}}

#define BeginEnumMediaTypes(pPin, pEnumMediaTypes, pMediaType) \
{Com::SmartPtr<IEnumMediaTypes> pEnumMediaTypes; \
  if(pPin && SUCCEEDED(pPin->EnumMediaTypes(&pEnumMediaTypes))) \
{ \
  AM_MEDIA_TYPE* pMediaType = NULL; \
  for(; S_OK == pEnumMediaTypes->Next(1, &pMediaType, NULL); DeleteMediaType(pMediaType), pMediaType = NULL) \
{ \

#define EndEnumMediaTypes(pMediaType) } if(pMediaType) DeleteMediaType(pMediaType); }}

#define BeginEnumSysDev(clsid, pMoniker) \
{Com::SmartPtr<ICreateDevEnum> pDevEnum4$##clsid; \
  pDevEnum4$##clsid.CoCreateInstance(CLSID_SystemDeviceEnum); \
  Com::SmartPtr<IEnumMoniker> pClassEnum4$##clsid; \
  if(SUCCEEDED(pDevEnum4$##clsid->CreateClassEnumerator(clsid, &pClassEnum4$##clsid, 0)) \
  && pClassEnum4$##clsid) \
{ \
  for(Com::SmartPtr<IMoniker> pMoniker; pClassEnum4$##clsid->Next(1, &pMoniker, 0) == S_OK; pMoniker = NULL) \
{ \

#define EndEnumSysDev }}}

#define QI(i) (riid == __uuidof(i)) ? GetInterface((i*)this, ppv) :
#define QI2(i) (riid == IID_##i) ? GetInterface((i*)this, ppv) :

template <typename T> __inline void INITDDSTRUCT(T& dd)
{
    ZeroMemory(&dd, sizeof(dd));
    dd.dwSize = sizeof(dd);
}

#define countof(array) (sizeof(array)/sizeof(array[0]))

template <class T>
static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
  *phr = S_OK;
    CUnknown* punk = DNew T(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
  return punk;
}

template<class T, typename SEP>
T Explode(T str, std::list<T>& sl, SEP sep, size_t limit = 0)
{
	while (sl.empty())
    sl.pop_back();

	for(ptrdiff_t i = 0, j = 0; ; i = j+1)
	{
		j = str.Find(sep, i);

		if(j < 0 || sl.size() == limit-1)
		{
			sl.push_back(str.Mid(i).Trim());
			break;
		}
		else
		{
			sl.push_back(str.Mid(i, j-i).Trim());
		}		
	}

	return sl.front();
}