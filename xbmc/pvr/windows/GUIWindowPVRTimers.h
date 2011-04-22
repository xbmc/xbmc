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

namespace PVR
{
  class CGUIWindowPVR;

  class CGUIWindowPVRTimers : public CGUIWindowPVRCommon
  {
    friend class CGUIWindowPVR;

  public:
    CGUIWindowPVRTimers(CGUIWindowPVR *parent);
    virtual ~CGUIWindowPVRTimers(void) {};

    virtual void GetContextButtons(int itemNumber, CContextButtons &buttons) const;
    virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
    virtual void UpdateData(void);

  private:
    virtual bool OnClickButton(CGUIMessage &message);
    virtual bool OnClickList(CGUIMessage &message);

    virtual bool OnContextButtonActivate(CFileItem *item, CONTEXT_BUTTON button);
    virtual bool OnContextButtonAdd(CFileItem *item, CONTEXT_BUTTON button);
    virtual bool OnContextButtonDelete(CFileItem *item, CONTEXT_BUTTON button);
    virtual bool OnContextButtonEdit(CFileItem *item, CONTEXT_BUTTON button);
    virtual bool OnContextButtonRename(CFileItem *item, CONTEXT_BUTTON button);
  };
}
