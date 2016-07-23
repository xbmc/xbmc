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

#include "addons/RepositoryUpdater.h"
#include "guilib/GUIDialog.h"
#include "utils/Observer.h"

namespace GAME
{
  class IControllerList;
  class IFeatureList;

  class CGUIControllerWindow : public CGUIDialog
  {
  public:
    CGUIControllerWindow(void);
    virtual ~CGUIControllerWindow(void);

    // implementation of CGUIControl via CGUIDialog
    virtual bool OnMessage(CGUIMessage& message) override;

  protected:
    // implementation of CGUIWindow via CGUIDialog
    virtual void OnInitWindow(void) override;
    virtual void OnDeinitWindow(int nextWindowID) override;

  private:
    void OnControllerFocused(unsigned int controllerIndex);
    void OnControllerSelected(unsigned int controllerIndex);
    void OnFeatureFocused(unsigned int featureIndex);
    void OnFeatureSelected(unsigned int featureIndex);
    void OnEvent(const ADDON::CRepositoryUpdater::RepositoryUpdated& event);
    void UpdateButtons(void);

    // Action for the available button
    void GetMoreControllers(void);
    void ResetController(void);
    void ShowHelp(void);

    IControllerList* m_controllerList;
    IFeatureList*    m_featureList;
  };
}
