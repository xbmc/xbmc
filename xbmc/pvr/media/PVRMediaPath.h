/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CDateTime;

namespace PVR
{
class CPVRMediaPath
{
public:
  static const std::string PATH_MEDIA;
  static const std::string PATH_TV_MEDIA;
  static const std::string PATH_RADIO_MEDIA;

  explicit CPVRMediaPath(const std::string& strPath);
  CPVRMediaPath(bool bRadio);
  CPVRMediaPath(bool bRadio,
                const std::string& strDirectory,
                const std::string& strTitle,
                int iSeason,
                int iEpisode,
                int iYear,
                const std::string& strSubtitle,
                const CDateTime& mediaTagTime,
                const std::string& strId);

  operator std::string() const { return m_path; }

  bool IsValid() const { return m_bValid; }

  const std::string& GetPath() const { return m_path; }
  bool IsMediaRoot() const { return m_bRoot; }
  bool IsRadio() const { return m_bRadio; }
  bool IsTV() const { return !IsRadio(); }
  std::string GetUnescapedDirectoryPath() const;
  std::string GetUnescapedSubDirectoryPath(const std::string& strPath) const;

  const std::string GetTitle() const;
  void AppendSegment(const std::string& strSegment);

private:
  static std::string TrimSlashes(const std::string& strString);
  size_t GetDirectoryPathPosition() const;

  bool m_bValid;
  bool m_bRoot;
  bool m_bRadio;
  std::string m_directoryPath;
  std::string m_params;
  std::string m_path;
};
} // namespace PVR
