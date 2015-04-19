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

#include "addons/Addon.h"
#include "windows/GUIMediaWindow.h"
#include "ThumbLoader.h"

class CFileItem;
class CFileItemList;

class CGUIWindowAddonBrowser : public CGUIMediaWindow
{
public:
  CGUIWindowAddonBrowser(void);
  virtual ~CGUIWindowAddonBrowser(void);
  virtual bool OnMessage(CGUIMessage& message);

  /*! \brief Popup a selection dialog with a list of addons of the given type
   \param type the type of addon wanted
   \param addonID [out] the addon ID of the selected item
   \param showNone whether there should be a "None" item in the list (defaults to false)
   \param showDetails whether to show details of the addons or not
   \param showInstalled whether installed addons should be in the list
   \param showInstallable whether installable addons should be in the list
   \param showMore whether to show the "Get More" button (only makes sense if showInstalled is true and showInstallable is false)
   \return 1 if an addon was selected, 2 if "Get More" was chosen, or 0 if an error occurred or if the selection process was cancelled
   */
  static int SelectAddonID(ADDON::TYPE type, std::string &addonID, bool showNone = false, bool showDetails = true, bool showInstalled = true, bool showInstallable = false, bool showMore = true);
  static int SelectAddonID(const std::vector<ADDON::TYPE> &types, std::string &addonID, bool showNone = false, bool showDetails = true, bool showInstalled = true, bool showInstallable = false, bool showMore = true);
  /*! \brief Popup a selection dialog with a list of addons of the given type
   \param type the type of addon wanted
   \param addonIDs [out] array of selected addon IDs
   \param showNone whether there should be a "None" item in the list (defaults to false)
   \param showDetails whether to show details of the addons or not
   \param multipleSelection allow selection of multiple addons, if set to true showNone will automaticly switch to false
   \param showInstalled whether installed addons should be in the list
   \param showInstallable whether installable addons should be in the list
   \param showMore whether to show the "Get More" button (only makes sense if showInstalled is true and showInstallable is false)
   \return 1 if an addon was selected or multiple selection was specified, 2 if "Get More" was chosen, or 0 if an error occurred or if the selection process was cancelled
   */
  static int SelectAddonID(ADDON::TYPE type, std::vector<std::string> &addonIDs, bool showNone = false, bool showDetails = true, bool multipleSelection = true, bool showInstalled = true, bool showInstallable = false, bool showMore = true);
  static int SelectAddonID(const std::vector<ADDON::TYPE> &types, std::vector<std::string> &addonIDs, bool showNone = false, bool showDetails = true, bool multipleSelection = true, bool showInstalled = true, bool showInstallable = false, bool showMore = true);
  
protected:
  /* \brief set label2 of an item based on the Addon.Status property
   \param item the item to update
   */
  void SetItemLabel2(CFileItemPtr item);

  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual bool OnClick(int iItem);
  virtual void UpdateButtons();
  virtual bool GetDirectory(const std::string &strDirectory, CFileItemList &items);
  virtual bool Update(const std::string &strDirectory, bool updateFilterPath = true);
  virtual std::string GetStartFolder(const std::string &dir);

private:
  void SetProperties();
  CProgramThumbLoader m_thumbLoader;
};

