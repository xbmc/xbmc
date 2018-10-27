/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

class CGUISpinControl;
class CGUITextBox;

namespace PVR
{
  class CGUIDialogPVRRadioRDSInfo : public CGUIDialog
  {
  public:
    CGUIDialogPVRRadioRDSInfo(void);
    ~CGUIDialogPVRRadioRDSInfo(void) override = default;
    bool OnMessage(CGUIMessage& message) override;
    bool HasListItems() const override { return true; }
    CFileItemPtr GetCurrentListItem(int offset = 0) override;

  protected:
    void OnInitWindow() override;
    void OnDeinitWindow(int nextWindowID) override;

  private:
    void InitControls(CGUISpinControl* spin, uint32_t iSpinLabelId, uint32_t iSpinControlId, bool& bSpinLabelPresent,
                      CGUITextBox* textbox, const std::string& textboxValue);
    void UpdateControls(CGUISpinControl* spin, uint32_t iSpinLabelId, uint32_t iSpinControlId, bool& bSpinLabelPresent,
                        CGUITextBox* textbox, const std::string& textboxNewValue, std::string& textboxCurrentValue);

    CFileItemPtr m_rdsItem;

    bool m_InfoPresent = false;
    bool m_LabelInfoNewsPresent = false;
    std::string m_LabelInfoNews;

    bool m_LabelInfoNewsLocalPresent = false;
    std::string m_LabelInfoNewsLocal;

    bool m_LabelInfoWeatherPresent = false;
    std::string m_LabelInfoWeather;

    bool m_LabelInfoLotteryPresent = false;
    std::string m_LabelInfoLottery;

    bool m_LabelInfoSportPresent = false;
    std::string m_LabelInfoSport;

    bool m_LabelInfoStockPresent = false;
    std::string m_LabelInfoStock;

    bool m_LabelInfoOtherPresent = false;
    std::string m_LabelInfoOther;

    bool m_LabelInfoCinemaPresent = false;
    std::string m_LabelInfoCinema;

    bool m_LabelInfoHoroscopePresent = false;
    std::string m_LabelInfoHoroscope;
  };
}
