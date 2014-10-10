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

