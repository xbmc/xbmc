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

#include "system.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include "PVRRecordingsPath.h"

using namespace PVR;

const std::string CPVRRecordingsPath::PATH_RECORDINGS               = "pvr://recordings/";
const std::string CPVRRecordingsPath::PATH_ACTIVE_TV_RECORDINGS     = "pvr://recordings/tv/active/";
const std::string CPVRRecordingsPath::PATH_ACTIVE_RADIO_RECORDINGS  = "pvr://recordings/radio/active/";
const std::string CPVRRecordingsPath::PATH_DELETED_TV_RECORDINGS    = "pvr://recordings/tv/deleted/";
const std::string CPVRRecordingsPath::PATH_DELETED_RADIO_RECORDINGS = "pvr://recordings/radio/deleted/";

CPVRRecordingsPath::CPVRRecordingsPath(const std::string &strPath)
{
  std::string strVarPath(TrimSlashes(strPath));
  const std::vector<std::string> segments = URIUtils::SplitPath(strVarPath);

  m_bValid  = ((segments.size() >= 4) && // at least pvr://recordings/[tv|radio]/[active|deleted]
               StringUtils::StartsWith(strVarPath, "pvr://") &&
               (segments.at(1) == "recordings") &&
               ((segments.at(2) == "tv") || (segments.at(2) == "radio")) &&
               ((segments.at(3) == "active") || (segments.at(3) == "deleted")));
  m_bRoot   = (m_bValid && (segments.size() == 4));
  m_bRadio  = (m_bValid && (segments.at(2) == "radio"));
  m_bActive = (m_bValid && (segments.at(3) == "active"));

  if (m_bRoot)
    strVarPath.append("/");
  else
  {
    size_t paramStart = m_path.find(", TV");
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

CPVRRecordingsPath::CPVRRecordingsPath(bool bDeleted, bool bRadio)
: m_bValid(true),
  m_bRoot(true),
  m_bActive(!bDeleted),
  m_bRadio(bRadio),
  m_path(StringUtils::Format("pvr://recordings/%s/%s/", bRadio ? "radio" : "tv", bDeleted ? "deleted" : "active"))
{
}

CPVRRecordingsPath::CPVRRecordingsPath(bool bDeleted, bool bRadio,
                       const std::string &strDirectory, const std::string &strTitle,
                       int iSeason, int iEpisode, int iYear,
                       const std::string &strSubtitle, const std::string &strChannelName,
                       const CDateTime &recordingTime, const std::string &strId)
: m_bValid(true),
  m_bRoot(false),
  m_bActive(!bDeleted),
  m_bRadio(bRadio)
{
  std::string strDirectoryN(TrimSlashes(strDirectory));
  if (!strDirectoryN.empty())
    strDirectoryN = StringUtils::Format("%s/", strDirectoryN.c_str());

  std::string strTitleN(strTitle);
  StringUtils::Replace(strTitleN, '/', ' ');

  std::string strSeasonEpisodeN;
  if ((iSeason > -1 && iEpisode > -1 && (iSeason > 0 || iEpisode > 0)))
    strSeasonEpisodeN = StringUtils::Format("s%02de%02d", iSeason, iEpisode);
  if (!strSeasonEpisodeN.empty())
    strSeasonEpisodeN = StringUtils::Format(" %s", strSeasonEpisodeN.c_str());

  std::string strYearN(iYear > 0 ? StringUtils::Format(" (%i)", iYear) : "");

  std::string strSubtitleN;
  if (!strSubtitle.empty())
  {
    strSubtitleN = StringUtils::Format(" %s", strSubtitle.c_str());
    StringUtils::Replace(strSubtitleN, '/', ' ');
  }

  std::string strChannelNameN;
  if (!strChannelName.empty())
  {
    strChannelNameN = StringUtils::Format(" (%s)", strChannelName.c_str());
    StringUtils::Replace(strChannelNameN, '/', ' ');
  }

  m_directoryPath = StringUtils::Format("%s%s%s%s%s",
                                        strDirectoryN.c_str(), strTitleN.c_str(), strSeasonEpisodeN.c_str(),
                                        strYearN.c_str(), strSubtitleN.c_str());
  m_params = StringUtils::Format(", TV%s, %s, %s.pvr", strChannelNameN.c_str(), recordingTime.GetAsSaveString().c_str(), strId.c_str());
  m_path   = StringUtils::Format("pvr://recordings/%s/%s/%s%s", bRadio ? "radio" : "tv", bDeleted ? "deleted" : "active", m_directoryPath.c_str(), m_params.c_str());
}

std::string CPVRRecordingsPath::GetSubDirectoryPath(const std::string &strPath) const
{
  std::string strReturn;
  std::string strUsePath(TrimSlashes(strPath));

  /* adding "/" to make sure that base matches the complete folder name and not only parts of it */
  if (!m_directoryPath.empty() && (strUsePath.size() <= m_directoryPath.size() || !URIUtils::PathHasParent(strUsePath, m_directoryPath)))
    return strReturn;

  strUsePath.erase(0, m_directoryPath.size());

  /* check for more occurences */
  size_t iDelimiter = strUsePath.find('/');
  if (iDelimiter == std::string::npos)
    strReturn = strUsePath;
  else if (iDelimiter == 0)
    strReturn = strUsePath.substr(1);
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
                    "(19[0-9][0-9]|20[0-9][0-9])[0-9][0-9][0-9][0-9]_[0-9][0-9][0-9][0-9][0-9][0-9], (.*).pvr"))
    {
      if (reg.RegFind(m_path.c_str()) >= 0)
        return reg.GetMatch(2);
    }
  }
  return "";
}

void CPVRRecordingsPath::AppendSegment(const std::string &strSegment)
{
  if (!m_bValid || strSegment.empty() || strSegment == "/")
    return;

  std::string strVarSegment(TrimSlashes(strSegment));

  if (!m_directoryPath.empty() && m_directoryPath.back() != '/')
    m_directoryPath.push_back('/');

  m_directoryPath += strSegment;

  size_t paramStart = m_path.find(", TV");
  if (paramStart == std::string::npos)
  {
    if (!m_path.empty() && m_path.back() != '/')
      m_path.push_back('/');

    // append the segment
    m_path += strSegment;
  }
  else
  {
    if (m_path.back() != '/')
      m_path.insert(paramStart, "/");

    // insert the segment between end of current directory path and parameters
    m_path.insert(paramStart, strSegment);
  }

  m_bRoot = false;
}

std::string CPVRRecordingsPath::TrimSlashes(const std::string &strString)
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
