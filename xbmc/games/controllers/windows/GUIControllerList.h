/*
 *      Copyright (C) 2014-2016 Team Kodi
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

#include "IConfigurationWindow.h"
#include "games/controllers/ControllerTypes.h"
#include "utils/Observer.h"

class CGUIButtonControl;
class CGUIControlGroupList;
class CGUIWindow;

namespace GAME
{
  class CGUIControllerWindow;

  class CGUIControllerList : public IControllerList,
                             public Observer
  {
  public:
    CGUIControllerList(CGUIWindow* window, IFeatureList* featureList);
    virtual ~CGUIControllerList(void) { Deinitialize(); }

    // implementation of IControllerList
    virtual bool Initialize(void) override;
    virtual void Deinitialize(void) override;
    virtual void Refresh(void) override;
    virtual void OnFocus(unsigned int controllerIndex) override;
    virtual void OnSelect(unsigned int controllerIndex) override;
    virtual void ResetController(void) override;

    // implementation of Observer
    virtual void Notify(const Observable& obs, const ObservableMessage msg) override;

  private:
    bool RefreshControllers(void);

    void CleanupButtons(void);

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
