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

#include "GUIListItemLayout.h"
#include "FileItem.h"
#include "GUIControlFactory.h"
#include "GUIInfoManager.h"
#include "GUIListLabel.h"
#include "GUIImage.h"
#include "utils/XBMCTinyXML.h"

using namespace std;

CGUIListItemLayout::CGUIListItemLayout()
: m_group(0, 0, 0, 0, 0, 0)
{
  m_width = 0;
  m_height = 0;
  m_condition = 0;
  m_focused = false;
  m_invalidated = true;
  m_group.SetPushUpdates(true);
}

CGUIListItemLayout::CGUIListItemLayout(const CGUIListItemLayout &from)
: m_group(from.m_group), m_isPlaying(from.m_isPlaying)
{
  m_width = from.m_width;
  m_height = from.m_height;
  m_focused = from.m_focused;
  m_condition = from.m_condition;
  m_invalidated = true;
}

CGUIListItemLayout::~CGUIListItemLayout()
{
}

bool CGUIListItemLayout::IsAnimating(ANIMATION_TYPE animType)
{
  return m_group.IsAnimating(animType);
}

void CGUIListItemLayout::ResetAnimation(ANIMATION_TYPE animType)
{
  return m_group.ResetAnimation(animType);
}

float CGUIListItemLayout::Size(ORIENTATION orientation) const
{
  return (orientation == HORIZONTAL) ? m_width : m_height;
}

void CGUIListItemLayout::Process(CGUIListItem *item, int parentID, unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_invalidated)
  { // need to update our item
    m_invalidated = false;
    // could use a dynamic cast here if RTTI was enabled.  As it's not,
    // let's use a static cast with a virtual base function
    CFileItem *fileItem = item->IsFileItem() ? (CFileItem *)item : new CFileItem(*item);
    m_isPlaying.Update(item);
    m_group.SetInvalid();
    m_group.UpdateInfo(fileItem);
    // delete our temporary fileitem
    if (!item->IsFileItem())
      delete fileItem;
  }

  // update visibility, and render
  m_group.SetState(item->IsSelected() || m_isPlaying, m_focused);
  m_group.UpdateVisibility(item);
  m_group.DoProcess(currentTime, dirtyregions);
}

void CGUIListItemLayout::Render(CGUIListItem *item, int parentID)
{
  m_group.DoRender();
}

void CGUIListItemLayout::SetFocusedItem(unsigned int focus)
{
  m_group.SetFocusedItem(focus);
}

unsigned int CGUIListItemLayout::GetFocusedItem() const
{
  return m_group.GetFocusedItem();
}

void CGUIListItemLayout::SetWidth(float width)
{
  m_group.EnlargeWidth(width - m_width);
  m_width = width;
  SetInvalid();
}

void CGUIListItemLayout::SetHeight(float height)
{
  m_group.EnlargeHeight(height - m_height);
  m_height = height;
  SetInvalid();
}

void CGUIListItemLayout::SelectItemFromPoint(const CPoint &point)
{
  m_group.SelectItemFromPoint(point);
}

bool CGUIListItemLayout::MoveLeft()
{
  return m_group.MoveLeft();
}

bool CGUIListItemLayout::MoveRight()
{
  return m_group.MoveRight();
}

bool CGUIListItemLayout::CheckCondition()
{
  return !m_condition || g_infoManager.GetBoolValue(m_condition);
}

void CGUIListItemLayout::LoadControl(TiXmlElement *child, CGUIControlGroup *group)
{
  if (!group) return;

  CRect rect(group->GetXPosition(), group->GetYPosition(), group->GetXPosition() + group->GetWidth(), group->GetYPosition() + group->GetHeight());

  CGUIControlFactory factory;
  CGUIControl *control = factory.Create(0, rect, child, true);  // true indicating we're inside a list for the
                                                                // different label control + defaults.
  if (control)
  {
    group->AddControl(control);
    if (control->IsGroup())
    {
      TiXmlElement *grandChild = child->FirstChildElement("control");
      while (grandChild)
      {
        LoadControl(grandChild, (CGUIControlGroup *)control);
        grandChild = grandChild->NextSiblingElement("control");
      }
    }
  }
}

void CGUIListItemLayout::LoadLayout(TiXmlElement *layout, int context, bool focused)
{
  m_focused = focused;
  layout->QueryFloatAttribute("width", &m_width);
  layout->QueryFloatAttribute("height", &m_height);
  const char *condition = layout->Attribute("condition");
  if (condition)
    m_condition = g_infoManager.Register(condition, context);
  m_isPlaying.Parse("listitem.isplaying", context);
  TiXmlElement *child = layout->FirstChildElement("control");
  m_group.SetWidth(m_width);
  m_group.SetHeight(m_height);
  while (child)
  {
    LoadControl(child, &m_group);
    child = child->NextSiblingElement("control");
  }
  // ensure width and height are valid
  m_width = std::max(1.0f, m_width);
  m_height = std::max(1.0f, m_height);
}

//#ifdef PRE_SKIN_VERSION_9_10_COMPATIBILITY
void CGUIListItemLayout::CreateListControlLayouts(float width, float height, bool focused, const CLabelInfo &labelInfo, const CLabelInfo &labelInfo2, const CTextureInfo &texture, const CTextureInfo &textureFocus, float texHeight, float iconWidth, float iconHeight, const CStdString &nofocusCondition, const CStdString &focusCondition)
{
  m_width = width;
  m_height = height;
  m_focused = focused;
  m_isPlaying.Parse("listitem.isplaying", 0);
  CGUIImage *tex = new CGUIImage(0, 0, 0, 0, width, texHeight, texture);
  tex->SetVisibleCondition(nofocusCondition);
  m_group.AddControl(tex);
  if (focused)
  {
    CGUIImage *tex = new CGUIImage(0, 0, 0, 0, width, texHeight, textureFocus);
    tex->SetVisibleCondition(focusCondition);
    m_group.AddControl(tex);
  }
  CGUIImage *image = new CGUIImage(0, 0, 8, 0, iconWidth, texHeight, CTextureInfo(""));
  image->SetInfo(CGUIInfoLabel("$INFO[ListItem.Icon]", "", m_group.GetParentID()));
  image->SetAspectRatio(CAspectRatio::AR_KEEP);
  m_group.AddControl(image);
  float x = iconWidth + labelInfo.offsetX + 10;
  CGUIListLabel *label = new CGUIListLabel(0, 0, x, labelInfo.offsetY, width - x - 18, height, labelInfo, CGUIInfoLabel("$INFO[ListItem.Label]", "", m_group.GetParentID()), false);
  m_group.AddControl(label);
  x = labelInfo2.offsetX ? labelInfo2.offsetX : m_width - 16;
  label = new CGUIListLabel(0, 0, x, labelInfo2.offsetY, x - iconWidth - 20, height, labelInfo2, CGUIInfoLabel("$INFO[ListItem.Label2]", "", m_group.GetParentID()), false);
  m_group.AddControl(label);
}
//#endif

void CGUIListItemLayout::FreeResources(bool immediately)
{
  m_group.FreeResources(immediately);
}

#ifdef _DEBUG
void CGUIListItemLayout::DumpTextureUse()
{
  m_group.DumpTextureUse();
}
#endif
