/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "guilib/GUIImage.h"
#include "threads/CriticalSection.h"

namespace KODI
{
namespace GAME
{
  class CGUIGameController : public CGUIImage
  {
  public:
    CGUIGameController(int parentID, int controlID, float posX, float posY, float width, float height);
    CGUIGameController(const CGUIGameController &from);

    virtual ~CGUIGameController() = default;

    // implementation of CGUIControl via CGUIImage
    virtual CGUIGameController* Clone(void) const override;
    virtual void Render(void) override;

    void ActivateController(const ControllerPtr& controller);

  private:
    ControllerPtr       m_currentController;
    CCriticalSection    m_mutex;
  };
}
}
