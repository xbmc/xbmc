/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIStaticItem.h"

#include "GUIControlFactory.h"
#include "GUIInfoManager.h"
#include "guilib/GUIComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"

using namespace KODI::GUILIB;

CGUIStaticItem::CGUIStaticItem(const TiXmlElement *item, int parentID) : CFileItem()
{
  m_visState = false;

  assert(item);

  GUIINFO::CGUIInfoLabel label, label2, thumb, icon;
  CGUIControlFactory::GetInfoLabel(item, "label", label, parentID);
  CGUIControlFactory::GetInfoLabel(item, "label2", label2, parentID);
  CGUIControlFactory::GetInfoLabel(item, "thumb", thumb, parentID);
  CGUIControlFactory::GetInfoLabel(item, "icon", icon, parentID);
  const char *id = item->Attribute("id");
  std::string condition;
  CGUIControlFactory::GetConditionalVisibility(item, condition);
  SetVisibleCondition(condition, parentID);
  CGUIControlFactory::GetActions(item, "onclick", m_clickActions);
  SetLabel(label.GetLabel(parentID));
  SetLabel2(label2.GetLabel(parentID));
  SetArt("thumb", thumb.GetLabel(parentID, true));
  SetArt("icon", icon.GetLabel(parentID, true));
  if (!label.IsConstant())
    m_info.emplace_back(label, "label");
  if (!label2.IsConstant())
    m_info.emplace_back(label2, "label2");
  if (!thumb.IsConstant())
    m_info.emplace_back(thumb, "thumb");
  if (!icon.IsConstant())
    m_info.emplace_back(icon, "icon");
  m_iprogramCount = id ? atoi(id) : 0;
  // add any properties
  const TiXmlElement *property = item->FirstChildElement("property");
  while (property)
  {
    std::string name = XMLUtils::GetAttribute(property, "name");
    GUIINFO::CGUIInfoLabel prop;
    if (!name.empty() && CGUIControlFactory::GetInfoLabelFromElement(property, prop, parentID))
    {
      SetProperty(name, prop.GetLabel(parentID, true).c_str());
      if (!prop.IsConstant())
        m_info.emplace_back(prop, name);
    }
    property = property->NextSiblingElement("property");
  }
}

CGUIStaticItem::CGUIStaticItem(const CFileItem &item)
: CFileItem(item)
{
  m_visState = false;
}

CGUIStaticItem::CGUIStaticItem(const CGUIStaticItem& other)
  : CFileItem(other),
    m_info(other.m_info),
    m_visCondition(other.m_visCondition),
    m_visState(other.m_visState),
    m_clickActions(other.m_clickActions)
{
}

void CGUIStaticItem::UpdateProperties(int contextWindow)
{
  for (const auto& i : m_info)
  {
    const GUIINFO::CGUIInfoLabel& info = i.first;
    const std::string& name = i.second;
    bool preferTexture = StringUtils::CompareNoCase("label", name, 5) != 0;
    const std::string& value(info.GetLabel(contextWindow, preferTexture));
    if (StringUtils::EqualsNoCase(name, "label"))
      SetLabel(value);
    else if (StringUtils::EqualsNoCase(name, "label2"))
      SetLabel2(value);
    else if (StringUtils::EqualsNoCase(name, "thumb"))
      SetArt("thumb", value);
    else if (StringUtils::EqualsNoCase(name, "icon"))
      SetArt("icon", value);
    else
      SetProperty(name, value.c_str());
  }
}

bool CGUIStaticItem::UpdateVisibility(int contextWindow)
{
  if (!m_visCondition)
    return false;
  bool state = m_visCondition->Get(contextWindow);
  if (state != m_visState)
  {
    m_visState = state;
    return true;
  }
  return false;
}

bool CGUIStaticItem::IsVisible() const
{
  if (m_visCondition)
    return m_visState;
  return true;
}

void CGUIStaticItem::SetVisibleCondition(const std::string &condition, int context)
{
  m_visCondition = CServiceBroker::GetGUI()->GetInfoManager().Register(condition, context);
  m_visState = false;
}
