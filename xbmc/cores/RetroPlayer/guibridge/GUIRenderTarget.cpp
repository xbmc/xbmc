/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRenderTarget.h"

#include "cores/RetroPlayer/guicontrols/GUIGameControl.h"
#include "cores/RetroPlayer/guiwindows/GameWindowFullScreen.h"
#include "cores/RetroPlayer/rendering/IRenderManager.h"

using namespace KODI;
using namespace RETRO;

// --- CGUIRenderTarget --------------------------------------------------------

CGUIRenderTarget::CGUIRenderTarget(IRenderManager* renderManager) : m_renderManager(renderManager)
{
}

// --- CGUIRenderControl -------------------------------------------------------

CGUIRenderControl::CGUIRenderControl(IRenderManager* renderManager, CGUIGameControl& gameControl)
  : CGUIRenderTarget(renderManager), m_gameControl(gameControl)
{
}

void CGUIRenderControl::Render()
{
  m_renderManager->RenderControl(true, true, m_gameControl.GetRenderRegion(),
                                 m_gameControl.GetRenderSettings());
}

void CGUIRenderControl::RenderEx()
{
  //! @todo
  // m_renderManager->RenderControl(false, false, m_gameControl.GetRenderRegion(),
  // m_gameControl.GetRenderSettings());
}

// --- CGUIRenderFullScreen ----------------------------------------------------

CGUIRenderFullScreen::CGUIRenderFullScreen(IRenderManager* renderManager,
                                           CGameWindowFullScreen& window)
  : CGUIRenderTarget(renderManager), m_window(window)
{
}

void CGUIRenderFullScreen::Render()
{
  m_renderManager->RenderWindow(true, m_window.GetCoordsRes());
}

void CGUIRenderFullScreen::RenderEx()
{
  //! @todo
  // m_renderManager->RenderWindow(false, m_window.GetCoordsRes());
}

void CGUIRenderFullScreen::ClearBackground()
{
  m_renderManager->ClearBackground();
}
