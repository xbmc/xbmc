/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIControlGroupMask.h"

#include "GUITexture.h"
#include "ServiceBroker.h"
#include "guilib/GUIControl.h"
#include "guilib/GUIControlGroup.h"
#include "utils/Geometry.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

using namespace KODI;

CGUIControlGroupMask::CGUIControlGroupMask(int parentID,
                                               int controlID,
                                               float posX,
                                               float posY,
                                               float width,
                                               float height)
  : CGUIControlGroup(parentID, controlID, posX, posY, width, height)
{
  ControlType = GUICONTROL_GROUPMASK;
}

CGUIControlGroupMask::CGUIControlGroupMask(const CGUIControlGroupMask& control)
  : CGUIControlGroup(control)
{
  ControlType = GUICONTROL_GROUPMASK;
}

void CGUIControlGroupMask::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  CGUIControl::Process(currentTime, dirtyregions);

  // we need a stencil mask if have a rotation (or in case we have more complex geometry)
  if (m_transform.HasRotation())
    m_stencilLayer = CServiceBroker::GetWinSystem()->GetGfxContext().AddStencilRectToBucket(m_renderRegion);

  CPoint pos(GetPosition());
  CServiceBroker::GetWinSystem()->GetGfxContext().SetOrigin(pos.x, pos.y);

  for (auto* control : m_children)
  {
    control->UpdateVisibility(nullptr);
    // dirty regions are greedy atm, and should be restricted to the m_renderRegion
    control->DoProcess(currentTime, dirtyregions);
  }

  CServiceBroker::GetWinSystem()->GetGfxContext().RestoreOrigin();
}

void CGUIControlGroupMask::Render()
{
  // limit of all available stencil layers has been reached. 
  if (m_stencilLayer == STENCIL_LAYER::INVALID)
    return;

  const CRect oldScissor = CServiceBroker::GetWinSystem()->GetGfxContext().GetScissors();
  CRect region = m_renderRegion;
  region.Intersect(oldScissor);
  
  CServiceBroker::GetWinSystem()->GetGfxContext().SetScissors(region);
  
  if (m_stencilLayer != STENCIL_LAYER::NONE)
  {
    // rendering just once is enough, as long as it is the first pass.
    const RENDER_ORDER renderOrder = CServiceBroker::GetWinSystem()->GetGfxContext().GetRenderOrder();
    if (renderOrder == RENDER_ORDER_FRONT_TO_BACK || renderOrder == RENDER_ORDER_ALL_BACK_TO_FRONT)
      CGUITexture::DrawStencilQuad(m_renderRegion, m_stencilLayer);

    CServiceBroker::GetWinSystem()->GetGfxContext().SetStencilLayer(m_stencilLayer);
  }

  CGUIControlGroup::Render();

  if (m_stencilLayer != STENCIL_LAYER::NONE)
    CServiceBroker::GetWinSystem()->GetGfxContext().RestoreStencilLayer();
    
  CServiceBroker::GetWinSystem()->GetGfxContext().SetScissors(oldScissor);
}

EVENT_RESULT CGUIControlGroupMask::SendMouseEvent(const CPoint& point, const MOUSE::CMouseEvent& event)
{
  // transform our position into child coordinates
  CPoint childPoint(point);
  m_transform.InverseTransformPosition(childPoint.x, childPoint.y);

  // bail if we don't hit the masked area
  // all checks are done on a transformed quad
  if (!HitTest(childPoint))
  {
    m_focusedControl = 0;
    return EVENT_RESULT_UNHANDLED;
  }

  return CGUIControlGroup::SendMouseEvent(point, event);
}
