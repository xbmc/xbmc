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

#include "GUIButtonControl.h"
#include "GUIWindowManager.h"
#include "GUIDialog.h"
#include "utils/CharsetConverter.h"
#include "GUIFontManager.h"

using namespace std;

CGUIButtonControl::CGUIButtonControl(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus, const CLabelInfo& labelInfo)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
    , m_imgFocus(posX, posY, width, height, textureFocus)
    , m_imgNoFocus(posX, posY, width, height, textureNoFocus)
    , m_label(posX, posY, width, height, labelInfo)
    , m_label2(posX, posY, width, height, labelInfo)
{
  m_bSelected = false;
  m_alpha = 255;
  m_focusCounter = 0;
  ControlType = GUICONTROL_BUTTON;
}

CGUIButtonControl::~CGUIButtonControl(void)
{
}

void CGUIButtonControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_bInvalidated)
  {
    m_imgFocus.SetWidth(m_width);
    m_imgFocus.SetHeight(m_height);

    m_imgNoFocus.SetWidth(m_width);
    m_imgNoFocus.SetHeight(m_height);
  }

  if (HasFocus())
  {
    unsigned int alphaChannel = m_alpha;
    if (m_pulseOnSelect)
    {
      unsigned int alphaCounter = m_focusCounter + 2;
      if ((alphaCounter % 128) >= 64)
        alphaChannel = alphaCounter % 64;
      else
        alphaChannel = 63 - (alphaCounter % 64);

      alphaChannel += 192;
      alphaChannel = (unsigned int)((float)m_alpha * (float)alphaChannel / 255.0f);
    }
    if (m_imgFocus.SetAlpha((unsigned char)alphaChannel))
      MarkDirtyRegion();

    m_imgFocus.SetVisible(true);
    m_imgNoFocus.SetVisible(false);
    m_focusCounter++;
  }
  else
  {
    m_imgFocus.SetVisible(false);
    m_imgNoFocus.SetVisible(true);
  }

  m_imgFocus.Process(currentTime);
  m_imgNoFocus.Process(currentTime);

  ProcessText(currentTime);
  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIButtonControl::Render()
{
  m_imgFocus.Render();
  m_imgNoFocus.Render();

  RenderText();
  CGUIControl::Render();
}

void CGUIButtonControl::RenderText()
{
  m_label.Render();
  m_label2.Render();
}

CGUILabel::COLOR CGUIButtonControl::GetTextColor() const
{
  if (IsDisabled())
    return CGUILabel::COLOR_DISABLED;
  if (HasFocus())
    return CGUILabel::COLOR_FOCUSED;
  return CGUILabel::COLOR_TEXT;
}

void CGUIButtonControl::ProcessText(unsigned int currentTime)
{
  CRect labelRenderRect = m_label.GetRenderRect();
  CRect label2RenderRect = m_label2.GetRenderRect();

  bool changed = m_label.SetMaxRect(m_posX, m_posY, m_width, m_height);
  changed |= m_label.SetText(m_info.GetLabel(m_parentID));
  changed |= m_label.SetScrolling(HasFocus());

  // render the second label if it exists
  CStdString label2(m_info2.GetLabel(m_parentID));
  changed |= m_label2.SetMaxRect(m_posX, m_posY, m_width, m_height);
  changed |= m_label2.SetText(label2);
  if (!label2.IsEmpty())
  {
    changed |= m_label2.SetAlign(XBFONT_RIGHT | (m_label.GetLabelInfo().align & XBFONT_CENTER_Y) | XBFONT_TRUNCATED);
    changed |= m_label2.SetScrolling(HasFocus());

    // If overlapping was corrected - compare render rects to determine
    // if they changed since last frame.
    if (CGUILabel::CheckAndCorrectOverlap(m_label, m_label2))
      changed |= (m_label.GetRenderRect()  != labelRenderRect ||
                  m_label2.GetRenderRect() != label2RenderRect);

    changed |= m_label2.SetColor(GetTextColor());
  }
  changed |= m_label.SetColor(GetTextColor());
  if (changed)
    MarkDirtyRegion();
}

bool CGUIButtonControl::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SELECT_ITEM)
  {
    OnClick();
    return true;
  }
  return CGUIControl::OnAction(action);
}

bool CGUIButtonControl::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID())
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      SetLabel(message.GetLabel());
      return true;
    }
    if (message.GetMessage() == GUI_MSG_LABEL2_SET)
    {
      SetLabel2(message.GetLabel());
      return true;
    }
    if (message.GetMessage() == GUI_MSG_SELECTED)
    {
      if (!m_bSelected)
        SetInvalid();
      m_bSelected = true;
      return true;
    }
    if (message.GetMessage() == GUI_MSG_DESELECTED)
    {
      if (m_bSelected)
        SetInvalid();
      m_bSelected = false;
      return true;
    }
  }

  return CGUIControl::OnMessage(message);
}

void CGUIButtonControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_focusCounter = 0;
  m_imgFocus.AllocResources();
  m_imgNoFocus.AllocResources();
  if (!m_width)
    m_width = m_imgFocus.GetWidth();
  if (!m_height)
    m_height = m_imgFocus.GetHeight();
}

void CGUIButtonControl::FreeResources(bool immediately)
{
  CGUIControl::FreeResources(immediately);
  m_imgFocus.FreeResources(immediately);
  m_imgNoFocus.FreeResources(immediately);
}

void CGUIButtonControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgFocus.DynamicResourceAlloc(bOnOff);
  m_imgNoFocus.DynamicResourceAlloc(bOnOff);
}

void CGUIButtonControl::SetInvalid()
{
  CGUIControl::SetInvalid();
  m_label.SetInvalid();
  m_label2.SetInvalid();
  m_imgFocus.SetInvalid();
  m_imgNoFocus.SetInvalid();
}

void CGUIButtonControl::SetLabel(const string &label)
{ // NOTE: No fallback for buttons at this point
  m_info.SetLabel(label, "", GetParentID());
  SetInvalid();
}

void CGUIButtonControl::SetLabel2(const string &label2)
{ // NOTE: No fallback for buttons at this point
  m_info2.SetLabel(label2, "", GetParentID());
  SetInvalid();
}

void CGUIButtonControl::SetPosition(float posX, float posY)
{
  CGUIControl::SetPosition(posX, posY);
  m_imgFocus.SetPosition(posX, posY);
  m_imgNoFocus.SetPosition(posX, posY);
}

void CGUIButtonControl::SetAlpha(unsigned char alpha)
{
  if (m_alpha != alpha)
    MarkDirtyRegion();
  m_alpha = alpha;
}

bool CGUIButtonControl::UpdateColors()
{
  bool changed = CGUIControl::UpdateColors();
  changed |= m_label.UpdateColors();
  changed |= m_imgFocus.SetDiffuseColor(m_diffuseColor);
  changed |= m_imgNoFocus.SetDiffuseColor(m_diffuseColor);

  return changed;
}

CRect CGUIButtonControl::CalcRenderRegion() const
{
  CRect buttonRect = CGUIControl::CalcRenderRegion();
  CRect textRect = m_label.GetRenderRect();
  buttonRect.Union(textRect);
  return buttonRect;
}

EVENT_RESULT CGUIButtonControl::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_LEFT_CLICK)
  {
    OnAction(CAction(ACTION_SELECT_ITEM));
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

CStdString CGUIButtonControl::GetDescription() const
{
  CStdString strLabel(m_info.GetLabel(m_parentID));
  return strLabel;
}

CStdString CGUIButtonControl::GetLabel2() const
{
  CStdString strLabel(m_info2.GetLabel(m_parentID));
  return strLabel;
}

void CGUIButtonControl::PythonSetLabel(const CStdString &strFont, const string &strText, color_t textColor, color_t shadowColor, color_t focusedColor)
{
  m_label.GetLabelInfo().font = g_fontManager.GetFont(strFont);
  m_label.GetLabelInfo().textColor = textColor;
  m_label.GetLabelInfo().focusedColor = focusedColor;
  m_label.GetLabelInfo().shadowColor = shadowColor;
  SetLabel(strText);
}

void CGUIButtonControl::PythonSetDisabledColor(color_t disabledColor)
{
  m_label.GetLabelInfo().disabledColor = disabledColor;
}

void CGUIButtonControl::SettingsCategorySetTextAlign(uint32_t align)
{
  m_label.SetAlign(align);
}

void CGUIButtonControl::OnClick()
{
  // Save values, as the click message may deactivate the window
  int controlID = GetID();
  int parentID = GetParentID();
  CGUIAction clickActions = m_clickActions;

  // button selected, send a message
  CGUIMessage msg(GUI_MSG_CLICKED, controlID, parentID, 0);
  SendWindowMessage(msg);

  clickActions.ExecuteActions(controlID, parentID);
}

void CGUIButtonControl::OnFocus()
{
  m_focusActions.ExecuteActions(GetID(), GetParentID());
}

void CGUIButtonControl::OnUnFocus()
{
  m_unfocusActions.ExecuteActions(GetID(), GetParentID());
}

void CGUIButtonControl::SetSelected(bool bSelected)
{
  if (m_bSelected != bSelected)
  {
    m_bSelected = bSelected;
    SetInvalid();
  }
}
