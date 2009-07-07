#pragma once
/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIDialog.h"
#include "GUITeletextBox.h"
#include "GUIViewControl.h"
#include "utils/TeletextRender.h"

class CGUIDialogTVTeletext : public CGUIDialog
                           , public cRenderTTPage
                           , private CThread 
{
public:
  CGUIDialogTVTeletext(void);
  virtual ~CGUIDialogTVTeletext(void);
  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual bool OnAction(const CAction& action);
  virtual void OnWindowLoaded();
  virtual void OnWindowUnload();
  void Clear();
  void Update();
  virtual void Process();
    
private:
  enum Direction { DirectionForward, DirectionBackward };
  void SetNumber(int i);
  bool CheckFirstSubPage(int startWith);
  void SetPreviousPage(int oldPage, int oldSubPage, int newPage);
  int nextValidPageNumber(int start, Direction direction);
  void ShowPageNumber();
  void ShowPage();
  void DrawDisplay();
  void ChangePageRelative(Direction direction);
  void ChangeSubPageRelative(Direction direction);
  bool CheckPage();

  int m_currentPage;
  int m_currentSubPage;
  int m_cursorPos;
  int m_previousPage;
  int m_previousSubPage;
  int m_pageBeforeNumberInput;
  int m_CurrentChannel;
  int m_TeletextSupported;
  bool m_pageFound;
  bool m_Boxed;     // Page is 'boxed mode' transparent
  bool m_Blinked;   // Blinking text internal state
  bool m_Concealed; // Hidden text internal state
  DWORD m_updateStart;

};
