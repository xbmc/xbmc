/*
 *      Copyright (C) 2005-2017 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "BinaryAddonExtensions.h"

#include "utils/XMLUtils.h"

using namespace ADDON;

bool CBinaryAddonExtensions::ParseExtension(const TiXmlElement* element)
{
  const char* cstring; /* "C" string point where parts from TinyXML becomes
                          stored, is used as this to prevent double use of
                          calls and to prevent not wanted "C++" throws if
                          std::string want to become set with nullptr! */

  cstring = element->Attribute("point");
  m_point = cstring ? cstring : "";

  EXT_VALUE extension;
  const TiXmlAttribute* attribute = element->FirstAttribute();
  while (attribute)
  {
    std::string name = attribute->Name();
    if (name != "point")
    {
      cstring = attribute->Value();
      if (cstring)
      {
        std::string value = cstring;
        name = "@" + name;
        extension.push_back(std::make_pair(name, SExtValue(value)));
      }
    }
    attribute = attribute->Next();
  }
  if (!extension.empty())
    m_values.push_back(std::pair<std::string, EXT_VALUE>("", extension));

  const TiXmlElement* childElement = element->FirstChildElement();
  while (childElement)
  {
    cstring = childElement->Value();
    if (cstring)
    {
      std::string id = cstring;

      EXT_VALUE extension;
      const TiXmlAttribute* attribute = childElement->FirstAttribute();
      while (attribute)
      {
        std::string name = attribute->Name();
        if (name != "point")
        {
          cstring = attribute->Value();
          if (cstring)
          {
            std::string value = cstring;
            name = id + "@" + name;
            extension.push_back(std::make_pair(name, SExtValue(value)));
          }
        }
        attribute = attribute->Next();
      }

      cstring = childElement->GetText();
      if (cstring)
        extension.push_back(std::make_pair(id, SExtValue(cstring)));

      if (!extension.empty())
        m_values.push_back(std::make_pair(id, extension));

      if (!cstring)
      {
        const TiXmlElement* childSubElement = childElement->FirstChildElement();
        if (childSubElement)
        {
          CBinaryAddonExtensions subElement;
          if (subElement.ParseExtension(childElement))
            m_children.push_back(std::make_pair(id, subElement));
        }
      }
    }
    childElement = childElement->NextSiblingElement();
  }

  return true;
}

const SExtValue CBinaryAddonExtensions::GetValue(const std::string& id) const
{
  for (auto values : m_values)
  {
    for (auto value : values.second)
    {
      if (value.first == id)
        return value.second;
    }
  }
  return SExtValue("");
}

const EXT_VALUES& CBinaryAddonExtensions::GetValues() const
{
  return m_values;
}

const CBinaryAddonExtensions* CBinaryAddonExtensions::GetElement(const std::string& id) const
{
  for (EXT_ELEMENTS::const_iterator it = m_children.begin(); it != m_children.end(); ++it)
  {
    if (it->first == id)
      return &it->second;
  }

  return nullptr;
}

const EXT_ELEMENTS CBinaryAddonExtensions::GetElements(const std::string& id) const
{
  if (id.empty())
    return m_children;

  EXT_ELEMENTS children;
  for (auto child : m_children)
  {
    if (child.first == id)
      children.push_back(std::make_pair(child.first, child.second));
  }
  return children;
}

void CBinaryAddonExtensions::Insert(const std::string& id, const std::string& value)
{
  EXT_VALUE extension;
  extension.push_back(std::make_pair(id, SExtValue(value)));
  m_values.push_back(std::make_pair(id, extension));
}
