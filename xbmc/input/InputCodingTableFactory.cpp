/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
