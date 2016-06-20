#pragma once
/*
 *      Copyright (C) 2012-2015 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guilib/GUIDialog.h"

namespace PVR
{
  class CGUIDialogPVRRadioRDSInfo : public CGUIDialog
  {
  public:
    CGUIDialogPVRRadioRDSInfo(void);
    virtual ~CGUIDialogPVRRadioRDSInfo(void) {}
    virtual bool OnMessage(CGUIMessage& message);
    virtual bool HasListItems() const { return true; };
    virtual CFileItemPtr GetCurrentListItem(int offset = 0);

  protected:
    virtual void OnInitWindow();
    virtual void OnDeinitWindow(int nextWindowID);

    CFileItemPtr m_rdsItem;

  private:
    bool m_InfoPresent;
    bool m_LabelInfoNewsPresent;
    std::string m_LabelInfoNews;

    bool m_LabelInfoNewsLocalPresent;
    std::string m_LabelInfoNewsLocal;

    bool m_LabelInfoWeatherPresent;
    std::string m_LabelInfoWeather;

    bool m_LabelInfoLotteryPresent;
    std::string m_LabelInfoLottery;

    bool m_LabelInfoSportPresent;
    std::string m_LabelInfoSport;

    bool m_LabelInfoStockPresent;
    std::string m_LabelInfoStock;

    bool m_LabelInfoOtherPresent;
    std::string m_LabelInfoOther;

    bool m_LabelInfoCinemaPresent;
    std::string m_LabelInfoCinema;

    bool m_LabelInfoHoroscopePresent;
    std::string m_LabelInfoHoroscope;
  };
}
