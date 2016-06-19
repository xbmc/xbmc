#pragma once
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

#include "utils/IArchivable.h"
#include "utils/ISerializable.h"
#include "XBDateTime.h"

#include <memory>
#include <deque>
#include <string>

namespace PVR
{
  class CPVRRadioRDSInfoTag;
  typedef std::shared_ptr<CPVRRadioRDSInfoTag> CPVRRadioRDSInfoTagPtr;

class CPVRRadioRDSInfoTag : public IArchivable, public ISerializable
{
public:
  /*!
   * @brief Create a new empty event .
   */
  static CPVRRadioRDSInfoTagPtr CreateDefaultTag();

private:
  /*!
   * @brief Create a new empty event.
   */
  CPVRRadioRDSInfoTag(void);

  /*!
   * @brief Prevent copy construction, even for CEpgInfoTag instances and friends.
   * Note: Only declared, but intentionally not implemented
   *       to prevent compiler generated copy ctor and to force.
   *       a linker error in case somebody tries to call it.
   */
  CPVRRadioRDSInfoTag(const CPVRRadioRDSInfoTag& tag);

  /*!
   * @brief Prevent copy construction, even for CEpgInfoTag instances and friends.
   * Note: Only declared, but intentionally not implemented
   *       to prevent compiler generated copy ctor and to force.
   *       a linker error in case somebody tries to call it.
   */
  const CPVRRadioRDSInfoTag& operator =(const CPVRRadioRDSInfoTag& tag);
  
public:
  virtual ~CPVRRadioRDSInfoTag();

  bool operator ==(const CPVRRadioRDSInfoTag& tag) const;
  bool operator !=(const CPVRRadioRDSInfoTag& tag) const;

  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value) const;

  void Clear();
  void ResetSongInformation();

  /**! Basic RDS related information */
  void SetSpeechActive(bool active);
  void SetLanguage(const std::string& strLanguage);
  const std::string& GetLanguage() const;
  void SetCountry(const std::string& strCountry);
  const std::string& GetCountry() const;

  /**! RDS Radiotext related information */
  void SetTitle(const std::string& strTitle);
  void SetBand(const std::string& strBand);
  void SetArtist(const std::string& strArtist);
  void SetComposer(const std::string& strComposer);
  void SetConductor(const std::string& strConductor);
  void SetAlbum(const std::string& strAlbum);
  void SetComment(const std::string& strComment);
  void SetAlbumTrackNumber(int track);

  const std::string& GetTitle() const;
  const std::string& GetBand() const;
  const std::string& GetArtist() const;
  const std::string& GetComposer() const;
  const std::string& GetConductor() const;
  const std::string& GetAlbum() const;
  const std::string& GetComment() const;
  int GetAlbumTrackNumber() const;

  void SetProgStation(const std::string& strProgStation);
  void SetProgStyle(const std::string& strProgStyle);
  void SetProgHost(const std::string& strProgHost);
  void SetProgWebsite(const std::string& strWebsite);
  void SetProgNow(const std::string& strNow);
  void SetProgNext(const std::string& strNext);
  void SetPhoneHotline(const std::string& strHotline);
  void SetEMailHotline(const std::string& strHotline);
  void SetPhoneStudio(const std::string& strPhone);
  void SetEMailStudio(const std::string& strEMail);
  void SetSMSStudio(const std::string& strSMS);

  const std::string& GetProgStation() const;
  const std::string& GetProgStyle() const;
  const std::string& GetProgHost() const;
  const std::string& GetProgWebsite() const;
  const std::string& GetProgNow() const;
  const std::string& GetProgNext() const;
  const std::string& GetPhoneHotline() const;
  const std::string& GetEMailHotline() const;
  const std::string& GetPhoneStudio() const;
  const std::string& GetEMailStudio() const;
  const std::string& GetSMSStudio() const;

  void SetInfoNews(const std::string& strNews);
  const std::string GetInfoNews() const;

  void SetInfoNewsLocal(const std::string& strNews);
  const std::string GetInfoNewsLocal() const;

  void SetInfoSport(const std::string& strSport);
  const std::string GetInfoSport() const;

  void SetInfoStock(const std::string& strSport);
  const std::string GetInfoStock() const;

  void SetInfoWeather(const std::string& strWeather);
  const std::string GetInfoWeather() const;

  void SetInfoHoroscope(const std::string& strHoroscope);
  const std::string GetInfoHoroscope() const;

  void SetInfoCinema(const std::string& strCinema);
  const std::string GetInfoCinema() const;

  void SetInfoLottery(const std::string& strLottery);
  const std::string GetInfoLottery() const;

  void SetInfoOther(const std::string& strOther);
  const std::string GetInfoOther() const;

  void SetEditorialStaff(const std::string& strEditorialStaff);
  const std::string GetEditorialStaff() const;

  void SetRadioStyle(const std::string& style) { m_strRadioStyle = style; }
  const std::string GetRadioStyle() const { return m_strRadioStyle; }
  void SetPlayingRadiotext(bool yesNo) { m_bHaveRadiotext = yesNo; }
  bool IsPlayingRadiotext() { return m_bHaveRadiotext; }
  void SetPlayingRadiotextPlus(bool yesNo) { m_bHaveRadiotextPlus = yesNo; }
  bool IsPlayingRadiotextPlus() { return m_bHaveRadiotextPlus; }

protected:
  /*! \brief Trim whitespace off the given string
   *  \param value string to trim
   *  \return trimmed value, with spaces removed from left and right, as well as carriage returns from the right.
   */
  std::string Trim(const std::string &value) const;

  bool m_RDS_SpeechActive;

  std::string m_strLanguage;
  std::string m_strCountry;
  std::string m_strTitle;
  std::string m_strBand;
  std::string m_strArtist;
  std::string m_strComposer;
  std::string m_strConductor;
  std::string m_strAlbum;
  std::string m_strComment;
  int         m_iAlbumTracknumber;
  std::string m_strRadioStyle;

  std::deque<std::string> m_strInfoNews;
  std::deque<std::string> m_strInfoNewsLocal;
  std::deque<std::string> m_strInfoSport;
  std::deque<std::string> m_strInfoStock;
  std::deque<std::string> m_strInfoWeather;
  std::deque<std::string> m_strInfoLottery;
  std::deque<std::string> m_strInfoOther;
  std::deque<std::string> m_strInfoHoroscope;
  std::deque<std::string> m_strInfoCinema;
  std::deque<std::string> m_strEditorialStaff;
  std::string m_strProgStyle;
  std::string m_strProgHost;
  std::string m_strProgStation;
  std::string m_strProgWebsite;
  std::string m_strProgNow;
  std::string m_strProgNext;
  std::string m_strPhoneHotline;
  std::string m_strEMailHotline;
  std::string m_strPhoneStudio;
  std::string m_strEMailStudio;
  std::string m_strSMSStudio;

  bool m_bHaveRadiotext;
  bool m_bHaveRadiotextPlus;
};
}
