/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIGameController.h"

#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "utils/log.h"

#include <mutex>

using namespace KODI;
using namespace GAME;

CGUIGameController::CGUIGameController(
    int parentID, int controlID, float posX, float posY, float width, float height)
  : CGUIImage(parentID, controlID, posX, posY, width, height, CTextureInfo())
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_GAMECONTROLLER;
}

CGUIGameController::CGUIGameController(const CGUIGameController& from) : CGUIImage(from)
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

  std::unique_lock<CCriticalSection> lock(m_mutex);

  if (m_currentController)
  {
    //! @todo Render pressed buttons
  }
}

void CGUIGameController::ActivateController(const ControllerPtr& controller)
{
  std::unique_lock<CCriticalSection> lock(m_mutex);

  if (controller && controller != m_currentController)
  {
    m_currentController = controller;

    lock.unlock();

    //! @todo Sometimes this fails on window init
    SetFileName(m_currentController->Layout().ImagePath());
  }
}
