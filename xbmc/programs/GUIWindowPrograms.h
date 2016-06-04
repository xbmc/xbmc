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
#include "dialogs/GUIDialogProgress.h"
#include "ThumbLoader.h"

class CGUIWindowPrograms :
      public CGUIMediaWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowPrograms(void);
  virtual ~CGUIWindowPrograms(void);
  virtual bool OnMessage(CGUIMessage& message) override;
  virtual void OnItemInfo(int iItem);
protected:
  virtual void OnItemLoaded(CFileItem* pItem) override {};
  virtual bool Update(const std::string& strDirectory, bool updateFilterPath = true) override;
  virtual bool OnPlayMedia(int iItem);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
  bool OnAddMediaSource() override;
  virtual std::string GetStartFolder(const std::string &dir) override;

  CGUIDialogProgress* m_dlgProgress;

  CProgramThumbLoader m_thumbLoader;
};
