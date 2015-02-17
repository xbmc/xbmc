#pragma once

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

#include "threads/CriticalSection.h"
#include "utils/StdString.h"

struct DSFiltersInfo
{
  CStdString lpstrGuid;
  CStdString lpstrName;
};

class CDSFilterEnumerator
{
public:
  CDSFilterEnumerator(void);
  HRESULT GetDSFilters(std::vector<DSFiltersInfo>& pFilters);

private:
  CCriticalSection m_critSection;
  void AddFilter(std::vector<DSFiltersInfo>& pFilters, CStdStringW lpGuid, CStdStringW lpName);
  static bool compare_by_word(const DSFiltersInfo& lhs, const DSFiltersInfo& rhs);
  
};

