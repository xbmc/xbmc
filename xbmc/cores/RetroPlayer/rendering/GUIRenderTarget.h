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
#pragma once

namespace KODI
{
namespace RETRO
{
  class CGameWindowFullScreen;
  class CGUIGameControl;
  class IRenderManager;

  // --- CGUIRenderTarget ------------------------------------------------------

  class CGUIRenderTarget
  {
  public:
    CGUIRenderTarget(IRenderManager *renderManager);

    virtual ~CGUIRenderTarget() = default;

    virtual void Render() = 0;

    virtual void RenderEx() = 0;

    virtual void ClearBackground() { } //! @todo

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
