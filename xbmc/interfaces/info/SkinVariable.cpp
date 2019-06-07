/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SkinVariable.h"

#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "utils/XBMCTinyXML.h"

using namespace INFO;
using namespace KODI;

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
      CSkinVariableString::ConditionLabelPair pair;
      const char *condition = valuenode->Attribute("condition");
      if (condition)
        pair.m_condition = CServiceBroker::GetGUI()->GetInfoManager().Register(condition, context);

      auto label = valuenode->FirstChild() ? valuenode->FirstChild()->ValueStr() : "";
      pair.m_label = GUILIB::GUIINFO::CGUIInfoLabel(label);
      tmp->m_conditionLabelPairs.push_back(pair);
      if (!pair.m_condition)
        break; // once we reach default value (without condition) break iterating

      valuenode = valuenode->NextSiblingElement("value");
    }
    if (!tmp->m_conditionLabelPairs.empty())
      return tmp;
    delete tmp;
  }
  return NULL;
}

CSkinVariableString::CSkinVariableString() = default;

int CSkinVariableString::GetContext() const
{
  return m_context;
}

const std::string& CSkinVariableString::GetName() const
{
  return m_name;
}

std::string CSkinVariableString::GetValue(bool preferImage /* = false */, const CGUIListItem *item /* = nullptr */) const
{
  for (const auto& it : m_conditionLabelPairs)
  {
    if (!it.m_condition || it.m_condition->Get(item))
    {
      if (item)
        return it.m_label.GetItemLabel(item, preferImage);
      else
        return it.m_label.GetLabel(m_context, preferImage);
    }
  }
  return "";
}
