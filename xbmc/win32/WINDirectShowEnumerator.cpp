/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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

#include "system.h"
#include "WINDirectShowEnumerator.h"
#include "SingleLock.h"
#include "log.h"
#include "CharsetConverter.h"
#include "streams.h"
#include "uuids.h"//CLSID_SystemDeviceEnum and CLSID_AudioRendererCategory
#include "DShowUtil/regkey.h"

CDirectShowEnumerator::CDirectShowEnumerator(void)
{
}

CDirectShowEnumerator::~CDirectShowEnumerator(void)
{
  vDSFilterInfo.clear();
}


std::vector<DSFilterInfo> CDirectShowEnumerator::GetAudioRenderers()
{
  CSingleLock lock (m_critSection);
  vDSFilterInfo.clear();
  ICreateDevEnum *sys_dev_enum = NULL;
  IEnumMoniker *enum_moniker = NULL;
  HRESULT hr;
  int ret = -1;
  do {
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&sys_dev_enum);
    if (FAILED(hr)) 
      break;
    //would it be better to enum with this flag CDEF_DEVMON_FILTER
    //this is enumerating only the native DirectShow filters.
    hr = sys_dev_enum->CreateClassEnumerator(CLSID_AudioRendererCategory, &enum_moniker, 0);
    if (hr != NOERROR)
    {
      SAFE_RELEASE(sys_dev_enum);
      break;
    }
    
  //vDSFilterInfo.push_back(
    IMoniker			*moniker = NULL;
    IPropertyBag		*propbag = NULL;
    ULONG f;
    while (enum_moniker->Next(1, &moniker, &f) == NOERROR) 
    {
      if (SUCCEEDED(moniker->BindToStorage(NULL, NULL, IID_IPropertyBag, (void**)&propbag)))
      {
        VARIANT				var;
				VariantInit(&var);
        CStdString filterName;
        CStdString filterGuid;
        if (SUCCEEDED(propbag->Read(L"FriendlyName", &var, 0)))
          filterName = CStdString(var.bstrVal);
        VariantClear(&var);
        VariantInit(&var);
        if (SUCCEEDED(propbag->Read(L"CLSID", &var, 0)))
          filterGuid = CStdString(var.bstrVal);
        directshow_add_filter(filterGuid,filterName);
      }
      SAFE_RELEASE(moniker);
    }
    SAFE_RELEASE(enum_moniker);
  } while (0);

  SAFE_RELEASE(sys_dev_enum);
  return vDSFilterInfo;
}

bool CDirectShowEnumerator::directshow_add_filter(LPCTSTR lpstrGuid, LPCTSTR lpcstrName)
{
  struct DSFilterInfo dInfo;
  dInfo.lpstrGuid = lpstrGuid;
  dInfo.lpstrName = lpcstrName;
  g_charsetConverter.unknownToUTF8(dInfo.lpstrName);
  CDirectShowEnumerator::vDSFilterInfo.push_back(dInfo);
  CLog::Log(LOGDEBUG, "%s - found Device: %s with guid %s", __FUNCTION__,lpcstrName,lpstrGuid);
  return true;
}


void CDirectShowEnumerator::ForceStableCodecs()
{
  //This is the list of codecs that are considered stable in ffdshow
  CStdString strRegKey;
  strRegKey.Format("Software\\Gnu\\ffdshow");
  RegKey ffReg(HKEY_CURRENT_USER ,strRegKey.c_str() ,false);
  ffReg.setValue("cscd",DWORD(1)); ffReg.setValue("div3",DWORD(1)); ffReg.setValue("duck",DWORD(1));
  ffReg.setValue("dx50",DWORD(1)); ffReg.setValue("ffv1",DWORD(1)); ffReg.setValue("flv1",DWORD(1));
  ffReg.setValue("fvfw",DWORD(1)); ffReg.setValue("h261",DWORD(1)); ffReg.setValue("h263",DWORD(1));
  ffReg.setValue("h264",DWORD(1)); ffReg.setValue("hfyu",DWORD(1)); ffReg.setValue("iv32",DWORD(1));
  ffReg.setValue("mjpg",DWORD(1)); ffReg.setValue("mp41",DWORD(1)); ffReg.setValue("mp42",DWORD(1));
  ffReg.setValue("mp43",DWORD(1)); ffReg.setValue("mp4v",DWORD(1)); ffReg.setValue("mszh",DWORD(1));
  ffReg.setValue("png1",DWORD(1)); ffReg.setValue("qtrle",DWORD(1)); ffReg.setValue("qtrpza",DWORD(1));
  ffReg.setValue("raw_rawv",DWORD(1)); ffReg.setValue("rt21",DWORD(1)); ffReg.setValue("svq1",DWORD(1));
  ffReg.setValue("svq3",DWORD(1)); ffReg.setValue("theo",DWORD(1)); ffReg.setValue("vp3",DWORD(1));
  ffReg.setValue("vp5",DWORD(1)); ffReg.setValue("vp6",DWORD(1)); ffReg.setValue("vp6f",DWORD(1));
  ffReg.setValue("xvid",DWORD(1)); ffReg.setValue("zlib",DWORD(1)); ffReg.setValue("wmv1",DWORD(1));
  ffReg.setValue("wmv2",DWORD(1)); ffReg.setValue("wmv3",DWORD(1)); ffReg.setValue("wvc1",DWORD(1));
  ffReg.setValue("cavs",DWORD(1)); ffReg.setValue("mpegAVI",DWORD(1)); ffReg.setValue("em2v",DWORD(1));
  ffReg.setValue("avrn",DWORD(1)); ffReg.setValue("dvsd",DWORD(1)); ffReg.setValue("cdvc",DWORD(1));
  ffReg.setValue("cyuv",DWORD(1)); ffReg.setValue("asv1",DWORD(1)); ffReg.setValue("vcr1",DWORD(1));
  ffReg.setValue("cram",DWORD(1)); ffReg.setValue("cvid",DWORD(1)); ffReg.setValue("rv10",DWORD(1));
  ffReg.setValue("rle",DWORD(1)); ffReg.setValue("8bps",DWORD(1)); ffReg.setValue("tscc",DWORD(1));
  ffReg.setValue("qpeg",DWORD(1)); ffReg.setValue("loco",DWORD(1)); ffReg.setValue("wnv1",DWORD(1));
  ffReg.setValue("zmbv",DWORD(1)); ffReg.setValue("ulti",DWORD(1)); ffReg.setValue("vixl",DWORD(1));
  ffReg.setValue("aasc",DWORD(1));
}