/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "guilib/GUIDialog.h"
#include "pvr/channels/PVRChannelNumber.h"

#include <map>
#include <string>

namespace PVR
{
  class CPVREpgSearchFilter;

  class CGUIDialogPVRGuideSearch : public CGUIDialog
  {
  public:
    CGUIDialogPVRGuideSearch();
    ~CGUIDialogPVRGuideSearch() override = default;
    bool OnMessage(CGUIMessage& message) override;
    void OnWindowLoaded() override;

    void SetFilterData(CPVREpgSearchFilter* searchFilter) { m_searchFilter = searchFilter; }
    bool IsConfirmed() const { return m_bConfirmed; }
    bool IsCanceled() const { return m_bCanceled; }

  protected:
    void OnInitWindow() override;

  private:
    void OnSearch();
    void UpdateChannelSpin();
    void UpdateGroupsSpin();
    void UpdateGenreSpin();
    void UpdateDurationSpin();
    CDateTime ReadDateTime(const std::string& strDate, const std::string& strTime) const;
    void Update();

    bool IsRadioSelected(int controlID);
    int GetSpinValue(int controlID);
    std::string GetEditValue(int controlID);

    bool m_bConfirmed = false;
    bool m_bCanceled = false;
    CPVREpgSearchFilter* m_searchFilter;
    std::map<int, CPVRChannelNumber> m_channelNumbersMap;
  };
}
