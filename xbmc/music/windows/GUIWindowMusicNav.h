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

#include "GUIWindowMusicBase.h"
#include "utils/Stopwatch.h"

class CFileItemList;

class CGUIWindowMusicNav : public CGUIWindowMusicBase
{
public:

  CGUIWindowMusicNav(void);
  ~CGUIWindowMusicNav(void) override;

  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;
  void FrameMove() override;

protected:
  void OnItemLoaded(CFileItem* pItem) override {};
  // override base class methods
  bool Update(const std::string &strDirectory, bool updateFilterPath = true) override;
  bool GetDirectory(const std::string &strDirectory, CFileItemList &items) override;
  void UpdateButtons() override;
  void PlayItem(int iItem) override;
  void OnWindowLoaded() override;
  void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
  bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
  bool OnClick(int iItem, const std::string &player = "") override;
  std::string GetStartFolder(const std::string &url) override;

  bool GetSongsFromPlayList(const std::string& strPlayList, CFileItemList &items);
  std::string GetQuickpathName(const std::string& strPath) const;

  VECSOURCES m_shares;

  // searching
  void OnSearchUpdate();
  void AddSearchFolder();
  CStopWatch m_searchTimer; ///< Timer to delay a search while more characters are entered
  bool m_searchWithEdit;    ///< Whether the skin supports the new edit control searching
};
