/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRRecordingsPath.h"

#include "URL.h"
#include "XBDateTime.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <string>
#include <vector>

using namespace PVR;

const std::string CPVRRecordingsPath::PATH_RECORDINGS = "pvr://recordings/";
const std::string CPVRRecordingsPath::PATH_ACTIVE_TV_RECORDINGS = "pvr://recordings/tv/active/";
const std::string CPVRRecordingsPath::PATH_ACTIVE_RADIO_RECORDINGS =
    "pvr://recordings/radio/active/";
const std::string CPVRRecordingsPath::PATH_DELETED_TV_RECORDINGS = "pvr://recordings/tv/deleted/";
const std::string CPVRRecordingsPath::PATH_DELETED_RADIO_RECORDINGS =
    "pvr://recordings/radio/deleted/";

CPVRRecordingsPath::CPVRRecordingsPath(const std::string& strPath)
{
  std::string strVarPath(TrimSlashes(strPath));
  const std::vector<std::string> segments = URIUtils::SplitPath(strVarPath);

  m_bValid = ((segments.size() >= 4) && // at least pvr://recordings/[tv|radio]/[active|deleted]
              StringUtils::StartsWith(strVarPath, "pvr://") && (segments.at(1) == "recordings") &&
              ((segments.at(2) == "tv") || (segments.at(2) == "radio")) &&
              ((segments.at(3) == "active") || (segments.at(3) == "deleted")));
  if (m_bValid)
  {
    m_bRoot = (m_bValid && (segments.size() == 4));
    m_bRadio = (m_bValid && (segments.at(2) == "radio"));
    m_bActive = (m_bValid && (segments.at(3) == "active"));

    if (m_bRoot)
      strVarPath.append("/");
    else
    {
      size_t paramStart = strVarPath.find(", TV");
      if (paramStart == std::string::npos)
        m_directoryPath = strVarPath.substr(GetDirectoryPathPosition());
      else
      {
        size_t dirStart = GetDirectoryPathPosition();
        m_directoryPath = strVarPath.substr(dirStart, paramStart - dirStart);
        m_params = strVarPath.substr(paramStart);
      }
    }

    m_path = strVarPath;
  }
  else
  {
    m_bRoot = false;
    m_bActive = false;
    m_bRadio = false;
  }
}

CPVRRecordingsPath::CPVRRecordingsPath(bool bDeleted, bool bRadio)
  : m_bValid(true),
    m_bRoot(true),
    m_bActive(!bDeleted),
    m_bRadio(bRadio),
    m_path(StringUtils::Format(
        "pvr://recordings/{}/{}/", bRadio ? "radio" : "tv", bDeleted ? "deleted" : "active"))
{
}

CPVRRecordingsPath::CPVRRecordingsPath(bool bDeleted,
                                       bool bRadio,
                                       const std::string& strDirectory,
                                       const std::string& strTitle,
                                       int iSeason,
                                       int iEpisode,
                                       int iYear,
                                       const std::string& strSubtitle,
                                       const std::string& strChannelName,
                                       const CDateTime& recordingTime,
                                       const std::string& strId)
  : m_bValid(true), m_bRoot(false), m_bActive(!bDeleted), m_bRadio(bRadio)
{
  std::string strDirectoryN(TrimSlashes(strDirectory));
  if (!strDirectoryN.empty())
    strDirectoryN = StringUtils::Format("{}/", strDirectoryN);

  std::string strTitleN(strTitle);
  strTitleN = CURL::Encode(strTitleN);

  std::string strSeasonEpisodeN;
  if ((iSeason > -1 && iEpisode > -1 && (iSeason > 0 || iEpisode > 0)))
    strSeasonEpisodeN = StringUtils::Format("s{:02}e{:02}", iSeason, iEpisode);
  if (!strSeasonEpisodeN.empty())
    strSeasonEpisodeN = StringUtils::Format(" {}", strSeasonEpisodeN);

  std::string strYearN(iYear > 0 ? StringUtils::Format(" ({})", iYear) : "");

  std::string strSubtitleN;
  if (!strSubtitle.empty())
  {
    strSubtitleN = StringUtils::Format(" {}", strSubtitle);
    strSubtitleN = CURL::Encode(strSubtitleN);
  }

  std::string strChannelNameN;
  if (!strChannelName.empty())
  {
    strChannelNameN = StringUtils::Format(" ({})", strChannelName);
    strChannelNameN = CURL::Encode(strChannelNameN);
  }

  m_directoryPath = StringUtils::Format("{}{}{}{}{}", strDirectoryN, strTitleN, strSeasonEpisodeN,
                                        strYearN, strSubtitleN);
  m_params = StringUtils::Format(", TV{}, {}, {}.pvr", strChannelNameN,
                                 recordingTime.GetAsSaveString(), strId);
  m_path = StringUtils::Format("pvr://recordings/{}/{}/{}{}", bRadio ? "radio" : "tv",
                               bDeleted ? "deleted" : "active", m_directoryPath, m_params);
}

std::string CPVRRecordingsPath::GetUnescapedDirectoryPath() const
{
  return CURL::Decode(m_directoryPath);
}

std::string CPVRRecordingsPath::GetUnescapedSubDirectoryPath(const std::string& strPath) const
{
  // note: strPath must be unescaped.

  std::string strReturn;
  std::string strUsePath(TrimSlashes(strPath));

  const std::string strUnescapedDirectoryPath(GetUnescapedDirectoryPath());

  /* adding "/" to make sure that base matches the complete folder name and not only parts of it */
  if (!strUnescapedDirectoryPath.empty() &&
      (strUsePath.size() <= strUnescapedDirectoryPath.size() ||
       !URIUtils::PathHasParent(strUsePath, strUnescapedDirectoryPath)))
    return strReturn;

  strUsePath.erase(0, strUnescapedDirectoryPath.size());
  strUsePath = TrimSlashes(strUsePath);

  /* check for more occurrences */
  size_t iDelimiter = strUsePath.find('/');
  if (iDelimiter == std::string::npos)
    strReturn = strUsePath;
  else
    strReturn = strUsePath.substr(0, iDelimiter);

  return strReturn;
}

const std::string CPVRRecordingsPath::GetTitle() const
{
  if (m_bValid)
  {
    CRegExp reg(true);
    if (reg.RegComp("pvr://recordings/(.*/)*(.*), TV( \\(.*\\))?, "
                    "(19[0-9][0-9]|20[0-9][0-9])[0-9][0-9][0-9][0-9]_[0-9][0-9][0-9][0-9][0-9][0-9]"
                    ", (.*).pvr"))
    {
      if (reg.RegFind(m_path.c_str()) >= 0)
        return reg.GetMatch(2);
    }
  }
  return "";
}

void CPVRRecordingsPath::AppendSegment(const std::string& strSegment)
{
  if (!m_bValid || strSegment.empty() || strSegment == "/")
    return;

  std::string strVarSegment(TrimSlashes(strSegment));
  strVarSegment = CURL::Encode(strVarSegment);

  if (!m_directoryPath.empty() && m_directoryPath.back() != '/')
    m_directoryPath.push_back('/');

  m_directoryPath += strVarSegment;
  m_directoryPath.push_back('/');

  size_t paramStart = m_path.find(", TV");
  if (paramStart == std::string::npos)
  {
    if (!m_path.empty() && m_path.back() != '/')
      m_path.push_back('/');

    // append the segment
    m_path += strVarSegment;
    m_path.push_back('/');
  }
  else
  {
    if (m_path.back() != '/')
      m_path.insert(paramStart, "/");

    // insert the segment between end of current directory path and parameters
    m_path.insert(paramStart, strVarSegment);
  }

  m_bRoot = false;
}

std::string CPVRRecordingsPath::TrimSlashes(const std::string& strString)
{
  std::string strTrimmed(strString);
  while (!strTrimmed.empty() && strTrimmed.front() == '/')
    strTrimmed.erase(0, 1);

  while (!strTrimmed.empty() && strTrimmed.back() == '/')
    strTrimmed.pop_back();

  return strTrimmed;
}

size_t CPVRRecordingsPath::GetDirectoryPathPosition() const
{
  if (m_bActive)
  {
    if (m_bRadio)
      return PATH_ACTIVE_RADIO_RECORDINGS.size();
    else
      return PATH_ACTIVE_TV_RECORDINGS.size();
  }
  else
  {
    if (m_bRadio)
      return PATH_DELETED_RADIO_RECORDINGS.size();
    else
      return PATH_DELETED_TV_RECORDINGS.size();
  }
  // unreachable
}
