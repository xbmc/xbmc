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

#include "GUICheckMarkControl.h"
#include "utils/CharsetConverter.h"
#include "GUIFontManager.h"
#include "Key.h"

using namespace std;

CGUICheckMarkControl::CGUICheckMarkControl(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& textureCheckMark, const CTextureInfo& textureCheckMarkNF, float checkWidth, float checkHeight, const CLabelInfo &labelInfo)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
    , m_imgCheckMark(posX, posY, checkWidth, checkHeight, textureCheckMark)
    , m_imgCheckMarkNoFocus(posX, posY, checkWidth, checkHeight, textureCheckMarkNF)
    , m_label(posX, posY, width, height, labelInfo)
{
  m_strLabel = "";
  m_bSelected = false;
  m_label.GetLabelInfo().align |= XBFONT_CENTER_Y;
  ControlType = GUICONTROL_CHECKMARK;
}

CGUICheckMarkControl::~CGUICheckMarkControl(void)
{}

void CGUICheckMarkControl::Render()
{
  m_label.SetText(m_strLabel);

  float textWidth = m_label.GetTextWidth();
  m_width = textWidth + 5 + m_imgCheckMark.GetWidth();
  m_height = m_imgCheckMark.GetHeight();

  float textPosX = m_posX;
  float checkMarkPosX = m_posX;

  if (m_label.GetLabelInfo().align & (XBFONT_RIGHT | XBFONT_CENTER_X))
    textPosX += m_imgCheckMark.GetWidth() + 5;
  else
    checkMarkPosX += textWidth + 5;

  m_label.SetMaxRect(textPosX, m_posY, textWidth, m_height);
  m_label.SetColor(GetTextColor());
  m_label.Render();

  if (m_bSelected)
  {
    m_imgCheckMark.SetPosition(checkMarkPosX, m_posY);
    m_imgCheckMark.Render();
  }
  else
  {
    m_imgCheckMarkNoFocus.SetPosition(checkMarkPosX, m_posY);
    m_imgCheckMarkNoFocus.Render();
  }
  CGUIControl::Render();
}

CGUILabel::COLOR CGUICheckMarkControl::GetTextColor() const
{
  if (IsDisabled())
    return CGUILabel::COLOR_DISABLED;
  else if (HasFocus())
    return CGUILabel::COLOR_FOCUSED;
  return CGUILabel::COLOR_TEXT;
}

bool CGUICheckMarkControl::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SELECT_ITEM)
  {
    m_bSelected = !m_bSelected;
    CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(), action.GetID());
    SendWindowMessage(msg);
    return true;
  }
  return CGUIControl::OnAction(action);
}

bool CGUICheckMarkControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      m_strLabel = message.GetLabel();
      return true;
    }
  }
  if (CGUIControl::OnMessage(message)) return true;
  return false;
}

void CGUICheckMarkControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_imgCheckMark.AllocResources();
  m_imgCheckMarkNoFocus.AllocResources();
}

void CGUICheckMarkControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_imgCheckMark.FreeResources();
  m_imgCheckMarkNoFocus.FreeResources();
}

void CGUICheckMarkControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgCheckMark.DynamicResourceAlloc(bOnOff);
  m_imgCheckMarkNoFocus.DynamicResourceAlloc(bOnOff);
}

void CGUICheckMarkControl::SetInvalid()
{
  CGUIControl::SetInvalid();
  m_label.SetInvalid();
  m_imgCheckMark.SetInvalid();
  m_imgCheckMarkNoFocus.SetInvalid();
}

void CGUICheckMarkControl::SetSelected(bool bOnOff)
{
  m_bSelected = bOnOff;
}

bool CGUICheckMarkControl::GetSelected() const
{
  return m_bSelected;
}

bool CGUICheckMarkControl::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_LEFT_CLICK)
  {
    OnAction(CAction(ACTION_SELECT_ITEM));
    return true;
  }
  return false;
}

void CGUICheckMarkControl::SetLabel(const string &label)
{
  m_strLabel = label;
}

void CGUICheckMarkControl::PythonSetLabel(const CStdString &strFont, const string &strText, color_t textColor)
{
  m_label.GetLabelInfo().font = g_fontManager.GetFont(strFont);
  m_label.GetLabelInfo().textColor = textColor;
  m_label.GetLabelInfo().focusedColor = textColor;
  m_strLabel = strText;
}

void CGUICheckMarkControl::PythonSetDisabledColor(color_t disabledColor)
{
  m_label.GetLabelInfo().disabledColor = disabledColor;
}

void CGUICheckMarkControl::UpdateColors()
{
  m_label.UpdateColors();
  CGUIControl::UpdateColors();
  m_imgCheckMark.SetDiffuseColor(m_diffuseColor);
  m_imgCheckMarkNoFocus.SetDiffuseColor(m_diffuseColor);
}
