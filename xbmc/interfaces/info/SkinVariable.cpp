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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "SkinVariable.h"
#include "GUIInfoManager.h"
#include "utils/XBMCTinyXML.h"

using namespace std;
using namespace INFO;

#define DEFAULT_VALUE -1

const CSkinVariableString* CSkinVariable::CreateFromXML(const TiXmlElement& node, int context)
{
  const char* name = node.Attribute("name");
  if (name)
  {
    CSkinVariableString* tmp = new CSkinVariableString;
    tmp->m_name = name;
    tmp->m_context = context;
    const TiXmlElement* valuenode = node.FirstChildElement("value");
    while (valuenode)
    {
      if (valuenode->FirstChild())
      {
        CSkinVariableString::ConditionLabelPair pair;
        if (valuenode->Attribute("condition"))
          pair.m_condition = g_infoManager.Register(valuenode->Attribute("condition"), context);
        else
          pair.m_condition = DEFAULT_VALUE;

        pair.m_label = CGUIInfoLabel(valuenode->FirstChild()->Value());
        tmp->m_conditionLabelPairs.push_back(pair);
        if (pair.m_condition == DEFAULT_VALUE)
          break; // once we reach default value (without condition) break iterating
      }
      valuenode = valuenode->NextSiblingElement("value");
    }
    if (tmp->m_conditionLabelPairs.size() > 0)
      return tmp;
    delete tmp;
  }
  return NULL;
}

CSkinVariableString::CSkinVariableString()
{
}

int CSkinVariableString::GetContext() const
{
  return m_context;
}

const CStdString& CSkinVariableString::GetName() const
{
  return m_name;
}

CStdString CSkinVariableString::GetValue(bool preferImage /* = false*/, const CGUIListItem *item /* = NULL */)
{
  for (VECCONDITIONLABELPAIR::const_iterator it = m_conditionLabelPairs.begin() ; it != m_conditionLabelPairs.end(); it++)
  {
    if (it->m_condition == DEFAULT_VALUE || g_infoManager.GetBoolValue(it->m_condition, item))
    {
      if (item)
        return it->m_label.GetItemLabel(item, preferImage);
      else
        return it->m_label.GetLabel(m_context, preferImage);
    }
  }
  return "";
}
