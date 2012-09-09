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

#include "GUIWindowPVRCommon.h"
#include "utils/Observer.h"

namespace PVR
{
  class CGUIWindowPVR;

  class CGUIWindowPVRTimers : public CGUIWindowPVRCommon, private Observer
  {
    friend class CGUIWindowPVR;

  public:
    CGUIWindowPVRTimers(CGUIWindowPVR *parent);
    virtual ~CGUIWindowPVRTimers(void) {};

    void GetContextButtons(int itemNumber, CContextButtons &buttons) const;
    bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
    void UpdateData(bool bUpdateSelectedFile = true);
    void Notify(const Observable &obs, const ObservableMessage msg);
    void UnregisterObservers(void);
    void ResetObservers(void);

  private:
    bool OnClickButton(CGUIMessage &message);
    bool OnClickList(CGUIMessage &message);

    bool OnContextButtonActivate(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonAdd(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonDelete(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonEdit(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonRename(CFileItem *item, CONTEXT_BUTTON button);
  };
}
