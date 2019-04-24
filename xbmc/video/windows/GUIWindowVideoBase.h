/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "windows/GUIMediaWindow.h"
#include "video/VideoDatabase.h"
#include "PlayListPlayer.h"
#include "video/VideoThumbLoader.h"

enum VideoSelectAction
{
  SELECT_ACTION_CHOOSE          = 0,
  SELECT_ACTION_PLAY_OR_RESUME,
  SELECT_ACTION_RESUME,
  SELECT_ACTION_INFO,
  SELECT_ACTION_MORE,
  SELECT_ACTION_PLAY,
  SELECT_ACTION_PLAYPART,
  SELECT_ACTION_QUEUE
};

class CGUIWindowVideoBase : public CGUIMediaWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowVideoBase(int id, const std::string &xmlFile);
  ~CGUIWindowVideoBase(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;

  void PlayMovie(const CFileItem *item, const std::string &player = "");
  static void GetResumeItemOffset(const CFileItem *item, int64_t& startoffset, int& partNumber);
  static bool HasResumeItemOffset(const CFileItem *item);

  void AddToDatabase(int iItem);
  virtual void OnItemInfo(const CFileItem& fileItem, ADDON::ScraperPtr& scraper);


  /*! \brief Show the resume menu for this item (if it has a resume bookmark)
   If a resume bookmark is found, we set the item's m_lStartOffset to STARTOFFSET_RESUME.
   Note that we do this in favour of setting the resume point, as we need additional
   information from the database (in particular, the playerState) when resuming some items
   (eg ISO/VIDEO_TS).
   \param item item to check for a resume bookmark
   \return true if an option was chosen, false if the resume menu was cancelled.
   */
  static bool ShowResumeMenu(CFileItem &item);

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

  /*! \brief checks the database for a resume position and puts together a string
   \param item selected item
   \return string containing the resume position or an empty string if there is no resume position
   */
  static std::string GetResumeString(const CFileItem &item);

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
  virtual void OnDeleteItem(CFileItemPtr pItem);
  void OnDeleteItem(int iItem) override;
  virtual void DoSearch(const std::string& strSearch, CFileItemList& items) {};
  std::string GetStartFolder(const std::string &dir) override;

  bool OnClick(int iItem, const std::string &player = "") override;
  bool OnSelect(int iItem) override;
  /*! \brief react to an Info action on a view item
   \param item the selected item
   \return true if the action is performed, false otherwise
   */
  bool OnItemInfo(int item);
  /*! \brief perform a given action on a file
   \param item the selected item
   \param action the action to perform
   \return true if the action is performed, false otherwise
   */
  bool OnFileAction(int item, int action, std::string player);

  void OnRestartItem(int iItem, const std::string &player = "");
  bool OnResumeItem(int iItem, const std::string &player = "");
  void PlayItem(int iItem, const std::string &player = "");
  bool OnPlayMedia(int iItem, const std::string &player = "") override;
  bool OnPlayAndQueueMedia(const CFileItemPtr &item, std::string player = "") override;
  using CGUIMediaWindow::LoadPlayList;
  void LoadPlayList(const std::string& strPlayList, int iPlayList = PLAYLIST_VIDEO);

  bool ShowIMDB(CFileItemPtr item, const ADDON::ScraperPtr& content, bool fromDB);

  void AddItemToPlayList(const CFileItemPtr &pItem, CFileItemList &queuedItems);

  void OnSearch();
  void OnSearchItemFound(const CFileItem* pSelItem);
  int GetScraperForItem(CFileItem *item, ADDON::ScraperPtr &info, VIDEO::SScanSettings& settings);

  static bool OnUnAssignContent(const std::string &path, int header, int text);

  static bool StackingAvailable(const CFileItemList &items);

  bool OnPlayStackPart(int item);

  CGUIDialogProgress* m_dlgProgress;
  CVideoDatabase m_database;

  CVideoThumbLoader m_thumbLoader;
  bool m_stackingAvailable;
};
