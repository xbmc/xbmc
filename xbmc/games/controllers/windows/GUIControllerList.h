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

/*!
 * \ingroup games
 */
class CGUIControllerList : public IControllerList
{
public:
  /*!
   * \brief Create a GUI controller list
   *
   * \param window The GUI window handle
   * \param featureList The controller interface for the feature list
   * \param gameClient A gameclient used to filter controllers
   * \param controllerId A controller ID used to filter controllers (only the
   *                     specified controller will be shown, or the default
   *                     controller if the specified controller isn't installed)
   */
  CGUIControllerList(CGUIWindow* window,
                     IFeatureList* featureList,
                     GameClientPtr gameClient,
                     std::string controllerId);
  ~CGUIControllerList() override { Deinitialize(); }

  // implementation of IControllerList
  bool Initialize() override;
  void Deinitialize() override;
  bool Refresh(const std::string& controllerId) override;
  void OnFocus(unsigned int controllerIndex) override;
  void OnSelect(unsigned int controllerIndex) override;
  int GetFocusedController() const override { return m_focusedController; }
  void ResetController() override;

private:
  bool RefreshControllers(void);

  void CleanupButtons(void);
  void OnEvent(const ADDON::AddonEvent& event);

  // GUI stuff
  CGUIWindow* const m_guiWindow;
  IFeatureList* const m_featureList;
  CGUIControlGroupList* m_controllerList = nullptr;
  CGUIButtonControl* m_controllerButton = nullptr;

  // Game stuff
  ControllerVector m_controllers;
  int m_focusedController = -1; // Initially unfocused
  GameClientPtr m_gameClient;
  std::string m_controllerId;
};
} // namespace GAME
} // namespace KODI
