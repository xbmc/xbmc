/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRRadioRDSInfoTag.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "utils/Archive.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace PVR;

CPVRRadioRDSInfoTag::CPVRRadioRDSInfoTag(void)
{
  Clear();
}

void CPVRRadioRDSInfoTag::Serialize(CVariant& value) const
{
  CSingleLock lock(m_critSection);

  value["strLanguage"] = m_strLanguage;
  value["strCountry"] = m_strCountry;
  value["strTitle"] = m_strTitle;
  value["strBand"] = m_strBand;
  value["strArtist"] = m_strArtist;
  value["strComposer"] = m_strComposer;
  value["strConductor"] = m_strConductor;
  value["strAlbum"] = m_strAlbum;
  value["iAlbumTracknumber"] = m_iAlbumTracknumber;
  value["strProgStation"] = m_strProgStation;
  value["strProgStyle"] = m_strProgStyle;
  value["strProgHost"] = m_strProgHost;
  value["strProgWebsite"] = m_strProgWebsite;
  value["strProgNow"] = m_strProgNow;
  value["strProgNext"] = m_strProgNext;
  value["strPhoneHotline"] = m_strPhoneHotline;
  value["strEMailHotline"] = m_strEMailHotline;
  value["strPhoneStudio"] = m_strPhoneStudio;
  value["strEMailStudio"] = m_strEMailStudio;
  value["strSMSStudio"] = m_strSMSStudio;
  value["strRadioStyle"] = m_strRadioStyle;
}

void CPVRRadioRDSInfoTag::Archive(CArchive& ar)
{
  CSingleLock lock(m_critSection);

  if (ar.IsStoring())
  {
    ar << m_strLanguage;
    ar << m_strCountry;
    ar << m_strTitle;
    ar << m_strBand;
    ar << m_strArtist;
    ar << m_strComposer;
    ar << m_strConductor;
    ar << m_strAlbum;
    ar << m_iAlbumTracknumber;
    ar << m_strProgStation;
    ar << m_strProgStyle;
    ar << m_strProgHost;
    ar << m_strProgWebsite;
    ar << m_strProgNow;
    ar << m_strProgNext;
    ar << m_strPhoneHotline;
    ar << m_strEMailHotline;
    ar << m_strPhoneStudio;
    ar << m_strEMailStudio;
    ar << m_strSMSStudio;
    ar << m_strRadioStyle;
  }
  else
  {
    ar >> m_strLanguage;
    ar >> m_strCountry;
    ar >> m_strTitle;
    ar >> m_strBand;
    ar >> m_strArtist;
    ar >> m_strComposer;
    ar >> m_strConductor;
    ar >> m_strAlbum;
    ar >> m_iAlbumTracknumber;
    ar >> m_strProgStation;
    ar >> m_strProgStyle;
    ar >> m_strProgHost;
    ar >> m_strProgWebsite;
    ar >> m_strProgNow;
    ar >> m_strProgNext;
    ar >> m_strPhoneHotline;
    ar >> m_strEMailHotline;
    ar >> m_strPhoneStudio;
    ar >> m_strEMailStudio;
    ar >> m_strSMSStudio;
    ar >> m_strRadioStyle;
  }
}

bool CPVRRadioRDSInfoTag::operator==(const CPVRRadioRDSInfoTag &right) const
{
  if (this == &right)
    return true;

  CSingleLock lock(m_critSection);
  return (m_strLanguage == right.m_strLanguage &&
          m_strCountry == right.m_strCountry &&
          m_strTitle == right.m_strTitle &&
          m_strBand == right.m_strBand &&
          m_strArtist == right.m_strArtist &&
          m_strComposer == right.m_strComposer &&
          m_strConductor == right.m_strConductor &&
          m_strAlbum == right.m_strAlbum &&
          m_iAlbumTracknumber == right.m_iAlbumTracknumber &&
          m_strInfoNews == right.m_strInfoNews &&
          m_strInfoNewsLocal == right.m_strInfoNewsLocal &&
          m_strInfoSport == right.m_strInfoSport &&
          m_strInfoStock == right.m_strInfoStock &&
          m_strInfoWeather == right.m_strInfoWeather &&
          m_strInfoLottery == right.m_strInfoLottery &&
          m_strInfoOther == right.m_strInfoOther &&
          m_strProgStyle == right.m_strProgStyle &&
          m_strProgHost == right.m_strProgHost &&
          m_strProgStation == right.m_strProgStation &&
          m_strProgWebsite == right.m_strProgWebsite &&
          m_strProgNow == right.m_strProgNow &&
          m_strProgNext == right.m_strProgNext &&
          m_strPhoneHotline == right.m_strPhoneHotline &&
          m_strEMailHotline == right.m_strEMailHotline &&
          m_strPhoneStudio == right.m_strPhoneStudio &&
          m_strEMailStudio == right.m_strEMailStudio &&
          m_strSMSStudio == right.m_strSMSStudio &&
          m_strRadioStyle == right.m_strRadioStyle &&
          m_strInfoHoroscope == right.m_strInfoHoroscope &&
          m_strInfoCinema == right.m_strInfoCinema &&
          m_strComment == right.m_strComment &&
          m_strEditorialStaff == right.m_strEditorialStaff &&
          m_bHaveRadiotext == right.m_bHaveRadiotext &&
          m_bHaveRadiotextPlus == right.m_bHaveRadiotextPlus);
}

bool CPVRRadioRDSInfoTag::operator !=(const CPVRRadioRDSInfoTag& right) const
{
  if (this == &right)
    return false;

  return !(*this == right);
}

void CPVRRadioRDSInfoTag::Clear()
{
  CSingleLock lock(m_critSection);

  m_RDS_SpeechActive = false;

  ResetSongInformation();

  m_strLanguage.erase();
  m_strCountry.erase();
  m_strComment.erase();
  m_strInfoNews.Clear();
  m_strInfoNewsLocal.Clear();
  m_strInfoSport.Clear();
  m_strInfoStock.Clear();
  m_strInfoWeather.Clear();
  m_strInfoLottery.Clear();
  m_strInfoOther.Clear();
  m_strInfoHoroscope.Clear();
  m_strInfoCinema.Clear();
  m_strProgStyle.erase();
  m_strProgHost.erase();
  m_strProgStation.erase();
  m_strProgWebsite.erase();
  m_strPhoneHotline.erase();
  m_strEMailHotline.erase();
  m_strPhoneStudio.erase();
  m_strEMailStudio.erase();
  m_strSMSStudio.erase();
  m_strRadioStyle = "unknown";
  m_strEditorialStaff.Clear();

  m_bHaveRadiotext = false;
  m_bHaveRadiotextPlus = false;
}

void CPVRRadioRDSInfoTag::ResetSongInformation()
{
  CSingleLock lock(m_critSection);

  m_strTitle.erase();
  m_strBand.erase();
  m_strArtist.erase();
  m_strComposer.erase();
  m_strConductor.erase();
  m_strAlbum.erase();
  m_iAlbumTracknumber = 0;
}

void CPVRRadioRDSInfoTag::SetSpeechActive(bool active)
{
  CSingleLock lock(m_critSection);
  m_RDS_SpeechActive = active;
}

void CPVRRadioRDSInfoTag::SetLanguage(const std::string& strLanguage)
{
  CSingleLock lock(m_critSection);
  m_strLanguage = Trim(strLanguage);
}

const std::string& CPVRRadioRDSInfoTag::GetLanguage() const
{
  CSingleLock lock(m_critSection);
  return m_strLanguage;
}

void CPVRRadioRDSInfoTag::SetCountry(const std::string& strCountry)
{
  CSingleLock lock(m_critSection);
  m_strCountry = Trim(strCountry);
}

const std::string& CPVRRadioRDSInfoTag::GetCountry() const
{
  CSingleLock lock(m_critSection);
  return m_strCountry;
}

void CPVRRadioRDSInfoTag::SetTitle(const std::string& strTitle)
{
  CSingleLock lock(m_critSection);
  m_strTitle = Trim(strTitle);
}

void CPVRRadioRDSInfoTag::SetArtist(const std::string& strArtist)
{
  CSingleLock lock(m_critSection);
  m_strArtist = Trim(strArtist);
}

void CPVRRadioRDSInfoTag::SetBand(const std::string& strBand)
{
  CSingleLock lock(m_critSection);
  m_strBand = Trim(strBand);
  g_charsetConverter.unknownToUTF8(m_strBand);
}

void CPVRRadioRDSInfoTag::SetComposer(const std::string& strComposer)
{
  CSingleLock lock(m_critSection);
  m_strComposer = Trim(strComposer);
  g_charsetConverter.unknownToUTF8(m_strComposer);
}

void CPVRRadioRDSInfoTag::SetConductor(const std::string& strConductor)
{
  CSingleLock lock(m_critSection);
  m_strConductor = Trim(strConductor);
  g_charsetConverter.unknownToUTF8(m_strConductor);
}

void CPVRRadioRDSInfoTag::SetAlbum(const std::string& strAlbum)
{
  CSingleLock lock(m_critSection);
  m_strAlbum = Trim(strAlbum);
  g_charsetConverter.unknownToUTF8(m_strAlbum);
}

void CPVRRadioRDSInfoTag::SetAlbumTrackNumber(int track)
{
  CSingleLock lock(m_critSection);
  m_iAlbumTracknumber = track;
}

void CPVRRadioRDSInfoTag::SetComment(const std::string& strComment)
{
  CSingleLock lock(m_critSection);
  m_strComment = Trim(strComment);
  g_charsetConverter.unknownToUTF8(m_strComment);
}

const std::string& CPVRRadioRDSInfoTag::GetTitle() const
{
  CSingleLock lock(m_critSection);
  return m_strTitle;
}

const std::string& CPVRRadioRDSInfoTag::GetArtist() const
{
  CSingleLock lock(m_critSection);
  return m_strArtist;
}

const std::string& CPVRRadioRDSInfoTag::GetBand() const
{
  CSingleLock lock(m_critSection);
  return m_strBand;
}

const std::string& CPVRRadioRDSInfoTag::GetComposer() const
{
  CSingleLock lock(m_critSection);
  return m_strComposer;
}

const std::string& CPVRRadioRDSInfoTag::GetConductor() const
{
  CSingleLock lock(m_critSection);
  return m_strConductor;
}

const std::string& CPVRRadioRDSInfoTag::GetAlbum() const
{
  CSingleLock lock(m_critSection);
  return m_strAlbum;
}

int CPVRRadioRDSInfoTag::GetAlbumTrackNumber() const
{
  CSingleLock lock(m_critSection);
  return m_iAlbumTracknumber;
}

const std::string& CPVRRadioRDSInfoTag::GetComment() const
{
  CSingleLock lock(m_critSection);
  return m_strComment;
}

void CPVRRadioRDSInfoTag::SetInfoNews(const std::string& strNews)
{
  CSingleLock lock(m_critSection);
  m_strInfoNews.Add(strNews);
}

const std::string CPVRRadioRDSInfoTag::GetInfoNews() const
{
  CSingleLock lock(m_critSection);
  return m_strInfoNews.GetText();
}

void CPVRRadioRDSInfoTag::SetInfoNewsLocal(const std::string& strNews)
{
  CSingleLock lock(m_critSection);
  m_strInfoNewsLocal.Add(strNews);
}

const std::string CPVRRadioRDSInfoTag::GetInfoNewsLocal() const
{
  CSingleLock lock(m_critSection);
  return m_strInfoNewsLocal.GetText();
}

void CPVRRadioRDSInfoTag::SetInfoSport(const std::string& strSport)
{
  CSingleLock lock(m_critSection);
  m_strInfoSport.Add(strSport);
}

const std::string CPVRRadioRDSInfoTag::GetInfoSport() const
{
  CSingleLock lock(m_critSection);
  return m_strInfoSport.GetText();
}

void CPVRRadioRDSInfoTag::SetInfoStock(const std::string& strStock)
{
  CSingleLock lock(m_critSection);
  m_strInfoStock.Add(strStock);
}

const std::string CPVRRadioRDSInfoTag::GetInfoStock() const
{
  CSingleLock lock(m_critSection);
  return m_strInfoStock.GetText();
}

void CPVRRadioRDSInfoTag::SetInfoWeather(const std::string& strWeather)
{
  CSingleLock lock(m_critSection);
  m_strInfoWeather.Add(strWeather);
}

const std::string CPVRRadioRDSInfoTag::GetInfoWeather() const
{
  CSingleLock lock(m_critSection);
  return m_strInfoWeather.GetText();
}

void CPVRRadioRDSInfoTag::SetInfoLottery(const std::string& strLottery)
{
  CSingleLock lock(m_critSection);
  m_strInfoLottery.Add(strLottery);
}

const std::string CPVRRadioRDSInfoTag::GetInfoLottery() const
{
  CSingleLock lock(m_critSection);
  return m_strInfoLottery.GetText();
}

void CPVRRadioRDSInfoTag::SetEditorialStaff(const std::string& strEditorialStaff)
{
  CSingleLock lock(m_critSection);
  m_strEditorialStaff.Add(strEditorialStaff);
}

const std::string CPVRRadioRDSInfoTag::GetEditorialStaff() const
{
  CSingleLock lock(m_critSection);
  return m_strEditorialStaff.GetText();
}

void CPVRRadioRDSInfoTag::SetInfoHoroscope(const std::string& strHoroscope)
{
  CSingleLock lock(m_critSection);
  m_strInfoHoroscope.Add(strHoroscope);
}

const std::string CPVRRadioRDSInfoTag::GetInfoHoroscope() const
{
  CSingleLock lock(m_critSection);
  return m_strInfoHoroscope.GetText();
}

void CPVRRadioRDSInfoTag::SetInfoCinema(const std::string& strCinema)
{
  CSingleLock lock(m_critSection);
  m_strInfoCinema.Add(strCinema);
}

const std::string CPVRRadioRDSInfoTag::GetInfoCinema() const
{
  CSingleLock lock(m_critSection);
  return m_strInfoCinema.GetText();
}

void CPVRRadioRDSInfoTag::SetInfoOther(const std::string& strOther)
{
  CSingleLock lock(m_critSection);
  m_strInfoOther.Add(strOther);
}

const std::string CPVRRadioRDSInfoTag::GetInfoOther() const
{
  CSingleLock lock(m_critSection);
  return m_strInfoOther.GetText();
}

void CPVRRadioRDSInfoTag::SetProgStation(const std::string& strProgStation)
{
  CSingleLock lock(m_critSection);
  m_strProgStation = Trim(strProgStation);
  g_charsetConverter.unknownToUTF8(m_strProgStation);
}

void CPVRRadioRDSInfoTag::SetProgHost(const std::string& strProgHost)
{
  CSingleLock lock(m_critSection);
  m_strProgHost = Trim(strProgHost);
  g_charsetConverter.unknownToUTF8(m_strProgHost);
}

void CPVRRadioRDSInfoTag::SetProgStyle(const std::string& strProgStyle)
{
  CSingleLock lock(m_critSection);
  m_strProgStyle = Trim(strProgStyle);
  g_charsetConverter.unknownToUTF8(m_strProgStyle);
}

void CPVRRadioRDSInfoTag::SetProgWebsite(const std::string& strWebsite)
{
  CSingleLock lock(m_critSection);
  m_strProgWebsite = Trim(strWebsite);
  g_charsetConverter.unknownToUTF8(m_strProgWebsite);
}

void CPVRRadioRDSInfoTag::SetProgNow(const std::string& strNow)
{
  CSingleLock lock(m_critSection);
  m_strProgNow = Trim(strNow);
  g_charsetConverter.unknownToUTF8(m_strProgNow);
}

void CPVRRadioRDSInfoTag::SetProgNext(const std::string& strNext)
{
  CSingleLock lock(m_critSection);
  m_strProgNext = Trim(strNext);
  g_charsetConverter.unknownToUTF8(m_strProgNext);
}

void CPVRRadioRDSInfoTag::SetPhoneHotline(const std::string& strHotline)
{
  CSingleLock lock(m_critSection);
  m_strPhoneHotline = Trim(strHotline);
  g_charsetConverter.unknownToUTF8(m_strPhoneHotline);
}

void CPVRRadioRDSInfoTag::SetEMailHotline(const std::string& strHotline)
{
  CSingleLock lock(m_critSection);
  m_strEMailHotline = Trim(strHotline);
  g_charsetConverter.unknownToUTF8(m_strEMailHotline);
}

void CPVRRadioRDSInfoTag::SetPhoneStudio(const std::string& strPhone)
{
  CSingleLock lock(m_critSection);
  m_strPhoneStudio = Trim(strPhone);
  g_charsetConverter.unknownToUTF8(m_strPhoneStudio);
}

void CPVRRadioRDSInfoTag::SetEMailStudio(const std::string& strEMail)
{
  CSingleLock lock(m_critSection);
  m_strEMailStudio = Trim(strEMail);
  g_charsetConverter.unknownToUTF8(m_strEMailStudio);
}

void CPVRRadioRDSInfoTag::SetSMSStudio(const std::string& strSMS)
{
  CSingleLock lock(m_critSection);
  m_strSMSStudio = Trim(strSMS);
  g_charsetConverter.unknownToUTF8(m_strSMSStudio);
}

const std::string& CPVRRadioRDSInfoTag::GetProgStyle() const
{
  CSingleLock lock(m_critSection);
  return m_strProgStyle;
}

const std::string& CPVRRadioRDSInfoTag::GetProgHost() const
{
  CSingleLock lock(m_critSection);
  return m_strProgHost;
}

const std::string& CPVRRadioRDSInfoTag::GetProgStation() const
{
  CSingleLock lock(m_critSection);
  return m_strProgStation;
}

const std::string& CPVRRadioRDSInfoTag::GetProgWebsite() const
{
  CSingleLock lock(m_critSection);
  return m_strProgWebsite;
}

const std::string& CPVRRadioRDSInfoTag::GetProgNow() const
{
  CSingleLock lock(m_critSection);
  return m_strProgNow;
}

const std::string& CPVRRadioRDSInfoTag::GetProgNext() const
{
  CSingleLock lock(m_critSection);
  return m_strProgNext;
}

const std::string& CPVRRadioRDSInfoTag::GetPhoneHotline() const
{
  CSingleLock lock(m_critSection);
  return m_strPhoneHotline;
}

const std::string& CPVRRadioRDSInfoTag::GetEMailHotline() const
{
  CSingleLock lock(m_critSection);
  return m_strEMailHotline;
}

const std::string& CPVRRadioRDSInfoTag::GetPhoneStudio() const
{
  CSingleLock lock(m_critSection);
  return m_strPhoneStudio;
}

const std::string& CPVRRadioRDSInfoTag::GetEMailStudio() const
{
  CSingleLock lock(m_critSection);
  return m_strEMailStudio;
}

const std::string& CPVRRadioRDSInfoTag::GetSMSStudio() const
{
  CSingleLock lock(m_critSection);
  return m_strSMSStudio;
}

void CPVRRadioRDSInfoTag::SetRadioStyle(const std::string& style)
{
  CSingleLock lock(m_critSection);
  m_strRadioStyle = style;
}

const std::string CPVRRadioRDSInfoTag::GetRadioStyle() const
{
  CSingleLock lock(m_critSection);
  return m_strRadioStyle;
}

void CPVRRadioRDSInfoTag::SetPlayingRadiotext(bool yesNo)
{
  CSingleLock lock(m_critSection);
  m_bHaveRadiotext = yesNo;
}

bool CPVRRadioRDSInfoTag::IsPlayingRadiotext() const
{
  CSingleLock lock(m_critSection);
  return m_bHaveRadiotext;
}

void CPVRRadioRDSInfoTag::SetPlayingRadiotextPlus(bool yesNo)
{
  CSingleLock lock(m_critSection);
  m_bHaveRadiotextPlus = yesNo;
}

bool CPVRRadioRDSInfoTag::IsPlayingRadiotextPlus() const
{
  CSingleLock lock(m_critSection);
  return m_bHaveRadiotextPlus;
}

std::string CPVRRadioRDSInfoTag::Trim(const std::string &value)
{
  std::string trimmedValue(value);
  StringUtils::TrimLeft(trimmedValue);
  StringUtils::TrimRight(trimmedValue, " \n\r");
  return trimmedValue;
}

bool CPVRRadioRDSInfoTag::Info::operator==(const CPVRRadioRDSInfoTag::Info &right) const
{
  if (this == &right)
    return true;

  return (m_infoText == right.m_infoText &&
          m_data == right.m_data);
}

void CPVRRadioRDSInfoTag::Info::Clear()
{
  m_data.clear();
  m_infoText.clear();
}

void CPVRRadioRDSInfoTag::Info::Add(const std::string& text)
{
  std::string tmp = Trim(text);
  g_charsetConverter.unknownToUTF8(tmp);

  if (std::find(m_data.begin(), m_data.end(), text) != m_data.end())
    return;

  if (m_data.size() >= 10)
    m_data.pop_front();

  m_data.emplace_back(std::move(tmp));

  m_infoText.clear();
  for (const std::string& data : m_data)
  {
    m_infoText += data;
    m_infoText += '\n';
  }

  // send a message to all windows to tell them to update the radiotext
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_RADIOTEXT);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}
