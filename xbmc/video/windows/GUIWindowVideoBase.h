#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "windows/GUIMediaWindow.h"
#include "video/VideoDatabase.h"
#include "PlayListPlayer.h"
#include "video/VideoThumbLoader.h"

class CGUIWindowVideoBase : public CGUIMediaWindow, public IBackgroundLoaderObserver, public IStreamDetailsObserver
{
public:
  CGUIWindowVideoBase(int id, const CStdString &xmlFile);
  virtual ~CGUIWindowVideoBase(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

  void PlayMovie(const CFileItem *item);
  static void GetResumeItemOffset(const CFileItem *item, int& startoffset, int& partNumber);
  static bool HasResumeItemOffset(const CFileItem *item);

  void AddToDatabase(int iItem);
  virtual void OnInfo(CFileItem* pItem, const ADDON::ScraperPtr& scraper);
  virtual void OnStreamDetails(const CStreamDetails &details, const CStdString &strFileName, long lFileId);
  static void MarkWatched(const CFileItemPtr &pItem, bool bMark);
  static void UpdateVideoTitle(const CFileItem* pItem);

  /*! \brief Show dialog allowing selection of wanted playback item */
  static bool ShowPlaySelection(CFileItemPtr& item);
  static bool ShowPlaySelection(CFileItemPtr& item, const CStdString& directory);


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
  static void AppendAndClearSearchItems(CFileItemList &searchItems, const CStdString &prependLabel, CFileItemList &results);

  /*! \brief Prompt the user for assigning content to a path.
   Based on changes, we then call OnUnassignContent, update or refresh scraper information in the database
   and optionally start a scan
   \param path the path to assign content for
   */
  static void OnAssignContent(const CStdString &path);

  /*! \brief checks the database for a resume position and puts together a string
   \param item selected item
   \return string containing the resume position or an empty string if there is no resume position
   */
  static CStdString GetResumeString(const CFileItem &item);

protected:
  void OnScan(const CStdString& strPath, bool scanAll = false);
  virtual void OnInitWindow();
  virtual void UpdateButtons();
  virtual bool Update(const CStdString &strDirectory, bool updateFilterPath = true);
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void OnItemLoaded(CFileItem* pItem) {};
  virtual void OnPrepareFileItems(CFileItemList &items);

  virtual bool CheckFilterAdvanced(CFileItemList &items) const;
  virtual bool CanContainFilter(const CStdString &strDirectory) const;

  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  void GetNonContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual void OnQueueItem(int iItem);
  virtual void OnDeleteItem(CFileItemPtr pItem);
  virtual void OnDeleteItem(int iItem);
  virtual void DoSearch(const CStdString& strSearch, CFileItemList& items) {};
  virtual CStdString GetStartFolder(const CStdString &dir);

  bool OnClick(int iItem);
  bool OnSelect(int iItem);
  /*! \brief react to an Info action on a view item
   \param item the selected item
   \return true if the action is performed, false otherwise
   */
  bool OnInfo(int item);
  /*! \brief perform a given action on a file
   \param item the selected item
   \param action the action to perform
   \return true if the action is performed, false otherwise
   */
  bool OnFileAction(int item, int action);

  void OnRestartItem(int iItem);
  bool OnResumeItem(int iItem);
  void PlayItem(int iItem);
  virtual bool OnPlayMedia(int iItem);
  virtual bool OnPlayAndQueueMedia(const CFileItemPtr &item);
  void LoadPlayList(const CStdString& strPlayList, int iPlayList = PLAYLIST_VIDEO);

  bool ShowIMDB(CFileItem *item, const ADDON::ScraperPtr& content);

  void AddItemToPlayList(const CFileItemPtr &pItem, CFileItemList &queuedItems);

  void OnSearch();
  void OnSearchItemFound(const CFileItem* pSelItem);
  int GetScraperForItem(CFileItem *item, ADDON::ScraperPtr &info, VIDEO::SScanSettings& settings);

  static bool OnUnAssignContent(const CStdString &path, int label1, int label2, int label3);

  bool StackingAvailable(const CFileItemList &items) const;

  bool OnPlayStackPart(int item);

  CGUIDialogProgress* m_dlgProgress;
  CVideoDatabase m_database;

  CVideoThumbLoader m_thumbLoader;
  bool m_stackingAvailable;
};
