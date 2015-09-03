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

#include "GUIStaticItem.h"
#include "utils/XMLUtils.h"
#include "GUIControlFactory.h"
#include "GUIInfoManager.h"
#include "utils/Variant.h"
#include "utils/StringUtils.h"

using namespace std;

CGUIStaticItem::CGUIStaticItem(const TiXmlElement *item, int parentID) : CFileItem()
{
  m_visState = false;

  assert(item);

  // check whether we're using the more verbose method...
  const TiXmlNode *click = item->FirstChild("onclick");
  if (click && click->FirstChild())
  {
    CGUIInfoLabel label, label2, thumb, icon;
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
    SetIconImage(icon.GetLabel(parentID, true));
    if (!label.IsConstant())  m_info.push_back(make_pair(label, "label"));
    if (!label2.IsConstant()) m_info.push_back(make_pair(label2, "label2"));
    if (!thumb.IsConstant())  m_info.push_back(make_pair(thumb, "thumb"));
    if (!icon.IsConstant())   m_info.push_back(make_pair(icon, "icon"));
    m_iprogramCount = id ? atoi(id) : 0;
    // add any properties
    const TiXmlElement *property = item->FirstChildElement("property");
    while (property)
    {
      std::string name = XMLUtils::GetAttribute(property, "name");
      CGUIInfoLabel prop;
      if (!name.empty() && CGUIControlFactory::GetInfoLabelFromElement(property, prop, parentID))
      {
        SetProperty(name, prop.GetLabel(parentID, true).c_str());
        if (!prop.IsConstant())
          m_info.push_back(make_pair(prop, name));
      }
      property = property->NextSiblingElement("property");
    }
  }
  else
  {
    std::string label, label2, thumb, icon;
    label  = XMLUtils::GetAttribute(item, "label");  label  = CGUIControlFactory::FilterLabel(label);
    label2 = XMLUtils::GetAttribute(item, "label2"); label2 = CGUIControlFactory::FilterLabel(label2);
    thumb  = XMLUtils::GetAttribute(item, "thumb");  thumb  = CGUIControlFactory::FilterLabel(thumb);
    icon   = XMLUtils::GetAttribute(item, "icon");   icon   = CGUIControlFactory::FilterLabel(icon);
    const char *id = item->Attribute("id");
    SetLabel(CGUIInfoLabel::GetLabel(label, parentID));
    SetPath(item->FirstChild()->Value());
    SetLabel2(CGUIInfoLabel::GetLabel(label2, parentID));
    SetArt("thumb", CGUIInfoLabel::GetLabel(thumb, parentID, true));
    SetIconImage(CGUIInfoLabel::GetLabel(icon, parentID, true));
    m_iprogramCount = id ? atoi(id) : 0;
  }
}

CGUIStaticItem::CGUIStaticItem(const CFileItem &item)
: CFileItem(item)
{
  m_visState = false;
}

void CGUIStaticItem::UpdateProperties(int contextWindow)
{
  for (InfoVector::const_iterator i = m_info.begin(); i != m_info.end(); ++i)
  {
    const CGUIInfoLabel &info = i->first;
    const std::string &name = i->second;
    bool preferTexture = strnicmp("label", name.c_str(), 5) != 0;
    std::string value(info.GetLabel(contextWindow, preferTexture));
    if (StringUtils::EqualsNoCase(name, "label"))
      SetLabel(value);
    else if (StringUtils::EqualsNoCase(name, "label2"))
      SetLabel2(value);
    else if (StringUtils::EqualsNoCase(name, "thumb"))
      SetArt("thumb", value);
    else if (StringUtils::EqualsNoCase(name, "icon"))
      SetIconImage(value);
    else
      SetProperty(name, value.c_str());
  }
}

bool CGUIStaticItem::UpdateVisibility(int contextWindow)
{
  if (!m_visCondition)
    return false;
  bool state = m_visCondition->Get();
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
  m_visCondition = g_infoManager.Register(condition, context);
  m_visState = false;
}
