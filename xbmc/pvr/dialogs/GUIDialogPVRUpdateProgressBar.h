#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "guilib/GUIDialog.h"

class CGUIDialogPVRUpdateProgressBar : public CGUIDialog
{
public:
  CGUIDialogPVRUpdateProgressBar(void);
  virtual ~CGUIDialogPVRUpdateProgressBar(void) {}
  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();
  void SetProgress(int currentItem, int itemCount);
  void SetHeader(const CStdString& strHeader);
  void SetTitle(const CStdString& strTitle);
  void UpdateState();

protected:
  CStdString        m_strTitle;
  CStdString        m_strHeader;
  CCriticalSection  m_critical;
  float             m_fPercentDone;
  int               m_currentItem;
  int               m_itemCount;
};
