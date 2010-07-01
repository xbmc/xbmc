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

#ifdef HAS_DS_PLAYER

#include "system.h"
#include "WINDirectShowEnumerator.h"
#include "SingleLock.h"
#include "log.h"
#include "CharsetConverter.h"
#include "streams.h"
#include "DShowUtil/DShowUtil.h"

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
  
  Com::SmartPtr<IPropertyBag> propBag;

  BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker)
  {
    if (SUCCEEDED(pMoniker->BindToStorage(NULL, NULL, IID_IPropertyBag, (void**) &propBag)))
    {
      _variant_t var;

      CStdStringW filterName;
      CStdStringW filterGuid;

      if (SUCCEEDED(propBag->Read(L"FriendlyName", &var, 0)))
        filterName = CStdStringW(var.bstrVal);

      var.Clear();

      if (SUCCEEDED(propBag->Read(L"CLSID", &var, 0)))
        filterGuid = CStdStringW(var.bstrVal);

      AddFilter(filterGuid, filterName);

      propBag = NULL;
    }
  }
  EndEnumSysDev;


  return vDSFilterInfo;
}

bool CDirectShowEnumerator::AddFilter( CStdStringW lpGuid, CStdStringW lpName )
{
  DSFilterInfo filterInfo;
  filterInfo.lpstrGuid = lpGuid;

  g_charsetConverter.wToUTF8(lpName, filterInfo.lpstrName);

  vDSFilterInfo.push_back(filterInfo);
  
  CLog::Log(LOGDEBUG, "Found DirectShow device \"%s\" (guid: %s)", filterInfo.lpstrName.c_str(), filterInfo.lpstrGuid.c_str());
  return true;
}

#endif