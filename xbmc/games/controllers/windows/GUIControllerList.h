/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IConfigurationWindow.h"
#include "addons/AddonEvents.h"
#include "games/GameTypes.h"
#include "games/controllers/ControllerTypes.h"

#include <set>
#include <string>

class CGUIButtonControl;
class CGUIControlGroupList;
class CGUIWindow;

namespace KODI
{
namespace GAME
{
  class CGUIControllerWindow;

  class CGUIControllerList : public IControllerList
  {
  public:
    CGUIControllerList(CGUIWindow* window, IFeatureList* featureList, GameClientPtr gameClient);
    virtual ~CGUIControllerList(void) { Deinitialize(); }

    // implementation of IControllerList
    virtual bool Initialize(void) override;
    virtual void Deinitialize(void) override;
    virtual bool Refresh(const std::string& controllerId) override;
    virtual void OnFocus(unsigned int controllerIndex) override;
    virtual void OnSelect(unsigned int controllerIndex) override;
    virtual int GetFocusedController() const override { return m_focusedController; }
    virtual void ResetController(void) override;

  private:
    bool RefreshControllers(void);

    void CleanupButtons(void);
    void OnEvent(const ADDON::AddonEvent& event);

    // GUI stuff
    CGUIWindow* const     m_guiWindow;
    IFeatureList* const   m_featureList;
    CGUIControlGroupList* m_controllerList;
    CGUIButtonControl*    m_controllerButton;

    // Game stuff
    ControllerVector      m_controllers;
    int                   m_focusedController;
    GameClientPtr         m_gameClient;
  };
}
}
