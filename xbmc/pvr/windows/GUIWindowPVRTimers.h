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

#include "GUIWindowPVRBase.h"

#include <memory>

class CFileItem;
typedef std::shared_ptr<CFileItem> CFileItemPtr;

namespace PVR
{
  class CGUIWindowPVRTimers : public CGUIWindowPVRBase
  {
  public:
    CGUIWindowPVRTimers(bool bRadio);
    virtual ~CGUIWindowPVRTimers(void) {};

    bool OnMessage(CGUIMessage& message);
    bool OnAction(const CAction &action);
    void GetContextButtons(int itemNumber, CContextButtons &buttons);
    bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
    bool Update(const std::string &strDirectory, bool updateFilterPath = true);
    void UpdateButtons(void);
    void UnregisterObservers(void);
    void ResetObservers(void);

  protected:
    std::string GetDirectoryPath(void);

  private:
    bool ActionDeleteTimer(CFileItem *item);
    bool ActionShowTimer(CFileItem *item);
    bool ShowTimerSettings(CFileItem *item);
    bool ShowNewTimerDialog(void);

    bool OnContextButtonActivate(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonAdd(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonDelete(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonEdit(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonRename(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonInfo(CFileItem *item, CONTEXT_BUTTON button);

    CFileItemPtr m_currentFileItem;
  };
}
