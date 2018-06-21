/*
 *      Copyright (C) 2005-2018 Team Kodi
 *      http://kodi.tv
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

#pragma once

/*!
\file GUIWindowMusicBase.h
\brief
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

  void DoScan(const std::string &strPath, bool bRescan = false);
  void RefreshContent(const std::string& strContent);

  /*! \brief Once a music source is added, store source in library, and prompt
  the user to scan this folder into the library
  \param oldName the original music source name 
  \param source details of the music source (just added or edited)
  */
  static void OnAssignContent(const std::string& oldName, const CMediaSource& source);

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

  virtual void OnScan(int iItem, bool bPromptRescan = false);

  bool CheckFilterAdvanced(CFileItemList &items) const override;
  bool CanContainFilter(const std::string &strDirectory) const override;

  // new methods
  virtual void PlayItem(int iItem);
  bool OnPlayMedia(int iItem, const std::string &player = "") override;

  void RetrieveMusicInfo();
  void OnItemInfo(int iItem);
  void OnItemInfoAll(const std::string strPath, bool refresh = false);
  virtual void OnQueueItem(int iItem);
  enum ALLOW_SELECTION { SELECTION_ALLOWED = 0, SELECTION_AUTO, SELECTION_FORCED };

  void OnRipTrack(int iItem);
  void LoadPlayList(const std::string& strPlayList) override;
  virtual void OnRemoveSource(int iItem);

  typedef std::vector <CFileItem*>::iterator ivecItems; ///< CFileItem* vector Iterator
  CGUIDialogProgress* m_dlgProgress; ///< Progress dialog

  CMusicDatabase m_musicdatabase;
  MUSIC_INFO::CMusicInfoLoader m_musicInfoLoader;

  CMusicThumbLoader m_thumbLoader;
};
