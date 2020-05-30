/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRenderTargetFactory.h"

#include "GUIRenderTarget.h"

using namespace KODI;
using namespace RETRO;

CGUIRenderTargetFactory::CGUIRenderTargetFactory(IRenderManager* renderManager)
  : m_renderManager(renderManager)
{
}

CGUIRenderTarget* CGUIRenderTargetFactory::CreateRenderFullScreen(CGameWindowFullScreen& window)
{
  return new CGUIRenderFullScreen(m_renderManager, window);
}

CGUIRenderTarget* CGUIRenderTargetFactory::CreateRenderControl(CGUIGameControl& gameControl)
{
  return new CGUIRenderControl(m_renderManager, gameControl);
}
