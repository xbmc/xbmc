/*!
\file GUIDialog.h
\brief
*/

#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "GUIWindow.h"

/*!
 \ingroup winmsg
 \brief
 */
class CGUIDialog :
      public CGUIWindow
{
public:
  CGUIDialog(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUIDialog(void);

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();

  void DoModal(int iWindowID = WINDOW_INVALID, const CStdString &param = ""); // modal
  void Show(); // modeless

  virtual void Close(bool forceClose = false);
  virtual bool IsDialogRunning() const { return m_bRunning; };
  virtual bool IsDialog() const { return true;};
  virtual bool IsModalDialog() const { return m_bModal; };

  virtual bool IsAnimating(ANIMATION_TYPE animType);

  void SetAutoClose(unsigned int timeoutMs);
protected:
  virtual bool RenderAnimation(DWORD time);
  virtual void SetDefaults();
  virtual void OnWindowLoaded();

  friend class CApplicationMessenger;
  void DoModal_Internal(int iWindowID = WINDOW_INVALID, const CStdString &param = ""); // modal
  void Show_Internal(); // modeless

  bool m_bRunning;
  bool m_bModal;
  bool m_dialogClosing;
  bool m_autoClosing;
  DWORD m_showStartTime;
  DWORD m_showDuration;
};
