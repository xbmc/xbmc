/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
