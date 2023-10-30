/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRRadioRDSInfo.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUISpinControl.h"
#include "guilib/GUITextBox.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/channels/PVRChannel.h"
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

CGUIDialogPVRRadioRDSInfo::CGUIDialogPVRRadioRDSInfo()
  : CGUIDialog(WINDOW_DIALOG_PVR_RADIO_RDS_INFO, "DialogPVRRadioRDSInfo.xml")
  , m_InfoNews(29916, INFO_NEWS)
  , m_InfoNewsLocal(29917, INFO_NEWS_LOCAL)
  , m_InfoSport(29918, INFO_SPORT)
  , m_InfoWeather(400, INFO_WEATHER)
  , m_InfoLottery(29919, INFO_LOTTERY)
  , m_InfoStock(29920, INFO_STOCK)
  , m_InfoOther(29921, INFO_OTHER)
  , m_InfoCinema(19602, INFO_CINEMA)
  , m_InfoHoroscope(29922, INFO_HOROSCOPE)
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
      const std::shared_ptr<const CPVRChannel> channel =
          CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();
      if (!channel)
        return false;

      const std::shared_ptr<const CPVRRadioRDSInfoTag> currentRDS = channel->GetRadioRDSInfoTag();
      if (!currentRDS)
        return false;

      const CGUISpinControl* spin = static_cast<CGUISpinControl*>(GetControl(SPIN_CONTROL_INFO));
      if (!spin)
        return false;

      CGUITextBox* textbox = static_cast<CGUITextBox*>(GetControl(TEXT_INFO));
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
    if (message.GetParam1() == GUI_MSG_UPDATE_RADIOTEXT && IsActive())
    {
      UpdateInfoControls();
    }
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogPVRRadioRDSInfo::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  InitInfoControls();
}

void CGUIDialogPVRRadioRDSInfo::InitInfoControls()
{
  SET_CONTROL_HIDDEN(CONTROL_INFO_LIST);

  CGUISpinControl* spin = static_cast<CGUISpinControl*>(GetControl(SPIN_CONTROL_INFO));
  if (spin)
    spin->Clear();

  CGUITextBox* textbox = static_cast<CGUITextBox*>(GetControl(TEXT_INFO));

  m_InfoNews.Init(spin, textbox);
  m_InfoNewsLocal.Init(spin, textbox);
  m_InfoSport.Init(spin, textbox);
  m_InfoWeather.Init(spin, textbox);
  m_InfoLottery.Init(spin, textbox);
  m_InfoStock.Init(spin, textbox);
  m_InfoOther.Init(spin, textbox);
  m_InfoCinema.Init(spin, textbox);
  m_InfoHoroscope.Init(spin, textbox);

  if (spin && textbox)
    UpdateInfoControls();
}

void CGUIDialogPVRRadioRDSInfo::UpdateInfoControls()
{
  const std::shared_ptr<const CPVRChannel> channel =
      CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();
  if (!channel)
    return;

  const std::shared_ptr<const CPVRRadioRDSInfoTag> currentRDS = channel->GetRadioRDSInfoTag();
  if (!currentRDS)
    return;

  bool bInfoPresent = m_InfoNews.Update(currentRDS->GetInfoNews());
  bInfoPresent |= m_InfoNewsLocal.Update(currentRDS->GetInfoNewsLocal());
  bInfoPresent |= m_InfoSport.Update(currentRDS->GetInfoSport());
  bInfoPresent |= m_InfoWeather.Update(currentRDS->GetInfoWeather());
  bInfoPresent |= m_InfoLottery.Update(currentRDS->GetInfoLottery());
  bInfoPresent |= m_InfoStock.Update(currentRDS->GetInfoStock());
  bInfoPresent |= m_InfoOther.Update(currentRDS->GetInfoOther());
  bInfoPresent |= m_InfoCinema.Update(currentRDS->GetInfoCinema());
  bInfoPresent |= m_InfoHoroscope.Update(currentRDS->GetInfoHoroscope());

  if (bInfoPresent)
    SET_CONTROL_VISIBLE(CONTROL_INFO_LIST);
}

CGUIDialogPVRRadioRDSInfo::InfoControl::InfoControl(uint32_t iSpinLabelId, uint32_t iSpinControlId)
: m_iSpinLabelId(iSpinLabelId),
  m_iSpinControlId(iSpinControlId)
{
}

void CGUIDialogPVRRadioRDSInfo::InfoControl::Init(CGUISpinControl* spin, CGUITextBox* textbox)
{
  m_spinControl = spin;
  m_textbox = textbox;
  m_bSpinLabelPresent = false;
  m_textboxValue.clear();
}

bool CGUIDialogPVRRadioRDSInfo::InfoControl::Update(const std::string& textboxValue)
{
  if (m_spinControl && m_textbox && !textboxValue.empty())
  {
    if (!m_bSpinLabelPresent)
    {
      m_spinControl->AddLabel(g_localizeStrings.Get(m_iSpinLabelId), m_iSpinControlId);
      m_bSpinLabelPresent = true;
    }

    if (m_textboxValue != textboxValue)
    {
      m_spinControl->SetValue(m_iSpinControlId);
      m_textboxValue = textboxValue;
      m_textbox->SetInfo(textboxValue);
      return true;
    }
  }
  return false;
}
