#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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
#include "ContactThumbLoader.h"

class CGUIDialogProgress;

class CGUIWindowContacts : public CGUIMediaWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowContacts(void);
  virtual ~CGUIWindowContacts(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnInitWindow();
  void DoScan(const CStdString &strPath);

protected:
  virtual void OnScan(int iItem);
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList& items);
  virtual void OnInfo(int item);
  virtual bool OnClick(int iItem);
  virtual void UpdateButtons();
  virtual void OnPrepareFileItems(CFileItemList& items);
  virtual bool Update(const CStdString &strDirectory, bool updateFilterPath = true);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual CStdString GetStartFolder(const CStdString &dir);

  void OnRegenerateThumbs();
  virtual bool OnPlayMedia(int iItem);
  bool ShowContact(int iItem, bool startSlideShow);
  void OnShowContactRecursive(const CStdString& strPath);
  void OnSlideShow(const CStdString& strContact);
  void OnSlideShow();
  void OnSlideShowRecursive(const CStdString& strContact);
  void OnSlideShowRecursive();
  virtual void OnItemLoaded(CFileItem* pItem);
  virtual void LoadPlayList(const CStdString& strPlayList);

  CGUIDialogProgress* m_dlgProgress;

  CContactThumbLoader m_thumbLoader;
  bool m_slideShowStarted;
};
