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

#include "SkinVariable.h"
#include "GUIInfoManager.h"
#include "tinyXML/tinyxml.h"

using namespace std;
using namespace INFO;

#define DEFAULT_VALUE -1

void CSkinVariable::LoadFromXML(const TiXmlElement *root)
{
  const TiXmlElement* node = root->FirstChildElement("variable");
  while (node)
  {
    if (node->Attribute("name") && node->FirstChild())
    {
      const char* type = node->Attribute("type");

      // parameter type will be used in future for numeric and bool variables
      if (!type || strnicmp(type, "text", 4) == 0)
      {
        CSkinVariableString tmp;
        tmp.m_name = node->Attribute("name");
        const TiXmlElement* valuenode = node->FirstChildElement("value");
        while (valuenode)
        {
          if (valuenode->FirstChild())
          {
            CSkinVariableString::ConditionLabelPair pair;
            if (valuenode->Attribute("condition"))
              pair.m_condition = g_infoManager.Register(valuenode->Attribute("condition"));
            else
              pair.m_condition = DEFAULT_VALUE;

            pair.m_label = CGUIInfoLabel(valuenode->FirstChild()->Value());
            tmp.m_conditionLabelPairs.push_back(pair);
            if (pair.m_condition == DEFAULT_VALUE)
              break; // once we reach default value (without condition) break iterating
          }
          valuenode = valuenode->NextSiblingElement("value");
        }
        if (tmp.m_conditionLabelPairs.size() > 0)
          g_infoManager.RegisterSkinVariableString(tmp);
      }
    }
    node = node->NextSiblingElement("variable");
  }
}

CSkinVariableString::CSkinVariableString()
{
}

const CStdString& CSkinVariableString::GetName() const
{
  return m_name;
}

CStdString CSkinVariableString::GetValue(int contextWindow, bool preferImage /* = false*/, const CGUIListItem *item /* = NULL */)
{
  for (VECCONDITIONLABELPAIR::const_iterator it = m_conditionLabelPairs.begin() ; it != m_conditionLabelPairs.end(); it++)
  {
    if (it->m_condition == DEFAULT_VALUE || g_infoManager.GetBoolValue(it->m_condition, item))
    {
      if (item)
        return it->m_label.GetItemLabel(item, preferImage);
      else
        return it->m_label.GetLabel(contextWindow, preferImage);
    }
  }
  return "";
}
