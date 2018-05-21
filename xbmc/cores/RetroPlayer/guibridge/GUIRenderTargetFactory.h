/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
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
    CGUIRenderTargetFactory(IRenderManager *renderManager);

    /*!
     * \brief Create a render target for the fullscreen window
     */
    CGUIRenderTarget *CreateRenderFullScreen(CGameWindowFullScreen &window);

    /*!
     * \brief Create a render target for a game control
     */
    CGUIRenderTarget *CreateRenderControl(CGUIGameControl &gameControl);

  private:
    // Construction parameters
    IRenderManager *m_renderManager;
  };
}
}
