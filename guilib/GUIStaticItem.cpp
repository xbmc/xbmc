/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "GUIStaticItem.h"
#include "XMLUtils.h"
#include "GUIControlFactory.h"

using namespace std;

CGUIStaticItem::CGUIStaticItem(const TiXmlElement *item, int parentID) : CFileItem()
{
  assert(item);

  // check whether we're using the more verbose method...
  const TiXmlNode *click = item->FirstChild("onclick");
  if (click && click->FirstChild())
  {
    CGUIInfoLabel label, label2, thumb, icon;
    CGUIControlFactory::GetInfoLabel(item, "label", label);
    CGUIControlFactory::GetInfoLabel(item, "label2", label2);
    CGUIControlFactory::GetInfoLabel(item, "thumb", thumb);
    CGUIControlFactory::GetInfoLabel(item, "icon", icon);
    const char *id = item->Attribute("id");
    int visibleCondition = 0;
    CGUIControlFactory::GetConditionalVisibility(item, visibleCondition);
    // multiple action strings are concat'd together, separated with " , "
    vector<CGUIActionDescriptor> actions;
    CGUIControlFactory::GetMultipleString(item, "onclick", actions);
    for (vector<CGUIActionDescriptor>::iterator it = actions.begin(); it != actions.end(); ++it)
    {
      (*it).m_action.Replace(",", ",,");
      if (m_strPath.length() > 0)
      {
        m_strPath   += " , ";
      }
      m_strPath += (*it).m_action;
    }
    SetLabel(label.GetLabel(parentID));
    SetLabel2(label2.GetLabel(parentID));
    SetThumbnailImage(thumb.GetLabel(parentID, true));
    SetIconImage(icon.GetLabel(parentID, true));
    if (!label.IsConstant())  m_info.push_back(make_pair(label, "label"));
    if (!label2.IsConstant()) m_info.push_back(make_pair(label2, "label2"));
    if (!thumb.IsConstant())  m_info.push_back(make_pair(thumb, "thumb"));
    if (!icon.IsConstant())   m_info.push_back(make_pair(icon, "icon"));
    m_iprogramCount = id ? atoi(id) : 0;
    m_idepth = visibleCondition;
    // add any properties
    const TiXmlElement *property = item->FirstChildElement("property");
    while (property)
    {
      CStdString name = property->Attribute("name");
      CGUIInfoLabel prop;
      if (!name.IsEmpty() && CGUIControlFactory::GetInfoLabelFromElement(property, prop))
      {
        SetProperty(name, prop.GetLabel(parentID, true));
        if (!prop.IsConstant())
          m_info.push_back(make_pair(prop, name));
      }
      property = property->NextSiblingElement("property");
    }
  }
  else
  {
    CStdString label, label2, thumb, icon;
    label  = item->Attribute("label");  label  = CGUIControlFactory::FilterLabel(label);
    label2 = item->Attribute("label2"); label2 = CGUIControlFactory::FilterLabel(label2);
    thumb  = item->Attribute("thumb");  thumb  = CGUIControlFactory::FilterLabel(thumb);
    icon   = item->Attribute("icon");   icon   = CGUIControlFactory::FilterLabel(icon);
    const char *id = item->Attribute("id");
    SetLabel(CGUIInfoLabel::GetLabel(label, parentID));
    m_strPath = item->FirstChild()->Value();
    SetLabel2(CGUIInfoLabel::GetLabel(label2, parentID));
    SetThumbnailImage(CGUIInfoLabel::GetLabel(thumb, parentID, true));
    SetIconImage(CGUIInfoLabel::GetLabel(icon, parentID, true));
    m_iprogramCount = id ? atoi(id) : 0;
    m_idepth = 0;  // no visibility condition
  }
}
    
void CGUIStaticItem::UpdateProperties(int contextWindow)
{
  for (InfoVector::const_iterator i = m_info.begin(); i != m_info.end(); i++)
  {
    const CGUIInfoLabel &info = i->first;
    const CStdString &name = i->second;
    bool preferTexture = strnicmp("label", name.c_str(), 5) != 0;
    CStdString value(info.GetLabel(contextWindow, preferTexture));
    if (name.Equals("label"))
      SetLabel(value);
    else if (name.Equals("label2"))
      SetLabel2(value);
    else if (name.Equals("thumb"))
      SetThumbnailImage(value);
    else if (name.Equals("icon"))
      SetIconImage(value);
    else
      SetProperty(name, value);
  }
}
