/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "include.h"
#include "GUIButtonControl.h"
#include "GUIWindowManager.h"
#include "GUIDialog.h"
#include "utils/CharsetConverter.h"
#include "GUIFontManager.h"

using namespace std;

CGUIButtonControl::CGUIButtonControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus, const CLabelInfo& labelInfo)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
    , m_imgFocus(posX, posY, width, height, textureFocus)
    , m_imgNoFocus(posX, posY, width, height, textureNoFocus)
    , m_textLayout(labelInfo.font, false), m_textLayout2(labelInfo.font, false)
{
  m_bSelected = false;
  m_alpha = 255;
  m_dwFocusCounter = 0;
  m_label = labelInfo;
  ControlType = GUICONTROL_BUTTON;
}

CGUIButtonControl::~CGUIButtonControl(void)
{
}

void CGUIButtonControl::Render()
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
    if (m_pulseOnSelect)
    {
      DWORD dwAlphaCounter = m_dwFocusCounter + 2;
      DWORD dwAlphaChannel;
      if ((dwAlphaCounter % 128) >= 64)
        dwAlphaChannel = dwAlphaCounter % 64;
      else
        dwAlphaChannel = 63 - (dwAlphaCounter % 64);

      dwAlphaChannel += 192;
      dwAlphaChannel = (DWORD)((float)m_alpha * (float)dwAlphaChannel / 255.0f);
      m_imgFocus.SetAlpha((unsigned char)dwAlphaChannel);
    }
    m_imgFocus.SetVisible(true);
    m_imgNoFocus.SetVisible(false);
    m_dwFocusCounter++;
  }
  else
  {
    m_imgFocus.SetVisible(false);
    m_imgNoFocus.SetVisible(true);
  }
  // render both so the visibility settings cause the frame counter to resetcorrectly
  m_imgFocus.Render();
  m_imgNoFocus.Render();

  RenderText();
  CGUIControl::Render();
}

void CGUIButtonControl::RenderText()
{
  m_textLayout.Update(m_info.GetLabel(m_dwParentID));

  float fPosX = m_posX + m_label.offsetX;
  float fPosY = m_posY + m_label.offsetY;

  if (m_label.align & XBFONT_RIGHT)
    fPosX = m_posX + m_width - m_label.offsetX;

  if (m_label.align & XBFONT_CENTER_X)
    fPosX = m_posX + m_width / 2;

  if (m_label.align & XBFONT_CENTER_Y)
    fPosY = m_posY + m_height / 2;

  if (IsDisabled())
    m_textLayout.Render( fPosX, fPosY, m_label.angle, m_label.disabledColor, m_label.shadowColor, m_label.align, m_label.width, true);
  else if (HasFocus() && m_label.focusedColor)
    m_textLayout.Render( fPosX, fPosY, m_label.angle, m_label.focusedColor, m_label.shadowColor, m_label.align, m_label.width);
  else
    m_textLayout.Render( fPosX, fPosY, m_label.angle, m_label.textColor, m_label.shadowColor, m_label.align, m_label.width);

  // render the second label if it exists
  CStdString label2(m_info2.GetLabel(m_dwParentID));
  if (!label2.IsEmpty())
  {
    float textWidth, textHeight;
    m_textLayout.GetTextExtent(textWidth, textHeight);
    m_textLayout2.Update(label2);

    float width = m_width - 2 * m_label.offsetX - textWidth - 5;
    if (width < 0) width = 0;
    fPosX = m_posX + m_width - m_label.offsetX;
    DWORD dwAlign = XBFONT_RIGHT | (m_label.align & XBFONT_CENTER_Y) | XBFONT_TRUNCATED;

    if (IsDisabled() )
      m_textLayout2.Render( fPosX, fPosY, m_label.angle, m_label.disabledColor, m_label.shadowColor, dwAlign, width, true);
    else if (HasFocus() && m_label.focusedColor)
      m_textLayout2.Render( fPosX, fPosY, m_label.angle, m_label.focusedColor, m_label.shadowColor, dwAlign, width);
    else
      m_textLayout2.Render( fPosX, fPosY, m_label.angle, m_label.textColor, m_label.shadowColor, dwAlign, width);
  }
}

bool CGUIButtonControl::OnAction(const CAction &action)
{
  if (action.wID == ACTION_SELECT_ITEM)
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
      m_bSelected = true;
      return true;
    }
    if (message.GetMessage() == GUI_MSG_DESELECTED)
    {
      m_bSelected = false;
      return true;
    }
  }

  return CGUIControl::OnMessage(message);
}

void CGUIButtonControl::PreAllocResources()
{
  CGUIControl::PreAllocResources();
  m_imgFocus.PreAllocResources();
  m_imgNoFocus.PreAllocResources();
}

void CGUIButtonControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_dwFocusCounter = 0;
  m_imgFocus.AllocResources();
  m_imgNoFocus.AllocResources();
  if (!m_width)
    m_width = m_imgFocus.GetWidth();
  if (!m_height)
    m_height = m_imgFocus.GetHeight();
}

void CGUIButtonControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_imgFocus.FreeResources();
  m_imgNoFocus.FreeResources();
}

void CGUIButtonControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgFocus.DynamicResourceAlloc(bOnOff);
  m_imgNoFocus.DynamicResourceAlloc(bOnOff);
}

void CGUIButtonControl::SetLabel(const string &label)
{ // NOTE: No fallback for buttons at this point
  m_info.SetLabel(label, "");
}

void CGUIButtonControl::SetLabel2(const string &label2)
{ // NOTE: No fallback for buttons at this point
  m_info2.SetLabel(label2, "");
}

void CGUIButtonControl::SetPosition(float posX, float posY)
{
  CGUIControl::SetPosition(posX, posY);

  m_imgFocus.SetPosition(posX, posY);
  m_imgNoFocus.SetPosition(posX, posY);
}

void CGUIButtonControl::SetAlpha(unsigned char alpha)
{
  m_alpha = alpha;
  m_imgFocus.SetAlpha(alpha);
  m_imgNoFocus.SetAlpha(alpha);
}

void CGUIButtonControl::UpdateDiffuseColor()
{
  CGUIControl::UpdateDiffuseColor();
  m_imgFocus.SetDiffuseColor(m_diffuseColor);
  m_imgNoFocus.SetDiffuseColor(m_diffuseColor);
}

bool CGUIButtonControl::OnMouseClick(DWORD dwButton, const CPoint &point)
{
  if (dwButton == MOUSE_LEFT_BUTTON)
  {
    g_Mouse.SetState(MOUSE_STATE_CLICK);
    CAction action;
    action.wID = ACTION_SELECT_ITEM;
    OnAction(action);
    return true;
  }
  return false;
}

CStdString CGUIButtonControl::GetDescription() const
{
  CStdString strLabel(m_info.GetLabel(m_dwParentID));
  return strLabel;
}

CStdString CGUIButtonControl::GetLabel2() const
{
  CStdString strLabel(m_info2.GetLabel(m_dwParentID));
  return strLabel;
}

void CGUIButtonControl::PythonSetLabel(const CStdString &strFont, const string &strText, DWORD dwTextColor, DWORD dwShadowColor, DWORD dwFocusedColor)
{
  m_label.font = g_fontManager.GetFont(strFont);
  m_label.textColor = dwTextColor;
  m_label.focusedColor = dwFocusedColor;
  m_label.shadowColor = dwShadowColor;
  SetLabel(strText);
}

void CGUIButtonControl::PythonSetDisabledColor(DWORD dwDisabledColor)
{
  m_label.disabledColor = dwDisabledColor;
}

void CGUIButtonControl::RAMSetTextColor(DWORD dwTextColor)
{
  m_label.textColor = dwTextColor;
}

void CGUIButtonControl::SettingsCategorySetTextAlign(DWORD dwAlign)
{
  m_label.align = dwAlign;
}

void CGUIButtonControl::OnClick()
{
  // Save values, as the click message may deactivate the window
  DWORD dwControlID = GetID();
  DWORD dwParentID = GetParentID();
  vector<CStdString> clickActions = m_clickActions;

  // button selected, send a message
  CGUIMessage msg(GUI_MSG_CLICKED, dwControlID, dwParentID, 0);
  SendWindowMessage(msg);

  // and execute our actions
  for (unsigned int i = 0; i < clickActions.size(); i++)
  {
    CGUIMessage message(GUI_MSG_EXECUTE, dwControlID, dwParentID);
    message.SetStringParam(clickActions[i]);
    g_graphicsContext.SendMessage(message);
  }
}

void CGUIButtonControl::OnFocus()
{
  for (unsigned int i = 0; i < m_focusActions.size(); i++)
  {
    CGUIMessage message(GUI_MSG_EXECUTE, m_dwControlID, m_dwParentID);
    message.SetStringParam(m_focusActions[i]);
    m_gWindowManager.SendThreadMessage(message);
  }
}

void CGUIButtonControl::OnUnFocus()
{
  for (unsigned int i = 0; i < m_unfocusActions.size(); i++)
  {
    CGUIMessage message(GUI_MSG_EXECUTE, m_dwControlID, m_dwParentID);
    message.SetStringParam(m_unfocusActions[i]);
    m_gWindowManager.SendThreadMessage(message);
  }
}

void CGUIButtonControl::SetSelected(bool bSelected)
{
  if (m_bSelected != bSelected)
  {
    m_bSelected = bSelected;
    SetInvalid();
  }
}

