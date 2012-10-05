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

#include "GUIMoverControl.h"
#include "GUIWindowManager.h"
#include "Key.h"
#include "utils/TimeUtils.h"

// time to reset accelerated cursors (digital movement)
#define MOVE_TIME_OUT 500L

CGUIMoverControl::CGUIMoverControl(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
    , m_imgFocus(posX, posY, width, height, textureFocus)
    , m_imgNoFocus(posX, posY, width, height, textureNoFocus)
{
  m_frameCounter = 0;
  m_lastMoveTime = 0;
  m_fSpeed = 1.0;
  m_fAnalogSpeed = 2.0f; // TODO: implement correct analog speed
  m_fAcceleration = 0.2f; // TODO: implement correct computation of acceleration
  m_fMaxSpeed = 10.0;  // TODO: implement correct computation of maxspeed
  ControlType = GUICONTROL_MOVER;
  SetLimits(0, 0, 720, 576); // defaults
  SetLocation(0, 0, false);  // defaults
}

CGUIMoverControl::~CGUIMoverControl(void)
{}

void CGUIMoverControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
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
    unsigned int alphaCounter = m_frameCounter + 2;
    unsigned int alphaChannel;
    if ((alphaCounter % 128) >= 64)
      alphaChannel = alphaCounter % 64;
    else
      alphaChannel = 63 - (alphaCounter % 64);

    alphaChannel += 192;
    if (SetAlpha( (unsigned char)alphaChannel ))
      MarkDirtyRegion();
    m_imgFocus.SetVisible(true);
    m_imgNoFocus.SetVisible(false);
    m_frameCounter++;
  }
  else
  {
    if (SetAlpha(0xff))
      MarkDirtyRegion();
    m_imgFocus.SetVisible(false);
    m_imgNoFocus.SetVisible(true);
  }
  m_imgFocus.Process(currentTime);
  m_imgNoFocus.Process(currentTime);

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIMoverControl::Render()
{
  // render both so the visibility settings cause the frame counter to resetcorrectly
  m_imgFocus.Render();
  m_imgNoFocus.Render();
  CGUIControl::Render();
}

bool CGUIMoverControl::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SELECT_ITEM)
  {
    // button selected - send message to parent
    CGUIMessage message(GUI_MSG_CLICKED, GetID(), GetParentID());
    SendWindowMessage(message);
    return true;
  }
  if (action.GetID() == ACTION_ANALOG_MOVE)
  {
    //  if (m_dwAllowedDirections == ALLOWED_DIRECTIONS_UPDOWN)
    //   Move(0, (int)(-m_fAnalogSpeed*action.GetAmount(1)));
    //  else if (m_dwAllowedDirections == ALLOWED_DIRECTIONS_LEFTRIGHT)
    //   Move((int)(m_fAnalogSpeed*action.GetAmount()), 0);
    //  else // ALLOWED_DIRECTIONS_ALL
    Move((int)(m_fAnalogSpeed*action.GetAmount()), (int)( -m_fAnalogSpeed*action.GetAmount(1)));
    return true;
  }
  // base class
  return CGUIControl::OnAction(action);
}

void CGUIMoverControl::OnUp()
{
  // if (m_dwAllowedDirections == ALLOWED_DIRECTIONS_LEFTRIGHT) return;
  UpdateSpeed(DIRECTION_UP);
  Move(0, (int) - m_fSpeed);
}

void CGUIMoverControl::OnDown()
{
  // if (m_dwAllowedDirections == ALLOWED_DIRECTIONS_LEFTRIGHT) return;
  UpdateSpeed(DIRECTION_DOWN);
  Move(0, (int)m_fSpeed);
}

void CGUIMoverControl::OnLeft()
{
  // if (m_dwAllowedDirections == ALLOWED_DIRECTIONS_UPDOWN) return;
  UpdateSpeed(DIRECTION_LEFT);
  Move((int) - m_fSpeed, 0);
}

void CGUIMoverControl::OnRight()
{
  // if (m_dwAllowedDirections == ALLOWED_DIRECTIONS_UPDOWN) return;
  UpdateSpeed(DIRECTION_RIGHT);
  Move((int)m_fSpeed, 0);
}

EVENT_RESULT CGUIMoverControl::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_DRAG)
  {
    if (event.m_state == 1)
    { // grab exclusive access
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, GetID(), GetParentID());
      SendWindowMessage(msg);
    }
    else if (event.m_state == 3)
    { // release exclusive access
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, 0, GetParentID());
      SendWindowMessage(msg);
    }
    Move((int)event.m_offsetX, (int)event.m_offsetY);
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

void CGUIMoverControl::UpdateSpeed(int nDirection)
{
  if (CTimeUtils::GetFrameTime() - m_lastMoveTime > MOVE_TIME_OUT)
  {
    m_fSpeed = 1;
    m_nDirection = DIRECTION_NONE;
  }
  m_lastMoveTime = CTimeUtils::GetFrameTime();
  if (nDirection == m_nDirection)
  { // accelerate
    m_fSpeed += m_fAcceleration;
    if (m_fSpeed > m_fMaxSpeed) m_fSpeed = m_fMaxSpeed;
  }
  else
  { // reset direction and speed
    m_fSpeed = 1;
    m_nDirection = nDirection;
  }
}

void CGUIMoverControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_frameCounter = 0;
  m_imgFocus.AllocResources();
  m_imgNoFocus.AllocResources();
  float width = m_width ? m_width : m_imgFocus.GetWidth();
  float height = m_height ? m_height : m_imgFocus.GetHeight();
  SetWidth(width);
  SetHeight(height);
}

void CGUIMoverControl::FreeResources(bool immediately)
{
  CGUIControl::FreeResources(immediately);
  m_imgFocus.FreeResources(immediately);
  m_imgNoFocus.FreeResources(immediately);
}

void CGUIMoverControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgFocus.DynamicResourceAlloc(bOnOff);
  m_imgNoFocus.DynamicResourceAlloc(bOnOff);
}

void CGUIMoverControl::SetInvalid()
{
  CGUIControl::SetInvalid();
  m_imgFocus.SetInvalid();
  m_imgNoFocus.SetInvalid();
}

void CGUIMoverControl::Move(int iX, int iY)
{
  int iLocX = m_iLocationX + iX;
  int iLocY = m_iLocationY + iY;
  // check if we are within the bounds
  if (iLocX < m_iX1) iLocX = m_iX1;
  if (iLocY < m_iY1) iLocY = m_iY1;
  if (iLocX > m_iX2) iLocX = m_iX2;
  if (iLocY > m_iY2) iLocY = m_iY2;
  // ok, now set the location of the mover
  SetLocation(iLocX, iLocY);
}

void CGUIMoverControl::SetLocation(int iLocX, int iLocY, bool bSetPosition)
{
  if (bSetPosition) SetPosition(GetXPosition() + iLocX - m_iLocationX, GetYPosition() + iLocY - m_iLocationY);
  m_iLocationX = iLocX;
  m_iLocationY = iLocY;
}

void CGUIMoverControl::SetPosition(float posX, float posY)
{
  CGUIControl::SetPosition(posX, posY);
  m_imgFocus.SetPosition(posX, posY);
  m_imgNoFocus.SetPosition(posX, posY);
}

bool CGUIMoverControl::SetAlpha(unsigned char alpha)
{
  return m_imgFocus.SetAlpha(alpha) | 
         m_imgNoFocus.SetAlpha(alpha);
}

bool CGUIMoverControl::UpdateColors()
{
  bool changed = CGUIControl::UpdateColors();
  changed |= m_imgFocus.SetDiffuseColor(m_diffuseColor);
  changed |= m_imgNoFocus.SetDiffuseColor(m_diffuseColor);

  return changed;
}

void CGUIMoverControl::SetLimits(int iX1, int iY1, int iX2, int iY2)
{
  m_iX1 = iX1;
  m_iY1 = iY1;
  m_iX2 = iX2;
  m_iY2 = iY2;
}
