/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DAVCommon.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

using namespace XFILE;

/*
 * Return true if pElement value is equal value without namespace.
 *
 * if pElement is <DAV:foo> and value is foo then ValueWithoutNamespace is true
 */
bool CDAVCommon::ValueWithoutNamespace(const TiXmlNode *pNode, const std::string& value)
{
  const TiXmlElement *pElement;

  if (!pNode)
  {
    return false;
  }

  pElement = pNode->ToElement();

  if (!pElement)
  {
    return false;
  }

  std::vector<std::string> tag = StringUtils::Split(pElement->ValueStr(), ":", 2);

  if (tag.size() == 1 && tag[0] == value)
  {
    return true;
  }
  else if (tag.size() == 2 && tag[1] == value)
  {
    return true;
  }
  else if (tag.size() > 2)
  {
    CLog::Log(LOGERROR, "%s - Splitting %s failed, size(): %lu, value: %s", __FUNCTION__, pElement->Value(), (unsigned long int)tag.size(), value.c_str());
  }

  return false;
}

/*
 * Search for <status> and return its content
 */
std::string CDAVCommon::GetStatusTag(const TiXmlElement *pElement)
{
  const TiXmlElement *pChild;

  for (pChild = pElement->FirstChildElement(); pChild != 0; pChild = pChild->NextSiblingElement())
  {
    if (ValueWithoutNamespace(pChild, "status"))
    {
      return pChild->NoChildren() ? "" : pChild->FirstChild()->ValueStr();
    }
  }

  return "";
}

