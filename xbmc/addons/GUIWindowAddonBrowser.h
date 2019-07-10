/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Addon.h"
#include "AddonEvents.h"
#include "RepositoryUpdater.h"
#include "ThumbLoader.h"
#include "windows/GUIMediaWindow.h"

#include <string>
#include <vector>


class CFileItem;
class CFileItemList;

class CGUIWindowAddonBrowser : public CGUIMediaWindow
{
public:
  CGUIWindowAddonBrowser(void);
  ~CGUIWindowAddonBrowser(void) override;
  bool OnMessage(CGUIMessage& message) override;

  /*! \brief Popup a selection dialog with a list of addons of the given type
   \param type the type of addon wanted
   \param addonID [in/out] the addon ID of the (pre) selected item
   \param showNone whether there should be a "None" item in the list (defaults to false)
   \param showDetails whether to show details of the addons or not
   \param showInstalled whether installed addons should be in the list
   \param showInstallable whether installable addons should be in the list
   \param showMore whether to show the "Get More" button (only makes sense if showInstalled is true and showInstallable is false)
   \return 1 if an addon was selected or multiple selection was specified, 2 if "Get More" was chosen, 0 if the selection process was cancelled or -1 if an error occurred or
   */
  static int SelectAddonID(ADDON::TYPE type, std::string &addonID, bool showNone = false, bool showDetails = true, bool showInstalled = true, bool showInstallable = false, bool showMore = true);
  static int SelectAddonID(const std::vector<ADDON::TYPE> &types, std::string &addonID, bool showNone = false, bool showDetails = true, bool showInstalled = true, bool showInstallable = false, bool showMore = true);
  /*! \brief Popup a selection dialog with a list of addons of the given type
   \param type the type of addon wanted
   \param addonIDs [in/out] array of (pre) selected addon IDs
   \param showNone whether there should be a "None" item in the list (defaults to false)
   \param showDetails whether to show details of the addons or not
   \param multipleSelection allow selection of multiple addons, if set to true showNone will automatically switch to false
   \param showInstalled whether installed addons should be in the list
   \param showInstallable whether installable addons should be in the list
   \param showMore whether to show the "Get More" button (only makes sense if showInstalled is true and showInstallable is false)
   \return 1 if an addon was selected or multiple selection was specified, 2 if "Get More" was chosen, 0 if the selection process was cancelled or -1 if an error occurred or
   */
  static int SelectAddonID(ADDON::TYPE type, std::vector<std::string> &addonIDs, bool showNone = false, bool showDetails = true, bool multipleSelection = true, bool showInstalled = true, bool showInstallable = false, bool showMore = true);
  static int SelectAddonID(const std::vector<ADDON::TYPE> &types, std::vector<std::string> &addonIDs, bool showNone = false, bool showDetails = true, bool multipleSelection = true, bool showInstalled = true, bool showInstallable = false, bool showMore = true);

  bool UseFileDirectories() override { return false; }

  static void InstallFromZip();

protected:
  bool OnClick(int iItem, const std::string &player = "") override;
  void UpdateButtons() override;
  bool GetDirectory(const std::string &strDirectory, CFileItemList &items) override;
  bool Update(const std::string &strDirectory, bool updateFilterPath = true) override;
  std::string GetStartFolder(const std::string &dir) override;

  std::string GetRootPath() const override { return "addons://"; }

private:
  void SetProperties();
  void UpdateStatus(const CFileItemPtr& item);
  void OnEvent(const ADDON::CRepositoryUpdater::RepositoryUpdated& event);
  void OnEvent(const ADDON::AddonEvent& event);
  CProgramThumbLoader m_thumbLoader;
};

