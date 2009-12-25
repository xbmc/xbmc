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

    hr = sys_dev_enum->CreateClassEnumerator(CLSID_AudioRendererCategory, &enum_moniker, 0);
    if (hr != NOERROR) 
      break;
    
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
        {
          filterName = CStdString(var.bstrVal);
        }
        VariantClear(&var);
        VariantInit(&var);
        if (SUCCEEDED(propbag->Read(L"CLSID", &var, 0)))
        {
          filterGuid = CStdString(var.bstrVal);
          
        }
        directshow_add_filter(filterGuid,filterName);

      }
      
      
    }   
  } while (0);

  return vDSFilterInfo;
}

bool CDirectShowEnumerator::directshow_add_filter(LPCTSTR lpstrGuid, LPCTSTR lpcstrName)
{
  struct DSFilterInfo dInfo;
  dInfo.lpstrGuid = lpstrGuid;
  dInfo.lpstrName = lpcstrName;
  g_charsetConverter.unknownToUTF8(dInfo.lpstrName);
  CDirectShowEnumerator::vDSFilterInfo.push_back(dInfo);
  CLog::Log(LOGDEBUG, "%s - found Device: %s", __FUNCTION__,lpcstrName);
  return true;
}