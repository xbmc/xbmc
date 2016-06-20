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

#include "Application.h"
#include "FileItem.h"
#include "GUIDialogPVRRadioRDSInfo.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/GUISpinControl.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUITextBox.h"
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
  , m_InfoPresent(false)
  , m_LabelInfoNewsPresent(false)
  , m_LabelInfoNewsLocalPresent(false)
  , m_LabelInfoWeatherPresent(false)
  , m_LabelInfoLotteryPresent(false)
  , m_LabelInfoSportPresent(false)
  , m_LabelInfoStockPresent(false)
  , m_LabelInfoOtherPresent(false)
  , m_LabelInfoCinemaPresent(false)
  , m_LabelInfoHoroscopePresent(false)
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
      CGUISpinControl *spin = (CGUISpinControl *)GetControl(SPIN_CONTROL_INFO);
      if (!spin) return true;

      CGUITextBox *textbox = (CGUITextBox *)GetControl(TEXT_INFO);
      if (!textbox) return true;

      PVR::CPVRRadioRDSInfoTagPtr currentRDS = g_application.CurrentFileItem().GetPVRRadioRDSInfoTag();
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
    if (IsActive() && message.GetParam1() == GUI_MSG_UPDATE_RADIOTEXT &&
        g_application.m_pPlayer->IsPlaying() &&
        g_application.CurrentFileItem().HasPVRRadioRDSInfoTag())
    {
      PVR::CPVRRadioRDSInfoTagPtr currentRDS = g_application.CurrentFileItem().GetPVRRadioRDSInfoTag();
      CGUISpinControl *spin = (CGUISpinControl *)GetControl(SPIN_CONTROL_INFO);
      CGUITextBox *textbox = (CGUITextBox *)GetControl(TEXT_INFO);

      if (currentRDS->GetInfoNews().size())
      {
        if (!m_LabelInfoNewsPresent)
        {
          spin->AddLabel(g_localizeStrings.Get(29916), INFO_NEWS);
          m_LabelInfoNewsPresent = true;
          m_InfoPresent = true;
        }

        if (m_LabelInfoNews != currentRDS->GetInfoNews())
        {
          spin->SetValue(INFO_NEWS);
          m_LabelInfoNews = currentRDS->GetInfoNews();
          textbox->SetInfo(m_LabelInfoNews);
        }
      }
      if (currentRDS->GetInfoNewsLocal().size())
      {
        if (!m_LabelInfoNewsLocalPresent)
        {
          spin->AddLabel(g_localizeStrings.Get(29917), INFO_NEWS_LOCAL);
          m_LabelInfoNewsLocalPresent = true;
          m_InfoPresent = true;
        }

        if (m_LabelInfoNewsLocal != currentRDS->GetInfoNewsLocal())
        {
          spin->SetValue(INFO_NEWS_LOCAL);
          m_LabelInfoNewsLocal = currentRDS->GetInfoNewsLocal();
          textbox->SetInfo(m_LabelInfoNewsLocal);
        }
      }
      if (currentRDS->GetInfoSport().size())
      {
        if (!m_LabelInfoSportPresent)
        {
          spin->AddLabel(g_localizeStrings.Get(29918), INFO_SPORT);
          m_LabelInfoSportPresent = true;
          m_InfoPresent = true;
        }

        if (m_LabelInfoSport != currentRDS->GetInfoSport())
        {
          spin->SetValue(INFO_SPORT);
          m_LabelInfoSport = currentRDS->GetInfoSport();
          textbox->SetInfo(m_LabelInfoSport);
        }
      }
      if (currentRDS->GetInfoWeather().size())
      {
        if (!m_LabelInfoWeatherPresent)
        {
          spin->AddLabel(g_localizeStrings.Get(400), INFO_WEATHER);
          m_LabelInfoWeatherPresent = true;
          m_InfoPresent = true;
        }

        if (m_LabelInfoWeather != currentRDS->GetInfoWeather())
        {
          spin->SetValue(INFO_WEATHER);
          m_LabelInfoWeather = currentRDS->GetInfoWeather();
          textbox->SetInfo(m_LabelInfoWeather);
        }
      }
      if (currentRDS->GetInfoLottery().size())
      {
        if (!m_LabelInfoLotteryPresent)
        {
          spin->AddLabel(g_localizeStrings.Get(29919), INFO_LOTTERY);
          m_LabelInfoLotteryPresent = true;
          m_InfoPresent = true;
        }

        if (m_LabelInfoLottery != currentRDS->GetInfoLottery())
        {
          spin->SetValue(INFO_LOTTERY);
          m_LabelInfoLottery = currentRDS->GetInfoLottery();
          textbox->SetInfo(m_LabelInfoLottery);
        }
      }
      if (currentRDS->GetInfoStock().size())
      {
        if (!m_LabelInfoStockPresent)
        {
          spin->AddLabel(g_localizeStrings.Get(29920), INFO_STOCK);
          m_LabelInfoStockPresent = true;
          m_InfoPresent = true;
        }

        if (m_LabelInfoStock != currentRDS->GetInfoStock())
        {
          spin->SetValue(INFO_STOCK);
          m_LabelInfoStock = currentRDS->GetInfoStock();
          textbox->SetInfo(m_LabelInfoStock);
        }
      }
      if (currentRDS->GetInfoOther().size())
      {
        if (!m_LabelInfoOtherPresent)
        {
          spin->AddLabel(g_localizeStrings.Get(29921), INFO_OTHER);
          m_LabelInfoOtherPresent = true;
          m_InfoPresent = true;
        }

        if (m_LabelInfoOther != currentRDS->GetInfoOther())
        {
          spin->SetValue(INFO_OTHER);
          m_LabelInfoOther = currentRDS->GetInfoOther();
          textbox->SetInfo(m_LabelInfoOther);
        }
      }
      if (currentRDS->GetInfoCinema().size())
      {
        if (!m_LabelInfoCinemaPresent)
        {
          spin->AddLabel(g_localizeStrings.Get(19602), INFO_CINEMA);
          m_LabelInfoCinemaPresent = true;
          m_InfoPresent = true;
        }

        if (m_LabelInfoCinema != currentRDS->GetInfoCinema())
        {
          spin->SetValue(INFO_CINEMA);
          m_LabelInfoCinema = currentRDS->GetInfoCinema();
          textbox->SetInfo(m_LabelInfoCinema);
        }
      }
      if (currentRDS->GetInfoHoroscope().size())
      {
        if (!m_LabelInfoHoroscopePresent)
        {
          spin->AddLabel(g_localizeStrings.Get(29922), INFO_HOROSCOPE);
          m_LabelInfoHoroscopePresent = true;
          m_InfoPresent = true;
        }

        if (m_LabelInfoHoroscope != currentRDS->GetInfoHoroscope())
        {
          spin->SetValue(INFO_HOROSCOPE);
          m_LabelInfoHoroscope = currentRDS->GetInfoHoroscope();
          textbox->SetInfo(m_LabelInfoHoroscope);
        }
      }
      if (m_InfoPresent)
        SET_CONTROL_VISIBLE(CONTROL_INFO_LIST);
      else
        SET_CONTROL_HIDDEN(CONTROL_INFO_LIST);
    }
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogPVRRadioRDSInfo::OnInitWindow()
{
  // call init
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

  PVR::CPVRRadioRDSInfoTagPtr currentRDS = g_application.CurrentFileItem().GetPVRRadioRDSInfoTag();

  CGUISpinControl *spin = (CGUISpinControl *)GetControl(SPIN_CONTROL_INFO);
  if (!spin) return;
  spin->Clear();

  CGUITextBox *textbox = (CGUITextBox *)GetControl(TEXT_INFO);
  if (!textbox) return;

  if (currentRDS->GetInfoNews().size())
  {
    spin->AddLabel(g_localizeStrings.Get(29916), INFO_NEWS);
    textbox->SetInfo(currentRDS->GetInfoNews());
    spin->SetValue(INFO_NEWS);
    m_LabelInfoNewsPresent = true;
    m_InfoPresent = true;
  }
  if (currentRDS->GetInfoNewsLocal().size())
  {
    spin->AddLabel(g_localizeStrings.Get(29917), INFO_NEWS_LOCAL);
    if (!m_InfoPresent)
    {
      textbox->SetInfo(currentRDS->GetInfoNewsLocal());
      spin->SetValue(INFO_NEWS_LOCAL);
      m_LabelInfoNewsLocalPresent = true;
      m_InfoPresent = true;
    }
  }
  if (currentRDS->GetInfoSport().size())
  {
    spin->AddLabel(g_localizeStrings.Get(29918), INFO_SPORT);
    if (!m_InfoPresent)
    {
      textbox->SetInfo(currentRDS->GetInfoSport());
      spin->SetValue(INFO_SPORT);
      m_LabelInfoSportPresent = true;
      m_InfoPresent = true;
    }
  }
  if (currentRDS->GetInfoWeather().size())
  {
    spin->AddLabel(g_localizeStrings.Get(400), INFO_WEATHER);
    if (!m_InfoPresent)
    {
      textbox->SetInfo(currentRDS->GetInfoWeather());
      spin->SetValue(INFO_WEATHER);
      m_LabelInfoWeatherPresent = true;
      m_InfoPresent = true;
    }
  }
  if (currentRDS->GetInfoLottery().size())
  {
    spin->AddLabel(g_localizeStrings.Get(29919), INFO_LOTTERY);
    if (!m_InfoPresent)
    {
      textbox->SetInfo(currentRDS->GetInfoLottery());
      spin->SetValue(INFO_LOTTERY);
      m_LabelInfoLotteryPresent = true;
      m_InfoPresent = true;
    }
  }
  if (currentRDS->GetInfoStock().size())
  {
    spin->AddLabel(g_localizeStrings.Get(29920), INFO_STOCK);
    if (!m_InfoPresent)
    {
      textbox->SetInfo(currentRDS->GetInfoStock());
      spin->SetValue(INFO_STOCK);
      m_LabelInfoStockPresent = true;
      m_InfoPresent = true;
    }
  }
  if (currentRDS->GetInfoOther().size())
  {
    spin->AddLabel(g_localizeStrings.Get(29921), INFO_OTHER);
    if (!m_InfoPresent)
    {
      textbox->SetInfo(currentRDS->GetInfoOther());
      spin->SetValue(INFO_OTHER);
      m_LabelInfoOtherPresent = true;
      m_InfoPresent = true;
    }
  }
  if (currentRDS->GetInfoCinema().size())
  {
    spin->AddLabel(g_localizeStrings.Get(19602), INFO_CINEMA);
    if (!m_InfoPresent)
    {
      textbox->SetInfo(currentRDS->GetInfoCinema());
      spin->SetValue(INFO_CINEMA);
      m_LabelInfoCinemaPresent = true;
      m_InfoPresent = true;
    }
  }
  if (currentRDS->GetInfoHoroscope().size())
  {
    spin->AddLabel(g_localizeStrings.Get(29922), INFO_HOROSCOPE);
    if (!m_InfoPresent)
    {
      textbox->SetInfo(currentRDS->GetInfoHoroscope());
      spin->SetValue(INFO_HOROSCOPE);
      m_LabelInfoHoroscopePresent = true;
      m_InfoPresent = true;
    }
  }

  if (m_InfoPresent)
    SET_CONTROL_VISIBLE(CONTROL_INFO_LIST);
  else
    SET_CONTROL_HIDDEN(CONTROL_INFO_LIST);
}

void CGUIDialogPVRRadioRDSInfo::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

CFileItemPtr CGUIDialogPVRRadioRDSInfo::GetCurrentListItem(int offset)
{
  return m_rdsItem;
}
