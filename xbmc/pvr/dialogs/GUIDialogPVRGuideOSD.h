#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#ifndef DIALOGS_GUILIB_GUIDIALOG_H_INCLUDED
#define DIALOGS_GUILIB_GUIDIALOG_H_INCLUDED
#include "guilib/GUIDialog.h"
#endif

#ifndef DIALOGS_VIEW_GUIVIEWCONTROL_H_INCLUDED
#define DIALOGS_VIEW_GUIVIEWCONTROL_H_INCLUDED
#include "view/GUIViewControl.h"
#endif


class CFileItemList;

namespace PVR
{
  class CGUIDialogPVRGuideOSD : public CGUIDialog
  {
  public:
    CGUIDialogPVRGuideOSD(void);
    virtual ~CGUIDialogPVRGuideOSD(void);
    virtual bool OnMessage(CGUIMessage& message);
    virtual void OnWindowLoaded();
    virtual void OnWindowUnload();

  protected:
    virtual void OnInitWindow();
    virtual void OnDeinitWindow(int nextWindowID);

    void ShowInfo(int iItem);
    void Clear();

    CGUIControl *GetFirstFocusableControl(int id);

    CFileItemList    *m_vecItems;
    CGUIViewControl   m_viewControl;
  };
}
