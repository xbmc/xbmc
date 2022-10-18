/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/addoninfo/AddonInfo.h"
#include "guilib/GUIDialog.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ADDON
{
class IAddon;
using AddonPtr = std::shared_ptr<IAddon>;

} // namespace ADDON

enum class Reactivate : bool
{
  CHOICE_YES = true,
  CHOICE_NO = false,
};

enum class PerformButtonFocus : bool
{
  CHOICE_YES = true,
  CHOICE_NO = false,
};

enum class EntryPoint : int
{
  INSTALL,
  UPDATE,
  SHOW_DEPENDENCIES,
};

struct CInstalledWithAvailable
{
  CInstalledWithAvailable(const ADDON::DependencyInfo& depInfo,
                          const std::shared_ptr<ADDON::IAddon>& installed,
                          const std::shared_ptr<ADDON::IAddon>& available)
    : m_depInfo(depInfo), m_installed(installed), m_available(available)
  {
  }

  /*!
   * @brief Returns true if the currently installed dependency version is up to date
   * or the dependency is not available from a repository
   */
  bool IsInstalledUpToDate() const;

  ADDON::DependencyInfo m_depInfo;
  std::shared_ptr<ADDON::IAddon> m_installed;
  std::shared_ptr<ADDON::IAddon> m_available;
};

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
  void UpdateControls(PerformButtonFocus performButtonFocus);

  void OnUpdate();
  void OnSelectVersion();
  void OnInstall();
  void OnUninstall();
  void OnEnableDisable();
  void OnSettings();
  void OnSelect();
  void OnToggleAutoUpdates();
  int AskForVersion(std::vector<std::pair<ADDON::CAddonVersion, std::string>>& versions);

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
   * @brief Returns true if current addon can be show list about supported parts
   */
  bool CanShowSupportList() const;

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
   * @param[in] reactivate If true, reactivate info dialog when done
   * @param[in] entryPoint INSTALL, UPDATE or SHOW_DEPENDENCIES
   * @return True if okay was selected, false otherwise
   */
  bool ShowDependencyList(Reactivate reactivate, EntryPoint entryPoint);

  /*!
   * @brief Show a dialog with the addon's supported extensions and mimetypes.
   */
  void ShowSupportList();

  /*!
   * @brief Used to build up the dependency list shown by @ref ShowDependencyList()
   */
  void BuildDependencyList();

  CFileItemPtr m_item;
  ADDON::AddonPtr m_localAddon;
  bool m_addonEnabled = false;

  /*!< a switch to force @ref OnUninstall() to proceed without user interaction.
   *   useful for cases like where another repo’s version of an addon must
   *   be removed before installing a new version.
   */
  bool m_silentUninstall = false;

  bool m_showDepDialogOnInstall = false;
  std::vector<ADDON::DependencyInfo> m_deps;
  std::vector<CInstalledWithAvailable> m_depsInstalledWithAvailable;
};
