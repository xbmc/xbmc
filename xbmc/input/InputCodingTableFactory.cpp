/*
*      Copyright (C) 2005-2013 Team Kodi
*      http://kodi.tv
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

#include "InputCodingTableFactory.h"
#include "InputCodingTableBaiduPY.h"
#include "InputCodingTableBasePY.h"
#include "InputCodingTableKorean.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

IInputCodingTable* CInputCodingTableFactory::CreateCodingTable(const std::string& strTableName, const TiXmlElement* element)
{
  if (strTableName == "BaiduPY")
  {
    const char* apiurl = element->Attribute("apiurl");
    if (apiurl == nullptr)
    {
      CLog::Log(LOGWARNING, "CInputCodingTableFactory: invalid \"apiurl\" attribute");
      return nullptr;
    }
    return new CInputCodingTableBaiduPY(apiurl);
  }
  if (strTableName == "BasePY")
    return new CInputCodingTableBasePY();
  if (strTableName == "Korean")
    return new CInputCodingTableKorean();
  return nullptr;
}
