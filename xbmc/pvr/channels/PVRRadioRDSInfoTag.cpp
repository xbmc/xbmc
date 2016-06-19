/*
 *      Copyright (C) 2005-2015 Team KODI
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

#include "PVRRadioRDSInfoTag.h"
#include "GUIUserMessages.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "utils/Archive.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace PVR;

CPVRRadioRDSInfoTagPtr CPVRRadioRDSInfoTag::CreateDefaultTag()
{
  return CPVRRadioRDSInfoTagPtr(new CPVRRadioRDSInfoTag());
}

CPVRRadioRDSInfoTag::CPVRRadioRDSInfoTag(void)
{
  Clear();
}

CPVRRadioRDSInfoTag::~CPVRRadioRDSInfoTag()
{}

void CPVRRadioRDSInfoTag::Serialize(CVariant& value) const
{
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
  return !(*this != right);
}

bool CPVRRadioRDSInfoTag::operator !=(const CPVRRadioRDSInfoTag& tag) const
{
  if (this == &tag) return false;
  if (m_strLanguage != tag.m_strLanguage) return true;
  if (m_strCountry != tag.m_strCountry) return true;
  if (m_strTitle != tag.m_strTitle) return true;
  if (m_strBand != tag.m_strBand) return true;
  if (m_strArtist != tag.m_strArtist) return true;
  if (m_strComposer != tag.m_strComposer) return true;
  if (m_strConductor != tag.m_strConductor) return true;
  if (m_strAlbum != tag.m_strAlbum) return true;
  if (m_iAlbumTracknumber != tag.m_iAlbumTracknumber) return true;
  if (m_strInfoNews != tag.m_strInfoNews) return true;
  if (m_strInfoNewsLocal != tag.m_strInfoNewsLocal) return true;
  if (m_strInfoSport != tag.m_strInfoSport) return true;
  if (m_strInfoStock != tag.m_strInfoStock) return true;
  if (m_strInfoWeather != tag.m_strInfoWeather) return true;
  if (m_strInfoLottery != tag.m_strInfoLottery) return true;
  if (m_strInfoOther != tag.m_strInfoOther) return true;
  if (m_strProgStyle != tag.m_strProgStyle) return true;
  if (m_strProgHost != tag.m_strProgHost) return true;
  if (m_strProgStation != tag.m_strProgStation) return true;
  if (m_strProgWebsite != tag.m_strProgWebsite) return true;
  if (m_strProgNow != tag.m_strProgNow) return true;
  if (m_strProgNext != tag.m_strProgNext) return true;
  if (m_strPhoneHotline != tag.m_strPhoneHotline) return true;
  if (m_strEMailHotline != tag.m_strEMailHotline) return true;
  if (m_strPhoneStudio != tag.m_strPhoneStudio) return true;
  if (m_strEMailStudio != tag.m_strEMailStudio) return true;
  if (m_strSMSStudio != tag.m_strSMSStudio) return true;
  if (m_strRadioStyle != tag.m_strRadioStyle) return true;
  if (m_strInfoHoroscope != tag.m_strInfoHoroscope) return true;
  if (m_strInfoCinema != tag.m_strInfoCinema) return true;
  if (m_strComment != tag.m_strComment) return true;
  if (m_strEditorialStaff != tag.m_strEditorialStaff) return true;

  if (m_bHaveRadiotext != tag.m_bHaveRadiotext) return true;
  if (m_bHaveRadiotextPlus != tag.m_bHaveRadiotextPlus) return true;

  return false;
}

void CPVRRadioRDSInfoTag::Clear()
{
  m_RDS_SpeechActive = false;

  m_strLanguage.erase();
  m_strCountry.erase();
  m_strTitle.erase();
  m_strBand.erase();
  m_strArtist.erase();
  m_strComposer.erase();
  m_strConductor.erase();
  m_strAlbum.erase();
  m_strComment.erase();
  m_iAlbumTracknumber = 0;
  m_strInfoNews.clear();
  m_strInfoNewsLocal.clear();
  m_strInfoSport.clear();
  m_strInfoStock.clear();
  m_strInfoWeather.clear();
  m_strInfoLottery.clear();
  m_strInfoOther.clear();
  m_strInfoHoroscope.clear();
  m_strInfoCinema.clear();
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
  m_strEditorialStaff.clear();

  m_bHaveRadiotext = false;
  m_bHaveRadiotextPlus = false;
}

void CPVRRadioRDSInfoTag::ResetSongInformation()
{
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
  m_RDS_SpeechActive = active;
}

void CPVRRadioRDSInfoTag::SetLanguage(const std::string& strLanguage)
{
  m_strLanguage = Trim(strLanguage);
}

const std::string& CPVRRadioRDSInfoTag::GetLanguage() const
{
  return m_strLanguage;
}

void CPVRRadioRDSInfoTag::SetCountry(const std::string& strCountry)
{
  m_strCountry = Trim(strCountry);
}

const std::string& CPVRRadioRDSInfoTag::GetCountry() const
{
  return m_strCountry;
}

void CPVRRadioRDSInfoTag::SetTitle(const std::string& strTitle)
{
  m_strTitle = Trim(strTitle);
}

void CPVRRadioRDSInfoTag::SetArtist(const std::string& strArtist)
{
  m_strArtist = Trim(strArtist);
}

void CPVRRadioRDSInfoTag::SetBand(const std::string& strBand)
{
  m_strBand = Trim(strBand);
  g_charsetConverter.unknownToUTF8(m_strBand);
}

void CPVRRadioRDSInfoTag::SetComposer(const std::string& strComposer)
{
  m_strComposer = Trim(strComposer);
  g_charsetConverter.unknownToUTF8(m_strComposer);
}

void CPVRRadioRDSInfoTag::SetConductor(const std::string& strConductor)
{
  m_strConductor = Trim(strConductor);
  g_charsetConverter.unknownToUTF8(m_strConductor);
}

void CPVRRadioRDSInfoTag::SetAlbum(const std::string& strAlbum)
{
  m_strAlbum = Trim(strAlbum);
  g_charsetConverter.unknownToUTF8(m_strAlbum);
}

void CPVRRadioRDSInfoTag::SetAlbumTrackNumber(int track)
{
  m_iAlbumTracknumber = track;
}

void CPVRRadioRDSInfoTag::SetComment(const std::string& strComment)
{
  m_strComment = Trim(strComment);
  g_charsetConverter.unknownToUTF8(m_strComment);
}

const std::string& CPVRRadioRDSInfoTag::GetTitle() const
{
  return m_strTitle;
}

const std::string& CPVRRadioRDSInfoTag::GetArtist() const
{
  return m_strArtist;
}

const std::string& CPVRRadioRDSInfoTag::GetBand() const
{
  return m_strBand;
}

const std::string& CPVRRadioRDSInfoTag::GetComposer() const
{
  return m_strComposer;
}

const std::string& CPVRRadioRDSInfoTag::GetConductor() const
{
  return m_strConductor;
}

const std::string& CPVRRadioRDSInfoTag::GetAlbum() const
{
  return m_strAlbum;
}

int CPVRRadioRDSInfoTag::GetAlbumTrackNumber() const
{
  return m_iAlbumTracknumber;
}

const std::string& CPVRRadioRDSInfoTag::GetComment() const
{
  return m_strComment;
}

void CPVRRadioRDSInfoTag::SetInfoNews(const std::string& strNews)
{
  std::string tmpStr = Trim(strNews);
  g_charsetConverter.unknownToUTF8(tmpStr);

  for (unsigned i = 0; i < m_strInfoNews.size(); ++i)
  {
    if (m_strInfoNews[i].compare(tmpStr) == 0)
      return;
  }

  if (m_strInfoNews.size() >= 10)
    m_strInfoNews.pop_front();

  m_strInfoNews.emplace_back(std::move(tmpStr));

  // send a message to all windows to tell them to update the radiotext
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_RADIOTEXT);
  g_windowManager.SendThreadMessage(msg);
}

const std::string CPVRRadioRDSInfoTag::GetInfoNews() const
{
  std::string retStr = "";
  for (unsigned i = 0; i < m_strInfoNews.size(); ++i)
  {
    std::string tmpStr = m_strInfoNews[i];
    tmpStr.insert(tmpStr.end(),'\n');
    retStr += tmpStr;
  }

  return retStr;
}

void CPVRRadioRDSInfoTag::SetInfoNewsLocal(const std::string& strNews)
{
  std::string tmpStr = Trim(strNews);
  g_charsetConverter.unknownToUTF8(tmpStr);

  for (unsigned i = 0; i < m_strInfoNewsLocal.size(); ++i)
  {
    if (m_strInfoNewsLocal[i].compare(tmpStr) == 0)
      return;
  }

  if (m_strInfoNewsLocal.size() >= 10)
    m_strInfoNewsLocal.pop_back();

  m_strInfoNewsLocal.emplace_front(std::move(tmpStr));

  // send a message to all windows to tell them to update the radiotext
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_RADIOTEXT);
  g_windowManager.SendThreadMessage(msg);
}

const std::string CPVRRadioRDSInfoTag::GetInfoNewsLocal() const
{
  std::string retStr = "";
  for (unsigned i = 0; i < m_strInfoNewsLocal.size(); ++i)
  {
    std::string tmpStr = m_strInfoNewsLocal[i];
    tmpStr.insert(tmpStr.end(),'\n');
    retStr += tmpStr;
  }

  return retStr;
}

void CPVRRadioRDSInfoTag::SetInfoSport(const std::string& strSport)
{
  std::string tmpStr = Trim(strSport);
  g_charsetConverter.unknownToUTF8(tmpStr);

  for (unsigned i = 0; i < m_strInfoSport.size(); ++i)
  {
    if (m_strInfoSport[i].compare(tmpStr) == 0)
      return;
  }

  if (m_strInfoSport.size() >= 10)
    m_strInfoSport.pop_back();

  m_strInfoSport.emplace_front(std::move(tmpStr));

  // send a message to all windows to tell them to update the radiotext
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_RADIOTEXT);
  g_windowManager.SendThreadMessage(msg);
}

const std::string CPVRRadioRDSInfoTag::GetInfoSport() const
{
  std::string retStr = "";
  for (unsigned i = 0; i < m_strInfoSport.size(); ++i)
  {
    std::string tmpStr = m_strInfoSport[i];
    tmpStr.insert(tmpStr.end(),'\n');
    retStr += tmpStr;
  }

  return retStr;
}

void CPVRRadioRDSInfoTag::SetInfoStock(const std::string& strStock)
{
  std::string tmpStr = Trim(strStock);
  g_charsetConverter.unknownToUTF8(tmpStr);

  for (unsigned i = 0; i < m_strInfoStock.size(); ++i)
  {
    if (m_strInfoStock[i].compare(tmpStr) == 0)
      return;
  }

  if (m_strInfoStock.size() >= 10)
    m_strInfoStock.pop_back();

  m_strInfoStock.emplace_front(std::move(tmpStr));

  // send a message to all windows to tell them to update the radiotext
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_RADIOTEXT);
  g_windowManager.SendThreadMessage(msg);
}

const std::string CPVRRadioRDSInfoTag::GetInfoStock() const
{
  std::string retStr = "";
  for (unsigned i = 0; i < m_strInfoStock.size(); ++i)
  {
    std::string tmpStr = m_strInfoStock[i];
    tmpStr.insert(tmpStr.end(),'\n');
    retStr += tmpStr;
  }

  return retStr;
}

void CPVRRadioRDSInfoTag::SetInfoWeather(const std::string& strWeather)
{
  std::string tmpStr = Trim(strWeather);
  g_charsetConverter.unknownToUTF8(tmpStr);

  for (unsigned i = 0; i < m_strInfoWeather.size(); ++i)
  {
    if (m_strInfoWeather[i].compare(tmpStr) == 0)
      return;
  }

  if (m_strInfoWeather.size() >= 10)
    m_strInfoWeather.pop_back();

  m_strInfoWeather.emplace_front(std::move(tmpStr));
  // send a message to all windows to tell them to update the radiotext
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_RADIOTEXT);
  g_windowManager.SendThreadMessage(msg);
}

const std::string CPVRRadioRDSInfoTag::GetInfoWeather() const
{
  std::string retStr = "";
  for (unsigned i = 0; i < m_strInfoWeather.size(); ++i)
  {
    std::string tmpStr = m_strInfoWeather[i];
    tmpStr.insert(tmpStr.end(),'\n');
    retStr += tmpStr;
  }

  return retStr;
}

void CPVRRadioRDSInfoTag::SetInfoLottery(const std::string& strLottery)
{
  std::string tmpStr = Trim(strLottery);
  g_charsetConverter.unknownToUTF8(tmpStr);

  for (unsigned i = 0; i < m_strInfoLottery.size(); ++i)
  {
    if (m_strInfoLottery[i].compare(tmpStr) == 0)
      return;
  }

  if (m_strInfoLottery.size() >= 10)
    m_strInfoLottery.pop_back();

  m_strInfoLottery.emplace_front(std::move(tmpStr));
  // send a message to all windows to tell them to update the radiotext
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_RADIOTEXT);
  g_windowManager.SendThreadMessage(msg);
}

const std::string CPVRRadioRDSInfoTag::GetInfoLottery() const
{
  std::string retStr = "";
  for (unsigned i = 0; i < m_strInfoLottery.size(); ++i)
  {
    std::string tmpStr = m_strInfoLottery[i];
    tmpStr.insert(tmpStr.end(),'\n');
    retStr += tmpStr;
  }

  return retStr;
}

void CPVRRadioRDSInfoTag::SetEditorialStaff(const std::string& strEditorialStaff)
{
  std::string tmpStr = Trim(strEditorialStaff);
  g_charsetConverter.unknownToUTF8(tmpStr);

  for (unsigned i = 0; i < m_strEditorialStaff.size(); ++i)
  {
    if (m_strEditorialStaff[i].compare(tmpStr) == 0)
      return;
  }

  if (m_strEditorialStaff.size() >= 10)
    m_strEditorialStaff.pop_back();

  m_strEditorialStaff.push_front(tmpStr);
  // send a message to all windows to tell them to update the radiotext
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_RADIOTEXT);
  g_windowManager.SendThreadMessage(msg);
}

const std::string CPVRRadioRDSInfoTag::GetEditorialStaff() const
{
  std::string retStr = "";
  for (unsigned i = 0; i < m_strEditorialStaff.size(); ++i)
  {
    std::string tmpStr = m_strEditorialStaff[i];
    tmpStr.insert(tmpStr.end(),'\n');
    retStr += tmpStr;
  }

  return retStr;
}

void CPVRRadioRDSInfoTag::SetInfoHoroscope(const std::string& strHoroscope)
{
  std::string tmpStr = Trim(strHoroscope);
  g_charsetConverter.unknownToUTF8(tmpStr);

  for (unsigned i = 0; i < m_strInfoHoroscope.size(); ++i)
  {
    if (m_strInfoHoroscope[i].compare(tmpStr) == 0)
      return;
  }

  if (m_strInfoHoroscope.size() >= 10)
    m_strInfoHoroscope.pop_back();

  m_strInfoHoroscope.emplace_front(std::move(tmpStr));
  // send a message to all windows to tell them to update the radiotext
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_RADIOTEXT);
  g_windowManager.SendThreadMessage(msg);
}

const std::string CPVRRadioRDSInfoTag::GetInfoHoroscope() const
{
  std::string retStr = "";
  for (unsigned i = 0; i < m_strInfoHoroscope.size(); ++i)
  {
    std::string tmpStr = m_strInfoHoroscope[i];
    tmpStr.insert(tmpStr.end(),'\n');
    retStr += tmpStr;
  }

  return retStr;
}

void CPVRRadioRDSInfoTag::SetInfoCinema(const std::string& strCinema)
{
  std::string tmpStr = Trim(strCinema);
  g_charsetConverter.unknownToUTF8(tmpStr);

  for (unsigned i = 0; i < m_strInfoCinema.size(); ++i)
  {
    if (m_strInfoCinema[i].compare(tmpStr) == 0)
      return;
  }

  if (m_strInfoCinema.size() >= 10)
    m_strInfoCinema.pop_back();

  m_strInfoCinema.emplace_front(std::move(tmpStr));
  // send a message to all windows to tell them to update the fileitem radiotext
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_RADIOTEXT);
  g_windowManager.SendThreadMessage(msg);
}

const std::string CPVRRadioRDSInfoTag::GetInfoCinema() const
{
  std::string retStr = "";
  for (unsigned i = 0; i < m_strInfoCinema.size(); ++i)
  {
    std::string tmpStr = m_strInfoCinema[i];
    tmpStr.insert(tmpStr.end(),'\n');
    retStr += tmpStr;
  }

  return retStr;
}

void CPVRRadioRDSInfoTag::SetInfoOther(const std::string& strOther)
{
  std::string tmpStr = Trim(strOther);
  g_charsetConverter.unknownToUTF8(tmpStr);

  for (unsigned i = 0; i < m_strInfoOther.size(); ++i)
  {
    if (m_strInfoOther[i].compare(tmpStr) == 0)
      return;
  }

  if (m_strInfoOther.size() >= 10)
    m_strInfoOther.pop_back();

  m_strInfoOther.emplace_front(std::move(tmpStr));
  // send a message to all windows to tell them to update the fileitem radiotext
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_RADIOTEXT);
  g_windowManager.SendThreadMessage(msg);
}

const std::string CPVRRadioRDSInfoTag::GetInfoOther() const
{
  std::string retStr = "";
  for (unsigned i = 0; i < m_strInfoOther.size(); ++i)
  {
    std::string tmpStr = m_strInfoOther[i];
    tmpStr.insert(tmpStr.end(),'\n');
    retStr += tmpStr;
  }

  return retStr;
}

void CPVRRadioRDSInfoTag::SetProgStation(const std::string& strProgStation)
{
  m_strProgStation = Trim(strProgStation);
  g_charsetConverter.unknownToUTF8(m_strProgStation);
}

void CPVRRadioRDSInfoTag::SetProgHost(const std::string& strProgHost)
{
  m_strProgHost = Trim(strProgHost);
  g_charsetConverter.unknownToUTF8(m_strProgHost);
}

void CPVRRadioRDSInfoTag::SetProgStyle(const std::string& strProgStyle)
{
  m_strProgStyle = Trim(strProgStyle);
  g_charsetConverter.unknownToUTF8(m_strProgStyle);
}

void CPVRRadioRDSInfoTag::SetProgWebsite(const std::string& strWebsite)
{
  m_strProgWebsite = Trim(strWebsite);
  g_charsetConverter.unknownToUTF8(m_strProgWebsite);
}

void CPVRRadioRDSInfoTag::SetProgNow(const std::string& strNow)
{
  m_strProgNow = Trim(strNow);
  g_charsetConverter.unknownToUTF8(m_strProgNow);
}

void CPVRRadioRDSInfoTag::SetProgNext(const std::string& strNext)
{
  m_strProgNext = Trim(strNext);
  g_charsetConverter.unknownToUTF8(m_strProgNext);
}

void CPVRRadioRDSInfoTag::SetPhoneHotline(const std::string& strHotline)
{
  m_strPhoneHotline = Trim(strHotline);
  g_charsetConverter.unknownToUTF8(m_strPhoneHotline);
}

void CPVRRadioRDSInfoTag::SetEMailHotline(const std::string& strHotline)
{
  m_strEMailHotline = Trim(strHotline);
  g_charsetConverter.unknownToUTF8(m_strEMailHotline);
}

void CPVRRadioRDSInfoTag::SetPhoneStudio(const std::string& strPhone)
{
  m_strPhoneStudio = Trim(strPhone);
  g_charsetConverter.unknownToUTF8(m_strPhoneStudio);
}

void CPVRRadioRDSInfoTag::SetEMailStudio(const std::string& strEMail)
{
  m_strEMailStudio = Trim(strEMail);
  g_charsetConverter.unknownToUTF8(m_strEMailStudio);
}

void CPVRRadioRDSInfoTag::SetSMSStudio(const std::string& strSMS)
{
  m_strSMSStudio = Trim(strSMS);
  g_charsetConverter.unknownToUTF8(m_strSMSStudio);
}

const std::string& CPVRRadioRDSInfoTag::GetProgStyle() const
{
  return m_strProgStyle;
}

const std::string& CPVRRadioRDSInfoTag::GetProgHost() const
{
  return m_strProgHost;
}

const std::string& CPVRRadioRDSInfoTag::GetProgStation() const
{
  return m_strProgStation;
}

const std::string& CPVRRadioRDSInfoTag::GetProgWebsite() const
{
  return m_strProgWebsite;
}

const std::string& CPVRRadioRDSInfoTag::GetProgNow() const
{
  return m_strProgNow;
}

const std::string& CPVRRadioRDSInfoTag::GetProgNext() const
{
  return m_strProgNext;
}

const std::string& CPVRRadioRDSInfoTag::GetPhoneHotline() const
{
  return m_strPhoneHotline;
}

const std::string& CPVRRadioRDSInfoTag::GetEMailHotline() const
{
  return m_strEMailHotline;
}

const std::string& CPVRRadioRDSInfoTag::GetPhoneStudio() const
{
  return m_strPhoneStudio;
}

const std::string& CPVRRadioRDSInfoTag::GetEMailStudio() const
{
  return m_strEMailStudio;
}

const std::string& CPVRRadioRDSInfoTag::GetSMSStudio() const
{
  return m_strSMSStudio;
}

std::string CPVRRadioRDSInfoTag::Trim(const std::string &value) const
{
  std::string trimmedValue(value);
  StringUtils::TrimLeft(trimmedValue);
  StringUtils::TrimRight(trimmedValue, " \n\r");
  return trimmedValue;
}
