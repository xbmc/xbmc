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
  class CGUIGameRenderManager;

  enum class RENDER_HANDLE
  {
    CONTROL,
    WINDOW,
  };

  // --- CGUIRenderHandle ------------------------------------------------------

  class CGUIRenderHandle
  {
  public:
    CGUIRenderHandle(CGUIGameRenderManager &renderManager, RENDER_HANDLE type);
    virtual ~CGUIRenderHandle();

    RENDER_HANDLE Type() const { return m_type; }

    void Render();
    void RenderEx();
    bool IsDirty();
    void ClearBackground();

  private:
    // Construction parameters
    CGUIGameRenderManager &m_renderManager;
    const RENDER_HANDLE m_type;
  };

  // --- CGUIRenderControlHandle -----------------------------------------------

  class CGUIRenderControlHandle : public CGUIRenderHandle
  {
  public:
    CGUIRenderControlHandle(CGUIGameRenderManager &renderManager, CGUIGameControl &control);
    ~CGUIRenderControlHandle() override = default;

    CGUIGameControl &GetControl() { return m_control; }

  private:
    // Construction parameters
    CGUIGameControl &m_control;
  };

  // --- CGUIRenderFullScreenHandle --------------------------------------------

  class CGUIRenderFullScreenHandle : public CGUIRenderHandle
  {
  public:
    CGUIRenderFullScreenHandle(CGUIGameRenderManager &renderManager, CGameWindowFullScreen &window);
    ~CGUIRenderFullScreenHandle() override = default;

    CGameWindowFullScreen &GetWindow() { return m_window; }

  private:
    // Construction parameters
    CGameWindowFullScreen &m_window;
  };
}
}
