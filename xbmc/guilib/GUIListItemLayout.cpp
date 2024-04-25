/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIListItemLayout.h"

#include "FileItem.h"
#include "GUIControlFactory.h"
#include "GUIImage.h"
#include "GUIInfoManager.h"
#include "GUIListLabel.h"
#include "utils/XBMCTinyXML.h"

using namespace KODI::GUILIB;

CGUIListItemLayout::CGUIListItemLayout()
: m_group(0, 0, 0, 0, 0, 0)
{
  m_group.SetPushUpdates(true);
}

CGUIListItemLayout::CGUIListItemLayout(const CGUIListItemLayout& from)
  : CGUIListItemLayout(from, nullptr)
{
}

CGUIListItemLayout::CGUIListItemLayout(const CGUIListItemLayout& from, CGUIControl* control)
  : m_group(from.m_group),
    m_width(from.m_width),
    m_height(from.m_height),
    m_focused(from.m_focused),
    m_condition(from.m_condition),
    m_isPlaying(from.m_isPlaying),
    m_infoUpdateMillis(from.m_infoUpdateMillis)
{
  m_group.SetParentControl(control);
  m_infoUpdateTimeout.Set(m_infoUpdateMillis);

  // m_group was just created, cloned controls with resources must be allocated
  // before use
  m_group.AllocResources();
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
    CFileItem *fileItem = item->IsFileItem() ? static_cast<CFileItem*>(item) : new CFileItem(*item);
    m_isPlaying.Update(INFO::DEFAULT_CONTEXT, item);
    m_group.SetInvalid();
    m_group.UpdateInfo(fileItem);
    // delete our temporary fileitem
    if (!item->IsFileItem())
      delete fileItem;

    m_infoUpdateTimeout.Set(m_infoUpdateMillis);
  }
  else if (m_infoUpdateTimeout.IsTimePast())
  {
    m_isPlaying.Update(INFO::DEFAULT_CONTEXT, item);
    m_group.UpdateInfo(item);

    m_infoUpdateTimeout.Set(m_infoUpdateMillis);
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
  if (m_width != width)
  {
    m_group.EnlargeWidth(width - m_width);
    m_width = width;
    SetInvalid();
  }
}

void CGUIListItemLayout::SetHeight(float height)
{
  if (m_height != height)
  {
    m_group.EnlargeHeight(height - m_height);
    m_height = height;
    SetInvalid();
  }
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
  return !m_condition || m_condition->Get(INFO::DEFAULT_CONTEXT);
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
        LoadControl(grandChild, static_cast<CGUIControlGroup*>(control));
        grandChild = grandChild->NextSiblingElement("control");
      }
    }
  }
}

void CGUIListItemLayout::LoadLayout(TiXmlElement *layout, int context, bool focused, float maxWidth, float maxHeight)
{
  m_focused = focused;
  layout->QueryFloatAttribute("width", &m_width);
  layout->QueryFloatAttribute("height", &m_height);
  const char *condition = layout->Attribute("condition");
  if (condition)
    m_condition = CServiceBroker::GetGUI()->GetInfoManager().Register(condition, context);
  unsigned int infoupdatemillis = 0;
  layout->QueryUnsignedAttribute("infoupdate", &infoupdatemillis);
  if (infoupdatemillis > 0)
    m_infoUpdateMillis = std::chrono::milliseconds(infoupdatemillis);

  m_infoUpdateTimeout.Set(m_infoUpdateMillis);
  m_isPlaying.Parse("listitem.isplaying", context);
  // ensure width and height are valid
  if (!m_width)
    m_width = maxWidth;
  if (!m_height)
    m_height = maxHeight;
  m_width = std::max(1.0f, m_width);
  m_height = std::max(1.0f, m_height);
  m_group.SetWidth(m_width);
  m_group.SetHeight(m_height);

  TiXmlElement *child = layout->FirstChildElement("control");
  while (child)
  {
    LoadControl(child, &m_group);
    child = child->NextSiblingElement("control");
  }
}

//#ifdef GUILIB_PYTHON_COMPATIBILITY
void CGUIListItemLayout::CreateListControlLayouts(float width, float height, bool focused, const CLabelInfo &labelInfo, const CLabelInfo &labelInfo2, const CTextureInfo &texture, const CTextureInfo &textureFocus, float texHeight, float iconWidth, float iconHeight, const std::string &nofocusCondition, const std::string &focusCondition)
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
  image->SetInfo(GUIINFO::CGUIInfoLabel("$INFO[ListItem.Icon]", "", m_group.GetParentID()));
  image->SetAspectRatio(CAspectRatio::AR_KEEP);
  m_group.AddControl(image);
  float x = iconWidth + labelInfo.offsetX + 10;
  CGUIListLabel *label = new CGUIListLabel(0, 0, x, labelInfo.offsetY, width - x - 18, height, labelInfo, GUIINFO::CGUIInfoLabel("$INFO[ListItem.Label]", "", m_group.GetParentID()), CGUIControl::FOCUS);
  m_group.AddControl(label);
  x = labelInfo2.offsetX ? labelInfo2.offsetX : m_width - 16;
  label = new CGUIListLabel(0, 0, x, labelInfo2.offsetY, x - iconWidth - 20, height, labelInfo2, GUIINFO::CGUIInfoLabel("$INFO[ListItem.Label2]", "", m_group.GetParentID()), CGUIControl::FOCUS);
  m_group.AddControl(label);
}
//#endif

void CGUIListItemLayout::FreeResources(bool immediately)
{
  m_group.FreeResources(immediately);
}

void CGUIListItemLayout::AssignDepth()
{
  m_group.AssignDepth();
}

#ifdef _DEBUG
void CGUIListItemLayout::DumpTextureUse()
{
  m_group.DumpTextureUse();
}
#endif
