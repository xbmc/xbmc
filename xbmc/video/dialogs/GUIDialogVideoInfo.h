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

#include "guilib/GUIDialog.h"
#include "FileItem.h"

class CVideoDatabase;

class CGUIDialogVideoInfo :
      public CGUIDialog
{
public:
  CGUIDialogVideoInfo(void);
  virtual ~CGUIDialogVideoInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  void SetMovie(const CFileItem *item);
  bool NeedRefresh() const;
  bool RefreshAll() const;
  bool HasUpdatedThumb() const { return m_hasUpdatedThumb; };

  std::string GetThumbnail() const;
  virtual CFileItemPtr GetCurrentListItem(int offset = 0) { return m_movieItem; }
  const CFileItemList& CurrentDirectory() const { return *m_castList; };
  virtual bool HasListItems() const { return true; };

  static std::string ChooseArtType(const CFileItem &item, std::map<std::string, std::string> &currentArt);
  static void AddItemPathToFileBrowserSources(VECSOURCES &sources, const CFileItem &item);

  static int ManageVideoItem(const CFileItemPtr &item);
  static bool UpdateVideoItemTitle(const CFileItemPtr &pItem);
  static bool CanDeleteVideoItem(const CFileItemPtr &item);
  static bool DeleteVideoItemFromDatabase(const CFileItemPtr &item, bool unavailable = false);
  static bool DeleteVideoItem(const CFileItemPtr &item, bool unavailable = false);

  static bool ManageMovieSets(const CFileItemPtr &item);
  static bool GetMoviesForSet(const CFileItem *setItem, CFileItemList &originalMovies, CFileItemList &selectedMovies);
  static bool GetSetForMovie(const CFileItem *movieItem, CFileItemPtr &selectedSet);
  static bool SetMovieSet(const CFileItem *movieItem, const CFileItem *selectedSet);

  static bool GetItemsForTag(const std::string &strHeading, const std::string &type, CFileItemList &items, int idTag = -1, bool showAll = true);
  static bool AddItemsToTag(const CFileItemPtr &tagItem);
  static bool RemoveItemsFromTag(const CFileItemPtr &tagItem);

  static bool ManageVideoItemArtwork(const CFileItemPtr &item, const MediaType &type);

  static std::string GetLocalizedVideoType(const std::string &strType);
protected:
  virtual void OnInitWindow();
  void Update();
  void SetLabel(int iControl, const std::string& strLabel);

  // link cast to movies
  void ClearCastList();
  void OnSearch(std::string& strSearch);
  void DoSearch(std::string& strSearch, CFileItemList& items);
  void OnSearchItemFound(const CFileItem* pItem);
  void Play(bool resume = false);
  void OnGetArt();
  void OnGetFanart();
  void PlayTrailer();

  static bool UpdateVideoItemSortTitle(const CFileItemPtr &pItem);
  static bool LinkMovieToTvShow(const CFileItemPtr &item, bool bRemove, CVideoDatabase &database);

  /*! \brief Pop up a fanart chooser. Does not utilise remote URLs.
   \param videoItem the item to choose fanart for.
   */
  static bool OnGetFanart(const CFileItemPtr &videoItem);

  CFileItemPtr m_movieItem;
  CFileItemList *m_castList;
  bool m_bViewReview;
  bool m_bRefresh;
  bool m_bRefreshAll;
  bool m_hasUpdatedThumb;
};
