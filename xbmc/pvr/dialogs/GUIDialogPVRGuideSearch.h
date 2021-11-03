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

#include <map>
#include <memory>
#include <string>

namespace PVR
{
  class CPVREpgSearchFilter;
  class CPVRChannelGroupMember;

  class CGUIDialogPVRGuideSearch : public CGUIDialog
  {
  public:
    CGUIDialogPVRGuideSearch();
    ~CGUIDialogPVRGuideSearch() override = default;
    bool OnMessage(CGUIMessage& message) override;
    void OnWindowLoaded() override;

    void SetFilterData(const std::shared_ptr<CPVREpgSearchFilter>& searchFilter);

    enum class Result
    {
      SEARCH,
      SAVE,
      CANCEL
    };
    Result GetResult() const { return m_result; }

  protected:
    void OnInitWindow() override;

  private:
    void UpdateSearchFilter();
    void UpdateChannelSpin();
    void UpdateGroupsSpin();
    void UpdateGenreSpin();
    void UpdateDurationSpin();
    CDateTime ReadDateTime(const std::string& strDate, const std::string& strTime) const;
    void Update();

    bool IsRadioSelected(int controlID);
    int GetSpinValue(int controlID);
    std::string GetEditValue(int controlID);

    Result m_result = Result::CANCEL;
    std::shared_ptr<CPVREpgSearchFilter> m_searchFilter;
    std::map<int, std::shared_ptr<CPVRChannelGroupMember>> m_channelsMap;

    CDateTime m_startDateTime;
    CDateTime m_endDateTime;
  };
}
