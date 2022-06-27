/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "utils/IArchivable.h"
#include "utils/ISerializable.h"

#include <deque>
#include <string>

namespace PVR
{

class CPVRRadioRDSInfoTag final : public IArchivable, public ISerializable
{
public:
  CPVRRadioRDSInfoTag();

  bool operator ==(const CPVRRadioRDSInfoTag& right) const;
  bool operator !=(const CPVRRadioRDSInfoTag& right) const;

  void Archive(CArchive& ar) override;
  void Serialize(CVariant& value) const override;

  void Clear();
  void ResetSongInformation();

  /**! Basic RDS related information */
  void SetSpeechActive(bool active);
  void SetLanguage(const std::string& strLanguage);
  const std::string& GetLanguage() const;
  void SetCountry(const std::string& strCountry);
  const std::string& GetCountry() const;
  void SetRadioText(const std::string& strRadioText);
  std::string GetRadioText(unsigned int line) const;
  void SetProgramServiceText(const std::string& strPSText);

  /**! RDS RadioText related information */
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

  void SetRadioStyle(const std::string& style);
  const std::string GetRadioStyle() const;

  void SetPlayingRadioText(bool yesNo);
  bool IsPlayingRadioText() const;

  void SetPlayingRadioTextPlus(bool yesNo);
  bool IsPlayingRadioTextPlus() const;

private:
  CPVRRadioRDSInfoTag(const CPVRRadioRDSInfoTag& tag) = delete;
  const CPVRRadioRDSInfoTag& operator =(const CPVRRadioRDSInfoTag& tag) = delete;

  static std::string Trim(const std::string& value);

  mutable CCriticalSection m_critSection;

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
  int m_iAlbumTracknumber;
  std::string m_strRadioStyle;

  class Info
  {
  public:
    Info() = delete;
    Info(size_t maxSize, bool prependData) : m_maxSize(maxSize), m_prependData(prependData) {}

    bool operator==(const Info& right) const;

    size_t Size() const { return m_data.size(); }
    size_t MaxSize() const { return m_maxSize; }

    void Clear();
    void Add(const std::string& text);

    const std::string& GetText() const { return m_infoText; }
    std::string GetLine(unsigned int line) const
    {
      return line < m_data.size() ? m_data.at(line) : "";
    }

  private:
    const size_t m_maxSize = 10;
    const bool m_prependData = false;
    std::deque<std::string> m_data;
    std::string m_infoText;
  };

  Info m_strInfoNews{10, false};
  Info m_strInfoNewsLocal{10, false};
  Info m_strInfoSport{10, false};
  Info m_strInfoStock{10, false};
  Info m_strInfoWeather{10, false};
  Info m_strInfoLottery{10, false};
  Info m_strInfoOther{10, false};
  Info m_strInfoHoroscope{10, false};
  Info m_strInfoCinema{10, false};
  Info m_strEditorialStaff{10, false};

  Info m_strRadioText{6, true};

  Info m_strProgramServiceText{12, false};
  std::string m_strProgramServiceLine0;
  std::string m_strProgramServiceLine1;

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

  bool m_bHaveRadioText;
  bool m_bHaveRadioTextPlus;
};
}
