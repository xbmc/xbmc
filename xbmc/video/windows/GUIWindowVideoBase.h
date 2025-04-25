/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "playlists/PlayListTypes.h"
#include "video/VideoDatabase.h"
#include "video/VideoThumbLoader.h"
#include "video/guilib/VideoAction.h"
#include "windows/GUIMediaWindow.h"

class CGUIWindowVideoBase : public CGUIMediaWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowVideoBase(int id, const std::string &xmlFile);
  ~CGUIWindowVideoBase(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  bool OnPopupMenu(int iItem) override;

  /*! \brief Gets called to process the "info" action for the given file item
   Default implementation shows a dialog containing information for the movie/episode/...
   represented by the file item.
   \param fileItem the item for which information is to be presented.
   \return true if information was presented, false otherwise.
   */
  bool OnItemInfo(const CFileItem& fileItem);

  /*! \brief Append a set of search items to a results list using a specific prepend label
   Sorts the search items first, then appends with the given prependLabel to the results list.
   Then empty the search item list so it can be refilled.
   \param searchItems The search items to append.
   \param prependLabel the label that should be prepended to all search results.
   \param results the fileitemlist to append the search results to.
   \sa DoSearch
   */
  static void AppendAndClearSearchItems(CFileItemList &searchItems, const std::string &prependLabel, CFileItemList &results);

  /*! \brief Prompt the user for assigning content to a path.
   Based on changes, we then call OnUnassignContent, update or refresh scraper information in the database
   and optionally start a scan
   \param path the path to assign content for
   */
  static void OnAssignContent(const std::string &path);

  /*! \brief Load video information from the database for these items (public static version)
   Useful for grabbing information for file listings, from watched status to full metadata
   \param items the items to load information for.
   \param database open database object to retrieve the data from
   \param allowReplaceLabels allow label replacement if according GUI setting is enabled
   */
  static void LoadVideoInfo(CFileItemList& items,
                            CVideoDatabase& database,
                            bool allowReplaceLabels = true);

  bool PlayItem(const std::shared_ptr<CFileItem>& item, const std::string& player);
  void OnQueueItem(const std::shared_ptr<CFileItem>& item, int iItem, bool first = false);

protected:
  void OnScan(const std::string& strPath, bool scanAll = false);
  bool Update(const std::string &strDirectory, bool updateFilterPath = true) override;
  bool GetDirectory(const std::string &strDirectory, CFileItemList &items) override;
  void OnItemLoaded(CFileItem* pItem) override {};
  void GetGroupedItems(CFileItemList &items) override;

  bool CheckFilterAdvanced(CFileItemList &items) const override;
  bool CanContainFilter(const std::string &strDirectory) const override;

  void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
  bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
  virtual void OnQueueItem(int iItem, bool first = false);
  virtual void OnDeleteItem(const CFileItemPtr& pItem);
  void OnDeleteItem(int iItem) override;
  virtual void DoSearch(const std::string& strSearch, CFileItemList& items) {}
  std::string GetStartFolder(const std::string &dir) override;

  bool OnSelect(int iItem) override;
  /*! \brief react to an Info action on a view item
   \param item the selected item
   \return true if the action is performed, false otherwise
   */
  bool OnItemInfo(int item);
  void OnRestartItem(int iItem, const std::string &player = "");
  bool OnPlayOrResumeItem(int iItem, const std::string& player = "");
  bool OnPlayMedia(int iItem, const std::string &player = "") override;
  bool OnPlayMedia(const std::shared_ptr<CFileItem>& item, const std::string& player);
  bool OnPlayAndQueueMedia(const CFileItemPtr& item, const std::string& player = "") override;
  using CGUIMediaWindow::LoadPlayList;
  void LoadPlayList(const std::string& strPlayList,
                    KODI::PLAYLIST::Id playlistId = KODI::PLAYLIST::Id::TYPE_VIDEO);

  /*!
   \brief Lookup the information of an item and display an Info dialog
   If item has changed then refresh the active underlying list
   \param item the item to lookup
   \param content
   \return true: the information of the item was modified. false: no change.
   */
  bool ShowInfoAndRefresh(const CFileItemPtr& item, const ADDON::ScraperPtr& content);

  void OnSearch();
  void OnSearchItemFound(const CFileItem* pSelItem);
  int GetScraperForItem(CFileItem* item,
                        ADDON::ScraperPtr& info,
                        KODI::VIDEO::SScanSettings& settings);

  static bool OnUnAssignContent(const std::string &path, int header, int text);

  static bool StackingAvailable(const CFileItemList &items);

  void UpdateVideoVersionItems();
  void UpdateVideoVersionItemsLabel(const std::string& directory);

  CGUIDialogProgress* m_dlgProgress;
  CVideoDatabase m_database;

  CVideoThumbLoader m_thumbLoader;
  bool m_stackingAvailable;

private:
  /*!
   \brief Lookup the information of an item and display an Info dialog
   \param item the item to lookup
   \param content
   \return true: the information of the item was modified. false: no change.
   */
  bool ShowInfo(const CFileItemPtr& item, const ADDON::ScraperPtr& content);

  bool m_forceSelection;
};
