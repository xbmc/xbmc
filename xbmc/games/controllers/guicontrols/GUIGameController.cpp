/*
 *      Copyright (C) 2014-2017 Team Kodi
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

#include "GUIGameController.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

CGUIGameController::CGUIGameController(int parentID, int controlID, float posX, float posY, float width, float height)
  : CGUIImage(parentID, controlID, posX, posY, width, height, CTextureInfo())
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_GAMECONTROLLER;
}

CGUIGameController::CGUIGameController(const CGUIGameController &from)
  : CGUIImage(from)
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_GAMECONTROLLER;
}

CGUIGameController* CGUIGameController::Clone(void) const
{
  return new CGUIGameController(*this);
}

void CGUIGameController::Render(void)
{
  CGUIImage::Render();

  CSingleLock lock(m_mutex);

  if (m_currentController)
  {
    //! @todo Render pressed buttons
  }
}

void CGUIGameController::ActivateController(const ControllerPtr& controller)
{
  CSingleLock lock(m_mutex);

  if (controller && controller != m_currentController)
  {
    m_currentController = controller;

    lock.Leave();

    //! @todo Sometimes this fails on window init
    SetFileName(m_currentController->Layout().ImagePath());
  }
}
