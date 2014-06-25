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

#include "windows/GUIMediaWindow.h"
#include "music/MusicDatabase.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
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
class CGUIWindowMusicBase : public CGUIMediaWindow
{
public:
  CGUIWindowMusicBase(int id, const std::string &xmlFile);
  virtual ~CGUIWindowMusicBase(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual bool OnBack(int actionID);

  void OnInfo(CFileItem *pItem, bool bShowInfo = false);

protected:
  virtual void OnInitWindow();
  /*!
  \brief Will be called when an popup context menu has been asked for
  \param itemNumber List/thumb control item that has been clicked on
  */
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  void GetNonContextButtons(CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  /*!
  \brief Overwrite to update your gui buttons (visible, enable,...)
  */
  virtual void UpdateButtons();

  virtual bool GetDirectory(const std::string &strDirectory, CFileItemList &items);
  virtual void OnRetrieveMusicInfo(CFileItemList& items);
  void AddItemToPlayList(const CFileItemPtr &pItem, CFileItemList &queuedItems);
  virtual void OnScan(int iItem) {};
  void OnRipCD();
  virtual std::string GetStartFolder(const std::string &dir);

  virtual bool CheckFilterAdvanced(CFileItemList &items) const;
  virtual bool CanContainFilter(const std::string &strDirectory) const;

  // new methods
  virtual void PlayItem(int iItem);
  virtual bool OnPlayMedia(int iItem);

  void RetrieveMusicInfo();
  void OnInfo(int iItem, bool bShowInfo = true);
  void OnInfoAll(int iItem, bool bCurrent=false, bool refresh=false);
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
  virtual void LoadPlayList(const std::string& strPlayList);

  typedef std::vector <CFileItem*>::iterator ivecItems; ///< CFileItem* vector Iterator
  CGUIDialogProgress* m_dlgProgress; ///< Progress dialog

  // member variables to save frequently used CSettings (which is slow)
  bool m_hideExtensions;
  CMusicDatabase m_musicdatabase;
  MUSIC_INFO::CMusicInfoLoader m_musicInfoLoader;
};
