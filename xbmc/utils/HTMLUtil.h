#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "StdString.h"

namespace HTML
{
class CHTMLUtil
{
public:
  CHTMLUtil(void);
  virtual ~CHTMLUtil(void);
  int FindTag(const std::string& strHTML, const std::string& strTag, std::string& strtagFound, int iPos = 0) const;
  int FindClosingTag(const std::string& strHTML, const std::string& strTag, std::string& strtagFound, int iPos) const;
  void getValueOfTag(const std::string& strTagAndValue, std::string& strValue);
  void getAttributeOfTag(const std::string& strTagAndValue, const std::string& strTag, std::string& strValue);
  static void RemoveTags(std::string& strHTML);
  static void ConvertHTMLToW(const std::wstring& strHTML, std::wstring& strStripped);
};
}
