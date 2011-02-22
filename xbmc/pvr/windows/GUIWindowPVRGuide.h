#pragma once

/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "GUIWindowPVRCommon.h"
#include "guilib/GUIEPGGridContainer.h"

class CGUIWindowPVR;

class CGUIWindowPVRGuide : public CGUIWindowPVRCommon
{
  friend class CGUIWindowPVR;

public:
  CGUIWindowPVRGuide(CGUIWindowPVR *parent);
  virtual ~CGUIWindowPVRGuide(void) {};

  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons) const;
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual void OnInitWindow(void);
  virtual void UpdateData(void);

private:
  virtual bool IsSelectedList(CGUIMessage &message) const;
  virtual bool OnClickButton(CGUIMessage &message);
  virtual bool OnClickList(CGUIMessage &message);

  virtual bool OnContextButtonBegin(CFileItem *item, CONTEXT_BUTTON button);
  virtual bool OnContextButtonEnd(CFileItem *item, CONTEXT_BUTTON button);
  virtual bool OnContextButtonInfo(CFileItem *item, CONTEXT_BUTTON button);
  virtual bool OnContextButtonPlay(CFileItem *item, CONTEXT_BUTTON button);
  virtual bool OnContextButtonStartRecord(CFileItem *item, CONTEXT_BUTTON button);
  virtual bool OnContextButtonStopRecord(CFileItem *item, CONTEXT_BUTTON button);

  virtual void UpdateButtons(void);

  int m_iGuideView;
};
