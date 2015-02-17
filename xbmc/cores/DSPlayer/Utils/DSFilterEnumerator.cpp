/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *      Copyright (C) 2014-2015 Aracnoz
 *      http://github.com/aracnoz/xbmc
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
#include "DSFilterEnumerator.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "streams.h"

#include "DSUtil/DSUtil.h"
#include "DSUtil/SmartPtr.h"

CDSFilterEnumerator::CDSFilterEnumerator(void)
{
}

HRESULT CDSFilterEnumerator::GetDSFilters(std::vector<DSFiltersInfo>& pFilters)
{
  CSingleLock lock (m_critSection);
  pFilters.clear();

  Com::SmartPtr<IPropertyBag> propBag = NULL;
  BeginEnumSysDev(CLSID_LegacyAmFilterCategory, pMoniker)
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

      AddFilter(pFilters, filterGuid, filterName);
      propBag = NULL;
    } else
      return E_FAIL;
  }
  EndEnumSysDev;

  std::sort(pFilters.begin(), pFilters.end(), compare_by_word);

  return S_OK;
}

bool CDSFilterEnumerator::compare_by_word(const DSFiltersInfo& lhs, const DSFiltersInfo& rhs) {

  CStdString strLine1 = lhs.lpstrName;
  CStdString strLine2 = rhs.lpstrName;
  StringUtils::ToLower(strLine1);
  StringUtils::ToLower(strLine2);
  return strcmp(strLine1.c_str(), strLine2.c_str()) < 0;

}

void CDSFilterEnumerator::AddFilter(std::vector<DSFiltersInfo>& pFilters, CStdStringW lpGuid, CStdStringW lpName)
{
  DSFiltersInfo filterInfo;

  filterInfo.lpstrGuid = lpGuid;
  g_charsetConverter.wToUTF8(lpName, filterInfo.lpstrName);

  pFilters.push_back(filterInfo);
}

#endif