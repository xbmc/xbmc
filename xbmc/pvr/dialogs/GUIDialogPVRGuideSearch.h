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

#include "guilib/GUIDialog.h"
#include "XBDateTime.h"

namespace EPG
{
  struct EpgSearchFilter;
}

namespace PVR
{
  class CGUIDialogPVRGuideSearch : public CGUIDialog
  {
  public:
    CGUIDialogPVRGuideSearch(void);
    virtual ~CGUIDialogPVRGuideSearch(void) {}
    virtual bool OnMessage(CGUIMessage& message);
    virtual void OnWindowLoaded();

    void SetFilterData(EPG::EpgSearchFilter *searchFilter) { m_searchFilter = searchFilter; }
    bool IsConfirmed() const { return m_bConfirmed; }
    bool IsCanceled() const { return m_bCanceled; }
    void OnSearch();

  protected:
    virtual void OnInitWindow();

    void UpdateChannelSpin(void);
    void UpdateGroupsSpin(void);
    void UpdateGenreSpin(void);
    void UpdateDurationSpin(void);
    void ReadDateTime(const std::string &strDate, const std::string &strTime, CDateTime &dateTime) const;
    void Update();

    bool IsRadioSelected(int controlID);
    int  GetSpinValue(int controlID);
    std::string GetEditValue(int controlID);

    bool m_bConfirmed;
    bool m_bCanceled;
    EPG::EpgSearchFilter *m_searchFilter;
  };
}
