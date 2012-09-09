#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
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

#include "guilib/GUIDialog.h"

namespace EPG
{
  class CEpgInfoTag;
}

namespace PVR
{
  class CPVRTimerInfoTag;

  class CGUIDialogPVRGuideInfo : public CGUIDialog
  {
  public:
    CGUIDialogPVRGuideInfo(void);
    virtual ~CGUIDialogPVRGuideInfo(void);
    virtual bool OnMessage(CGUIMessage& message);
    virtual bool HasListItems() const { return true; };
    virtual CFileItemPtr GetCurrentListItem(int offset = 0);

    void SetProgInfo(const CFileItem *item);

  protected:
    void Update();
    bool ActionStartTimer(const EPG::CEpgInfoTag *tag);
    bool ActionCancelTimer(CFileItemPtr timer);

    bool OnClickButtonOK(CGUIMessage &message);
    bool OnClickButtonRecord(CGUIMessage &message);
    bool OnClickButtonSwitch(CGUIMessage &message);

    CFileItemPtr m_progItem;
  };
}
