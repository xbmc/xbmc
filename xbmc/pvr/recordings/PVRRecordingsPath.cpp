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

const std::string CPVRRecordingsPath::PATH_RECORDINGS         = "pvr://recordings/";
const std::string CPVRRecordingsPath::PATH_ACTIVE_RECORDINGS  = "pvr://recordings/active/";
const std::string CPVRRecordingsPath::PATH_DELETED_RECORDINGS = "pvr://recordings/deleted/";

CPVRRecordingsPath::CPVRRecordingsPath(const std::string &strPath)
{
  Init(strPath);
}

CPVRRecordingsPath::CPVRRecordingsPath(bool bDeleted)
: m_bValid(true),
  m_bRoot(true),
  m_bActive(!bDeleted),
  m_path(StringUtils::Format("pvr://recordings/%s/", bDeleted ? "deleted" : "active"))
{
}

CPVRRecordingsPath::CPVRRecordingsPath(bool bDeleted,
                       const std::string &strDirectory, const std::string &strTitle,
                       int iSeason, int iEpisode, int iYear,
                       const std::string &strSubtitle, const std::string &strChannelName,
                       const CDateTime &recordingTime)
: m_bValid(true),
  m_bRoot(false),
  m_bActive(!bDeleted)
{
  std::string strDirectoryN;
  if (!strDirectory.empty() && strDirectory != "/")
    strDirectoryN = StringUtils::Format("%s/", strDirectory.c_str());

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
  m_params = StringUtils::Format(", TV%s, %s.pvr", strChannelNameN.c_str(), recordingTime.GetAsSaveString().c_str());
  m_path   = StringUtils::Format("pvr://recordings/%s/%s%s", bDeleted ? "deleted" : "active", m_directoryPath.c_str(), m_params.c_str());
}

void CPVRRecordingsPath::Init(const std::string &strPath)
{
  std::string strVarPath(strPath);
  URIUtils::RemoveSlashAtEnd(strVarPath);

  const std::vector<std::string> segments = URIUtils::SplitPath(strVarPath);

  m_bValid  = ((segments.size() >= 3) && // at least pvr://recordings/[active|deleted]
               (segments.at(1) == "recordings") &&
               ((segments.at(2) == "active") || (segments.at(2) == "deleted")));
  m_bRoot   = (m_bValid && (segments.size() == 3));
  m_bActive = (m_bValid && (segments.at(2) == "active"));

  if (m_bRoot)
    strVarPath.append("/");
  else
  {
    size_t paramStart = m_path.find(", TV");
    if (paramStart == std::string::npos)
      m_directoryPath = strVarPath.substr(m_bActive ? PATH_ACTIVE_RECORDINGS.size(): PATH_DELETED_RECORDINGS.size());
    else
    {
      size_t dirStart = m_bActive ? PATH_ACTIVE_RECORDINGS.size(): PATH_DELETED_RECORDINGS.size();
      m_directoryPath = strVarPath.substr(dirStart, paramStart - dirStart);
      m_params = strVarPath.substr(paramStart);
    }
  }

  m_path = strVarPath;
}

std::string CPVRRecordingsPath::GetSubDirectoryPath(const std::string &strPath) const
{
  std::string strReturn;

  std::string strUsePath(strPath);
  while (!strUsePath.empty() && strUsePath.at(0) == '/')
    strUsePath.erase(0, 1);

  URIUtils::RemoveSlashAtEnd(strUsePath);

  /* adding "/" to make sure that base matches the complete folder name and not only parts of it */
  if (!m_directoryPath.empty() && (strUsePath.size() <= m_directoryPath.size() || !StringUtils::StartsWith(strUsePath, m_directoryPath + "/")))
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
                    "(19[0-9][0-9]|20[0-9][0-9])[0-9][0-9][0-9][0-9]_[0-9][0-9][0-9][0-9][0-9][0-9].pvr"))
    {
      if (reg.RegFind(m_path.c_str()) >= 0)
        return reg.GetMatch(2);
    }
  }
  return "";
}

void CPVRRecordingsPath::AppendSegment(const std::string &strSegment)
{
  m_directoryPath += strSegment;

  size_t paramStart = m_path.find(", TV");
  if (paramStart == std::string::npos)
  {
    // append the segment
    m_path += strSegment;
  }
  else
  {
    // insert the segment between end of current directory path and parameters
    m_path.insert(paramStart, strSegment);
  }
}
