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

#include "stdafx.h"
#include <Vfw.h>
#include "DSUtil.h"
#include "DshowCommon.h"
#include <moreuuids.h>

#include <initguid.h>
#include <d3dx9.h>
#include <dxva.h>
#include <dxva2api.h>

#pragma comment(lib, "Quartz.lib")

void setThreadName(DWORD dwThreadID,LPCSTR szThreadName)
{
#ifdef _DEBUG
 struct THREADNAME_INFO
  {
   DWORD dwType;     // must be 0x1000
   LPCSTR szName;    // pointer to name (in user addr space)
   DWORD dwThreadID; // thread ID (-1=caller thread)
   DWORD dwFlags;    // reserved for future use, must be zero
  } info;
 info.dwType=0x1000;
 info.szName=szThreadName;
 info.dwThreadID=dwThreadID;
 info.dwFlags=0;
 __try
  {
   RaiseException(0x406D1388,0,sizeof(info)/sizeof(DWORD),(ULONG_PTR*)&info);
  }
 __except(EXCEPTION_CONTINUE_EXECUTION)
  {
  }
#endif
}

/*template<class T, typename SEP>
T Explode(T str, std::list<T>& sl, SEP sep, size_t limit/* = 0/)
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
      sl.push_back(str.Mid(i, j-i).Trim());
  }

  return sl.front();
}*/

CStdStringW AnsiToUTF16(const CStdStringA strFrom)
{
  CStdStringW strTo;
  int nLen = MultiByteToWideChar(CP_ACP, 0, strFrom.c_str(), -1, NULL, NULL);
  wchar_t* strBuffer = strTo.GetBuffer(nLen + 1);
  strBuffer[0] = 0;
  MultiByteToWideChar(CP_ACP, 0, strFrom.c_str(), -1, strBuffer, nLen);
  strTo.ReleaseBuffer();
  return strTo;
}

bool IsPinConnected(IPin* pPin)
{
  CheckPointer(pPin, false);

  Com::SmartPtr<IPin> pPinTo;
  return ((SUCCEEDED(pPin->ConnectedTo(&pPinTo))) && pPinTo) ? true : false;
}

CStdStringW StringFromGUID(const GUID& guid)
{
  WCHAR null[128] = {0}, buff[128];
  StringFromGUID2(GUID_NULL, null, 127);
  return CStdStringW(StringFromGUID2(guid, buff, 127) > 0 ? buff : null);
}

HRESULT GUIDFromString(CStdStringW str, GUID& guid)
{
  guid = GUID_NULL;
  return CLSIDFromString(LPOLESTR(str.c_str()), &guid);
}

CStdStringW GetFilterPath(CLSID pClsid)
{
  CStdString strGuidD;
  strGuidD = StringFromGUID(pClsid);
  return GetFilterPath(strGuidD);
}

CStdStringA GetPinMainTypeString(IPin* pPin)
{
  AM_MEDIA_TYPE pMediaType;
  pPin->ConnectionMediaType(&pMediaType);
  if (pMediaType.majortype == MEDIATYPE_Audio)
    return CStdStringA("Audio pin");
  if (pMediaType.majortype == MEDIATYPE_Video)
    return CStdStringA("Video pin");
  if (pMediaType.majortype == MEDIATYPE_Text)
    return CStdStringA("Text pin");

  return CStdStringA("");
}

IBaseFilter* GetUpStreamFilter(IBaseFilter* pBF, IPin* pInputPin)
{
  return GetFilterFromPin(GetUpStreamPin(pBF, pInputPin));
}

IPin* GetUpStreamPin(IBaseFilter* pBF, IPin* pInputPin)
{
  BeginEnumPins(pBF, pEP, pPin)
  {
    if(pInputPin && pInputPin != pPin) continue;

    PIN_DIRECTION dir;
    Com::SmartPtr<IPin> pPinConnectedTo;
    if(SUCCEEDED(pPin->QueryDirection(&dir)) && dir == PINDIR_INPUT
      && SUCCEEDED(pPin->ConnectedTo(&pPinConnectedTo)))
    {
      IPin* pRet = pPinConnectedTo.Detach();
      pRet->Release();
      return(pRet);
    }
  }
  EndEnumPins

  return(NULL);
}

IBaseFilter* GetFilterFromPin(IPin* pPin)
{
  if(!pPin) return NULL;
  IBaseFilter* pBF = NULL;
  CPinInfo pi;
  if(pPin && SUCCEEDED(pPin->QueryPinInfo(&pi)))
    pBF = pi.pFilter;
  return(pBF);
}

IPin* InsertFilter(IPin* pPin, CStdStringW DisplayName, IGraphBuilder* pGB)
{
  do
  {
    if(!pPin || DisplayName.IsEmpty() || !pGB)
      break;

    PIN_DIRECTION dir;
    if(FAILED(pPin->QueryDirection(&dir)))
      break;

    IPin* pFrom;
    IPin* pTo;

    if(dir == PINDIR_INPUT)
    {
      pPin->ConnectedTo(&pFrom);
      pTo = pPin;
    }
    else if(dir == PINDIR_OUTPUT)
    {
      pFrom = pPin;
      pPin->ConnectedTo(&pTo);
    }
    
    if(!pFrom || !pTo)
      break;

    IBindCtx* pBindCtx;
    CreateBindCtx(0, &pBindCtx);

    IMoniker* pMoniker;
    ULONG chEaten;
    if(S_OK != MkParseDisplayName(pBindCtx, LPCOLESTR(DisplayName.c_str()), &chEaten, &pMoniker))
      break;

    IBaseFilter* pBF;
    if(FAILED(pMoniker->BindToObject(pBindCtx, 0, IID_IBaseFilter, (void**)&pBF)) || !pBF)
      break;

    IPropertyBag* pPB;
    if(FAILED(pMoniker->BindToStorage(pBindCtx, 0, IID_IPropertyBag, (void**)&pPB)))
      break;

    VARIANT var;
    if(FAILED(pPB->Read(LPCOLESTR(L"FriendlyName"), &var, NULL)))
      break;

    if(FAILED(pGB->AddFilter(pBF, CStdStringW(var.bstrVal))))
      break;

    IPin* pFromTo = GetFirstPin(pBF, PINDIR_INPUT);
    if(!pFromTo)
    {
      pGB->RemoveFilter(pBF);
      break;
    }

    if(FAILED(pGB->Disconnect(pFrom)) || FAILED(pGB->Disconnect(pTo)))
    {
      pGB->RemoveFilter(pBF);
      pGB->ConnectDirect(pFrom, pTo, NULL);
      break;
    }

    HRESULT hr;
    if(FAILED(hr = pGB->ConnectDirect(pFrom, pFromTo, NULL)))
    {
      pGB->RemoveFilter(pBF);
      pGB->ConnectDirect(pFrom, pTo, NULL);
      break;
    }

    IPin* pToFrom = GetFirstPin(pBF, PINDIR_OUTPUT);
    if(!pToFrom)
    {
      pGB->RemoveFilter(pBF);
      pGB->ConnectDirect(pFrom, pTo, NULL);
      break;
    }

    if(FAILED(pGB->ConnectDirect(pToFrom, pTo, NULL)))
    {
      pGB->RemoveFilter(pBF);
      pGB->ConnectDirect(pFrom, pTo, NULL);
      break;
    }

    pPin = pToFrom;
  }
  while(false);

  return(pPin);
}

DVD_HMSF_TIMECODE RT2HMSF(REFERENCE_TIME rt, double fps)
{
  DVD_HMSF_TIMECODE hmsf = 
  {
    (BYTE)((rt / 10000000 / 60 / 60)),
    (BYTE)((rt / 10000000 / 60 ) % 60),
    (BYTE)((rt / 10000000) % 60),
    (BYTE)(1.0 * ((rt / 10000) % 1000) * fps / 1000)
  };

  return hmsf;
}

REFERENCE_TIME HMSF2RT(DVD_HMSF_TIMECODE hmsf, double fps)
{
  if(fps == 0) 
  {
    hmsf.bFrames = 0; 
    fps = 1;
  }
  return (REFERENCE_TIME)((((REFERENCE_TIME)hmsf.bHours * 60 + hmsf.bMinutes)
    * 60 + hmsf.bSeconds) * 1000
    + 1.0 * hmsf.bFrames * 1000 / fps) * 10000;
}

bool ExtractBIH(const AM_MEDIA_TYPE* pmt, BITMAPINFOHEADER* bih)
{
  if(pmt && bih)
  {
    memset(bih, 0, sizeof(*bih));

    if(pmt->formattype == FORMAT_VideoInfo)
    {
      VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->pbFormat;
      memcpy(bih, &vih->bmiHeader, sizeof(BITMAPINFOHEADER));
      return true;
    }
    else if(pmt->formattype == FORMAT_VideoInfo2)
    {
      VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pmt->pbFormat;
      memcpy(bih, &vih->bmiHeader, sizeof(BITMAPINFOHEADER));
      return true;
    }
    else if(pmt->formattype == FORMAT_MPEGVideo)
    {
      VIDEOINFOHEADER* vih = &((MPEG1VIDEOINFO*)pmt->pbFormat)->hdr;
      memcpy(bih, &vih->bmiHeader, sizeof(BITMAPINFOHEADER));
      return true;
    }
    else if(pmt->formattype == FORMAT_MPEG2_VIDEO)
    {
      VIDEOINFOHEADER2* vih = &((MPEG2VIDEOINFO*)pmt->pbFormat)->hdr;
      memcpy(bih, &vih->bmiHeader, sizeof(BITMAPINFOHEADER));
      return true;
    }
    else if(pmt->formattype == FORMAT_DiracVideoInfo)
    {
      VIDEOINFOHEADER2* vih = &((DIRACINFOHEADER*)pmt->pbFormat)->hdr;
      memcpy(bih, &vih->bmiHeader, sizeof(BITMAPINFOHEADER));
      return true;
    }
  }
  
  return false;
}

bool ExtractBIH(IMediaSample* pMS, BITMAPINFOHEADER* bih)
{
  AM_MEDIA_TYPE* pmt = NULL;
  pMS->GetMediaType(&pmt);
  if(pmt)
  {
    bool fRet = ExtractBIH(pmt, bih);
    DeleteMediaType(pmt);
    return(fRet);
  }
  
  return(false);
}

bool ExtractAvgTimePerFrame(const AM_MEDIA_TYPE* pmt, REFERENCE_TIME& rtAvgTimePerFrame)
{
  if (pmt->formattype==FORMAT_VideoInfo)
    rtAvgTimePerFrame = ((VIDEOINFOHEADER*)pmt->pbFormat)->AvgTimePerFrame;
  else if (pmt->formattype==FORMAT_VideoInfo2)
    rtAvgTimePerFrame = ((VIDEOINFOHEADER2*)pmt->pbFormat)->AvgTimePerFrame;
  else if (pmt->formattype==FORMAT_MPEGVideo)
    rtAvgTimePerFrame = ((MPEG1VIDEOINFO*)pmt->pbFormat)->hdr.AvgTimePerFrame;
  else if (pmt->formattype==FORMAT_MPEG2Video)
    rtAvgTimePerFrame = ((MPEG2VIDEOINFO*)pmt->pbFormat)->hdr.AvgTimePerFrame;
  else
    return false;

  return true;
}

bool ExtractDim(const AM_MEDIA_TYPE* pmt, int& w, int& h, int& arx, int& ary)
{
  w = h = arx = ary = 0;

  if(pmt->formattype == FORMAT_VideoInfo || pmt->formattype == FORMAT_MPEGVideo)
  {
    VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->pbFormat;
    w = vih->bmiHeader.biWidth;
    h = abs(vih->bmiHeader.biHeight);
    arx = w * vih->bmiHeader.biYPelsPerMeter;
    ary = h * vih->bmiHeader.biXPelsPerMeter;
  }
  else if(pmt->formattype == FORMAT_VideoInfo2 || pmt->formattype == FORMAT_MPEG2_VIDEO || pmt->formattype == FORMAT_DiracVideoInfo)
  {
    VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pmt->pbFormat;
    w = vih->bmiHeader.biWidth;
    h = abs(vih->bmiHeader.biHeight);
    arx = vih->dwPictAspectRatioX;
    ary = vih->dwPictAspectRatioY;
  }
  else
  {
    return(false);
  }

  if(!arx || !ary)
  {
    arx = w;
    ary = h;
  }

  BYTE* ptr = NULL;
  DWORD len = 0;

  if(pmt->formattype == FORMAT_MPEGVideo)
  {
    ptr = ((MPEG1VIDEOINFO*)pmt->pbFormat)->bSequenceHeader;
    len = ((MPEG1VIDEOINFO*)pmt->pbFormat)->cbSequenceHeader;

    if(ptr && len >= 8 && *(DWORD*)ptr == 0xb3010000)
    {
      w = (ptr[4]<<4)|(ptr[5]>>4);
      h = ((ptr[5]&0xf)<<8)|ptr[6];
      float ar[] = 
      {
        1.0000f,1.0000f,0.6735f,0.7031f,
        0.7615f,0.8055f,0.8437f,0.8935f,
        0.9157f,0.9815f,1.0255f,1.0695f,
        1.0950f,1.1575f,1.2015f,1.0000f,
      };
      arx = (int)((float)w / ar[ptr[7]>>4] + 0.5);
      ary = h;
    }
  }
  else if(pmt->formattype == FORMAT_MPEG2_VIDEO)
  {
    ptr = (BYTE*)((MPEG2VIDEOINFO*)pmt->pbFormat)->dwSequenceHeader; 
    len = ((MPEG2VIDEOINFO*)pmt->pbFormat)->cbSequenceHeader;

    if(ptr && len >= 8 && *(DWORD*)ptr == 0xb3010000)
    {
      w = (ptr[4]<<4)|(ptr[5]>>4);
      h = ((ptr[5]&0xf)<<8)|ptr[6];
      struct {int x, y;} ar[] = {{w,h},{4,3},{16,9},{221,100},{w,h}};
      int i = min(max(ptr[7]>>4, 1), 5)-1;
      arx = ar[i].x;
      ary = ar[i].y;
    }
  }

  if(ptr && len >= 8)
  {

  }

  DWORD a = arx, b = ary;
    while(a) {int tmp = a; a = b % tmp; b = tmp;}
  if(b) arx /= b, ary /= b;

  return(true);
}

typedef struct
{
  CStdStringW path;
  HINSTANCE hInst;
  CLSID clsid;
} ExternalObject;

static std::vector<ExternalObject> s_extobjs;

HRESULT LoadExternalObject(CStdStringW path, REFCLSID clsid, REFIID iid, void** ppv)
{
  CheckPointer(ppv, E_POINTER);

  HINSTANCE hInst = NULL;
  bool fFound = false;

  for (std::vector<ExternalObject>::iterator it = s_extobjs.begin() ; it != s_extobjs.end(); it++)
  {
    if(!(*it).path.CompareNoCase(path))
    {
      hInst = (*it).hInst;
      fFound = true;
      break;
    }
  }

  HRESULT hr = E_FAIL;
  if(hInst || (hInst = CoLoadLibrary(_bstr_t(path), TRUE)))
  {
    typedef HRESULT (__stdcall * PDllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
    PDllGetClassObject p = (PDllGetClassObject)GetProcAddress(hInst, "DllGetClassObject");

    if(p && FAILED(hr = p(clsid, iid, ppv)))
    {
      Com::SmartPtr<IClassFactory> pCF;
      if(SUCCEEDED(hr = p(clsid, __uuidof(IClassFactory), (void**)&pCF)))
      {
        hr = pCF->CreateInstance(NULL, iid, ppv);
        /*if (FAILED(hr))
          CLog::Log(LOGERROR, "%s CreateInstance on IClassFactory failed! (hr : 0x%X). If you use ffdshow, please check that XBMC is on the white list!", __FUNCTION__, hr);*/
      }
    }
  }
  /*else 
    CLog::Log(LOGDEBUG, "%s CoLoadLibrary fails : %x", __FUNCTION__, GetLastError());*/

  if(FAILED(hr) && hInst && !fFound)
  {
    CoFreeLibrary(hInst);
    return hr;
  }

  if(hInst && !fFound)
  {
    ExternalObject eo;
    eo.path = path;
    eo.hInst = hInst;
    eo.clsid = clsid;
    s_extobjs.push_back(eo);
  }

  return hr;
}

HRESULT LoadExternalFilter(CStdStringW path, REFCLSID clsid, IBaseFilter** ppBF)
{
  return LoadExternalObject(path, clsid, __uuidof(IBaseFilter), (void**)ppBF);
}

HRESULT LoadExternalPropertyPage(IPersist* pP, REFCLSID clsid, IPropertyPage** ppPP)
{
  CLSID clsid2 = GUID_NULL;
  if(FAILED(pP->GetClassID(&clsid2))) return E_FAIL;

  for (std::vector<ExternalObject>::iterator it = s_extobjs.begin() ; it != s_extobjs.end(); it++)
  {
    if((*it).clsid == clsid2)
    {
      return LoadExternalObject((*it).path, clsid, __uuidof(IPropertyPage), (void**)ppPP);
    }
  }

  return E_FAIL;
}

void UnloadExternalObjects()
{
  int i = 1;
  for (std::vector<ExternalObject>::iterator it = s_extobjs.begin() ; it != s_extobjs.end(); it++, i++)
  {
    CoFreeLibrary((*it).hInst);
  }
  //CLog::Log(LOGDEBUG, "%s Filters unloaded : %d", __FUNCTION__, i);

  while (!s_extobjs.empty())
    s_extobjs.pop_back();
}

GUID GUIDFromString(CStdStringW str)
{
  GUID guid = GUID_NULL;
  HRESULT hr = CLSIDFromString(LPOLESTR(str.c_str()), &guid);
  ASSERT(SUCCEEDED(hr));
  return guid;
}

CStdStringW GetMediaTypeName(const GUID& guid)
{
  CStdStringW ret = guid == GUID_NULL 
    ? L"Any type" 
    : CStdStringW(GuidNames[guid]);

  if(ret == L"FOURCC GUID")
  {
    CStdStringW str;
    if(guid.Data1 >= 0x10000)
      str.Format(L"Video: %c%c%c%c", (guid.Data1>>0)&0xff, (guid.Data1>>8)&0xff, (guid.Data1>>16)&0xff, (guid.Data1>>24)&0xff);
    else
      str.Format(L"Audio: 0x%08x", guid.Data1);
    ret = str;
  }
  else if(ret == L"Unknown GUID Name")
  {
    WCHAR null[128] = {0}, buff[128];
    StringFromGUID2(GUID_NULL, null, 127);
    ret = CStdStringW(StringFromGUID2(guid, buff, 127) ? buff : null);
  }

  return ret;
}

CLSID GetCLSID(IBaseFilter* pBF)
{
  CLSID clsid = GUID_NULL;
  if(pBF) pBF->GetClassID(&clsid);
  return(clsid);
}

CLSID GetCLSID(IPin* pPin)
{
  return(GetCLSID(GetFilterFromPin(pPin)));
}


CStdStringW GetFilterName(IBaseFilter* pBF)
{
  CStdStringW name;
  CFilterInfo fi;
  if(pBF && SUCCEEDED(pBF->QueryFilterInfo(&fi)))
    name = fi.achName;
  return(name);
}

CStdStringW GetFilterPath(CStdString pClsid)
{

  CStdStringW subKey = _T("\\CLSID\\") + pClsid + _T("\\InprocServer32");
  HKEY hSubKey;

  DWORD res = RegOpenKeyExW(HKEY_CLASSES_ROOT, subKey.c_str(),
    0, KEY_READ, &hSubKey);
  if (res != ERROR_SUCCESS)
    return "";

  wchar_t tab[MAX_PATH] = {0};
  DWORD tabSize = MAX_PATH * sizeof(wchar_t);

  res = RegQueryValueExW(hSubKey, L"", 0,
    NULL, (LPBYTE) tab, &tabSize);
  if ( res != ERROR_SUCCESS )
  {
    RegCloseKey(hSubKey);
    return "";
  }
  
  RegCloseKey(hSubKey);

  CStdStringW returnVal = tab;
  return returnVal;
}

bool IsStreamEnd(IBaseFilter* pBF)
{
  int nIn, nOut, nInC, nOutC;
  CountPins(pBF, nIn, nOut, nInC, nOutC);
  return(nOut == 0);
}

bool IsVideoRenderer(IBaseFilter* pBF)
{
	int nIn, nOut, nInC, nOutC;
	CountPins(pBF, nIn, nOut, nInC, nOutC);

	if (nInC > 0 && nOut == 0) {
		BeginEnumPins(pBF, pEP, pPin) {
			AM_MEDIA_TYPE mt;
			if (S_OK != pPin->ConnectionMediaType(&mt)) {
				continue;
			}

			FreeMediaType(mt);

			return !!(mt.majortype == MEDIATYPE_Video);
			/*&& (mt.formattype == FORMAT_VideoInfo || mt.formattype == FORMAT_VideoInfo2));*/
		}
		EndEnumPins;
	}

	CLSID clsid;
	memcpy(&clsid, &GUID_NULL, sizeof(clsid));
	pBF->GetClassID(&clsid);

	return (clsid == CLSID_VideoRenderer || clsid == CLSID_VideoRendererDefault);
}

DEFINE_GUID(CLSID_ReClock,
	0x9dc15360, 0x914c, 0x46b8, 0xb9, 0xdf, 0xbf, 0xe6, 0x7f, 0xd3, 0x6c, 0x6a);

bool IsAudioWaveRenderer(IBaseFilter* pBF)
{
	int nIn, nOut, nInC, nOutC;
	CountPins(pBF, nIn, nOut, nInC, nOutC);

	if (nInC > 0 && nOut == 0 && Com::SmartQIPtr<IBasicAudio>(pBF)) {
		BeginEnumPins(pBF, pEP, pPin) {
			AM_MEDIA_TYPE mt;
			if (S_OK != pPin->ConnectionMediaType(&mt)) {
				continue;
			}

			FreeMediaType(mt);

			return !!(mt.majortype == MEDIATYPE_Audio);
			/*&& mt.formattype == FORMAT_WaveFormatEx);*/
		}
		EndEnumPins;
	}

	CLSID clsid;
	memcpy(&clsid, &GUID_NULL, sizeof(clsid));
	pBF->GetClassID(&clsid);

  return (clsid == CLSID_DSoundRender || clsid == CLSID_AudioRender || clsid == CLSID_ReClock || clsid == CLSID_SANEAR
		|| clsid == __uuidof(CNullAudioRenderer) || clsid == __uuidof(CNullUAudioRenderer));
}

HRESULT RemoveUnconnectedFilters(IFilterGraph2 *pGraph)
{
	if (!pGraph)
		return E_POINTER;

	// Go through the list of filters in the graph.
	BeginEnumFilters(pGraph, pEF, pBF)
	{

      if (GetCLSID(pBF) == CLSID_XySubFilter) {
        continue;
      }

      Com::SmartPtr<IPin>pPin;
      // Find a connected pin on this filter.
      if (SUCCEEDED(FindMatchingPin(pBF, MatchPinConnection(TRUE), &pPin)))
      {
        // If it's connected, don't remove the filter.
        continue;
      }
      RemoveFilter(pGraph, pBF);
      pEF->Reset();

	}
	EndEnumFilters

	return S_OK;
}

int CountPins(IBaseFilter* pBF, int& nIn, int& nOut, int& nInC, int& nOutC)
{
  nIn = nOut = 0;
  nInC = nOutC = 0;

  BeginEnumPins(pBF, pEP, pPin)
  {
    PIN_DIRECTION dir;
    if(SUCCEEDED(pPin->QueryDirection(&dir)))
    {
      Com::SmartPtr<IPin> pPinConnectedTo;
      pPin->ConnectedTo(&pPinConnectedTo);

      if(dir == PINDIR_INPUT) {nIn++; if(pPinConnectedTo) nInC++;}
      else if(dir == PINDIR_OUTPUT) {nOut++; if(pPinConnectedTo) nOutC++;}
    }
  }
  EndEnumPins

  return(nIn+nOut);
}

bool IsSplitter(IBaseFilter* pBF, bool fCountConnectedOnly)
{
  int nIn, nOut, nInC, nOutC;
  CountPins(pBF, nIn, nOut, nInC, nOutC);
  return(fCountConnectedOnly ? nOutC > 1 : nOut > 1);
}

IPin* GetFirstPin(IBaseFilter* pBF, PIN_DIRECTION dir)
{
  if(!pBF) return(NULL);

  BeginEnumPins(pBF, pEP, pPin)
  {
    PIN_DIRECTION dir2;
    pPin->QueryDirection(&dir2);
    if(dir == dir2)
    {
      IPin* pRet = pPin.Detach();
      pRet->Release();
      return(pRet);
    }
  }
  EndEnumPins

  return(NULL);
}

CStdStringW GetPinName(IPin* pPin)
{
  CStdStringW name;
  CPinInfo pi;
  if(pPin && SUCCEEDED(pPin->QueryPinInfo(&pi))) 
    name = pi.achName;
  if(!name.Find(_T("Apple"))) name.Delete(0,1);

  return(name);
}

void memsetd(void* dst, unsigned int c, int nbytes)
{
#ifdef _WIN64
  for (int i=0; i<nbytes/sizeof(DWORD); i++)
    ((DWORD*)dst)[i] = c;
#else
  __asm
  {
    mov eax, c
    mov ecx, nbytes
    shr ecx, 2
    mov edi, dst
    cld
    rep stosd
  }
#endif
}


CStdStringW UTF8To16(LPCSTR utf8)
{
  CStdStringW str;
  int n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0)-1;
  if(n < 0) return str;
  str.ReleaseBuffer(MultiByteToWideChar(CP_UTF8, 0, utf8, -1, str.GetBuffer(n), n+1)-1);
  return str;
}

int MakeAACInitData(BYTE* pData, int profile, int freq, int channels)
{
  int srate_idx;

  if(92017 <= freq) srate_idx = 0;
  else if(75132 <= freq) srate_idx = 1;
  else if(55426 <= freq) srate_idx = 2;
  else if(46009 <= freq) srate_idx = 3;
  else if(37566 <= freq) srate_idx = 4;
  else if(27713 <= freq) srate_idx = 5;
  else if(23004 <= freq) srate_idx = 6;
  else if(18783 <= freq) srate_idx = 7;
  else if(13856 <= freq) srate_idx = 8;
  else if(11502 <= freq) srate_idx = 9;
  else if(9391 <= freq) srate_idx = 10;
  else srate_idx = 11;

  pData[0] = ((abs(profile) + 1) << 3) | ((srate_idx & 0xe) >> 1);
  pData[1] = ((srate_idx & 0x1) << 7) | (channels << 3);

  int ret = 2;

  if(profile < 0)
  {
    freq *= 2;

    if(92017 <= freq) srate_idx = 0;
    else if(75132 <= freq) srate_idx = 1;
    else if(55426 <= freq) srate_idx = 2;
    else if(46009 <= freq) srate_idx = 3;
    else if(37566 <= freq) srate_idx = 4;
    else if(27713 <= freq) srate_idx = 5;
    else if(23004 <= freq) srate_idx = 6;
    else if(18783 <= freq) srate_idx = 7;
    else if(13856 <= freq) srate_idx = 8;
    else if(11502 <= freq) srate_idx = 9;
    else if(9391 <= freq) srate_idx = 10;
    else srate_idx = 11;

    pData[2] = 0x2B7>>3;
    pData[3] = (BYTE)((0x2B7<<5) | 5);
    pData[4] = (1<<7) | (srate_idx<<3);

    ret = 5;
  }

  return(ret);
}

typedef struct
{
  const GUID*        Guid;
  const char*        Description;
} DXVA2_DECODER;

static const DXVA2_DECODER DXVA2Decoder[] = 
{
  { &GUID_NULL,                       "Unknown" },
  { &GUID_NULL,                       "Not using DXVA" },
  { &DXVA2_ModeH264_A,                "H.264 motion compensation, no FGT" },
  { &DXVA2_ModeH264_B,                "H.264 motion compensation, FGT" },
  { &DXVA2_ModeH264_C,                "H.264 IDCT, no FGT" },
  { &DXVA2_ModeH264_D,                "H.264 IDCT, FGT" },
  { &DXVA2_ModeH264_E,                "H.264 bitstream decoder, no FGT" },
  { &DXVA2_ModeH264_F,                "H.264 bitstream decoder, FGT" },
  { &DXVA_Intel_H264_ClearVideo,      "H.264 bitstream decoder, ClearVideo(tm)" },
  { &DXVA2_ModeMPEG2_IDCT,            "MPEG-2 IDCT" },
  { &DXVA2_ModeMPEG2_MoComp,          "MPEG-2 motion compensation" },
  { &DXVA2_ModeMPEG2_VLD,             "MPEG-2 variable-length decoder" },
  { &DXVA_ModeMPEG2_A,                "MPEG-2 A (Motion comp)" },
  { &DXVA_ModeMPEG2_B,                "MPEG-2 B (Motion comp+blending)" },
  { &DXVA_ModeMPEG2_C,                "MPEG-2 C (IDCT)" },
  { &DXVA_ModeMPEG2_D,                "MPEG-2 D (IDCT+blending)" },
  { &DXVA2_ModeVC1_A,                 "VC-1 post processing" },
  { &DXVA2_ModeVC1_B,                 "VC-1 motion compensation" },
  { &DXVA2_ModeVC1_C,                 "VC-1 IDCT" },
  { &DXVA2_ModeVC1_D,                 "VC-1 bitstream decoder" },
  { &DXVA_Intel_VC1_ClearVideo,       "VC-1 bitstream ClearVideo(tm) decoder" },
  { &DXVA2_ModeWMV8_A,                "WMV 8 post processing." }, 
  { &DXVA2_ModeWMV8_B,                "WMV8 motion compensation" },
  { &DXVA2_ModeWMV9_A,                "WMV9 post processing" },
  { &DXVA2_ModeWMV9_B,                "WMV9 motion compensation" },
  { &DXVA2_ModeWMV9_C,                "WMV9 IDCT" },
};

const char* GetDXVAMode(const GUID* guidDecoder)
{
  int nPos = 0;

  for (int i=1; i<countof(DXVA2Decoder); i++)
  {
    if (*guidDecoder == *DXVA2Decoder[i].Guid)
    {
      nPos = i;
      break;
    }
  }

  return DXVA2Decoder[nPos].Description;
}

CStdStringA GetDshowError(HRESULT hr)
{
  char szErr[MAX_ERROR_TEXT_LEN];
  DWORD res = AMGetErrorTextA(hr, szErr, MAX_ERROR_TEXT_LEN);
  if (res > 0)
    return CStdString(szErr);
  return CStdString("");

}

CStdStringW AToW(CStdStringA str)
{
  CStdStringW ret;
  for(size_t i = 0, j = str.GetLength(); i < j; i++)
    ret += (WCHAR)(BYTE)str[i];
  return(ret);
}

CStdStringA WToA(CStdStringW str)
{
  CStdStringA ret;
  for(size_t i = 0, j = str.GetLength(); i < j; i++)
    ret += (CHAR)(WORD)str[i];
  return(ret);
}

static struct {LPCSTR name, iso6392, iso6391; LCID lcid;} s_isolangs[] =  // TODO : fill LCID !!!
{
  {"Abkhazian", "abk", "ab"},
  {"Achinese", "ace", ""},
  {"Acoli", "ach", ""},
  {"Adangme", "ada", ""},
  {"Afar", "aar", "aa"},
  {"Afrihili", "afh", ""},
  {"Afrikaans", "afr", "af",          MAKELCID( MAKELANGID(LANG_AFRIKAANS, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Afro-Asiatic (Other)", "afa", ""},
  {"Akan", "aka", "ak"},
  {"Akkadian", "akk", ""},
  {"Albanian", "alb", "sq"},
  {"Albanian", "sqi", "sq",          MAKELCID( MAKELANGID(LANG_ALBANIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Aleut", "ale", ""},
  {"Algonquian languages", "alg", ""},
  {"Altaic (Other)", "tut", ""},
  {"Amharic", "amh", "am"},
  {"Apache languages", "apa", ""},
  {"Arabic", "ara", "ar",            MAKELCID( MAKELANGID(LANG_ARABIC, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Aragonese", "arg", "an"},
  {"Aramaic", "arc", ""},
  {"Arapaho", "arp", ""},
  {"Araucanian", "arn", ""},
  {"Arawak", "arw", ""},
  {"Armenian", "arm", "hy",          MAKELCID( MAKELANGID(LANG_ARMENIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Armenian", "hye", "hy",          MAKELCID( MAKELANGID(LANG_ARMENIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Artificial (Other)", "art", ""},
  {"Assamese", "asm", "as",          MAKELCID( MAKELANGID(LANG_ASSAMESE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Asturian; Bable", "ast", ""},
  {"Athapascan languages", "ath", ""},
  {"Australian languages", "aus", ""},
  {"Austronesian (Other)", "map", ""},
  {"Avaric", "ava", "av"},
  {"Avestan", "ave", "ae"},
  {"Awadhi", "awa", ""},
  {"Aymara", "aym", "ay"},
  {"Azerbaijani", "aze", "az",        MAKELCID( MAKELANGID(LANG_AZERI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Bable; Asturian", "ast", ""},
  {"Balinese", "ban", ""},
  {"Baltic (Other)", "bat", ""},
  {"Baluchi", "bal", ""},
  {"Bambara", "bam", "bm"},
  {"Bamileke languages", "bai", ""},
  {"Banda", "bad", ""},
  {"Bantu (Other)", "bnt", ""},
  {"Basa", "bas", ""},
  {"Bashkir", "bak", "ba",          MAKELCID( MAKELANGID(LANG_BASHKIR, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Basque", "baq", "eu",            MAKELCID( MAKELANGID(LANG_BASQUE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Basque", "eus", "eu",            MAKELCID( MAKELANGID(LANG_BASQUE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Batak (Indonesia)", "btk", ""},
  {"Beja", "bej", ""},
  {"Belarusian", "bel", "be",          MAKELCID( MAKELANGID(LANG_BELARUSIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Bemba", "bem", ""},
  {"Bengali", "ben", "bn",          MAKELCID( MAKELANGID(LANG_BENGALI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Berber (Other)", "ber", ""},
  {"Bhojpuri", "bho", ""},
  {"Bihari", "bih", "bh"},
  {"Bikol", "bik", ""},
  {"Bini", "bin", ""},
  {"Bislama", "bis", "bi"},
  {"Bokmål, Norwegian; Norwegian Bokmål", "nob", "nb"},
  {"Bosnian", "bos", "bs"},
  {"Braj", "bra", ""},
  {"Breton", "bre", "br",            MAKELCID( MAKELANGID(LANG_BRETON, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Buginese", "bug", ""},
  {"Bulgarian", "bul", "bg",          MAKELCID( MAKELANGID(LANG_BULGARIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Buriat", "bua", ""},
  {"Burmese", "bur", "my"},
  {"Burmese", "mya", "my"},
  {"Caddo", "cad", ""},
  {"Carib", "car", ""},
  {"Spanish; Castilian", "spa", "es",      MAKELCID( MAKELANGID(LANG_SPANISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Catalan", "cat", "ca",          MAKELCID( MAKELANGID(LANG_CATALAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Caucasian (Other)", "cau", ""},
  {"Cebuano", "ceb", ""},
  {"Celtic (Other)", "cel", ""},
  {"Central American Indian (Other)", "cai", ""},
  {"Chagatai", "chg", ""},
  {"Chamic languages", "cmc", ""},
  {"Chamorro", "cha", "ch"},
  {"Chechen", "che", "ce"},
  {"Cherokee", "chr", ""},
  {"Chewa; Chichewa; Nyanja", "nya", "ny"},
  {"Cheyenne", "chy", ""},
  {"Chibcha", "chb", ""},
  {"Chichewa; Chewa; Nyanja", "nya", "ny"},
  {"Chinese", "chi", "zh",          MAKELCID( MAKELANGID(LANG_CHINESE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Chinese", "zho", "zh"},
  {"Chinook jargon", "chn", ""},
  {"Chipewyan", "chp", ""},
  {"Choctaw", "cho", ""},
  {"Chuang; Zhuang", "zha", "za"},
  {"Church Slavic; Old Church Slavonic", "chu", "cu"},
  {"Old Church Slavonic; Old Slavonic; ", "chu", "cu"},
  {"Church Slavonic; Old Bulgarian; Church Slavic;", "chu", "cu"},
  {"Old Slavonic; Church Slavonic; Old Bulgarian;", "chu", "cu"},
  {"Church Slavic; Old Church Slavonic", "chu", "cu"},
  {"Chuukese", "chk", ""},
  {"Chuvash", "chv", "cv"},
  {"Coptic", "cop", ""},
  {"Cornish", "cor", "kw"},
  {"Corsican", "cos", "co",          MAKELCID( MAKELANGID(LANG_CORSICAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Cree", "cre", "cr"},
  {"Creek", "mus", ""},
  {"Creoles and pidgins (Other)", "crp", ""},
  {"Creoles and pidgins,", "cpe", ""},
  //   {"English-based (Other)", "", ""},
  {"Creoles and pidgins,", "cpf", ""},
  //   {"French-based (Other)", "", ""},
  {"Creoles and pidgins,", "cpp", ""},
  //   {"Portuguese-based (Other)", "", ""},
  {"Croatian", "scr", "hr",          MAKELCID( MAKELANGID(LANG_CROATIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Croatian", "hrv", "hr",          MAKELCID( MAKELANGID(LANG_CROATIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Cushitic (Other)", "cus", ""},
  {"Czech", "cze", "cs",            MAKELCID( MAKELANGID(LANG_CZECH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Czech", "ces", "cs",            MAKELCID( MAKELANGID(LANG_CZECH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Dakota", "dak", ""},
  {"Danish", "dan", "da",            MAKELCID( MAKELANGID(LANG_DANISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Dargwa", "dar", ""},
  {"Dayak", "day", ""},
  {"Delaware", "del", ""},
  {"Dinka", "din", ""},
  {"Divehi", "div", "dv",            MAKELCID( MAKELANGID(LANG_DIVEHI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Dogri", "doi", ""},
  {"Dogrib", "dgr", ""},
  {"Dravidian (Other)", "dra", ""},
  {"Duala", "dua", ""},
  {"Dutch; Flemish", "dut", "nl",        MAKELCID( MAKELANGID(LANG_DUTCH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Dutch; Flemish", "nld", "nl",        MAKELCID( MAKELANGID(LANG_DUTCH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Dutch, Middle (ca. 1050-1350)", "dum", ""},
  {"Dyula", "dyu", ""},
  {"Dzongkha", "dzo", "dz"},
  {"Efik", "efi", ""},
  {"Egyptian (Ancient)", "egy", ""},
  {"Ekajuk", "eka", ""},
  {"Elamite", "elx", ""},
  {"English", "eng", "en",          MAKELCID( MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"English, Middle (1100-1500)", "enm", ""},
  {"English, Old (ca.450-1100)", "ang", ""},
  {"Esperanto", "epo", "eo"},
  {"Estonian", "est", "et",          MAKELCID( MAKELANGID(LANG_ESTONIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Ewe", "ewe", "ee"},
  {"Ewondo", "ewo", ""},
  {"Fang", "fan", ""},
  {"Fanti", "fat", ""},
  {"Faroese", "fao", "fo",          MAKELCID( MAKELANGID(LANG_FAEROESE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Fijian", "fij", "fj"},
  {"Finnish", "fin", "fi",          MAKELCID( MAKELANGID(LANG_FINNISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Finno-Ugrian (Other)", "fiu", ""},
  {"Flemish; Dutch", "dut", "nl"},
  {"Flemish; Dutch", "nld", "nl"},
  {"Fon", "fon", ""},
  {"French", "fre", "fr",            MAKELCID( MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"French", "fra*", "fr",          MAKELCID( MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"French", "fra", "fr",            MAKELCID( MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"French, Middle (ca.1400-1600)", "frm", ""},
  {"French, Old (842-ca.1400)", "fro", ""},
  {"Frisian", "fry", "fy",          MAKELCID( MAKELANGID(LANG_FRISIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Friulian", "fur", ""},
  {"Fulah", "ful", "ff"},
  {"Ga", "gaa", ""},
  {"Gaelic; Scottish Gaelic", "gla", "gd",  MAKELCID( MAKELANGID(LANG_GALICIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Gallegan", "glg", "gl"},
  {"Ganda", "lug", "lg"},
  {"Gayo", "gay", ""},
  {"Gbaya", "gba", ""},
  {"Geez", "gez", ""},
  {"Georgian", "geo", "ka",          MAKELCID( MAKELANGID(LANG_GEORGIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Georgian", "kat", "ka",          MAKELCID( MAKELANGID(LANG_GEORGIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"German", "ger", "de",            MAKELCID( MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"German", "deu", "de",            MAKELCID( MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"German, Low; Saxon, Low; Low German; Low Saxon", "nds", ""},
  {"German, Middle High (ca.1050-1500)", "gmh", ""},
  {"German, Old High (ca.750-1050)", "goh", ""},
  {"Germanic (Other)", "gem", ""},
  {"Gikuyu; Kikuyu", "kik", "ki"},
  {"Gilbertese", "gil", ""},
  {"Gondi", "gon", ""},
  {"Gorontalo", "gor", ""},
  {"Gothic", "got", ""},
  {"Grebo", "grb", ""},
  {"Greek, Ancient (to 1453)", "grc", "",    MAKELCID( MAKELANGID(LANG_GREEK, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Greek, Modern (1453-)", "gre", "el",    MAKELCID( MAKELANGID(LANG_GREEK, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Greek, Modern (1453-)", "ell", "el",    MAKELCID( MAKELANGID(LANG_GREEK, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Greenlandic; Kalaallisut", "kal", "kl",  MAKELCID( MAKELANGID(LANG_GREENLANDIC, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Guarani", "grn", "gn"},
  {"Gujarati", "guj", "gu",          MAKELCID( MAKELANGID(LANG_GUJARATI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Gwich´in", "gwi", ""},
  {"Haida", "hai", ""},
  {"Hausa", "hau", "ha",            MAKELCID( MAKELANGID(LANG_HAUSA, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Hawaiian", "haw", ""},
  {"Hebrew", "heb", "he",            MAKELCID( MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Herero", "her", "hz"},
  {"Hiligaynon", "hil", ""},
  {"Himachali", "him", ""},
  {"Hindi", "hin", "hi",            MAKELCID( MAKELANGID(LANG_HINDI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Hiri Motu", "hmo", "ho"},
  {"Hittite", "hit", ""},
  {"Hmong", "hmn", ""},
  {"Hungarian", "hun", "hu",          MAKELCID( MAKELANGID(LANG_HUNGARIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Hupa", "hup", ""},
  {"Iban", "iba", ""},
  {"Icelandic", "ice", "is",          MAKELCID( MAKELANGID(LANG_ICELANDIC, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Icelandic", "isl", "is",          MAKELCID( MAKELANGID(LANG_ICELANDIC, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Ido", "ido", "io"},
  {"Igbo", "ibo", "ig",            MAKELCID( MAKELANGID(LANG_IGBO, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Ijo", "ijo", ""},
  {"Iloko", "ilo", ""},
  {"Inari Sami", "smn", ""},
  {"Indic (Other)", "inc", ""},
  {"Indo-European (Other)", "ine", ""},
  {"Indonesian", "ind", "id",          MAKELCID( MAKELANGID(LANG_INDONESIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Ingush", "inh", ""},
  {"Interlingua (International", "ina", "ia"},
  //   {"Auxiliary Language Association)", "", ""},
  {"Interlingue", "ile", "ie"},
  {"Inuktitut", "iku", "iu",          MAKELCID( MAKELANGID(LANG_INUKTITUT, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Inupiaq", "ipk", "ik"},
  {"Iranian (Other)", "ira", ""},
  {"Irish", "gle", "ga",            MAKELCID( MAKELANGID(LANG_IRISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Irish, Middle (900-1200)", "mga", "",    MAKELCID( MAKELANGID(LANG_IRISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Irish, Old (to 900)", "sga", "",      MAKELCID( MAKELANGID(LANG_IRISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Iroquoian languages", "iro", ""},
  {"Italian", "ita", "it",          MAKELCID( MAKELANGID(LANG_ITALIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Japanese", "jpn", "ja",          MAKELCID( MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Javanese", "jav", "jv"},
  {"Judeo-Arabic", "jrb", ""},
  {"Judeo-Persian", "jpr", ""},
  {"Kabardian", "kbd", ""},
  {"Kabyle", "kab", ""},
  {"Kachin", "kac", ""},
  {"Kalaallisut; Greenlandic", "kal", "kl"},
  {"Kamba", "kam", ""},
  {"Kannada", "kan", "kn",          MAKELCID( MAKELANGID(LANG_KANNADA, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Kanuri", "kau", "kr"},
  {"Kara-Kalpak", "kaa", ""},
  {"Karen", "kar", ""},
  {"Kashmiri", "kas", "ks",          MAKELCID( MAKELANGID(LANG_KASHMIRI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Kawi", "kaw", ""},
  {"Kazakh", "kaz", "kk",            MAKELCID( MAKELANGID(LANG_KAZAK, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Khasi", "kha", ""},
  {"Khmer", "khm", "km",            MAKELCID( MAKELANGID(LANG_KHMER, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Khoisan (Other)", "khi", ""},
  {"Khotanese", "kho", ""},
  {"Kikuyu; Gikuyu", "kik", "ki"},
  {"Kimbundu", "kmb", ""},
  {"Kinyarwanda", "kin", "rw",        MAKELCID( MAKELANGID(LANG_KINYARWANDA, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Kirghiz", "kir", "ky"},
  {"Komi", "kom", "kv"},
  {"Kongo", "kon", "kg"},
  {"Konkani", "kok", "",            MAKELCID( MAKELANGID(LANG_KONKANI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Korean", "kor", "ko",            MAKELCID( MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Kosraean", "kos", ""},
  {"Kpelle", "kpe", ""},
  {"Kru", "kro", ""},
  {"Kuanyama; Kwanyama", "kua", "kj"},
  {"Kumyk", "kum", ""},
  {"Kurdish", "kur", "ku"},
  {"Kurukh", "kru", ""},
  {"Kutenai", "kut", ""},
  {"Kwanyama, Kuanyama", "kua", "kj"},
  {"Ladino", "lad", ""},
  {"Lahnda", "lah", ""},
  {"Lamba", "lam", ""},
  {"Lao", "lao", "lo",            MAKELCID( MAKELANGID(LANG_LAO, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Latin", "lat", "la"},
  {"Latvian", "lav", "lv",          MAKELCID( MAKELANGID(LANG_LATVIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Letzeburgesch; Luxembourgish", "ltz", "lb"},
  {"Lezghian", "lez", ""},
  {"Limburgan; Limburger; Limburgish", "lim", "li"},
  {"Limburger; Limburgan; Limburgish;", "lim", "li"},
  {"Limburgish; Limburger; Limburgan", "lim", "li"},
  {"Lingala", "lin", "ln"},
  {"Lithuanian", "lit", "lt",          MAKELCID( MAKELANGID(LANG_LITHUANIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Low German; Low Saxon; German, Low; Saxon, Low", "nds", ""},
  {"Low Saxon; Low German; Saxon, Low; German, Low", "nds", ""},
  {"Lozi", "loz", ""},
  {"Luba-Katanga", "lub", "lu"},
  {"Luba-Lulua", "lua", ""},
  {"Luiseno", "lui", ""},
  {"Lule Sami", "smj", ""},
  {"Lunda", "lun", ""},
  {"Luo (Kenya and Tanzania)", "luo", ""},
  {"Lushai", "lus", ""},
  {"Luxembourgish; Letzeburgesch", "ltz", "lb",  MAKELCID( MAKELANGID(LANG_LUXEMBOURGISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Macedonian", "mac", "mk",          MAKELCID( MAKELANGID(LANG_MACEDONIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Macedonian", "mkd", "mk",          MAKELCID( MAKELANGID(LANG_MACEDONIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Madurese", "mad", ""},
  {"Magahi", "mag", ""},
  {"Maithili", "mai", ""},
  {"Makasar", "mak", ""},
  {"Malagasy", "mlg", "mg"},
  {"Malay", "may", "ms",            MAKELCID( MAKELANGID(LANG_MALAY, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Malay", "msa", "ms",            MAKELCID( MAKELANGID(LANG_MALAY, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Malayalam", "mal", "ml",          MAKELCID( MAKELANGID(LANG_MALAYALAM, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Maltese", "mlt", "mt",          MAKELCID( MAKELANGID(LANG_MALTESE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Manchu", "mnc", ""},
  {"Mandar", "mdr", ""},
  {"Mandingo", "man", ""},
  {"Manipuri", "mni", "",            MAKELCID( MAKELANGID(LANG_MANIPURI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Manobo languages", "mno", ""},
  {"Manx", "glv", "gv"},
  {"Maori", "mao", "mi",            MAKELCID( MAKELANGID(LANG_MAORI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Maori", "mri", "mi",            MAKELCID( MAKELANGID(LANG_MAORI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Marathi", "mar", "mr",          MAKELCID( MAKELANGID(LANG_MARATHI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Mari", "chm", ""},
  {"Marshallese", "mah", "mh"},
  {"Marwari", "mwr", ""},
  {"Masai", "mas", ""},
  {"Mayan languages", "myn", ""},
  {"Mende", "men", ""},
  {"Micmac", "mic", ""},
  {"Minangkabau", "min", ""},
  {"Miscellaneous languages", "mis", ""},
  {"Mohawk", "moh", "",            MAKELCID( MAKELANGID(LANG_MOHAWK, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Moldavian", "mol", "mo"},
  {"Mon-Khmer (Other)", "mkh", ""},
  {"Mongo", "lol", ""},
  {"Mongolian", "mon", "mn",          MAKELCID( MAKELANGID(LANG_MONGOLIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Mossi", "mos", ""},
  {"Multiple languages", "mul", ""},
  {"Munda languages", "mun", ""},
  {"Nahuatl", "nah", ""},
  {"Nauru", "nau", "na"},
  {"Navaho, Navajo", "nav", "nv"},
  {"Navajo; Navaho", "nav", "nv"},
  {"Ndebele, North", "nde", "nd"},
  {"Ndebele, South", "nbl", "nr"},
  {"Ndonga", "ndo", "ng"},
  {"Neapolitan", "nap", ""},
  {"Nepali", "nep", "ne",            MAKELCID( MAKELANGID(LANG_NEPALI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Newari", "new", ""},
  {"Nias", "nia", ""},
  {"Niger-Kordofanian (Other)", "nic", ""},
  {"Nilo-Saharan (Other)", "ssa", ""},
  {"Niuean", "niu", ""},
  {"Norse, Old", "non", ""},
  {"North American Indian (Other)", "nai", ""},
  {"Northern Sami", "sme", "se"},
  {"North Ndebele", "nde", "nd"},
  {"Norwegian", "nor", "no",          MAKELCID( MAKELANGID(LANG_NORWEGIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Norwegian Bokmål; Bokmål, Norwegian", "nob", "nb",  MAKELCID( MAKELANGID(LANG_NORWEGIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Norwegian Nynorsk; Nynorsk, Norwegian", "nno", "nn",  MAKELCID( MAKELANGID(LANG_NORWEGIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Nubian languages", "nub", ""},
  {"Nyamwezi", "nym", ""},
  {"Nyanja; Chichewa; Chewa", "nya", "ny"},
  {"Nyankole", "nyn", ""},
  {"Nynorsk, Norwegian; Norwegian Nynorsk", "nno", "nn"},
  {"Nyoro", "nyo", ""},
  {"Nzima", "nzi", ""},
  {"Occitan (post 1500},; Provençal", "oci", "oc",    MAKELCID( MAKELANGID(LANG_OCCITAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Ojibwa", "oji", "oj"},
  {"Old Bulgarian; Old Slavonic; Church Slavonic;", "chu", "cu"},
  {"Oriya", "ori", "or"},
  {"Oromo", "orm", "om"},
  {"Osage", "osa", ""},
  {"Ossetian; Ossetic", "oss", "os"},
  {"Ossetic; Ossetian", "oss", "os"},
  {"Otomian languages", "oto", ""},
  {"Pahlavi", "pal", ""},
  {"Palauan", "pau", ""},
  {"Pali", "pli", "pi"},
  {"Pampanga", "pam", ""},
  {"Pangasinan", "pag", ""},
  {"Panjabi", "pan", "pa"},
  {"Papiamento", "pap", ""},
  {"Papuan (Other)", "paa", ""},
  {"Persian", "per", "fa",        MAKELCID( MAKELANGID(LANG_PERSIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Persian", "fas", "fa",        MAKELCID( MAKELANGID(LANG_PERSIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Persian, Old (ca.600-400 B.C.)", "peo", ""},
  {"Philippine (Other)", "phi", ""},
  {"Phoenician", "phn", ""},
  {"Pohnpeian", "pon", ""},
  {"Polish", "pol", "pl",          MAKELCID( MAKELANGID(LANG_POLISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Portuguese", "por", "pt",        MAKELCID( MAKELANGID(LANG_PORTUGUESE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Prakrit languages", "pra", ""},
  {"Provençal; Occitan (post 1500)", "oci", "oc"},
  {"Provençal, Old (to 1500)", "pro", ""},
  {"Pushto", "pus", "ps"},
  {"Quechua", "que", "qu",        MAKELCID( MAKELANGID(LANG_QUECHUA, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Raeto-Romance", "roh", "rm"},
  {"Rajasthani", "raj", ""},
  {"Rapanui", "rap", ""},
  {"Rarotongan", "rar", ""},
  {"Reserved for local use", "qaa-qtz", ""},
  {"Romance (Other)", "roa", ""},
  {"Romanian", "rum", "ro",        MAKELCID( MAKELANGID(LANG_ROMANIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Romanian", "ron", "ro",        MAKELCID( MAKELANGID(LANG_ROMANIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Romany", "rom", ""},
  {"Rundi", "run", "rn"},
  {"Russian", "rus", "ru",        MAKELCID( MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Salishan languages", "sal", ""},
  {"Samaritan Aramaic", "sam", ""},
  {"Sami languages (Other)", "smi", ""},
  {"Samoan", "smo", "sm"},
  {"Sandawe", "sad", ""},
  {"Sango", "sag", "sg"},
  {"Sanskrit", "san", "sa",        MAKELCID( MAKELANGID(LANG_SANSKRIT, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Santali", "sat", ""},
  {"Sardinian", "srd", "sc"},
  {"Sasak", "sas", ""},
  {"Saxon, Low; German, Low; Low Saxon; Low German", "nds", ""},
  {"Scots", "sco", ""},
  {"Scottish Gaelic; Gaelic", "gla", "gd"},
  {"Selkup", "sel", ""},
  {"Semitic (Other)", "sem", ""},
  {"Serbian", "scc", "sr",        MAKELCID( MAKELANGID(LANG_SERBIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Serbian", "srp", "sr",        MAKELCID( MAKELANGID(LANG_SERBIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Serer", "srr", ""},
  {"Shan", "shn", ""},
  {"Shona", "sna", "sn"},
  {"Sichuan Yi", "iii", "ii"},
  {"Sidamo", "sid", ""},
  {"Sign languages", "sgn", ""},
  {"Siksika", "bla", ""},
  {"Sindhi", "snd", "sd",          MAKELCID( MAKELANGID(LANG_SINDHI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Sinhalese", "sin", "si",        MAKELCID( MAKELANGID(LANG_SINHALESE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Sino-Tibetan (Other)", "sit", ""},
  {"Siouan languages", "sio", ""},
  {"Skolt Sami", "sms", ""},
  {"Slave (Athapascan)", "den", ""},
  {"Slavic (Other)", "sla", ""},
  {"Slovak", "slo", "sk",          MAKELCID( MAKELANGID(LANG_SLOVAK, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Slovak", "slk", "sk",          MAKELCID( MAKELANGID(LANG_SLOVAK, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Slovenian", "slv", "sl",        MAKELCID( MAKELANGID(LANG_SLOVENIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Sogdian", "sog", ""},
  {"Somali", "som", "so"},
  {"Songhai", "son", ""},
  {"Soninke", "snk", ""},
  {"Sorbian languages", "wen", ""},
  {"Sotho, Northern", "nso", "",      MAKELCID( MAKELANGID(LANG_SOTHO, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Sotho, Southern", "sot", "st",    MAKELCID( MAKELANGID(LANG_SOTHO, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"South American Indian (Other)", "sai", ""},
  {"Southern Sami", "sma", ""},
  {"South Ndebele", "nbl", "nr"},
  {"Spanish; Castilian", "spa", "es",    MAKELCID( MAKELANGID(LANG_SPANISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Sukuma", "suk", ""},
  {"Sumerian", "sux", ""},
  {"Sundanese", "sun", "su"},
  {"Susu", "sus", ""},
  {"Swahili", "swa", "sw",        MAKELCID( MAKELANGID(LANG_SWAHILI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Swati", "ssw", "ss"},
  {"Swedish", "swe", "sv",        MAKELCID( MAKELANGID(LANG_SWEDISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Syriac", "syr", "",          MAKELCID( MAKELANGID(LANG_SYRIAC, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Tagalog", "tgl", "tl"},
  {"Tahitian", "tah", "ty"},
  {"Tai (Other)", "tai", ""},
  {"Tajik", "tgk", "tg",          MAKELCID( MAKELANGID(LANG_TAJIK, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Tamashek", "tmh", ""},
  {"Tamil", "tam", "ta",          MAKELCID( MAKELANGID(LANG_TAMIL, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Tatar", "tat", "tt",          MAKELCID( MAKELANGID(LANG_TATAR, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Telugu", "tel", "te",          MAKELCID( MAKELANGID(LANG_TELUGU, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Tereno", "ter", ""},
  {"Tetum", "tet", ""},
  {"Thai", "tha", "th",          MAKELCID( MAKELANGID(LANG_THAI, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Tibetan", "tib", "bo",        MAKELCID( MAKELANGID(LANG_TIBETAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Tibetan", "bod", "bo",        MAKELCID( MAKELANGID(LANG_TIBETAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Tigre", "tig", ""},
  {"Tigrinya", "tir", "ti",        MAKELCID( MAKELANGID(LANG_TIGRIGNA, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Timne", "tem", ""},
  {"Tiv", "tiv", ""},
  {"Tlingit", "tli", ""},
  {"Tok Pisin", "tpi", ""},
  {"Tokelau", "tkl", ""},
  {"Tonga (Nyasa)", "tog", ""},
  {"Tonga (Tonga Islands)", "ton", "to"},
  {"Tsimshian", "tsi", ""},
  {"Tsonga", "tso", "ts"},
  {"Tswana", "tsn", "tn",          MAKELCID( MAKELANGID(LANG_TSWANA, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Tumbuka", "tum", ""},
  {"Tupi languages", "tup", ""},
  {"Turkish", "tur", "tr",        MAKELCID( MAKELANGID(LANG_TURKISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Turkish, Ottoman (1500-1928)", "ota", "",  MAKELCID( MAKELANGID(LANG_TURKISH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Turkmen", "tuk", "tk",        MAKELCID( MAKELANGID(LANG_TURKMEN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Tuvalu", "tvl", ""},
  {"Tuvinian", "tyv", ""},
  {"Twi", "twi", "tw"},
  {"Ugaritic", "uga", ""},
  {"Uighur", "uig", "ug",          MAKELCID( MAKELANGID(LANG_UIGHUR, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Ukrainian", "ukr", "uk",        MAKELCID( MAKELANGID(LANG_UKRAINIAN, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Umbundu", "umb", ""},
  {"Undetermined", "und", ""},
  {"Urdu", "urd", "ur",          MAKELCID( MAKELANGID(LANG_URDU, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Uzbek", "uzb", "uz",          MAKELCID( MAKELANGID(LANG_UZBEK, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Vai", "vai", ""},
  {"Venda", "ven", "ve"},
  {"Vietnamese", "vie", "vi",        MAKELCID( MAKELANGID(LANG_VIETNAMESE, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Volapük", "vol", "vo"},
  {"Votic", "vot", ""},
  {"Wakashan languages", "wak", ""},
  {"Walamo", "wal", ""},
  {"Walloon", "wln", "wa"},
  {"Waray", "war", ""},
  {"Washo", "was", ""},
  {"Welsh", "wel", "cy",          MAKELCID( MAKELANGID(LANG_WELSH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Welsh", "cym", "cy",          MAKELCID( MAKELANGID(LANG_WELSH, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Wolof", "wol", "wo",          MAKELCID( MAKELANGID(LANG_WOLOF, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Xhosa", "xho", "xh",          MAKELCID( MAKELANGID(LANG_XHOSA, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Yakut", "sah", "",          MAKELCID( MAKELANGID(LANG_YAKUT, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Yao", "yao", ""},
  {"Yapese", "yap", ""},
  {"Yiddish", "yid", "yi"},
  {"Yoruba", "yor", "yo",          MAKELCID( MAKELANGID(LANG_YORUBA, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Yupik languages", "ypk", ""},
  {"Zande", "znd", ""},
  {"Zapotec", "zap", ""},
  {"Zenaga", "zen", ""},
  {"Zhuang; Chuang", "zha", "za"},
  {"Zulu", "zul", "zu",          MAKELCID( MAKELANGID(LANG_ZULU, SUBLANG_DEFAULT), SORT_DEFAULT)},
  {"Zuni", "zun", ""},
  {"Classical Newari", "nwc", ""},
  {"Klingon", "tlh", ""},
  {"Blin", "byn", ""},
  {"Lojban", "jbo", ""},
  {"Lower Sorbian", "dsb", ""},
  {"Upper Sorbian", "hsb", ""},
  {"Kashubian", "csb", ""},
  {"Crimean Turkish", "crh", ""},
  {"Erzya", "myv", ""},
  {"Moksha", "mdf", ""},
  {"Karachay-Balkar", "krc", ""},
  {"Adyghe", "ady", ""},
  {"Udmurt", "udm", ""},
  {"Dargwa", "dar", ""},
  {"Ingush", "inh", ""},
  {"Nogai", "nog", ""},
  {"Haitian", "hat", "ht"},
  {"Kalmyk", "xal", ""},
  {"", "", ""},
  {"No subtitles", "---", "", LCID_NOSUBTITLES},
};

LCID ProbeLangForLCID(LPCSTR code)
{
  if (strlen(code) == 3)
  {
    return ISO6392ToLcid(code);
  }
  else if (strlen(code) >= 2)
  {
    return ISO6391ToLcid(code);
  }
  return 0;
}

std::string ProbeLangForLanguage(LPCSTR code)
{
  if (strlen(code) == 3)
  {
    return ISO6392ToLanguage(code);
  }
  else if (strlen(code) >= 2)
  {
    return ISO6391ToLanguage(code);
  }
  return std::string();
}

CStdStringA ISO6391ToLanguage(LPCSTR code)
{
  CHAR tmp[2+1];
  strncpy_s(tmp, code, 2);
  tmp[2] = 0;
  _strlwr_s(tmp);
  for(int i = 0, j = countof(s_isolangs); i < j; i++)
    if(!strcmp(s_isolangs[i].iso6391, tmp))
    {
      CStdStringA ret = CStdStringA(s_isolangs[i].name);
      int i = ret.Find(';');
      if(i > 0) ret = ret.Left(i);
      return ret;
    }
  return "";
}

CStdStringA ISO6392ToLanguage(LPCSTR code)
{
  CHAR tmp[3+1];
  strncpy_s(tmp, code, 3);
  tmp[3] = 0;
  _strlwr_s(tmp);
  for(int i = 0, j = countof(s_isolangs); i < j; i++)
  {
    if(!strcmp(s_isolangs[i].iso6392, tmp))
    {
      CStdStringA ret = CStdStringA(s_isolangs[i].name);
      int i = ret.Find(';');
      if(i > 0) ret = ret.Left(i);
      return ret;
    }
  }
  return "";
}

LCID ISO6391ToLcid(LPCSTR code)
{
  CHAR tmp[3+1];
  strncpy_s(tmp, code, 3);
  tmp[3] = 0;
  _strlwr_s(tmp);
  for(int i = 0, j = countof(s_isolangs); i < j; i++)
  {
    if(!strcmp(s_isolangs[i].iso6391, code))
    {
      return s_isolangs[i].lcid;
    }
  }
  return 0;
}

LCID ISO6392ToLcid(LPCSTR code)
{
  CHAR tmp[3+1];
  strncpy_s(tmp, code, 3);
  tmp[3] = 0;
  _strlwr_s(tmp);
  for(int i = 0, j = countof(s_isolangs); i < j; i++)
  {
    if(!strcmp(s_isolangs[i].iso6392, tmp))
    {
      return s_isolangs[i].lcid;
    }
  }
  return 0;
}

const double Rec601_Kr = 0.299;
const double Rec601_Kb = 0.114;
const double Rec601_Kg = 0.587;
COLORREF YCrCbToRGB_Rec601(BYTE Y, BYTE Cr, BYTE Cb)
{

  double rp = Y + 2*(Cr-128)*(1.0-Rec601_Kr);
  double gp = Y - 2*(Cb-128)*(1.0-Rec601_Kb)*Rec601_Kb/Rec601_Kg - 2*(Cr-128)*(1.0-Rec601_Kr)*Rec601_Kr/Rec601_Kg;
  double bp = Y + 2*(Cb-128)*(1.0-Rec601_Kb);

  return RGB (fabs(rp), fabs(gp), fabs(bp));
}

DWORD YCrCbToRGB_Rec601(BYTE A, BYTE Y, BYTE Cr, BYTE Cb)
{

  double rp = Y + 2*(Cr-128)*(1.0-Rec601_Kr);
  double gp = Y - 2*(Cb-128)*(1.0-Rec601_Kb)*Rec601_Kb/Rec601_Kg - 2*(Cr-128)*(1.0-Rec601_Kr)*Rec601_Kr/Rec601_Kg;
  double bp = Y + 2*(Cb-128)*(1.0-Rec601_Kb);

  return D3DCOLOR_ARGB(A, (BYTE)fabs(rp), (BYTE)fabs(gp), (BYTE)fabs(bp));
}


const double Rec709_Kr = 0.2125;
const double Rec709_Kb = 0.0721;
const double Rec709_Kg = 0.7154;
COLORREF YCrCbToRGB_Rec709(BYTE Y, BYTE Cr, BYTE Cb)
{

  double rp = Y + 2*(Cr-128)*(1.0-Rec709_Kr);
  double gp = Y - 2*(Cb-128)*(1.0-Rec709_Kb)*Rec709_Kb/Rec709_Kg - 2*(Cr-128)*(1.0-Rec709_Kr)*Rec709_Kr/Rec709_Kg;
  double bp = Y + 2*(Cb-128)*(1.0-Rec709_Kb);

  return RGB (fabs(rp), fabs(gp), fabs(bp));
}

DWORD YCrCbToRGB_Rec709(BYTE A, BYTE Y, BYTE Cr, BYTE Cb)
{

  double rp = Y + 2*(Cr-128)*(1.0-Rec709_Kr);
  double gp = Y - 2*(Cb-128)*(1.0-Rec709_Kb)*Rec709_Kb/Rec709_Kg - 2*(Cr-128)*(1.0-Rec709_Kr)*Rec709_Kr/Rec709_Kg;
  double bp = Y + 2*(Cb-128)*(1.0-Rec709_Kb);

  return D3DCOLOR_ARGB (A, (BYTE)fabs(rp), (BYTE)fabs(gp), (BYTE)fabs(bp));
}

void memsetw(void* dst, unsigned short c, size_t nbytes)
{
  memsetd(dst, c << 16 | c, nbytes);

  size_t n = nbytes / 2;
  size_t o = (n / 2) * 2;
  if ((n - o) == 1)
    ((WORD*)dst)[o] = c;
}

