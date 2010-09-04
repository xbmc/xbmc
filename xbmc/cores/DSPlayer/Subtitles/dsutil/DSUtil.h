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

#define LCID_NOSUBTITLES      -1

extern int  CountPins(IBaseFilter* pBF, int& nIn, int& nOut, int& nInC, int& nOutC);
extern bool IsSplitter(IBaseFilter* pBF, bool fCountConnectedOnly = false);
extern IPin* GetFirstPin(IBaseFilter* pBF, PIN_DIRECTION dir = PINDIR_INPUT);
extern CStdStringW GetPinName(IPin* pPin);
extern void memsetd(void* dst, unsigned int c, int nbytes);
extern void memsetw(void* dst, unsigned short c, size_t nbytes);
extern CStdStringW UTF8To16(LPCSTR utf8);
extern CStdString ISO6392ToLanguage(LPCSTR code);
extern LCID    ISO6391ToLcid(LPCSTR code);
extern LCID    ISO6392ToLcid(LPCSTR code);
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
