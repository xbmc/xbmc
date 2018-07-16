/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
