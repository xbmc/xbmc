/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

// This file is already included in dsplayer. We need to choose what header to use!

#pragma once

#include "NullRenderers.h"
#include "HdmvClipInfo.h"
#include "H264Nalu.h"
//#include "MediaTypes.h"
#include "MediaTypeEx.h"
#include "vd.h"
#include "text.h"

#define LCID_NOSUBTITLES      -1

/*struct CFileStatus
{
  CTime m_ctime;          // creation date/time of file
  CTime m_mtime;          // last modification date/time of file
  CTime m_atime;          // last access date/time of file
  ULONGLONG m_size;            // logical size of file in bytes
  BYTE m_attribute;       // logical OR of CFile::Attribute enum values
  BYTE _m_padding;        // pad the structure to a WORD
  TCHAR m_szFullName[_MAX_PATH]; // absolute path name

#ifdef _DEBUG
  void Dump(CDumpContext& dc) const;
#endif
};*/

extern void DumpStreamConfig(TCHAR* fn, IAMStreamConfig* pAMVSCCap);
extern int  CountPins(IBaseFilter* pBF, int& nIn, int& nOut, int& nInC, int& nOutC);
extern bool IsSplitter(IBaseFilter* pBF, bool fCountConnectedOnly = false);
extern bool IsMultiplexer(IBaseFilter* pBF, bool fCountConnectedOnly = false);
extern bool IsStreamStart(IBaseFilter* pBF);
extern bool IsStreamEnd(IBaseFilter* pBF);
extern bool IsVideoRenderer(IBaseFilter* pBF);
extern bool IsAudioWaveRenderer(IBaseFilter* pBF);
extern IBaseFilter* GetUpStreamFilter(IBaseFilter* pBF, IPin* pInputPin = NULL);
extern IPin* GetUpStreamPin(IBaseFilter* pBF, IPin* pInputPin = NULL);
extern IPin* GetFirstPin(IBaseFilter* pBF, PIN_DIRECTION dir = PINDIR_INPUT);
extern IPin* GetFirstDisconnectedPin(IBaseFilter* pBF, PIN_DIRECTION dir);
extern void NukeDownstream(IBaseFilter* pBF, IFilterGraph* pFG);
extern void NukeDownstream(IPin* pPin, IFilterGraph* pFG);
extern IBaseFilter* FindFilter(LPCWSTR clsid, IFilterGraph* pFG);
extern IBaseFilter* FindFilter(const CLSID& clsid, IFilterGraph* pFG);
extern CStdStringW GetFilterName(IBaseFilter* pBF);
extern CStdStringW GetPinName(IPin* pPin);
extern IFilterGraph* GetGraphFromFilter(IBaseFilter* pBF);
extern IBaseFilter* GetFilterFromPin(IPin* pPin);
extern IPin* AppendFilter(IPin* pPin, CStdString DisplayName, IGraphBuilder* pGB);
extern IPin* InsertFilter(IPin* pPin, CStdString DisplayName, IGraphBuilder* pGB);
/*extern void ExtractMediaTypes(IPin* pPin, std::vector<GUID>& types);
extern void ExtractMediaTypes(IPin* pPin, std::list<CMediaType>& mts);*/
extern void ShowPPage(CStdString DisplayName, HWND hParentWnd);
extern void ShowPPage(IUnknown* pUnknown, HWND hParentWnd);
extern CLSID GetCLSID(IBaseFilter* pBF);
extern CLSID GetCLSID(IPin* pPin);
extern bool IsCLSIDRegistered(LPCTSTR clsid);
extern bool IsCLSIDRegistered(const CLSID& clsid);
extern void CStdStringToBin(CStdString str, std::vector<BYTE>& data);
extern CStdString BinToCStdString(BYTE* ptr, int len);
typedef enum {CDROM_NotFound, CDROM_Audio, CDROM_VideoCD, CDROM_DVDVideo, CDROM_Unknown} cdrom_t;
extern cdrom_t GetCDROMType(TCHAR drive, std::list<CStdString>& files);
extern CStdString GetDriveLabel(TCHAR drive);
extern bool GetKeyFrames(CStdString fn, std::vector<UINT>& kfs);
extern DVD_HMSF_TIMECODE RT2HMSF(REFERENCE_TIME rt, double fps = 0);
extern REFERENCE_TIME HMSF2RT(DVD_HMSF_TIMECODE hmsf, double fps = 0);
extern void memsetd(void* dst, unsigned int c, int nbytes);
extern bool ExtractBIH(const AM_MEDIA_TYPE* pmt, BITMAPINFOHEADER* bih);
extern bool ExtractBIH(IMediaSample* pMS, BITMAPINFOHEADER* bih);
extern bool ExtractAvgTimePerFrame(const AM_MEDIA_TYPE* pmt, REFERENCE_TIME& rtAvgTimePerFrame);
extern bool ExtractDim(const AM_MEDIA_TYPE* pmt, int& w, int& h, int& arx, int& ary);
extern bool MakeMPEG2MediaType(CMediaType& mt, BYTE* seqhdr, DWORD len, int w, int h);
extern unsigned __int64 GetFileVersion(LPCTSTR fn);
extern bool CreateFilter(CStdStringW DisplayName, IBaseFilter** ppBF, CStdStringW& FriendlyName);
extern IBaseFilter* AppendFilter(IPin* pPin, IMoniker* pMoniker, IGraphBuilder* pGB);
extern CStdStringW GetFriendlyName(CStdStringW DisplayName);
/*extern HRESULT LoadExternalObject(LPCTSTR path, REFCLSID clsid, REFIID iid, void** ppv);
extern HRESULT LoadExternalFilter(LPCTSTR path, REFCLSID clsid, IBaseFilter** ppBF);
extern HRESULT LoadExternalPropertyPage(IPersist* pP, REFCLSID clsid, IPropertyPage** ppPP);
extern void UnloadExternalObjects();*/
extern CStdString MakeFullPath(LPCTSTR path);
extern CStdString GetMediaTypeName(const GUID& guid);
extern GUID GUIDFromCStdString(CStdString str);
extern HRESULT GUIDFromCStdString(CStdString str, GUID& guid);
extern CStdString CStdStringFromGUID(const GUID& guid);
extern CStdStringW UTF8To16(LPCSTR utf8);
extern CStdStringA UTF16To8(LPCWSTR utf16);
extern CStdString ISO6391ToLanguage(LPCSTR code);
extern CStdString ISO6392ToLanguage(LPCSTR code);
extern LCID    ISO6391ToLcid(LPCSTR code);
extern LCID    ISO6392ToLcid(LPCSTR code);
extern CStdString ISO6391To6392(LPCSTR code);
extern CStdString ISO6392To6391(LPCSTR code);
extern CStdString LanguageToISO6392(LPCTSTR lang);
extern int MakeAACInitData(BYTE* pData, int profile, int freq, int channels);
//extern BOOL CFileGetStatus(LPCTSTR lpszFileName, CFileStatus& status);
extern bool DeleteRegKey(LPCTSTR pszKey, LPCTSTR pszSubkey);
extern bool SetRegKeyValue(LPCTSTR pszKey, LPCTSTR pszSubkey, LPCTSTR pszValueName, LPCTSTR pszValue);
extern bool SetRegKeyValue(LPCTSTR pszKey, LPCTSTR pszSubkey, LPCTSTR pszValue);
extern void RegisterSourceFilter(const CLSID& clsid, const GUID& subtype2, LPCTSTR chkbytes, LPCTSTR ext = NULL, ...);
extern void RegisterSourceFilter(const CLSID& clsid, const GUID& subtype2, const std::list<CStdString>& chkbytes, LPCTSTR ext = NULL, ...);
extern void UnRegisterSourceFilter(const GUID& subtype);
extern LPCTSTR GetDXVAMode(const GUID* guidDecoder);
extern void DumpBuffer(BYTE* pBuffer, int nSize);
extern CStdString ReftimeToString(const REFERENCE_TIME& rtVal);
REFERENCE_TIME StringToReftime(LPCTSTR strVal);
extern COLORREF YCrCbToRGB_Rec601(BYTE Y, BYTE Cr, BYTE Cb);
extern COLORREF YCrCbToRGB_Rec709(BYTE Y, BYTE Cr, BYTE Cb);
extern DWORD  YCrCbToRGB_Rec601(BYTE A, BYTE Y, BYTE Cr, BYTE Cb);
extern DWORD  YCrCbToRGB_Rec709(BYTE A, BYTE Y, BYTE Cr, BYTE Cb);

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

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) if(p) { p->Release(); p = NULL; }
#endif

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
