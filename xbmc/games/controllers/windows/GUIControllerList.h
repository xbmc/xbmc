/*
 *      Copyright (C) 2014-present Team Kodi
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
#pragma once

#include "IConfigurationWindow.h"
#include "addons/AddonEvents.h"
#include "addons/Addon.h"
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
    CGUIControllerList(CGUIWindow* window, IFeatureList* featureList);
    virtual ~CGUIControllerList(void) { Deinitialize(); }

    // implementation of IControllerList
    virtual bool Initialize(void) override;
    virtual void Deinitialize(void) override;
    virtual bool Refresh(void) override;
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
  };
}
}
