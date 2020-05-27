/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace RETRO
{
class CGameWindowFullScreen;
class CGUIGameControl;
class IRenderManager;
class CGUIRenderTarget;

class CGUIRenderTargetFactory
{
public:
  CGUIRenderTargetFactory(IRenderManager* renderManager);

  /*!
   * \brief Create a render target for the fullscreen window
   */
  CGUIRenderTarget* CreateRenderFullScreen(CGameWindowFullScreen& window);

  /*!
   * \brief Create a render target for a game control
   */
  CGUIRenderTarget* CreateRenderControl(CGUIGameControl& gameControl);

private:
  // Construction parameters
  IRenderManager* m_renderManager;
};
} // namespace RETRO
} // namespace KODI
