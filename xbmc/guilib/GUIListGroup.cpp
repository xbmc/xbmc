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

#include "GUIListGroup.h"
#include "GUIListLabel.h"
#include "GUIMultiSelectText.h"
#include "utils/log.h"

CGUIListGroup::CGUIListGroup(int parentID, int controlID, float posX, float posY, float width, float height)
: CGUIControlGroup(parentID, controlID, posX, posY, width, height)
{
  m_item = NULL;
  ControlType = GUICONTROL_LISTGROUP;
}

CGUIListGroup::CGUIListGroup(const CGUIListGroup &right)
: CGUIControlGroup(right)
{
  m_item = NULL;
  ControlType = GUICONTROL_LISTGROUP;
}

CGUIListGroup::~CGUIListGroup(void)
{
  FreeResources();
}

void CGUIListGroup::AddControl(CGUIControl *control, int position /*= -1*/)
{
  if (control)
  {
    if (!(control->GetControlType() == CGUIControl::GUICONTROL_LISTLABEL ||
          control->GetControlType() == CGUIControl::GUICONTROL_LISTGROUP ||
          control->GetControlType() == CGUIControl::GUICONTROL_IMAGE ||
          control->GetControlType() == CGUIControl::GUICONTROL_BORDEREDIMAGE ||
          control->GetControlType() == CGUIControl::GUICONTROL_MULTI_IMAGE ||
          control->GetControlType() == CGUIControl::GUICONTROL_MULTISELECT ||
          control->GetControlType() == CGUIControl::GUICONTROL_TEXTBOX ||
          control->GetControlType() == CGUIControl::GUICONTROL_PROGRESS))
      CLog::Log(LOGWARNING, "Trying to add unsupported control type %d", control->GetControlType());
  }
  CGUIControlGroup::AddControl(control, position);
}

void CGUIListGroup::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  g_graphicsContext.SetOrigin(m_posX, m_posY);

  CRect rect;
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    control->UpdateVisibility(m_item);
    unsigned int oldDirty = dirtyregions.size();
    control->DoProcess(currentTime, dirtyregions);
    if (control->IsVisible() || (oldDirty != dirtyregions.size())) // visible or dirty (was visible?)
      rect.Union(control->GetRenderRegion());
  }

  g_graphicsContext.RestoreOrigin();
  CGUIControl::Process(currentTime, dirtyregions);
  m_renderRegion = rect;
  m_item = NULL;
}

void CGUIListGroup::ResetAnimation(ANIMATION_TYPE type)
{
  CGUIControl::ResetAnimation(type);
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
    (*it)->ResetAnimation(type);
}

void CGUIListGroup::UpdateVisibility(const CGUIListItem *item)
{
  CGUIControlGroup::UpdateVisibility(item);
  m_item = item;
}

void CGUIListGroup::UpdateInfo(const CGUIListItem *item)
{
  for (iControls it = m_children.begin(); it != m_children.end(); it++)
  {
    (*it)->UpdateInfo(item);
    (*it)->UpdateVisibility(item);
  }
  // now we have to check our overlapping label pairs
  for (unsigned int i = 0; i < m_children.size(); i++)
  {
    if (m_children[i]->GetControlType() == CGUIControl::GUICONTROL_LISTLABEL && m_children[i]->IsVisible())
    {
      for (unsigned int j = i + 1; j < m_children.size(); j++)
      {
        if (m_children[j]->GetControlType() == CGUIControl::GUICONTROL_LISTLABEL && m_children[j]->IsVisible())
          CGUIListLabel::CheckAndCorrectOverlap(*(CGUIListLabel *)m_children[i], *(CGUIListLabel *)m_children[j]);
      }
    }
  }
}

void CGUIListGroup::EnlargeWidth(float difference)
{
  // Alters the width of the controls that have an ID of 1 to 14
  for (iControls it = m_children.begin(); it != m_children.end(); it++)
  {
    CGUIControl *child = *it;
    if (child->GetID() >= 1 && child->GetID() <= 14)
    {
      if (child->GetID() == 1)
      {
        child->SetWidth(child->GetWidth() + difference);
        child->SetVisible(child->GetWidth() > 10);
      }
      else
      {
        child->SetWidth(child->GetWidth() + difference);
      }
    }
  }
  SetInvalid();
}

void CGUIListGroup::EnlargeHeight(float difference)
{
  // Alters the height of the controls that have an ID of 1 to 14
  for (iControls it = m_children.begin(); it != m_children.end(); it++)
  {
    CGUIControl *child = *it;
    if (child->GetID() >= 1 && child->GetID() <= 14)
    {
      if (child->GetID() == 1)
      {
        child->SetHeight(child->GetHeight() + difference);
        child->SetVisible(child->GetHeight() > 10);
      }
      else
      {
        child->SetHeight(child->GetHeight() + difference);
      }
    }
  }
  SetInvalid();
}

void CGUIListGroup::SetInvalid()
{
  if (!m_bInvalidated)
  { // this can be triggered by an item change, so all children need invalidating rather than just the group
    for (iControls it = m_children.begin(); it != m_children.end(); ++it)
      (*it)->SetInvalid();
    CGUIControlGroup::SetInvalid();
  }
}

void CGUIListGroup::SetFocusedItem(unsigned int focus)
{
  for (iControls it = m_children.begin(); it != m_children.end(); it++)
  {
    if ((*it)->GetControlType() == CGUIControl::GUICONTROL_MULTISELECT)
      ((CGUIMultiSelectTextControl *)(*it))->SetFocusedItem(focus);
    else if ((*it)->GetControlType() == CGUIControl::GUICONTROL_LISTGROUP)
      ((CGUIListGroup *)(*it))->SetFocusedItem(focus);
    else
      (*it)->SetFocus(focus > 0);
  }
  SetFocus(focus > 0);
}

unsigned int CGUIListGroup::GetFocusedItem() const
{
  for (ciControls it = m_children.begin(); it != m_children.end(); it++)
  {
    if ((*it)->GetControlType() == CGUIControl::GUICONTROL_MULTISELECT && ((CGUIMultiSelectTextControl *)(*it))->GetFocusedItem())
      return ((CGUIMultiSelectTextControl *)(*it))->GetFocusedItem();
    else if ((*it)->GetControlType() == CGUIControl::GUICONTROL_LISTGROUP && ((CGUIListGroup *)(*it))->GetFocusedItem())
      return ((CGUIListGroup *)(*it))->GetFocusedItem();
  }
  return 0;
}

bool CGUIListGroup::MoveLeft()
{
  for (iControls it = m_children.begin(); it != m_children.end(); it++)
  {
    if ((*it)->GetControlType() == CGUIControl::GUICONTROL_MULTISELECT && ((CGUIMultiSelectTextControl *)(*it))->MoveLeft())
      return true;
    else if ((*it)->GetControlType() == CGUIControl::GUICONTROL_LISTGROUP && ((CGUIListGroup *)(*it))->MoveLeft())
      return true;
  }
  return false;
}

bool CGUIListGroup::MoveRight()
{
  for (iControls it = m_children.begin(); it != m_children.end(); it++)
  {
    if ((*it)->GetControlType() == CGUIControl::GUICONTROL_MULTISELECT && ((CGUIMultiSelectTextControl *)(*it))->MoveRight())
      return true;
    else if ((*it)->GetControlType() == CGUIControl::GUICONTROL_LISTGROUP && ((CGUIListGroup *)(*it))->MoveRight())
      return true;
  }
  return false;
}

void CGUIListGroup::SetState(bool selected, bool focused)
{
  for (iControls it = m_children.begin(); it != m_children.end(); it++)
  {
    if ((*it)->GetControlType() == CGUIControl::GUICONTROL_LISTLABEL)
    {
      CGUIListLabel *label = (CGUIListLabel *)(*it);
      label->SetSelected(selected);
      label->SetScrolling(focused);
    }
    else if ((*it)->GetControlType() == CGUIControl::GUICONTROL_LISTGROUP)
      ((CGUIListGroup *)(*it))->SetState(selected, focused);
  }
}

void CGUIListGroup::SelectItemFromPoint(const CPoint &point)
{
  CPoint controlCoords(point);
  m_transform.InverseTransformPosition(controlCoords.x, controlCoords.y);
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *child = *it;
    if (child->GetControlType() == CGUIControl::GUICONTROL_MULTISELECT)
      ((CGUIMultiSelectTextControl *)child)->SelectItemFromPoint(point);
    else if (child->GetControlType() == CGUIControl::GUICONTROL_LISTGROUP)
      ((CGUIListGroup *)child)->SelectItemFromPoint(point);
  }
}
