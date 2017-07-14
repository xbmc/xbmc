#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <utility>
#include <vector>

#include "guilib/GUIDialog.h"
#include "addons/IAddon.h"

class CGUIDialogAddonInfo : public CGUIDialog
{
public:
  CGUIDialogAddonInfo(void);
  ~CGUIDialogAddonInfo(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  
  CFileItemPtr GetCurrentListItem(int offset = 0) override { return m_item; }
  bool HasListItems() const override { return true; }

  static bool ShowForItem(const CFileItemPtr& item);

private:
  void OnInitWindow() override;

  /*! \brief Set the item to display addon info on.
   \param item to display
   \return true if we can display information, false otherwise
   */
  bool SetItem(const CFileItemPtr &item);
  void UpdateControls();

  void OnUpdate();
  void OnInstall();
  void OnUninstall();
  void OnEnableDisable();
  void OnSettings();
  void OnSelect();
  void OnToggleAutoUpdates();
  int AskForVersion(std::vector<std::pair<ADDON::AddonVersion, std::string>>& versions);

  /*! Returns true if current addon can be opened (i.e is a plugin)*/
  bool CanOpen() const;

  /*! Returns true if current addon can be run (i.e is a script)*/
  bool CanRun() const;

  /*!
   * Returns true if current addon is of a type that can only have one active
   * in use at a time and can be changed (e.g skins)*/
  bool CanUse() const;

  /*! \brief check if the add-on is a dependency of others, and if so prompt the user.
   \param heading the label for the heading of the prompt dialog
   \param line2 the action that could not be completed.
   \return true if prompted, false otherwise.
   */
  bool PromptIfDependency(int heading, int line2);

  /*! \brief Show a dialog with the addon's dependencies.
   *  \param deps List of dependencies
   *  \param reactivate If true, reactivate info dialog when done
   *  \return True if okay was selected, false otherwise
   */
  bool ShowDependencyList(const ADDON::ADDONDEPS& deps, bool reactivate);

  CFileItemPtr m_item;
  ADDON::AddonPtr m_localAddon;
  bool m_addonEnabled;
};

