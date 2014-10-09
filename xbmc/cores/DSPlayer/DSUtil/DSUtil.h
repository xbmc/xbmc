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
#include "vd.h"
#include "text.h"
#include "utils\StdString.h"


#define LCID_NOSUBTITLES      -1

static const GUID CLSID_NullRenderer =
  { 0xC1F400A4, 0x3F08, 0x11D3, { 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };

DEFINE_GUID(CLSID_ReClock, 
            0x9dc15360, 0x914c, 0x46b8, 0xb9, 0xdf, 0xbf, 0xe6, 0x7f, 0xd3, 0x6c, 0x6a);

#define DNew new

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

extern void setThreadName(DWORD dwThreadID, LPCSTR szThreadName);
extern bool IsPinConnected(IPin* pPin);
extern CStdStringW GetFilterPath(CStdString pClsid);
extern CStdStringW GetFilterPath(CLSID pClsid);
extern CStdStringW GetFilterName(IBaseFilter* pBF);
extern CStdStringA GetPinMainTypeString(IPin* pPin);
extern int  CountPins(IBaseFilter* pBF, int& nIn, int& nOut, int& nInC, int& nOutC);
extern bool IsSplitter(IBaseFilter* pBF, bool fCountConnectedOnly = false);
extern bool IsStreamEnd(IBaseFilter* pBF);
extern bool IsVideoRenderer(IBaseFilter* pBF);
extern bool IsAudioWaveRenderer(IBaseFilter* pBF);
extern HRESULT RemoveUnconnectedFilters(IFilterGraph2 *pGraph);
extern IPin* GetFirstPin(IBaseFilter* pBF, PIN_DIRECTION dir = PINDIR_INPUT);
extern CStdStringW GetPinName(IPin* pPin);
extern void memsetd(void* dst, unsigned int c, int nbytes);
extern void memsetw(void* dst, unsigned short c, size_t nbytes);
extern CStdStringW UTF8To16(LPCSTR utf8);
extern CStdStringA ISO6392ToLanguage(LPCSTR code);
extern CStdStringA ISO6391ToLanguage(LPCSTR code);
extern LCID    ProbeLangForLCID(LPCSTR code);
extern std::string ProbeLangForLanguage(LPCSTR code);
extern LCID    ISO6391ToLcid(LPCSTR code);
extern LCID    ISO6392ToLcid(LPCSTR code);
extern COLORREF YCrCbToRGB_Rec601(BYTE Y, BYTE Cr, BYTE Cb);
extern COLORREF YCrCbToRGB_Rec709(BYTE Y, BYTE Cr, BYTE Cb);
extern DWORD  YCrCbToRGB_Rec601(BYTE A, BYTE Y, BYTE Cr, BYTE Cb);
extern DWORD  YCrCbToRGB_Rec709(BYTE A, BYTE Y, BYTE Cr, BYTE Cb);
extern CStdStringW StringFromGUID(const GUID& guid);
extern IBaseFilter* GetUpStreamFilter(IBaseFilter* pBF, IPin* pInputPin = NULL);
extern IPin* GetUpStreamPin(IBaseFilter* pBF, IPin* pInputPin = NULL);
extern IBaseFilter* GetFilterFromPin(IPin* pPin);
extern IPin* InsertFilter(IPin* pPin, CStdStringW DisplayName, IGraphBuilder* pGB);
extern CLSID GetCLSID(IBaseFilter* pBF);
extern CLSID GetCLSID(IPin* pPin);
extern DVD_HMSF_TIMECODE RT2HMSF(REFERENCE_TIME rt, double fps = 0);
extern REFERENCE_TIME HMSF2RT(DVD_HMSF_TIMECODE hmsf, double fps = 0);
extern bool ExtractBIH(const AM_MEDIA_TYPE* pmt, BITMAPINFOHEADER* bih);
extern bool ExtractBIH(IMediaSample* pMS, BITMAPINFOHEADER* bih);
extern bool ExtractAvgTimePerFrame(const AM_MEDIA_TYPE* pmt, REFERENCE_TIME& rtAvgTimePerFrame);
extern bool ExtractDim(const AM_MEDIA_TYPE* pmt, int& w, int& h, int& arx, int& ary);
extern HRESULT LoadExternalObject(CStdStringW path, REFCLSID clsid, REFIID iid, void** ppv);
extern HRESULT LoadExternalFilter(CStdStringW path, REFCLSID clsid, IBaseFilter** ppBF);
extern HRESULT LoadExternalPropertyPage(IPersist* pP, REFCLSID clsid, IPropertyPage** ppPP);
extern void UnloadExternalObjects();
extern CStdStringW GetMediaTypeName(const GUID& guid);
extern GUID GUIDFromString(CStdStringW str);
extern HRESULT GUIDFromString(CStdStringW str, GUID& guid);
extern int MakeAACInitData(BYTE* pData, int profile, int freq, int channels);
extern const char* GetDXVAMode(const GUID* guidDecoder);
extern CStdStringA GetDshowError(HRESULT hr);

// Conversion
extern CStdStringW  AnsiToUTF16(const CStdStringA strFrom);
extern CStdStringW  AToW(CStdStringA str);
extern CStdStringA  WToA(CStdStringW str);

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

#ifndef CHECK_HR
#define CHECK_HR(hr) if (FAILED(hr)) { goto done; }
#endif

#define MAXULONG64  ((ULONG64)~((ULONG64)0))
#define MAXLONG64   ((LONG64)(MAXULONG64 >> 1))
#define MINLONG64   ((LONG64)~MAXLONG64)