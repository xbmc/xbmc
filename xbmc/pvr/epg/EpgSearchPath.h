/*
 *  Copyright (C) 2012-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace PVR
{
class CPVREpgSearchFilter;

class CPVREpgSearchPath
{
public:
  static const std::string PATH_SEARCH_DIALOG;
  static const std::string PATH_TV_SEARCH;
  static const std::string PATH_TV_SAVEDSEARCHES;
  static const std::string PATH_RADIO_SEARCH;
  static const std::string PATH_RADIO_SAVEDSEARCHES;

  explicit CPVREpgSearchPath(const std::string& strPath);
  explicit CPVREpgSearchPath(const CPVREpgSearchFilter& search);

  bool IsValid() const { return m_bValid; }

  const std::string& GetPath() const { return m_path; }
  bool IsSearchRoot() const { return m_bRoot; }
  bool IsRadio() const { return m_bRadio; }
  bool IsSavedSearchesRoot() const { return m_bSavedSearchesRoot; }
  bool IsSavedSearch() const { return m_bSavedSearch; }
  int GetId() const { return m_iId; }

private:
  bool Init(const std::string& strPath);

  std::string m_path;
  bool m_bValid = false;
  bool m_bRoot = false;
  bool m_bRadio = false;
  bool m_bSavedSearchesRoot = false;
  bool m_bSavedSearch = false;
  int m_iId = -1;
};
} // namespace PVR
