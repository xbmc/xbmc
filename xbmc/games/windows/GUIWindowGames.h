/*
 *      Copyright (C) 2012-2012 Team XBMC
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
#pragma once

#include "windows/GUIMediaWindow.h"

class CGUIDialogProgress;

class CGUIWindowGames : public CGUIMediaWindow
{
public:
  CGUIWindowGames();
  virtual ~CGUIWindowGames() { }
  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual void SetupShares();
  virtual bool OnClick(int itemNumber);
  void OnInfo(int itemNumber);
  bool PlayGame(const CFileItem &item);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual CStdString GetStartFolder(const CStdString &dir);

  CGUIDialogProgress *m_dlgProgress;
};
