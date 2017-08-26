/*!
\file GUIWindowMusicBase.h
\brief
*/
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

#include <vector>

#include "windows/GUIMediaWindow.h"
#include "music/MusicDatabase.h"
#include "music/infoscanner/MusicInfoScraper.h"
#include "PlayListPlayer.h"
#include "music/MusicInfoLoader.h"
#include "music/MusicThumbLoader.h"

/*!
 \ingroup windows
 \brief The base class for music windows

 CGUIWindowMusicBase is the base class for
 all music windows.
 */
class CGUIWindowMusicBase : public CGUIMediaWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowMusicBase(int id, const std::string &xmlFile);
  ~CGUIWindowMusicBase(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  bool OnBack(int actionID) override;

  void OnItemInfo(CFileItem *pItem, bool bShowInfo = false);

  void DoScan(const std::string &strPath);

  /*! \brief Prompt the user if he wants to start a scan for this folder
  \param path the path to assign content for
  */
  static void OnAssignContent(const std::string &path);
protected:
  void OnInitWindow() override;
  /*!
  \brief Will be called when an popup context menu has been asked for
  \param itemNumber List/thumb control item that has been clicked on
  */
  void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
  void GetNonContextButtons(CContextButtons &buttons);
  bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
  bool OnAddMediaSource() override;
  /*!
  \brief Overwrite to update your gui buttons (visible, enable,...)
  */
  void UpdateButtons() override;

  bool GetDirectory(const std::string &strDirectory, CFileItemList &items) override;
  virtual void OnRetrieveMusicInfo(CFileItemList& items);
  void OnPrepareFileItems(CFileItemList &items) override;
  void AddItemToPlayList(const CFileItemPtr &pItem, CFileItemList &queuedItems);
  void OnRipCD();
  std::string GetStartFolder(const std::string &dir) override;
  void OnItemLoaded(CFileItem* pItem) override {}

  virtual void OnScan(int iItem);

  bool CheckFilterAdvanced(CFileItemList &items) const override;
  bool CanContainFilter(const std::string &strDirectory) const override;

  // new methods
  virtual void PlayItem(int iItem);
  bool OnPlayMedia(int iItem, const std::string &player = "") override;

  void RetrieveMusicInfo();
  void OnItemInfo(int iItem, bool bShowInfo = true);
  void OnItemInfoAll(const std::string strPath, bool refresh = false);
  virtual void OnQueueItem(int iItem);
  enum ALLOW_SELECTION { SELECTION_ALLOWED = 0, SELECTION_AUTO, SELECTION_FORCED };
  bool FindAlbumInfo(const CFileItem* album, MUSIC_GRABBER::CMusicAlbumInfo& albumInfo, ALLOW_SELECTION allowSelection);
  bool FindArtistInfo(const CFileItem* artist, MUSIC_GRABBER::CMusicArtistInfo& artistInfo, ALLOW_SELECTION allowSelection);

  bool ShowAlbumInfo(const CFileItem *pItem, bool bShowInfo = true);
  void ShowArtistInfo(const CFileItem *pItem, bool bShowInfo = true);
  void ShowSongInfo(CFileItem* pItem);
  void UpdateThumb(const CAlbum &album, const std::string &path);

  void OnRipTrack(int iItem);
  void OnSearch();
  void LoadPlayList(const std::string& strPlayList) override;
  virtual void OnRemoveSource(int iItem);

  typedef std::vector <CFileItem*>::iterator ivecItems; ///< CFileItem* vector Iterator
  CGUIDialogProgress* m_dlgProgress; ///< Progress dialog

  CMusicDatabase m_musicdatabase;
  MUSIC_INFO::CMusicInfoLoader m_musicInfoLoader;

  CMusicThumbLoader m_thumbLoader;
};
