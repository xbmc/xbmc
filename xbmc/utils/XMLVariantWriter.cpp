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

#include "XMLVariantWriter.h"
#include "XMLUtils.h"

bool CXMLVariantWriter::Write(TiXmlElement *root, const CVariant &value)
{
  TiXmlElement valueNode("value");

  if (WriteValue(&valueNode, value))
    return root->InsertEndChild(valueNode) != NULL;
  else
    return false;
}

bool CXMLVariantWriter::WriteValue(TiXmlElement *valueNode, const CVariant &value)
{
  CStdString strValue;
  bool success = true;
  switch (value.type())
  {
  case CVariant::VariantTypeInteger:
    valueNode->SetAttribute("type", "integer");
    strValue.Format("%i", value.asInteger());
    success = valueNode->InsertEndChild(TiXmlText(strValue.c_str())) != NULL;
    break;
  case CVariant::VariantTypeUnsignedInteger:
    valueNode->SetAttribute("type", "unsignedinteger");
    strValue.Format("%i", value.asUnsignedInteger());
    success = valueNode->InsertEndChild(TiXmlText(strValue.c_str())) != NULL;
    break;
  case CVariant::VariantTypeDouble:
    valueNode->SetAttribute("type", "double");
    strValue.Format("%f", value.asFloat());
    success = valueNode->InsertEndChild(TiXmlText(strValue.c_str())) != NULL;
    break;
  case CVariant::VariantTypeBoolean:
    valueNode->SetAttribute("type", "boolean");
    success = valueNode->InsertEndChild(TiXmlText(value.asBoolean() ? "true" : "false")) != NULL;
    break;
  case CVariant::VariantTypeString:
    valueNode->SetAttribute("type", "string");
    success = valueNode->InsertEndChild(TiXmlText(value.asString())) != NULL;
    break;
  case CVariant::VariantTypeArray:
    valueNode->SetAttribute("type", "array");
    for (CVariant::const_iterator_array itr = value.begin_array(); itr != value.end_array() && success; itr++)
    {
      TiXmlElement memberNode("value");
      if (WriteValue(&memberNode, *itr))
        success &= valueNode->InsertEndChild(memberNode) != NULL;
      else
        success = false;
    }

    break;
  case CVariant::VariantTypeObject:
    valueNode->SetAttribute("type", "object");
    for (CVariant::const_iterator_map itr = value.begin_map(); itr != value.end_map() && success; itr++)
    {
      TiXmlElement memberNode("value");
      memberNode.SetAttribute("key", itr->first.c_str());
      if (WriteValue(&memberNode, itr->second))
        success &= valueNode->InsertEndChild(memberNode) != NULL;
      else
        success = false;
    }
    break;
  case CVariant::VariantTypeConstNull:
  case CVariant::VariantTypeNull:
  default:
    valueNode->SetAttribute("type", "null");
    break;
  }

  return success;
}
