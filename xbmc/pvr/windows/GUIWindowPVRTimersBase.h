/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include <memory>

#include "pvr/windows/GUIWindowPVRBase.h"

class CFileItem;
typedef std::shared_ptr<CFileItem> CFileItemPtr;

namespace PVR
{
  class CGUIWindowPVRTimersBase : public CGUIWindowPVRBase
  {
  public:
    CGUIWindowPVRTimersBase(bool bRadio, int id, const std::string &xmlFile);
    ~CGUIWindowPVRTimersBase(void) override;

    bool OnMessage(CGUIMessage& message) override;
    bool OnAction(const CAction &action) override;
    bool Update(const std::string &strDirectory, bool updateFilterPath = true) override;
    void UpdateButtons(void) override;

  private:
    bool ActionShowTimer(const CFileItemPtr &item);

    CFileItemPtr m_currentFileItem;
  };
}
