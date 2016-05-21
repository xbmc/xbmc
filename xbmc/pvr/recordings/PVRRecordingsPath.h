#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>

class CDateTime;

namespace PVR
{
  class CPVRRecordingsPath
  {
  public:
    static const std::string PATH_RECORDINGS;
    static const std::string PATH_ACTIVE_TV_RECORDINGS;
    static const std::string PATH_ACTIVE_RADIO_RECORDINGS;
    static const std::string PATH_DELETED_TV_RECORDINGS;
    static const std::string PATH_DELETED_RADIO_RECORDINGS;

    CPVRRecordingsPath(const std::string &strPath);
    CPVRRecordingsPath(bool bDeleted, bool bRadio);
    CPVRRecordingsPath(bool bDeleted, bool bRadio,
                       const std::string &strDirectory, const std::string &strTitle,
                       int iSeason, int iEpisode, int iYear,
                       const std::string &strSubtitle, const std::string &strChannelName,
                       const CDateTime &recordingTime, const std::string &strId);

    operator std::string() const { return m_path; }

    bool IsValid() const { return m_bValid; }

    const std::string &GetPath() const { return m_path; }
    bool IsRecordingsRoot() const { return m_bRoot; }
    bool IsActive() const { return m_bActive; }
    bool IsDeleted() const { return !IsActive(); }
    bool IsRadio() const { return m_bRadio; }
    bool IsTV() const { return !IsRadio(); }
    std::string GetDirectoryPath() const { return m_directoryPath; }
    std::string GetSubDirectoryPath(const std::string &strPath) const;

    const std::string GetTitle() const;
    void AppendSegment(const std::string &strSegment);

  private:
    static std::string TrimSlashes(const std::string &strString);
    size_t GetDirectoryPathPosition() const;

    bool m_bValid;
    bool m_bRoot;
    bool m_bActive;
    bool m_bRadio;
    std::string m_directoryPath;
    std::string m_params;
    std::string m_path;
  };
}
