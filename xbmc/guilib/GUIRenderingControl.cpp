/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRenderingControl.h"
#include "threads/SingleLock.h"
#include "guilib/IRenderingCallback.h"
#ifdef TARGET_WINDOWS
#include "rendering/dx/DeviceResources.h"
#endif

#define LABEL_ROW1 10
#define LABEL_ROW2 11
#define LABEL_ROW3 12

CGUIRenderingControl::CGUIRenderingControl(int parentID, int controlID, float posX, float posY, float width, float height)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
{
  ControlType = GUICONTROL_RENDERADDON;
  m_callback = NULL;
}

CGUIRenderingControl::CGUIRenderingControl(const CGUIRenderingControl &from)
: CGUIControl(from)
{
  ControlType = GUICONTROL_RENDERADDON;
  m_callback = NULL;
}

bool CGUIRenderingControl::InitCallback(IRenderingCallback *callback)
{
  if (!callback)
    return false;

  CSingleLock lock(m_rendering);
  CServiceBroker::GetWinSystem()->GetGfxContext().CaptureStateBlock();
  float x = CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalXCoord(GetXPosition(), GetYPosition());
  float y = CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalYCoord(GetXPosition(), GetYPosition());
  float w = CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalXCoord(GetXPosition() + GetWidth(), GetYPosition() + GetHeight()) - x;
  float h = CServiceBroker::GetWinSystem()->GetGfxContext().ScaleFinalYCoord(GetXPosition() + GetWidth(), GetYPosition() + GetHeight()) - y;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x + w > CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth()) w = CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth() - x;
  if (y + h > CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight()) h = CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight() - y;

  void *device = NULL;
#if TARGET_WINDOWS
  device = DX::DeviceResources::Get()->GetD3DDevice();
#endif
  if (callback->Create((int)(x+0.5f), (int)(y+0.5f), (int)(w+0.5f), (int)(h+0.5f), device))
    m_callback = callback;
  else
    return false;

  CServiceBroker::GetWinSystem()->GetGfxContext().ApplyStateBlock();
  return true;
}

void CGUIRenderingControl::UpdateVisibility(const CGUIListItem *item)
{
  // if made invisible, start timer, only free addonptr after
  // some period, configurable by window class
  CGUIControl::UpdateVisibility(item);
  if (!IsVisible() && m_callback)
    FreeResources();
}

void CGUIRenderingControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  //! @todo Add processing to the addon so it could mark when actually changing
  CSingleLock lock(m_rendering);
  if (m_callback && m_callback->IsDirty())
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIRenderingControl::Render()
{
  CSingleLock lock(m_rendering);
  if (m_callback)
  {
    // set the viewport - note: We currently don't have any control over how
    // the addon renders, so the best we can do is attempt to define
    // a viewport??
    CServiceBroker::GetWinSystem()->GetGfxContext().SetViewPort(m_posX, m_posY, m_width, m_height);
    CServiceBroker::GetWinSystem()->GetGfxContext().CaptureStateBlock();
    m_callback->Render();
    CServiceBroker::GetWinSystem()->GetGfxContext().ApplyStateBlock();
    CServiceBroker::GetWinSystem()->GetGfxContext().RestoreViewPort();
  }

  CGUIControl::Render();
}

void CGUIRenderingControl::FreeResources(bool immediately)
{
  CSingleLock lock(m_rendering);

  if (!m_callback) return;

  CServiceBroker::GetWinSystem()->GetGfxContext().CaptureStateBlock(); //! @todo locking
  m_callback->Stop();
  CServiceBroker::GetWinSystem()->GetGfxContext().ApplyStateBlock();
  m_callback = NULL;
}

bool CGUIRenderingControl::CanFocusFromPoint(const CPoint &point) const
{ // mouse is allowed to focus this control, but it doesn't actually receive focus
  return IsVisible() && HitTest(point);
}
