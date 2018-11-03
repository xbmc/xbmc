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

  protected:
    void OnInitWindow() override;

  private:
    class InfoControl
    {
    public:
      InfoControl(uint32_t iSpinLabelId, uint32_t iSpinControlId);
      void Init(CGUISpinControl* spin, CGUITextBox* textbox);
      bool Update(const std::string& textboxValue);

    private:
      CGUISpinControl* m_spinControl = nullptr;
      uint32_t m_iSpinLabelId = 0;
      uint32_t m_iSpinControlId = 0;
      CGUITextBox* m_textbox = nullptr;
      bool m_bSpinLabelPresent = false;
      std::string m_textboxValue;
    };

    void InitInfoControls();
    void UpdateInfoControls();

    InfoControl m_InfoNews;
    InfoControl m_InfoNewsLocal;
    InfoControl m_InfoSport;
    InfoControl m_InfoWeather;
    InfoControl m_InfoLottery;
    InfoControl m_InfoStock;
    InfoControl m_InfoOther;
    InfoControl m_InfoCinema;
    InfoControl m_InfoHoroscope;
  };
}
