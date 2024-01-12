/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "media/MediaType.h"

#include <memory>
#include <vector>

class CFileItem;
class CFileItemList;
class CMediaSource;
class CVideoDatabase;

class CGUIDialogVideoInfo :
      public CGUIDialog
{
public:
  CGUIDialogVideoInfo(void);
  ~CGUIDialogVideoInfo(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  void SetMovie(const CFileItem *item);
  bool NeedRefresh() const;
  bool RefreshAll() const;
  bool HasUpdatedThumb() const { return m_hasUpdatedThumb; }
  bool HasUpdatedUserrating() const { return m_hasUpdatedUserrating; }
  bool HasUpdatedItems() const { return m_hasUpdatedItems; }

  std::string GetThumbnail() const;
  std::shared_ptr<CFileItem> GetCurrentListItem(int offset = 0) override { return m_movieItem; }
  const CFileItemList& CurrentDirectory() const { return *m_castList; }
  bool HasListItems() const override { return true; }

  static void AddItemPathToFileBrowserSources(std::vector<CMediaSource>& sources,
                                              const CFileItem& item);

  static int ManageVideoItem(const std::shared_ptr<CFileItem>& item);
  static bool UpdateVideoItemTitle(const std::shared_ptr<CFileItem>& pItem);
  static bool CanDeleteVideoItem(const std::shared_ptr<CFileItem>& item);
  static bool DeleteVideoItemFromDatabase(const std::shared_ptr<CFileItem>& item,
                                          bool unavailable = false);
  static bool DeleteVideoItem(const std::shared_ptr<CFileItem>& item, bool unavailable = false);

  static bool ManageMovieSets(const std::shared_ptr<CFileItem>& item);
  static bool GetMoviesForSet(const CFileItem *setItem, CFileItemList &originalMovies, CFileItemList &selectedMovies);
  static bool GetSetForMovie(const CFileItem* movieItem, std::shared_ptr<CFileItem>& selectedSet);
  static bool SetMovieSet(const CFileItem *movieItem, const CFileItem *selectedSet);

  static void ManageVideoVersions(const std::shared_ptr<CFileItem>& item);

  static bool GetItemsForTag(const std::string &strHeading, const std::string &type, CFileItemList &items, int idTag = -1, bool showAll = true);
  static bool AddItemsToTag(const std::shared_ptr<CFileItem>& tagItem);
  static bool RemoveItemsFromTag(const std::shared_ptr<CFileItem>& tagItem);

  static bool ChooseAndManageVideoItemArtwork(const std::shared_ptr<CFileItem>& item);
  static bool ManageVideoItemArtwork(const std::shared_ptr<CFileItem>& item, const MediaType& type);

  static std::string GetLocalizedVideoType(const std::string &strType);

  static void ShowFor(const CFileItem& item);

protected:
  void OnInitWindow() override;
  void Update();
  void SetLabel(int iControl, const std::string& strLabel);
  void SetUserrating(int userrating) const;

  // link cast to movies
  void ClearCastList();
  /**
   * \brief Search the current directory for a string got from the virtual keyboard
   * \param strSearch The search string
   */
  void OnSearch(std::string& strSearch);
  /**
   * \brief Make the actual search for the OnSearch function.
   * \param strSearch The search string
   * \param items Items Found
   */
  void DoSearch(std::string& strSearch, CFileItemList& items) const;
  /**
   * \brief React on the selected search item
   * \param pItem Search result item
   */
  void OnSearchItemFound(const CFileItem* pItem);
  bool OnManageVideoVersions();
  bool OnManageVideoExtras();
  void Play(bool resume = false);
  void OnGetArt();
  void OnGetFanart();
  void OnSetUserrating() const;
  void PlayTrailer();

  static bool UpdateVideoItemSortTitle(const std::shared_ptr<CFileItem>& pItem);
  static bool LinkMovieToTvShow(const std::shared_ptr<CFileItem>& item,
                                bool bRemove,
                                CVideoDatabase& database);

  std::shared_ptr<CFileItem> m_movieItem;
  CFileItemList *m_castList;
  bool m_bViewReview = false;
  bool m_bRefresh = false;
  bool m_bRefreshAll = true;
  bool m_hasUpdatedThumb = false;
  bool m_hasUpdatedUserrating = false;
  int m_startUserrating = -1;
  bool m_hasUpdatedItems{false};

private:
  static bool ManageVideoItemArtwork(const std::shared_ptr<CFileItem>& item,
                                     const MediaType& mediaType,
                                     const std::string& artType);
  bool ChooseVideoVersion();
};
