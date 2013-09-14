/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "XMLVariantParser.h"
#include "XMLUtils.h"

CVariant CXMLVariantParser::Parse(const TiXmlElement *valueNode)
{
  CVariant value;
  Parse(valueNode, value);
  return value;
}

void CXMLVariantParser::Parse(const TiXmlElement *valueNode, CVariant &value)
{
  CStdString type = valueNode->Attribute("type");
  if (type.Equals("object"))
  {
    const TiXmlElement *memberNode = valueNode->FirstChildElement("value");
    while (memberNode)
    {
      const char *member = memberNode->Attribute("key");

      if (member)
        Parse(memberNode, value[member]);

      memberNode = memberNode->NextSiblingElement("value");
    }
  }
  else if (type.Equals("array"))
  {
    const TiXmlElement *memberNode = valueNode->FirstChildElement("value");
    while (memberNode)
    {
      CVariant member;
      Parse(memberNode, member);
      value.push_back(member);
      memberNode = memberNode->NextSiblingElement("value");
    }
  }
  else if (type.Equals("integer"))
  {
    value = (int64_t)atoi(valueNode->FirstChild()->Value());
  }
  else if (type.Equals("unsignedinteger"))
  {
    value = (uint64_t)atol(valueNode->FirstChild()->Value());
  }
  else if (type.Equals("boolean"))
  {
    CStdString boolean = valueNode->FirstChild()->Value();
    value = boolean == "true";
  }
  else if (type.Equals("string"))
  {
    value = valueNode->FirstChild()->Value();
  }
  else if (type.Equals("double"))
  {
    value = (float)atof(valueNode->FirstChild()->Value());
  }
}
