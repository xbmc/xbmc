/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRRadioRDSInfo.h"

#include "Application.h"
#include "FileItem.h"
#include "GUIUserMessages.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUISpinControl.h"
#include "guilib/GUITextBox.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"

#include "pvr/channels/PVRRadioRDSInfoTag.h"

using namespace PVR;

#define CONTROL_BTN_OK    10
#define SPIN_CONTROL_INFO 21
#define TEXT_INFO         22
#define CONTROL_NEXT_PAGE 60
#define CONTROL_INFO_LIST 70

#define INFO_NEWS         1
#define INFO_NEWS_LOCAL   2
#define INFO_SPORT        3
#define INFO_WEATHER      4
#define INFO_LOTTERY      5
#define INFO_STOCK        6
#define INFO_OTHER        7
#define INFO_CINEMA       8
#define INFO_HOROSCOPE    9

CGUIDialogPVRRadioRDSInfo::CGUIDialogPVRRadioRDSInfo(void)
  : CGUIDialog(WINDOW_DIALOG_PVR_RADIO_RDS_INFO, "DialogPVRRadioRDSInfo.xml")
  , m_rdsItem(new CFileItem)
{
}

bool CGUIDialogPVRRadioRDSInfo::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    int iControl = message.GetSenderId();

    if (iControl == CONTROL_BTN_OK)
    {
      Close();
      return true;
    }
    else if (iControl == SPIN_CONTROL_INFO)
    {
      const CPVRRadioRDSInfoTagPtr currentRDS = g_application.CurrentFileItem().GetPVRRadioRDSInfoTag();
      if (!currentRDS)
        return false;

      const CGUISpinControl *spin = static_cast<CGUISpinControl*>(GetControl(SPIN_CONTROL_INFO));
      if (!spin)
        return false;

      CGUITextBox *textbox = static_cast<CGUITextBox*>(GetControl(TEXT_INFO));
      if (!textbox)
        return false;

      switch (spin->GetValue())
      {
        case INFO_NEWS:
          textbox->SetInfo(currentRDS->GetInfoNews());
          break;
        case INFO_NEWS_LOCAL:
          textbox->SetInfo(currentRDS->GetInfoNewsLocal());
          break;
        case INFO_SPORT:
          textbox->SetInfo(currentRDS->GetInfoSport());
          break;
        case INFO_WEATHER:
          textbox->SetInfo(currentRDS->GetInfoWeather());
          break;
        case INFO_LOTTERY:
          textbox->SetInfo(currentRDS->GetInfoLottery());
          break;
        case INFO_STOCK:
          textbox->SetInfo(currentRDS->GetInfoStock());
          break;
        case INFO_OTHER:
          textbox->SetInfo(currentRDS->GetInfoOther());
          break;
        case INFO_CINEMA:
          textbox->SetInfo(currentRDS->GetInfoCinema());
          break;
        case INFO_HOROSCOPE:
          textbox->SetInfo(currentRDS->GetInfoHoroscope());
          break;
      }

      SET_CONTROL_VISIBLE(CONTROL_INFO_LIST);
    }
  }
  else if (message.GetMessage() == GUI_MSG_NOTIFY_ALL)
  {
    CGUISpinControl *spin = static_cast<CGUISpinControl*>(GetControl(SPIN_CONTROL_INFO));
    CGUITextBox *textbox = static_cast<CGUITextBox*>(GetControl(TEXT_INFO));

    if (IsActive() && spin && textbox && message.GetParam1() == GUI_MSG_UPDATE_RADIOTEXT &&
        g_application.GetAppPlayer().IsPlaying() &&
        g_application.CurrentFileItem().HasPVRRadioRDSInfoTag())
    {
      const CPVRRadioRDSInfoTagPtr currentRDS = g_application.CurrentFileItem().GetPVRRadioRDSInfoTag();

      UpdateControls(spin, 29916, INFO_NEWS, m_LabelInfoNewsPresent, textbox, currentRDS->GetInfoNews(), m_LabelInfoNews);
      UpdateControls(spin, 29917, INFO_NEWS_LOCAL, m_LabelInfoNewsLocalPresent, textbox, currentRDS->GetInfoNewsLocal(), m_LabelInfoNewsLocal);
      UpdateControls(spin, 29918, INFO_SPORT, m_LabelInfoSportPresent, textbox, currentRDS->GetInfoSport(), m_LabelInfoSport);
      UpdateControls(spin,   400, INFO_WEATHER, m_LabelInfoWeatherPresent, textbox, currentRDS->GetInfoWeather(), m_LabelInfoWeather);
      UpdateControls(spin, 29919, INFO_LOTTERY, m_LabelInfoLotteryPresent, textbox, currentRDS->GetInfoLottery(), m_LabelInfoLottery);
      UpdateControls(spin, 29920, INFO_STOCK, m_LabelInfoStockPresent, textbox, currentRDS->GetInfoStock(), m_LabelInfoStock);
      UpdateControls(spin, 29921, INFO_OTHER, m_LabelInfoOtherPresent, textbox, currentRDS->GetInfoOther(), m_LabelInfoOther);
      UpdateControls(spin, 19602, INFO_CINEMA, m_LabelInfoCinemaPresent, textbox, currentRDS->GetInfoCinema(), m_LabelInfoCinema);
      UpdateControls(spin, 29922, INFO_HOROSCOPE, m_LabelInfoHoroscopePresent, textbox, currentRDS->GetInfoHoroscope(), m_LabelInfoHoroscope);

      if (m_InfoPresent)
        SET_CONTROL_VISIBLE(CONTROL_INFO_LIST);
      else
        SET_CONTROL_HIDDEN(CONTROL_INFO_LIST);
    }
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogPVRRadioRDSInfo::InitControls(CGUISpinControl* spin, uint32_t iSpinLabelId, uint32_t iSpinControlId, bool& bSpinLabelPresent,
                                             CGUITextBox* textbox, const std::string& textboxValue)
{
  // if there is a text for the given spinner item...
  if (!textboxValue.empty())
  {
    // add a new spinner item to the control...
    spin->AddLabel(g_localizeStrings.Get(iSpinLabelId), iSpinControlId);
    bSpinLabelPresent = true;

    // and if it was the first item, fill the textbox and select the spinner item.
    if (!m_InfoPresent)
    {
      textbox->SetInfo(textboxValue);
      spin->SetValue(iSpinControlId);
      m_InfoPresent = true;
    }
  }
}

void CGUIDialogPVRRadioRDSInfo::UpdateControls(CGUISpinControl* spin, uint32_t iSpinLabelId, uint32_t iSpinControlId, bool& bSpinLabelPresent,
                                               CGUITextBox* textbox, const std::string& textboxNewValue, std::string& textboxCurrentValue)
{
  if (!textboxNewValue.empty())
  {
    if (!bSpinLabelPresent)
    {
      spin->AddLabel(g_localizeStrings.Get(iSpinLabelId), iSpinControlId);
      bSpinLabelPresent = true;
    }

    if (textboxCurrentValue != textboxNewValue)
    {
      spin->SetValue(iSpinControlId);
      textboxCurrentValue = textboxNewValue;
      textbox->SetInfo(textboxNewValue);
      m_InfoPresent = true;
    }
  }
}

void CGUIDialogPVRRadioRDSInfo::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  m_LabelInfoNewsPresent      = false;
  m_LabelInfoNewsLocalPresent = false;
  m_LabelInfoSportPresent     = false;
  m_LabelInfoWeatherPresent   = false;
  m_LabelInfoLotteryPresent   = false;
  m_LabelInfoStockPresent     = false;
  m_LabelInfoOtherPresent     = false;
  m_LabelInfoHoroscopePresent = false;
  m_LabelInfoCinemaPresent    = false;
  m_InfoPresent               = false;

  SET_CONTROL_HIDDEN(CONTROL_INFO_LIST);

  const CPVRRadioRDSInfoTagPtr currentRDS = g_application.CurrentFileItem().GetPVRRadioRDSInfoTag();
  if (!currentRDS)
    return;

  CGUISpinControl *spin = static_cast<CGUISpinControl*>(GetControl(SPIN_CONTROL_INFO));
  if (!spin)
    return; // not an error; not every skin must implement this.

  spin->Clear();

  CGUITextBox *textbox = static_cast<CGUITextBox*>(GetControl(TEXT_INFO));
  if (!textbox)
    return; // not an error; not every skin must implement this.

  InitControls(spin, 29916, INFO_NEWS, m_LabelInfoNewsPresent, textbox, currentRDS->GetInfoNews());
  InitControls(spin, 29917, INFO_NEWS_LOCAL, m_LabelInfoNewsLocalPresent, textbox, currentRDS->GetInfoNewsLocal());
  InitControls(spin, 29918, INFO_SPORT, m_LabelInfoSportPresent, textbox, currentRDS->GetInfoSport());
  InitControls(spin,   400, INFO_WEATHER, m_LabelInfoWeatherPresent, textbox, currentRDS->GetInfoWeather());
  InitControls(spin, 29919, INFO_LOTTERY, m_LabelInfoLotteryPresent, textbox, currentRDS->GetInfoLottery());
  InitControls(spin, 29920, INFO_STOCK, m_LabelInfoStockPresent, textbox, currentRDS->GetInfoStock());
  InitControls(spin, 29921, INFO_OTHER, m_LabelInfoOtherPresent, textbox, currentRDS->GetInfoOther());
  InitControls(spin, 19602, INFO_CINEMA, m_LabelInfoCinemaPresent, textbox, currentRDS->GetInfoCinema());
  InitControls(spin, 29922, INFO_HOROSCOPE, m_LabelInfoHoroscopePresent, textbox, currentRDS->GetInfoHoroscope());

  if (m_InfoPresent)
    SET_CONTROL_VISIBLE(CONTROL_INFO_LIST);
}

void CGUIDialogPVRRadioRDSInfo::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

CFileItemPtr CGUIDialogPVRRadioRDSInfo::GetCurrentListItem(int offset)
{
  return m_rdsItem;
}
