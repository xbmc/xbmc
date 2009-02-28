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
#include "GUISliderControl.h"
#include "utils/GUIInfoManager.h"
#include "GUIFontManager.h"
#include "GUITextLayout.h"

CGUISliderControl::CGUISliderControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CTextureInfo& backGroundTexture, const CTextureInfo& nibTexture, const CTextureInfo& nibTextureFocus, int iType)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
    , m_guiBackground(posX, posY, width, height, backGroundTexture)
    , m_guiMid(posX, posY, width, height, nibTexture)
    , m_guiMidFocus(posX, posY, width, height, nibTextureFocus)
{
  m_iType = iType;
  m_iPercent = 0;
  m_iStart = 0;
  m_iEnd = 100;
  m_fStart = 0.0f;
  m_fEnd = 1.0f;
  m_fInterval = 0.1f;
  m_iValue = 0;
  m_fValue = 0.0;
  m_controlOffsetX = 60;
  m_controlOffsetY = 0;
  ControlType = GUICONTROL_SLIDER;
  m_renderText = true;
  m_iInfoCode = 0;
}

CGUISliderControl::~CGUISliderControl(void)
{}


void CGUISliderControl::Render()
{
  if (m_bInvalidated)
  {
    m_guiBackground.SetPosition( GetXPosition(), GetYPosition());
  }
  float fRange, fPos, fPercent;

  if (!IsDisabled())
  {
    CStdString text;
    switch (m_iType)
    {
    case SPIN_CONTROL_TYPE_FLOAT_ABS:
    case SPIN_CONTROL_TYPE_FLOAT:
      if (m_iInfoCode) m_fValue = (float)g_infoManager.GetInt(m_iInfoCode);
      text.Format("%2.2f", m_iType==SPIN_CONTROL_TYPE_FLOAT_ABS?fabs(m_fValue):m_fValue);

      m_guiBackground.SetPosition(m_posX + m_controlOffsetX, m_posY + m_controlOffsetY);

      fRange = m_fEnd - m_fStart;
      fPos = m_fValue - m_fStart;
      fPercent = (fPos / fRange) * 100.0f;
      m_iPercent = (int) fPercent;
      break;

    case SPIN_CONTROL_TYPE_INT:
      if (m_iInfoCode) m_iValue = g_infoManager.GetInt(m_iInfoCode);
      text.Format("%i/%i", m_iValue, m_iEnd);

      m_guiBackground.SetPosition(m_posX + m_controlOffsetX, m_posY + m_controlOffsetY);

      fRange = (float)(m_iEnd - m_iStart);
      fPos = (float)(m_iValue - m_iStart);
      m_iPercent = (int) ((fPos / fRange) * 100.0f);
      break;
    default:
      if(m_iInfoCode) m_iPercent = g_infoManager.GetInt(m_iInfoCode);
    }
    if (m_renderText && text.size())
      CGUITextLayout::DrawText(g_fontManager.GetFont("font13"), m_posX, m_posY, 0xffffffff, 0, text, 0);

    float fScaleX, fScaleY;
    fScaleY = m_height == 0 ? 1.0f : m_height / m_guiBackground.GetTextureHeight();
    fScaleX = m_width == 0 ? 1.0f : m_width / m_guiBackground.GetTextureWidth();

    m_guiBackground.SetHeight(m_height);
    m_guiBackground.SetWidth(m_width);
    m_guiBackground.Render();

    float fWidth = (m_guiBackground.GetTextureWidth() - m_guiMid.GetTextureWidth())*fScaleX;

    fPos = (float)m_iPercent;
    fPos /= 100.0f;
    fPos *= fWidth;
    fPos += m_guiBackground.GetXPosition();
    //fPos += 10.0f;
    if ((int)fWidth > 1)
    {
      if (m_bHasFocus)
      {
        m_guiMidFocus.SetPosition(fPos, m_guiBackground.GetYPosition() );
        m_guiMidFocus.SetWidth(m_guiMidFocus.GetTextureWidth() * fScaleX);
        m_guiMidFocus.SetHeight(m_guiMidFocus.GetTextureHeight() * fScaleY);
        m_guiMidFocus.Render();
      }
      else
      {
        m_guiMid.SetPosition(fPos, m_guiBackground.GetYPosition() );
        m_guiMid.SetWidth(m_guiMid.GetTextureWidth()*fScaleX);
        m_guiMid.SetHeight(m_guiMid.GetTextureHeight()*fScaleY);
        m_guiMid.Render();
      }
    }
  }
  CGUIControl::Render();
}

bool CGUISliderControl::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    switch (message.GetMessage())
    {
    case GUI_MSG_ITEM_SELECT:
      SetPercentage( message.GetParam1() );
      return true;
      break;

    case GUI_MSG_LABEL_RESET:
      {
        SetPercentage(0);
        return true;
      }
      break;
    }
  }

  return CGUIControl::OnMessage(message);
}

bool CGUISliderControl::OnAction(const CAction &action)
{
  switch ( action.wID )
  {
  case ACTION_MOVE_LEFT:
    //case ACTION_OSD_SHOW_VALUE_MIN:
    Move( -1);
    return true;
    break;

  case ACTION_MOVE_RIGHT:
    //case ACTION_OSD_SHOW_VALUE_PLUS:
    Move(1);
    return true;
    break;
  default:
    return CGUIControl::OnAction(action);
  }
}

void CGUISliderControl::Move(int iNumSteps)
{
  switch (m_iType)
  {
  case SPIN_CONTROL_TYPE_FLOAT_ABS:
  case SPIN_CONTROL_TYPE_FLOAT:
    m_fValue += m_fInterval * iNumSteps;
    if (m_fValue < m_fStart) m_fValue = m_fStart;
    if (m_fValue > m_fEnd) m_fValue = m_fEnd;
    break;

  case SPIN_CONTROL_TYPE_INT:
    m_iValue += iNumSteps;
    if (m_iValue < m_iStart) m_iValue = m_iStart;
    if (m_iValue > m_iEnd) m_iValue = m_iEnd;
    break;

  default:
    m_iPercent += iNumSteps;
    if (m_iPercent < 0) m_iPercent = 0;
    if (m_iPercent > 100) m_iPercent = 100;
    break;
  }
  SEND_CLICK_MESSAGE(GetID(), GetParentID(), 0);
}

void CGUISliderControl::SetPercentage(int iPercent)
{
  if (iPercent > 100) iPercent = 100;
  if (iPercent < 0) iPercent = 0;
  m_iPercent = iPercent;
}

int CGUISliderControl::GetPercentage() const
{
  return m_iPercent;
}

void CGUISliderControl::SetIntValue(int iValue)
{
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT || m_iType == SPIN_CONTROL_TYPE_FLOAT_ABS)
    m_fValue = (float)iValue;
  else if (m_iType == SPIN_CONTROL_TYPE_INT)
    m_iValue = iValue;
  else
    SetPercentage(iValue);
}

int CGUISliderControl::GetIntValue() const
{
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT || m_iType == SPIN_CONTROL_TYPE_FLOAT_ABS)
    return (int)m_fValue;
  else if (m_iType == SPIN_CONTROL_TYPE_INT)
    return m_iValue;
  else
    return m_iPercent;
}

void CGUISliderControl::SetFloatValue(float fValue)
{
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT || m_iType == SPIN_CONTROL_TYPE_FLOAT_ABS)
    m_fValue = fValue;
  else if (m_iType == SPIN_CONTROL_TYPE_INT)
    m_iValue = (int)fValue;
  else
    SetPercentage((int)fValue);
}

float CGUISliderControl::GetFloatValue() const
{
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT || m_iType == SPIN_CONTROL_TYPE_FLOAT_ABS)
    return m_fValue;
  else if (m_iType == SPIN_CONTROL_TYPE_INT)
    return (float)m_iValue;
  else
    return (float)m_iPercent;
}

void CGUISliderControl::SetFloatInterval(float fInterval)
{
  m_fInterval = fInterval;
}

void CGUISliderControl::SetRange(int iStart, int iEnd)
{
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT || m_iType == SPIN_CONTROL_TYPE_FLOAT_ABS)
    SetFloatRange((float)iStart,(float)iEnd);
  else
  {
    m_iStart = iStart;
    m_iEnd = iEnd;
  }
}

void CGUISliderControl::SetFloatRange(float fStart, float fEnd)
{
  if (m_iType == SPIN_CONTROL_TYPE_INT)
    SetRange((int)fStart, (int)fEnd);
  else
  {
    m_fStart = fStart;
    m_fEnd = fEnd;
  }
}

void CGUISliderControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_guiBackground.FreeResources();
  m_guiMid.FreeResources();
  m_guiMidFocus.FreeResources();
}

void CGUISliderControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_guiBackground.DynamicResourceAlloc(bOnOff);
  m_guiMid.DynamicResourceAlloc(bOnOff);
  m_guiMidFocus.DynamicResourceAlloc(bOnOff);
}

void CGUISliderControl::PreAllocResources()
{
  CGUIControl::PreAllocResources();
  m_guiBackground.PreAllocResources();
  m_guiMid.PreAllocResources();
  m_guiMidFocus.PreAllocResources();
}

void CGUISliderControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_guiBackground.AllocResources();
  m_guiMid.AllocResources();
  m_guiMidFocus.AllocResources();
}

bool CGUISliderControl::HitTest(const CPoint &point) const
{
  if (m_guiBackground.HitTest(point)) return true;
  if (m_guiMid.HitTest(point)) return true;
  return false;
}

void CGUISliderControl::SetFromPosition(const CPoint &point)
{
  float fPercent = (point.x - m_guiBackground.GetXPosition()) / m_guiBackground.GetWidth();
  if (fPercent < 0) fPercent = 0;
  if (fPercent > 1) fPercent = 1;
  switch (m_iType)
  {
  case SPIN_CONTROL_TYPE_FLOAT:
  case SPIN_CONTROL_TYPE_FLOAT_ABS:
    m_fValue = m_fStart + (m_fEnd - m_fStart) * fPercent;
    break;

  case SPIN_CONTROL_TYPE_INT:
    m_iValue = (int)(m_iStart + (float)(m_iEnd - m_iStart) * fPercent + 0.49f);
    break;

  default:
    m_iPercent = (int)(fPercent * 100 + 0.49f);
    break;
  }
  SEND_CLICK_MESSAGE(GetID(), GetParentID(), 0);
}

bool CGUISliderControl::OnMouseClick(DWORD dwButton, const CPoint &point)
{
  g_Mouse.SetState(MOUSE_STATE_CLICK);
  // turn off any exclusive access, if it's on...
  g_Mouse.EndExclusiveAccess(GetID(), GetParentID());
  if (m_guiBackground.HitTest(point))
  { // set the position
    SetFromPosition(point);
    return true;
  }
  return false;
}

bool CGUISliderControl::OnMouseDrag(const CPoint &offset, const CPoint &point)
{
  g_Mouse.SetState(MOUSE_STATE_DRAG);
  // get exclusive access to the mouse
  g_Mouse.SetExclusiveAccess(GetID(), GetParentID(), point);
  // get the position of the mouse
  SetFromPosition(point);
  return true;
}

bool CGUISliderControl::OnMouseWheel(char wheel, const CPoint &point)
{ // move the slider 10 steps in the appropriate direction
  Move(wheel*10);
  return true;
}

void CGUISliderControl::SetInfo(int iInfo)
{
  m_iInfoCode = iInfo;
}

CStdString CGUISliderControl::GetDescription() const
{
  CStdString description;
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT)
  {
    if (m_formatString.IsEmpty())
      description.Format("%2.2f", m_fValue);
    else
      description.Format(m_formatString.c_str(), m_fValue);
  }
  else if (m_iType == SPIN_CONTROL_TYPE_FLOAT_ABS)
  {
    if (m_formatString.IsEmpty())
      description.Format("%2.2f", fabs(m_fValue));
    else
      description.Format(m_formatString.c_str(), fabs(m_fValue));
  }
  else if (m_iType == SPIN_CONTROL_TYPE_INT)
  {
    if (m_formatString.IsEmpty())
      description.Format("%i", m_iValue);
    else
      description.Format(m_formatString.c_str(), m_iValue);
  }
  else
  {
    if (m_formatString.IsEmpty())
      description.Format("%i%%", m_iPercent);
    else
      description.Format(m_formatString.c_str(), m_iPercent);
  }
  return description;
}

void CGUISliderControl::UpdateDiffuseColor()
{
  CGUIControl::UpdateDiffuseColor();
  m_guiBackground.SetDiffuseColor(m_diffuseColor);
  m_guiMid.SetDiffuseColor(m_diffuseColor);
  m_guiMidFocus.SetDiffuseColor(m_diffuseColor);
}

