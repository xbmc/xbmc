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

#include "GUIRenderingControl.h"
#include "GUIUserMessages.h"
#include "addons/Visualisation.h"
#include "threads/SingleLock.h"

using namespace std;
using namespace ADDON;

#define LABEL_ROW1 10
#define LABEL_ROW2 11
#define LABEL_ROW3 12

CGUIRenderingControl::CGUIRenderingControl(int parentID, int controlID, float posX, float posY, float width, float height)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
{
  ControlType = GUICONTROL_RENDERADDON;
}

CGUIRenderingControl::CGUIRenderingControl(const CGUIRenderingControl &from)
: CGUIControl(from)
{
  ControlType = GUICONTROL_RENDERADDON;
}

void CGUIRenderingControl::LoadAddon(const AddonPtr &addon)
{
  if (!addon)
    return;

  CSingleLock lock(m_rendering);
  g_graphicsContext.CaptureStateBlock();
  float x = g_graphicsContext.ScaleFinalXCoord(GetXPosition(), GetYPosition());
  float y = g_graphicsContext.ScaleFinalYCoord(GetXPosition(), GetYPosition());
  float w = g_graphicsContext.ScaleFinalXCoord(GetXPosition() + GetWidth(), GetYPosition() + GetHeight()) - x;
  float h = g_graphicsContext.ScaleFinalYCoord(GetXPosition() + GetWidth(), GetYPosition() + GetHeight()) - y;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x + w > g_graphicsContext.GetWidth()) w = g_graphicsContext.GetWidth() - x;
  if (y + h > g_graphicsContext.GetHeight()) h = g_graphicsContext.GetHeight() - y;

  VizPtr viz = boost::dynamic_pointer_cast<CVisualisation>(addon);
  if (viz && viz->Create((int)(x+0.5f), (int)(y+0.5f), (int)(w+0.5f), (int)(h+0.5f)))
  {
    m_addon = viz;
  }

  g_graphicsContext.ApplyStateBlock();
}

void CGUIRenderingControl::UpdateVisibility(const CGUIListItem *item)
{
  // if made invisible, start timer, only free addonptr after
  // some period, configurable by window class
  CGUIControl::UpdateVisibility(item);
  if (!IsVisible() && m_addon)
    FreeResources();
}

void CGUIRenderingControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  // TODO Add processing to the addon so it could mark when actually changing
  CSingleLock lock(m_rendering);
  if (m_addon)
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIRenderingControl::Render()
{
  CSingleLock lock(m_rendering);
  if (m_addon)
  {
    // set the viewport - note: We currently don't have any control over how
    // the addon renders, so the best we can do is attempt to define
    // a viewport??
    g_graphicsContext.SetViewPort(m_posX, m_posY, m_width, m_height);
    g_graphicsContext.CaptureStateBlock();
    m_addon->Render();
    g_graphicsContext.ApplyStateBlock();
    g_graphicsContext.RestoreViewPort();
  }

  CGUIControl::Render();
}

void CGUIRenderingControl::FreeResources(bool immediately)
{
  if (!m_addon) return;

  CSingleLock lock(m_rendering);
  g_graphicsContext.CaptureStateBlock(); //TODO locking
  m_addon->Stop();
  g_graphicsContext.ApplyStateBlock();
  m_addon.reset();
}

bool CGUIRenderingControl::CanFocusFromPoint(const CPoint &point) const
{ // mouse is allowed to focus this control, but it doesn't actually receive focus
  return IsVisible() && HitTest(point);
}
