/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/IArchivable.h"
#include "utils/ISerializable.h"
#include "utils/ISortable.h"

#include <string>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CGameInfoTag : public IArchivable, public ISerializable, public ISortable
{
public:
  CGameInfoTag() { Reset(); }
  CGameInfoTag(const CGameInfoTag& tag) { *this = tag; }
  CGameInfoTag& operator=(const CGameInfoTag& tag);
  virtual ~CGameInfoTag() = default;
  void Reset();

  bool operator==(const CGameInfoTag& tag) const;
  bool operator!=(const CGameInfoTag& tag) const { return !(*this == tag); }

  bool IsLoaded() const { return m_bLoaded; }
  void SetLoaded(bool bOnOff = true) { m_bLoaded = bOnOff; }

  // File path
  const std::string& GetURL() const { return m_strURL; }
  void SetURL(const std::string& strURL) { m_strURL = strURL; }

  // Title
  const std::string& GetTitle() const { return m_strTitle; }
  void SetTitle(const std::string& strTitle) { m_strTitle = strTitle; }

  // Platform
  const std::string& GetPlatform() const { return m_strPlatform; }
  void SetPlatform(const std::string& strPlatform) { m_strPlatform = strPlatform; }

  // Genres
  const std::vector<std::string>& GetGenres() const { return m_genres; }
  void SetGenres(const std::vector<std::string>& genres) { m_genres = genres; }

  // Developer
  const std::string& GetDeveloper() const { return m_strDeveloper; }
  void SetDeveloper(const std::string& strDeveloper) { m_strDeveloper = strDeveloper; }

  // Overview
  const std::string& GetOverview() const { return m_strOverview; }
  void SetOverview(const std::string& strOverview) { m_strOverview = strOverview; }

  // Year
  unsigned int GetYear() const { return m_year; }
  void SetYear(unsigned int year) { m_year = year; }

  // Game Code (ID)
  const std::string& GetID() const { return m_strID; }
  void SetID(const std::string& strID) { m_strID = strID; }

  // Region
  const std::string& GetRegion() const { return m_strRegion; }
  void SetRegion(const std::string& strRegion) { m_strRegion = strRegion; }

  // Publisher / Licensee
  const std::string& GetPublisher() const { return m_strPublisher; }
  void SetPublisher(const std::string& strPublisher) { m_strPublisher = strPublisher; }

  // Format (PAL/NTSC)
  const std::string& GetFormat() const { return m_strFormat; }
  void SetFormat(const std::string& strFormat) { m_strFormat = strFormat; }

  // Cartridge Type, e.g. "ROM+MBC5+RAM+BATT" or "CD"
  const std::string& GetCartridgeType() const { return m_strCartridgeType; }
  void SetCartridgeType(const std::string& strCartridgeType)
  {
    m_strCartridgeType = strCartridgeType;
  }

  // Game client add-on ID
  const std::string& GetGameClient() const { return m_strGameClient; }
  void SetGameClient(const std::string& strGameClient) { m_strGameClient = strGameClient; }

  void Archive(CArchive& ar) override;
  void Serialize(CVariant& value) const override;
  void ToSortable(SortItem& sortable, Field field) const override;

private:
  bool m_bLoaded;
  std::string m_strURL;
  std::string m_strTitle;
  std::string m_strPlatform;
  std::vector<std::string> m_genres;
  std::string m_strDeveloper;
  std::string m_strOverview;
  unsigned int m_year;
  std::string m_strID;
  std::string m_strRegion;
  std::string m_strPublisher;
  std::string m_strFormat;
  std::string m_strCartridgeType;
  std::string m_strGameClient;
};
} // namespace GAME
} // namespace KODI
