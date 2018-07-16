/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRenderHandle.h"
#include "GUIGameRenderManager.h"

using namespace KODI;
using namespace RETRO;

// --- CGUIRenderHandle --------------------------------------------------------

CGUIRenderHandle::CGUIRenderHandle(CGUIGameRenderManager &renderManager, RENDER_HANDLE type) :
  m_renderManager(renderManager),
  m_type(type)
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

void CGUIRenderHandle::RenderEx()
{
  m_renderManager.RenderEx(this);
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

CGUIRenderControlHandle::CGUIRenderControlHandle(CGUIGameRenderManager &renderManager, CGUIGameControl &control) :
  CGUIRenderHandle(renderManager, RENDER_HANDLE::CONTROL),
  m_control(control)
{
}

// --- CGUIRenderFullScreenHandle ----------------------------------------------

CGUIRenderFullScreenHandle::CGUIRenderFullScreenHandle(CGUIGameRenderManager &renderManager, CGameWindowFullScreen &window) :
  CGUIRenderHandle(renderManager, RENDER_HANDLE::WINDOW),
  m_window(window)
{
}
