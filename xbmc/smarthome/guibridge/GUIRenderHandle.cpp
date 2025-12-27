/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRenderHandle.h"

#include "SmartHomeGuiBridge.h"

using namespace KODI;
using namespace SMART_HOME;

// --- CGUIRenderHandle --------------------------------------------------------

CGUIRenderHandle::CGUIRenderHandle(CSmartHomeGuiBridge& renderManager)
  : m_renderManager(renderManager)
{
}

CGUIRenderHandle::~CGUIRenderHandle()
{
  m_renderManager.UnregisterHandle(this);
}

void CGUIRenderHandle::Render()
{
  m_renderManager.Render(this);
}

bool CGUIRenderHandle::IsDirty()
{
  return m_renderManager.IsDirty(this);
}

void CGUIRenderHandle::ClearBackground()
{
  m_renderManager.ClearBackground(this);
}

// --- CGUIRenderControlHandle -------------------------------------------------

CGUIRenderControlHandle::CGUIRenderControlHandle(CSmartHomeGuiBridge& renderManager,
                                                 CGUICameraControl& control)
  : CGUIRenderHandle(renderManager), m_control(control)
{
}
