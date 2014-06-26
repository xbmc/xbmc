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

#include "guilib/GUIDialog.h"
#include "view/GUIViewControl.h"
#include "utils/Stopwatch.h"

class CFileItemList;

class CGUIWindowLoginScreen : public CGUIWindow
{
public:
  CGUIWindowLoginScreen(void);
  virtual ~CGUIWindowLoginScreen(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual bool OnBack(int actionID);
  virtual void FrameMove();
  virtual bool HasListItems() const { return true; };
  virtual CFileItemPtr GetCurrentListItem(int offset = 0);
  int GetViewContainerID() const { return m_viewControl.GetCurrentControl(); };
  static void LoadProfile(unsigned int profile);

protected:
  virtual void OnInitWindow();
  virtual void OnWindowLoaded();
  virtual void OnWindowUnload();
  void Update();
  void SetLabel(int iControl, const std::string& strLabel);

  bool OnPopupMenu(int iItem);
  CGUIViewControl m_viewControl;
  CFileItemList* m_vecItems;

  int m_iSelectedItem;
  CStopWatch watch;
};
