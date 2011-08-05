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
#include "AudioEnumerator.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"
#include "streams.h"

#include "DSUtil/DSUtil.h"
#include "DSUtil/SmartPtr.h"

CAudioEnumerator::CAudioEnumerator(void)
{
}

HRESULT CAudioEnumerator::GetAudioRenderers(std::vector<DSFilterInfo>& pRenderers)
{
  CSingleLock lock (m_critSection);
  pRenderers.clear();

  Com::SmartPtr<IPropertyBag> propBag = NULL;
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

      AddFilter(pRenderers, filterGuid, filterName);
      propBag = NULL;
    } else
      return E_FAIL;
  }
  EndEnumSysDev;

  return S_OK;
}

void CAudioEnumerator::AddFilter( std::vector<DSFilterInfo>& pRenderers, CStdStringW lpGuid, CStdStringW lpName )
{
  DSFilterInfo filterInfo;

  filterInfo.lpstrGuid = lpGuid;
  g_charsetConverter.wToUTF8(lpName, filterInfo.lpstrName);

  pRenderers.push_back(filterInfo);
  
  CLog::Log(LOGDEBUG, "Found audio renderer device \"%s\" (guid: %s)", filterInfo.lpstrName.c_str(), filterInfo.lpstrGuid.c_str());
}

#endif