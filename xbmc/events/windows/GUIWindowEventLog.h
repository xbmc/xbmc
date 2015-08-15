#pragma once
/*
 *      Copyright (C) 2015 Team Kodi
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

#include "windows/GUIMediaWindow.h"

class CGUIWindowEventLog : public CGUIMediaWindow
{
public:
  CGUIWindowEventLog();
  virtual ~CGUIWindowEventLog();

  // specialization of CGUIControl
  virtual bool OnMessage(CGUIMessage& message);

protected:
  // specialization of CGUIMediaWindow
  virtual bool OnSelect(int item);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual void UpdateButtons();
  virtual bool GetDirectory(const std::string &strDirectory, CFileItemList &items);
  virtual std::string GetStartFolder(const std::string &dir);

  bool OnSelect(CFileItemPtr item);
  bool OnDelete(CFileItemPtr item);
  bool OnExecute(CFileItemPtr item);

  void OnEventAdded(CFileItemPtr item);
  void OnEventRemoved(CFileItemPtr item);
};
