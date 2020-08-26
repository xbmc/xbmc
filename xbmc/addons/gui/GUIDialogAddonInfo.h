/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"
#include "guilib/GUIDialog.h"

#include <string>
#include <utility>
#include <vector>

class CGUIDialogAddonInfo : public CGUIDialog
{
public:
  CGUIDialogAddonInfo(void);
  ~CGUIDialogAddonInfo(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;

  CFileItemPtr GetCurrentListItem(int offset = 0) override { return m_item; }
  bool HasListItems() const override { return true; }

  static bool ShowForItem(const CFileItemPtr& item);

private:
  void OnInitWindow() override;

  /*!
   * @brief Set the item to display addon info on.
   *
   * @param[in] item to display
   * @return true if we can display information, false otherwise
   */
  bool SetItem(const CFileItemPtr& item);
  void UpdateControls();

  void OnUpdate();
  void OnInstall();
  void OnUninstall();
  void OnEnableDisable();
  void OnSettings();
  void OnSelect();
  void OnToggleAutoUpdates();
  int AskForVersion(std::vector<std::pair<ADDON::AddonVersion, std::string>>& versions);

  /*!
   * @brief Returns true if current addon can be opened (i.e is a plugin)
   */
  bool CanOpen() const;

  /*!
   * @brief Returns true if current addon can be run (i.e is a script)
   */
  bool CanRun() const;

  /*!
   * @brief Returns true if current addon is of a type that can only have one active
   * in use at a time and can be changed (e.g skins)
   */
  bool CanUse() const;

  /*!
   * @brief check if the add-on is a dependency of others, and if so prompt the user.
   *
   * @param[in] heading the label for the heading of the prompt dialog
   * @param[in] line2 the action that could not be completed.
   * @return true if prompted, false otherwise.
   */
  bool PromptIfDependency(int heading, int line2);

  /*!
   * @brief Show a dialog with the addon's dependencies.
   *
   * @param[in] deps List of dependencies
   * @param[in] reactivate If true, reactivate info dialog when done
   * @return True if okay was selected, false otherwise
   */
  bool ShowDependencyList(const std::vector<ADDON::DependencyInfo>& deps, bool reactivate);

  CFileItemPtr m_item;
  ADDON::AddonPtr m_localAddon;
  bool m_addonEnabled = false;

  /*!< a switch to force @ref OnUninstall() to proceed without user interaction.
   *   useful for cases like where another repo’s version of an addon must
   *   be removed before installing a new version.
   */
  bool m_silentUninstall = false;
};
