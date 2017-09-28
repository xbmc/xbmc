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

#include "threads/CriticalSection.h"

#include <map>
#include <memory>

namespace KODI
{
namespace RETRO
{
  class CGameWindowFullScreen;
  class CGUIGameControl;
  class CGUIRenderTargetFactory;
  class CGUIRenderHandle;
  class CGUIRenderTarget;

  class CGUIGameRenderManager
  {
    friend class CGUIRenderHandle;

  public:
    CGUIGameRenderManager() = default;
    ~CGUIGameRenderManager();

    void RegisterFactory(CGUIRenderTargetFactory *factory);
    void UnregisterFactory();

    std::shared_ptr<CGUIRenderHandle> RegisterControl(CGUIGameControl &control);
    std::shared_ptr<CGUIRenderHandle> RegisterWindow(CGameWindowFullScreen &window);

  protected:
    // Functions exposed to friend class CGUIRenderHandle
    void UnregisterHandle(CGUIRenderHandle *handle);
    void Render(CGUIRenderHandle *handle);
    void RenderEx(CGUIRenderHandle *handle);
    void ClearBackground(CGUIRenderHandle *handle);
    bool IsDirty(CGUIRenderHandle *handle);

  private:
    void UpdateRenderTargets();

    CGUIRenderTarget *CreateRenderTarget(CGUIRenderHandle *handle);

    CGUIRenderTargetFactory *m_factory = nullptr;
    std::map<CGUIRenderHandle*, std::shared_ptr<CGUIRenderTarget>> m_renderTargets;

    CCriticalSection m_mutex;
  };
}
}
