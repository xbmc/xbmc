/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "DAVCommon.h"
#include "utils/StringUtils.h"
#include "utils/log.h"   

using namespace XFILE;

/*
 * Return true if pElement value is equal value without namespace.
 *
 * if pElement is <DAV:foo> and value is foo then ValueWithoutNamespace is true
 */
bool CDAVCommon::ValueWithoutNamespace(const TiXmlNode *pNode, const CStdString& value)
{
  CStdStringArray tag;
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

  StringUtils::SplitString(pElement->Value(), ":", tag, 2);

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
CStdString CDAVCommon::GetStatusTag(const TiXmlElement *pElement)
{
  const TiXmlElement *pChild;

  for (pChild = pElement->FirstChild()->ToElement(); pChild != 0; pChild = pChild->NextSibling()->ToElement())
  {
    if (ValueWithoutNamespace(pChild, "status"))
    {
      return CStdString(pChild->GetText());
    }
  }

  return "";
}

