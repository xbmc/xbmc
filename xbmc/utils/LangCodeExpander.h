#pragma once
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

#include "utils/StdString.h"
#include <map>

class TiXmlElement;

class CLangCodeExpander
{
public:

  CLangCodeExpander(void);
  ~CLangCodeExpander(void);

  bool Lookup(CStdString& desc, const CStdString& code);
  bool Lookup(CStdString& desc, const int code);
  bool ConvertToTwoCharCode(const CStdString& lang, CStdString& code);
#ifdef TARGET_WINDOWS
  bool ConvertTwoToThreeCharCode(CStdString& strThreeCharCode, const CStdString& strTwoCharCode, bool localeHack = false);
  bool ConvertToThreeCharCode(CStdString& strThreeCharCode, const CStdString& strCharCode, bool localeHack = false);
#else
  bool ConvertTwoToThreeCharCode(CStdString& strThreeCharCode, const CStdString& strTwoCharCode);
  bool ConvertToThreeCharCode(CStdString& strThreeCharCode, const CStdString& strCharCode);
#endif

#ifdef TARGET_WINDOWS
  bool ConvertLinuxToWindowsRegionCodes(const CStdString& strTwoCharCode, CStdString& strThreeCharCode);
  bool ConvertWindowsToGeneralCharCode(const CStdString& strWindowsCharCode, CStdString& strThreeCharCode);
#endif

  void LoadUserCodes(const TiXmlElement* pRootElement);
  void Clear();
protected:
  void CodeToString(long code, CStdString& ret);

  typedef std::map<CStdString, CStdString> STRINGLOOKUPTABLE;
  STRINGLOOKUPTABLE m_mapUser;

  bool LookupInDb(CStdString& desc, const CStdString& code);
  bool LookupInMap(CStdString& desc, const CStdString& code);
  bool ReverseLookup(const CStdString& desc, CStdString& code);
};

extern CLangCodeExpander g_LangCodeExpander;
