/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/RepositoryUpdater.h"
#include "games/GameTypes.h"
#include "guilib/GUIDialog.h"

#include <memory>

namespace KODI
{
namespace GAME
{
class CControllerInstaller;
class IControllerList;
class IFeatureList;

/*!
 * \ingroup games
 */
class CGUIControllerWindow : public CGUIDialog
{
public:
  CGUIControllerWindow(void);
  ~CGUIControllerWindow() override;

  // implementation of CGUIControl via CGUIDialog
  void DoProcess(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;
  bool OnMessage(CGUIMessage& message) override;

protected:
  // implementation of CGUIWindow via CGUIDialog
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

private:
  void OnControllerFocused(unsigned int controllerIndex);
  void OnControllerSelected(unsigned int controllerIndex);
  void OnFeatureFocused(unsigned int featureIndex);
  void OnFeatureSelected(unsigned int featureIndex);
  void UpdateButtons(void);

  // Callbacks for events
  void OnEvent(const ADDON::CRepositoryUpdater::RepositoryUpdated& event);
  void OnEvent(const ADDON::AddonEvent& event);

  // Action for the available button
  void GetMoreControllers(void);
  void GetAllControllers();
  void ResetController(void);
  void ShowHelp(void);
  void ShowButtonCaptureDialog(void);

  IControllerList* m_controllerList = nullptr;
  IFeatureList* m_featureList = nullptr;

  // Game parameters
  GameClientPtr m_gameClient;
  std::string m_controllerId;

  // Controller parameters
  std::unique_ptr<CControllerInstaller> m_installer;
};
} // namespace GAME
} // namespace KODI
