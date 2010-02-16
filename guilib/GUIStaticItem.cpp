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
    CStdString label, label2, thumb, icon;
    XMLUtils::GetString(item, "label", label);   label  = CGUIControlFactory::FilterLabel(label);
    XMLUtils::GetString(item, "label2", label2); label2 = CGUIControlFactory::FilterLabel(label2);
    XMLUtils::GetString(item, "thumb", thumb);   thumb  = CGUIControlFactory::FilterLabel(thumb);
    XMLUtils::GetString(item, "icon", icon);     icon   = CGUIControlFactory::FilterLabel(icon);
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
    SetLabel(CGUIInfoLabel::GetLabel(label, parentID));
    SetLabel2(CGUIInfoLabel::GetLabel(label2, parentID));
    SetThumbnailImage(CGUIInfoLabel::GetLabel(thumb, parentID, true));
    SetIconImage(CGUIInfoLabel::GetLabel(icon, parentID, true));
    if (label.Find("$INFO") >= 0) SetProperty("info:label", label);
    if (label2.Find("$INFO") >= 0) SetProperty("info:label2", label2);
    if (icon.Find("$INFO") >= 0) SetProperty("info:icon", icon);
    if (thumb.Find("$INFO") >= 0) SetProperty("info:thumb", thumb);
    m_iprogramCount = id ? atoi(id) : 0;
    m_idepth = visibleCondition;
    // add any properties
    const TiXmlElement *property = item->FirstChildElement("property");
    while (property)
    {
      CStdString name = property->Attribute("name");
      CStdString value = property->FirstChild() ? property->FirstChild()->Value() : "";
      if (!name.IsEmpty() && !value.IsEmpty())
      {
        value = CGUIControlFactory::FilterLabel(value);
        SetProperty(name, CGUIInfoLabel::GetLabel(value, parentID));
        if (value.Find("$INFO") >= 0)
          SetProperty("info:" + name, value);
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
  for (PropertyMap::const_iterator i = m_mapProperties.begin(); i != m_mapProperties.end(); i++)
  {
    if (i->first.Left(5).Equals("info:"))
    {
      // prefer images if it's not label or label2
      CStdString prop(i->first.Mid(5));
      CStdString info(CGUIInfoLabel::GetLabel(i->second, contextWindow, !prop.Left(5).Equals("label")));
      if (prop.Equals("label"))
        SetLabel(info);
      else if (prop.Equals("label2"))
        SetLabel2(info);
      else if (prop.Equals("thumb"))
        SetThumbnailImage(info);
      else if (prop.Equals("icon"))
        SetIconImage(info);
      else
        SetProperty(prop, info);
    }
  }
}
