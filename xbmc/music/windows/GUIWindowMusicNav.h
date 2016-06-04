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
  virtual ~CGUIWindowMusicNav(void);

  virtual bool OnMessage(CGUIMessage& message) override;
  virtual bool OnAction(const CAction& action) override;
  virtual void FrameMove() override;

protected:
  virtual void OnItemLoaded(CFileItem* pItem) override {};
  // override base class methods
  virtual bool Update(const std::string &strDirectory, bool updateFilterPath = true) override;
  virtual bool GetDirectory(const std::string &strDirectory, CFileItemList &items) override;
  virtual void UpdateButtons() override;
  virtual void PlayItem(int iItem) override;
  virtual void OnWindowLoaded() override;
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
  virtual bool OnClick(int iItem, const std::string &player = "") override;
  virtual std::string GetStartFolder(const std::string &url) override;

  bool GetSongsFromPlayList(const std::string& strPlayList, CFileItemList &items);
  std::string GetQuickpathName(const std::string& strPath) const;

  VECSOURCES m_shares;

  // searching
  void OnSearchUpdate();
  void AddSearchFolder();
  CStopWatch m_searchTimer; ///< Timer to delay a search while more characters are entered
  bool m_searchWithEdit;    ///< Whether the skin supports the new edit control searching
};
