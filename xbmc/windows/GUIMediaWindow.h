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

#include "dialogs/GUIDialogContextMenu.h"
#include "filesystem/DirectoryHistory.h"
#include "filesystem/VirtualDirectory.h"
#include "guilib/GUIWindow.h"
#include "playlists/SmartPlayList.h"
#include "view/GUIViewControl.h"

class CFileItemList;
class CGUIViewState;

// base class for all media windows
class CGUIMediaWindow : public CGUIWindow
{
public:
  CGUIMediaWindow(int id, const char *xmlFile);
  virtual ~CGUIMediaWindow(void);

  // specializations of CGUIControl
  virtual bool OnAction(const CAction &action) override;
  virtual bool OnBack(int actionID) override;
  virtual bool OnMessage(CGUIMessage& message) override;

  // specializations of CGUIWindow
  virtual void OnWindowLoaded() override;
  virtual void OnWindowUnload() override;
  virtual void OnInitWindow() override;
  virtual bool IsMediaWindow() const  override { return true; }
  int GetViewContainerID() const  override { return m_viewControl.GetCurrentControl(); }
  int GetViewCount() const  override { return m_viewControl.GetViewModeCount(); };
  virtual bool HasListItems() const  override { return true; }
  virtual CFileItemPtr GetCurrentListItem(int offset = 0) override;

  // custom methods
  virtual bool CanFilterAdvanced() { return m_canFilterAdvanced; }
  virtual bool IsFiltered();
  virtual bool IsSameStartFolder(const std::string &dir);

  virtual std::string GetRootPath() const { return ""; }

  const CFileItemList &CurrentDirectory() const;
  const CGUIViewState *GetViewState() const;

protected:
  // specializations of CGUIControlGroup
  virtual CGUIControl *GetFirstFocusableControl(int id) override;

  // specializations of CGUIWindow
  virtual void LoadAdditionalTags(TiXmlElement *root) override;

  // custom methods
  virtual void SetupShares();
  virtual bool GoParentFolder();
  virtual bool OnClick(int iItem, const std::string &player = "");

  /* \brief React to a "Select" action on an item in a view.
   \param item selected item.
   \return true if the action is handled, false otherwise.
   */
  virtual bool OnSelect(int item);
  virtual bool OnPopupMenu(int iItem);

  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual bool OnAddMediaSource() { return false; };

  virtual void FormatItemLabels(CFileItemList &items, const LABEL_MASKS &labelMasks);
  virtual void UpdateButtons();
  virtual void SaveControlStates() override;
  virtual void RestoreControlStates() override;

  virtual bool GetDirectory(const std::string &strDirectory, CFileItemList &items);
  /*! \brief Retrieves the items from the given path and updates the list
   \param strDirectory The path to the directory to get the items from
   \param updateFilterPath Whether to update the filter path in m_strFilterPath or not
   \return true if the list was successfully updated otherwise false
   \sa GetDirectory
   \sa m_vecItems
   \sa m_strFilterPath
   */
  virtual bool Update(const std::string &strDirectory, bool updateFilterPath = true);
  /*! \brief Refreshes the current list by retrieving the lists's path
   \return true if the list was successfully refreshed otherwise false
   \sa Update
   \sa GetDirectory
   */
  virtual bool Refresh(bool clearCache = false);

  virtual void FormatAndSort(CFileItemList &items);
  virtual void OnPrepareFileItems(CFileItemList &items);
  virtual void OnCacheFileItems(CFileItemList &items);
  virtual void GetGroupedItems(CFileItemList &items) { }

  void ClearFileItems();
  virtual void SortItems(CFileItemList &items);

  /*! \brief Check if the given list can be advance filtered or not
   \param items List of items to check
   \return true if the list can be advance filtered otherwise false
   */
  virtual bool CheckFilterAdvanced(CFileItemList &items) const { return false; }
  /*! \brief Check if the given path can contain a "filter" parameter
   \param strDirectory Path to check
   \return true if the given path can contain a "filter" parameter otherwise false
   */
  virtual bool CanContainFilter(const std::string &strDirectory) const { return false; }
  virtual void UpdateFilterPath(const std::string &strDirector, const CFileItemList &items, bool updateFilterPath);
  virtual bool Filter(bool advanced = true);

  /* \brief Called on response to a GUI_MSG_FILTER_ITEMS message
   Filters the current list with the given filter using FilterItems()
   \param filter the filter to use.
   \sa FilterItems
   */
  void OnFilterItems(const std::string &filter);

  /* \brief Retrieve the filtered item list
   \param filter filter to apply
   \param items CFileItemList to filter
   \sa OnFilterItems
   */
  virtual bool GetFilteredItems(const std::string &filter, CFileItemList &items);

  /* \brief Retrieve the advance filtered item list
  \param items CFileItemList to filter
  \param hasNewItems Whether the filtered item list contains new items
                     which were not present in the original list
  \sa GetFilteredItems
  */
  virtual bool GetAdvanceFilteredItems(CFileItemList &items);

  // check for a disc or connection
  virtual bool HaveDiscOrConnection(const std::string& strPath, int iDriveType);
  void ShowShareErrorMessage(CFileItem* pItem);

  void SaveSelectedItemInHistory();
  void RestoreSelectedItemFromHistory();
  void GetDirectoryHistoryString(const CFileItem* pItem, std::string& strHistoryString);
  void SetHistoryForPath(const std::string& strDirectory);
  virtual void LoadPlayList(const std::string& strFileName) {}
  virtual bool OnPlayMedia(int iItem, const std::string &player = "");
  virtual bool OnPlayAndQueueMedia(const CFileItemPtr &item, std::string player = "");
  void UpdateFileList();
  virtual void OnDeleteItem(int iItem);
  void OnRenameItem(int iItem);

  bool WaitForNetwork() const;

  /*! \brief Translate the folder to start in from the given quick path
   \param dir the folder the user wants
   \return the resulting path */
  virtual std::string GetStartFolder(const std::string &url);

  /*! \brief Utility method to remove the given parameter from a path/URL
   \param strDirectory Path/URL from which to remove the given parameter
   \param strParameter Parameter to remove from the given path/URL
   \return Path/URL without the given parameter
   */
  static std::string RemoveParameterFromPath(const std::string &strDirectory, const std::string &strParameter);

  void ProcessRenderLoop(bool renderOnly = false);

  XFILE::CVirtualDirectory m_rootDir;
  CGUIViewControl m_viewControl;

  // current path and history
  CFileItemList* m_vecItems;
  CFileItemList* m_unfilteredItems;        ///< \brief items prior to filtering using FilterItems()
  CDirectoryHistory m_history;
  std::unique_ptr<CGUIViewState> m_guiState;

  // save control state on window exit
  int m_iLastControl;
  std::string m_startDirectory;

  CSmartPlaylist m_filter;
  bool m_canFilterAdvanced;
  /*! \brief Contains the path used for filtering (including any active filter)

   When Update() is called with a path to e.g. a smartplaylist or
   a library node filter, that "original" path will be stored in
   m_vecItems->m_strPath. But the path used by XBMC to retrieve
   those items from the database (Videodb:// or musicdb://)
   is stored in this member variable to still have access to it
   because it is used for filtering by appending the currently active
   filter as a "filter" parameter to the filter path/URL.

   \sa Update
   */
  std::string m_strFilterPath;
};
