/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
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

#include "GUIRenderTargetFactory.h"
#include "GUIRenderTarget.h"

using namespace KODI;
using namespace RETRO;

CGUIRenderTargetFactory::CGUIRenderTargetFactory(IRenderManager *renderManager) :
  m_renderManager(renderManager)
{
}

CGUIRenderTarget *CGUIRenderTargetFactory::CreateRenderFullScreen(CGameWindowFullScreen &window)
{
  return new CGUIRenderFullScreen(m_renderManager, window);
}

CGUIRenderTarget *CGUIRenderTargetFactory::CreateRenderControl(CGUIGameControl &gameControl)
{
  return new CGUIRenderControl(m_renderManager, gameControl);
}
