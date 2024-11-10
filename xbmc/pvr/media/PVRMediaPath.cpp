/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRMediaPath.h"

#include "URL.h"
#include "XBDateTime.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <string>
#include <vector>

using namespace PVR;

const std::string CPVRMediaPath::PATH_MEDIA = "pvr://media/";
const std::string CPVRMediaPath::PATH_TV_MEDIA = "pvr://media/tv/";
const std::string CPVRMediaPath::PATH_RADIO_MEDIA = "pvr://media/radio/";

CPVRMediaPath::CPVRMediaPath(const std::string& strPath)
{
  std::string strVarPath(TrimSlashes(strPath));
  const std::vector<std::string> segments = URIUtils::SplitPath(strVarPath);

  m_bValid = ((segments.size() >= 3) && // at least pvr://media/[tv|radio]
              StringUtils::StartsWith(strVarPath, "pvr://") && (segments.at(1) == "media") &&
              ((segments.at(2) == "tv") || (segments.at(2) == "radio")));
  if (m_bValid)
  {
    m_bRoot = (segments.size() == 3);
    m_bRadio = (segments.at(2) == "radio");

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
    m_bRadio = false;
  }
}

CPVRMediaPath::CPVRMediaPath(bool bRadio)
  : m_bValid(true),
    m_bRoot(true),
    m_bRadio(bRadio),
    m_path(StringUtils::Format("pvr://media/{}/", bRadio ? "radio" : "tv"))
{
}

CPVRMediaPath::CPVRMediaPath(bool bRadio,
                             const std::string& strDirectory,
                             const std::string& strTitle,
                             int iSeason,
                             int iEpisode,
                             int iYear,
                             const std::string& strSubtitle,
                             const CDateTime& mediaTagTime,
                             const std::string& strId)
  : m_bValid(true), m_bRoot(false), m_bRadio(bRadio)
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

  m_directoryPath = StringUtils::Format("{}{}{}{}{}", strDirectoryN, strTitleN, strSeasonEpisodeN,
                                        strYearN, strSubtitleN);
  m_params = StringUtils::Format(", TV, {}, {}.pvr", mediaTagTime.GetAsSaveString(), strId);
  m_path = StringUtils::Format("pvr://media/{}/{}{}", bRadio ? "radio" : "tv", m_directoryPath,
                               m_params);
}

std::string CPVRMediaPath::GetUnescapedDirectoryPath() const
{
  return CURL::Decode(m_directoryPath);
}

namespace
{
bool PathHasParent(const std::string& path, const std::string& parent)
{
  if (path == parent)
    return true;

  if (!parent.empty() && parent.back() != '/')
    return StringUtils::StartsWith(path, parent + '/');

  return StringUtils::StartsWith(path, parent);
}
} // unnamed namespace

std::string CPVRMediaPath::GetUnescapedSubDirectoryPath(const std::string& strPath) const
{
  // note: strPath must be unescaped.

  std::string strReturn;
  std::string strUsePath(TrimSlashes(strPath));

  const std::string strUnescapedDirectoryPath(GetUnescapedDirectoryPath());

  /* adding "/" to make sure that base matches the complete folder name and not only parts of it */
  if (!strUnescapedDirectoryPath.empty() &&
      (strUsePath.size() <= strUnescapedDirectoryPath.size() ||
       !PathHasParent(strUsePath, strUnescapedDirectoryPath)))
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

const std::string CPVRMediaPath::GetTitle() const
{
  if (m_bValid)
  {
    CRegExp reg(true);
    if (reg.RegComp("pvr://media/(.*/)*(.*), TV( \\(.*\\))?, "
                    "(19[0-9][0-9]|20[0-9][0-9])[0-9][0-9][0-9][0-9]_[0-9][0-9][0-9][0-9][0-9][0-9]"
                    ", (.*).pvr"))
    {
      if (reg.RegFind(m_path.c_str()) >= 0)
        return reg.GetMatch(2);
    }
  }
  return "";
}

void CPVRMediaPath::AppendSegment(const std::string& strSegment)
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

std::string CPVRMediaPath::TrimSlashes(const std::string& strString)
{
  std::string strTrimmed(strString);
  while (!strTrimmed.empty() && strTrimmed.front() == '/')
    strTrimmed.erase(0, 1);

  while (!strTrimmed.empty() && strTrimmed.back() == '/')
    strTrimmed.pop_back();

  return strTrimmed;
}

size_t CPVRMediaPath::GetDirectoryPathPosition() const
{
  if (m_bRadio)
    return PATH_RADIO_MEDIA.size();
  else
    return PATH_TV_MEDIA.size();
}
