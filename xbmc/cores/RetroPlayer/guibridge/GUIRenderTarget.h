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

  // --- CGUIRenderTarget ------------------------------------------------------

  /*!
   * \brief A target of rendering commands
   *
   * This class abstracts the destination of rendering commands. As a result,
   * controls and windows are given a unified API.
   */
  class CGUIRenderTarget
  {
  public:
    CGUIRenderTarget(IRenderManager *renderManager);

    virtual ~CGUIRenderTarget() = default;

    /*!
     * \brief Draw the frame to the rendering area
     */
    virtual void Render() = 0;

    /*!
     * \brief Draw the frame to the rendering area differently somehow
     */
    virtual void RenderEx() = 0;

    /*!
     * \brief Clear the background of the rendering area
     */
    virtual void ClearBackground() { } //! @todo

    /*!
     * \brief Check of the rendering area is dirty
     */
    virtual bool IsDirty() { return true; } //! @todo

  protected:
    // Construction parameters
    IRenderManager *const m_renderManager;
  };

  // --- CGUIRenderControl -----------------------------------------------------

  class CGUIRenderControl : public CGUIRenderTarget
  {
  public:
    CGUIRenderControl(IRenderManager *renderManager, CGUIGameControl &gameControl);
    ~CGUIRenderControl() override = default;

    // implementation of CGUIRenderTarget
    void Render() override;
    void RenderEx() override;

  private:
    // Construction parameters
    CGUIGameControl &m_gameControl;
  };

  // --- CGUIRenderFullScreen --------------------------------------------------

  class CGUIRenderFullScreen : public CGUIRenderTarget
  {
  public:
    CGUIRenderFullScreen(IRenderManager *renderManager, CGameWindowFullScreen &window);
    ~CGUIRenderFullScreen() override = default;

    // implementation of CGUIRenderTarget
    void Render() override;
    void RenderEx() override;
    void ClearBackground() override;

  private:
    // Construction parameters
    CGameWindowFullScreen &m_window;
  };
}
}
