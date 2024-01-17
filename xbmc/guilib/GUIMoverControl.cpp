/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIMoverControl.h"

#include "GUIMessage.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseEvent.h"
#include "input/mouse/MouseStat.h"
#include "utils/TimeUtils.h"

using namespace KODI;
using namespace UTILS;

CGUIMoverControl::CGUIMoverControl(int parentID,
                                   int controlID,
                                   float posX,
                                   float posY,
                                   float width,
                                   float height,
                                   const CTextureInfo& textureFocus,
                                   const CTextureInfo& textureNoFocus,
                                   UTILS::MOVING_SPEED::MapEventConfig& movingSpeedCfg)
  : CGUIControl(parentID, controlID, posX, posY, width, height),
    m_imgFocus(CGUITexture::CreateTexture(posX, posY, width, height, textureFocus)),
    m_imgNoFocus(CGUITexture::CreateTexture(posX, posY, width, height, textureNoFocus))
{
  m_frameCounter = 0;
  m_movingSpeed.AddEventMapConfig(movingSpeedCfg);
  m_fAnalogSpeed = 2.0f; //! @todo implement correct analog speed
  ControlType = GUICONTROL_MOVER;
  SetLimits(0, 0, 720, 576); // defaults
  SetLocation(0, 0, false);  // defaults
}

CGUIMoverControl::CGUIMoverControl(const CGUIMoverControl& control)
  : CGUIControl(control),
    m_imgFocus(control.m_imgFocus->Clone()),
    m_imgNoFocus(control.m_imgNoFocus->Clone()),
    m_frameCounter(control.m_frameCounter),
    m_movingSpeed(control.m_movingSpeed),
    m_fAnalogSpeed(control.m_fAnalogSpeed),
    m_iX1(control.m_iX1),
    m_iX2(control.m_iX2),
    m_iY1(control.m_iY1),
    m_iY2(control.m_iY2),
    m_iLocationX(control.m_iLocationX),
    m_iLocationY(control.m_iLocationY)
{
}

void CGUIMoverControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_bInvalidated)
  {
    m_imgFocus->SetWidth(m_width);
    m_imgFocus->SetHeight(m_height);

    m_imgNoFocus->SetWidth(m_width);
    m_imgNoFocus->SetHeight(m_height);
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
    m_imgFocus->SetVisible(true);
    m_imgNoFocus->SetVisible(false);
    m_frameCounter++;
  }
  else
  {
    if (SetAlpha(0xff))
      MarkDirtyRegion();
    m_imgFocus->SetVisible(false);
    m_imgNoFocus->SetVisible(true);
  }
  m_imgFocus->Process(currentTime);
  m_imgNoFocus->Process(currentTime);

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIMoverControl::Render()
{
  // render both so the visibility settings cause the frame counter to resetcorrectly
  m_imgFocus->Render();
  m_imgNoFocus->Render();
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
  Move(0, -static_cast<int>(m_movingSpeed.GetUpdatedDistance(MOVING_SPEED::EventType::UP)));
}

void CGUIMoverControl::OnDown()
{
  // if (m_dwAllowedDirections == ALLOWED_DIRECTIONS_LEFTRIGHT) return;
  Move(0, static_cast<int>(m_movingSpeed.GetUpdatedDistance(MOVING_SPEED::EventType::DOWN)));
}

void CGUIMoverControl::OnLeft()
{
  // if (m_dwAllowedDirections == ALLOWED_DIRECTIONS_UPDOWN) return;
  Move(-static_cast<int>(m_movingSpeed.GetUpdatedDistance(MOVING_SPEED::EventType::LEFT)), 0);
}

void CGUIMoverControl::OnRight()
{
  // if (m_dwAllowedDirections == ALLOWED_DIRECTIONS_UPDOWN) return;
  Move(static_cast<int>(m_movingSpeed.GetUpdatedDistance(MOVING_SPEED::EventType::RIGHT)), 0);
}

EVENT_RESULT CGUIMoverControl::OnMouseEvent(const CPoint& point, const MOUSE::CMouseEvent& event)
{
  if (event.m_id == ACTION_MOUSE_DRAG || event.m_id == ACTION_MOUSE_DRAG_END)
  {
    if (static_cast<HoldAction>(event.m_state) == HoldAction::DRAG)
    { // grab exclusive access
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, GetID(), GetParentID());
      SendWindowMessage(msg);
    }
    else if (static_cast<HoldAction>(event.m_state) == HoldAction::DRAG_END)
    { // release exclusive access
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, 0, GetParentID());
      SendWindowMessage(msg);
    }
    Move((int)event.m_offsetX, (int)event.m_offsetY);
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

void CGUIMoverControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_frameCounter = 0;
  m_imgFocus->AllocResources();
  m_imgNoFocus->AllocResources();
  float width = m_width ? m_width : m_imgFocus->GetWidth();
  float height = m_height ? m_height : m_imgFocus->GetHeight();
  SetWidth(width);
  SetHeight(height);
}

void CGUIMoverControl::FreeResources(bool immediately)
{
  CGUIControl::FreeResources(immediately);
  m_imgFocus->FreeResources(immediately);
  m_imgNoFocus->FreeResources(immediately);
}

void CGUIMoverControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgFocus->DynamicResourceAlloc(bOnOff);
  m_imgNoFocus->DynamicResourceAlloc(bOnOff);
}

void CGUIMoverControl::SetInvalid()
{
  CGUIControl::SetInvalid();
  m_imgFocus->SetInvalid();
  m_imgNoFocus->SetInvalid();
}

void CGUIMoverControl::Move(int iX, int iY)
{
  if (!m_enabled)
    return;

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
  m_imgFocus->SetPosition(posX, posY);
  m_imgNoFocus->SetPosition(posX, posY);
}

bool CGUIMoverControl::SetAlpha(unsigned char alpha)
{
  bool changed = m_imgFocus->SetAlpha(alpha);
  changed |= m_imgNoFocus->SetAlpha(alpha);
  return changed;
}

bool CGUIMoverControl::UpdateColors(const CGUIListItem* item)
{
  bool changed = CGUIControl::UpdateColors(nullptr);
  changed |= m_imgFocus->SetDiffuseColor(m_diffuseColor);
  changed |= m_imgNoFocus->SetDiffuseColor(m_diffuseColor);

  return changed;
}

void CGUIMoverControl::SetLimits(int iX1, int iY1, int iX2, int iY2)
{
  m_iX1 = iX1;
  m_iY1 = iY1;
  m_iX2 = iX2;
  m_iY2 = iY2;
}
