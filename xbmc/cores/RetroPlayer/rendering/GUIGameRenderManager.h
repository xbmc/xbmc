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

#include "cores/IPlayer.h"
#include "threads/CriticalSection.h"

#include <map>
#include <memory>

namespace KODI
{
namespace GAME
{
  class CDialogGameVideoSelect;
}

namespace RETRO
{
  class CGameWindowFullScreen;
  class CGUIGameControl;
  class CGUIGameVideoHandle;
  class CGUIRenderTargetFactory;
  class CGUIRenderHandle;
  class CGUIRenderTarget;
  class IRenderCallback;

  class CGUIGameRenderManager
  {
    friend class CGUIGameVideoHandle;
    friend class CGUIRenderHandle;

  public:
    CGUIGameRenderManager() = default;
    ~CGUIGameRenderManager();

    void RegisterPlayer(CGUIRenderTargetFactory *factory, IRenderCallback *callback);
    void UnregisterPlayer();

    std::shared_ptr<CGUIRenderHandle> RegisterControl(CGUIGameControl &control);
    std::shared_ptr<CGUIRenderHandle> RegisterWindow(CGameWindowFullScreen &window);
    std::shared_ptr<CGUIGameVideoHandle> RegisterDialog(GAME::CDialogGameVideoSelect &dialog);

  protected:
    // Functions exposed to friend class CGUIRenderHandle
    void UnregisterHandle(CGUIRenderHandle *handle);
    void Render(CGUIRenderHandle *handle);
    void RenderEx(CGUIRenderHandle *handle);
    void ClearBackground(CGUIRenderHandle *handle);
    bool IsDirty(CGUIRenderHandle *handle);

    // Functions exposed to friend class CGUIGameVideoHandle
    void UnregisterHandle(CGUIGameVideoHandle *handle) { }
    bool IsPlayingGame();
    bool SupportsRenderFeature(ERENDERFEATURE feature);
    bool SupportsScalingMethod(ESCALINGMETHOD method);

  private:
    void UpdateRenderTargets();

    CGUIRenderTarget *CreateRenderTarget(CGUIRenderHandle *handle);

    CGUIRenderTargetFactory *m_factory = nullptr;
    std::map<CGUIRenderHandle*, std::shared_ptr<CGUIRenderTarget>> m_renderTargets;
    CCriticalSection m_targetMutex;

    IRenderCallback *m_callback = nullptr;
    CCriticalSection m_callbackMutex;
  };
}
}
