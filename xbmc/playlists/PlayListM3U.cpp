/*
 *      Copyright (C) 2005-2015 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PlayListM3U.h"
#include "filesystem/File.h"
#include "URL.h"
#include "Util.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"
#include "music/tags/MusicInfoTag.h"

using namespace PLAYLIST;
using namespace XFILE;

const char* CPlayListM3U::StartMarker = "#EXTCPlayListM3U::M3U";
const char* CPlayListM3U::InfoMarker = "#EXTINF";
const char* CPlayListM3U::ArtistMarker = "#EXTART";
const char* CPlayListM3U::AlbumMarker = "#EXTALB";
const char* CPlayListM3U::PropertyMarker = "#KODIPROP";
const char* CPlayListM3U::VLCOptMarker = "#EXTVLCOPT";
const char* CPlayListM3U::StreamMarker = "#EXT-X-STREAM-INF";
const char* CPlayListM3U::BandwidthMarker = "BANDWIDTH";
const char* CPlayListM3U::OffsetMarker = "#EXT-KX-OFFSET";

// example m3u file:
//   #EXTM3U
//   #EXTART:Demo Artist
//   #EXTALB:Demo Album
//   #KODIPROP:name=value
//   #EXTINF:5,demo
//   E:\Program Files\Winamp3\demo.mp3



// example m3u8 containing streams of different bitrates
//   #EXTM3U
//   #EXT-X-STREAM-INF:PROGRAM-ID=1, BANDWIDTH=1600000
//   playlist_1600.m3u8
//   #EXT-X-STREAM-INF:PROGRAM-ID=1, BANDWIDTH=3000000
//   playlist_3000.m3u8
//   #EXT-X-STREAM-INF:PROGRAM-ID=1, BANDWIDTH=800000
//   playlist_800.m3u8


CPlayListM3U::CPlayListM3U(void)
{}

CPlayListM3U::~CPlayListM3U(void)
{}


bool CPlayListM3U::Load(const std::string& strFileName)
{
  char szLine[4096];
  std::string strLine;
  std::string strInfo;
  std::vector<std::pair<std::string, std::string> > properties;

  long lDuration = 0;
  int iStartOffset = 0;
  int iEndOffset = 0;

  Clear();

  m_strPlayListName = URIUtils::GetFileName(strFileName);
  URIUtils::GetParentPath(strFileName, m_strBasePath);

  CFile file;
  if (!file.Open(strFileName) )
  {
    file.Close();
    return false;
  }

  while (file.ReadString(szLine, 4095))
  {
    strLine = szLine;
    StringUtils::Trim(strLine);

    if (StringUtils::StartsWith(strLine, InfoMarker))
    {
      // start of info
      size_t iColon = strLine.find(":");
      size_t iComma = strLine.find(",");
      if (iColon != std::string::npos &&
          iComma != std::string::npos &&
          iComma > iColon)
      {
        // Read the info and duration
        iColon++;
        std::string strLength = strLine.substr(iColon, iComma - iColon);
        lDuration = atoi(strLength.c_str());
        iComma++;
        strInfo = strLine.substr(iComma);
        g_charsetConverter.unknownToUTF8(strInfo);
      }
    }
    else if (StringUtils::StartsWith(strLine, OffsetMarker))
    {
      size_t iColon = strLine.find(":");
      size_t iComma = strLine.find(",");
      if (iColon != std::string::npos &&
        iComma != std::string::npos &&
        iComma > iColon)
      {
        // Read the start and end offset
        iColon++;
        iStartOffset = atoi(strLine.substr(iColon, iComma - iColon).c_str());
        iComma++;
        iEndOffset = atoi(strLine.substr(iComma).c_str());
      }
    }
    else if (StringUtils::StartsWith(strLine, PropertyMarker)
    || StringUtils::StartsWith(strLine, VLCOptMarker))
    {
      size_t iColon = strLine.find(":");
      size_t iEqualSign = strLine.find("=");
      if (iColon != std::string::npos &&
        iEqualSign != std::string::npos &&
        iEqualSign > iColon)
      {
        std::string strFirst, strSecond;
        properties.push_back(std::make_pair(
          StringUtils::Trim((strFirst = strLine.substr(iColon+1, iEqualSign - iColon -1))),
          StringUtils::Trim((strSecond = strLine.substr(iEqualSign +1))))
          );
      }
    }
    else if (strLine != StartMarker &&
             !StringUtils::StartsWith(strLine, ArtistMarker) &&
             !StringUtils::StartsWith(strLine, AlbumMarker))
    {
      std::string strFileName = strLine;

      if (!strFileName.empty() && strFileName[0] == '#')
        continue; // assume a comment or something else we don't support

      // Skip self - do not load playlist recursively
      // We compare case-less in case user has input incorrect case of the current playlist
      if (StringUtils::EqualsNoCase(URIUtils::GetFileName(strFileName), m_strPlayListName))
        continue;

      if (strFileName.length() > 0)
      {
        g_charsetConverter.unknownToUTF8(strFileName);

        // If no info was read from from the extended tag information, use the file name
        if (strInfo.length() == 0)
        {
          strInfo = URIUtils::GetFileName(strFileName);
        }

        // should substitition occur befor or after charset conversion??
        strFileName = URIUtils::SubstitutePath(strFileName);

        // Get the full path file name and add it to the the play list
        CUtil::GetQualifiedFilename(m_strBasePath, strFileName);
        CFileItemPtr newItem(new CFileItem(strInfo));
        newItem->SetPath(strFileName);
        if (iStartOffset != 0 || iEndOffset != 0)
        {
          newItem->m_lStartOffset = iStartOffset;
          newItem->m_lStartPartNumber = 1;
          newItem->SetProperty("item_start", iStartOffset);
          newItem->m_lEndOffset = iEndOffset;
          // Prevent load message from file and override offset set here
          newItem->GetMusicInfoTag()->SetLoaded();
          newItem->GetMusicInfoTag()->SetTitle(strInfo);
          if (iEndOffset)
            lDuration = (iEndOffset - iStartOffset + 37) / 75;
        }
        if (newItem->IsVideo() && !newItem->HasVideoInfoTag()) // File is a video and needs a VideoInfoTag
          newItem->GetVideoInfoTag()->Reset(); // Force VideoInfoTag creation
        if (lDuration && newItem->IsAudio())
          newItem->GetMusicInfoTag()->SetDuration(lDuration);
        for (auto &prop : properties)
        {
          newItem->SetProperty(prop.first, prop.second);
        }
        Add(newItem);

        // Reset the values just in case there part of the file have the extended marker
        // and part don't
        strInfo = "";
        lDuration = 0;
        iStartOffset = 0;
        iEndOffset = 0;
        properties.clear();
      }
    }
  }

  file.Close();
  return true;
}

void CPlayListM3U::Save(const std::string& strFileName) const
{
  if (!m_vecItems.size())
    return;
  std::string strPlaylist = CUtil::MakeLegalPath(strFileName);
  CFile file;
  if (!file.OpenForWrite(strPlaylist,true))
  {
    CLog::Log(LOGERROR, "Could not save M3U playlist: [%s]", strPlaylist.c_str());
    return;
  }
  std::string strLine = StringUtils::Format("%s\n",StartMarker);
  if (file.Write(strLine.c_str(), strLine.size()) != static_cast<ssize_t>(strLine.size()))
    return; // error

  for (int i = 0; i < (int)m_vecItems.size(); ++i)
  {
    CFileItemPtr item = m_vecItems[i];
    std::string strDescription=item->GetLabel();
    g_charsetConverter.utf8ToStringCharset(strDescription);
    strLine = StringUtils::Format( "%s:%i,%s\n", InfoMarker, item->GetMusicInfoTag()->GetDuration() / 1000, strDescription.c_str() );
    if (file.Write(strLine.c_str(), strLine.size()) != static_cast<ssize_t>(strLine.size()))
      return; // error
    if (item->m_lStartOffset != 0 || item->m_lEndOffset != 0)
    {
      strLine = StringUtils::Format("%s:%i,%i\n", OffsetMarker, item->m_lStartOffset, item->m_lEndOffset);
      file.Write(strLine.c_str(),strLine.size());
    }
    std::string strFileName = ResolveURL(item);
    g_charsetConverter.utf8ToStringCharset(strFileName);
    strLine = StringUtils::Format("%s\n",strFileName.c_str());
    if (file.Write(strLine.c_str(), strLine.size()) != static_cast<ssize_t>(strLine.size()))
      return; // error
  }
  file.Close();
}

std::map< std::string, std::string > CPlayListM3U::ParseStreamLine(const std::string &streamLine)
{
  std::map< std::string, std::string > params;

  // ensure the line has something beyond the stream marker and ':'
  if (streamLine.size() < strlen(StreamMarker) + 2)
    return params;

  // get the actual params following the :
  std::string strParams(streamLine.substr(strlen(StreamMarker) + 1));

  // separate the parameters
  std::vector<std::string> vecParams = StringUtils::Split(strParams, ",");
  for (std::vector<std::string>::iterator i = vecParams.begin(); i != vecParams.end(); ++i)
  {
    // split the param, ensure there was an =
    StringUtils::Trim(*i);
    std::vector<std::string> vecTuple = StringUtils::Split(*i, "=");
    if (vecTuple.size() < 2)
      continue;

    // remove white space from name and value and store it in the dictionary
    StringUtils::Trim(vecTuple[0]);
    StringUtils::Trim(vecTuple[1]);
    params[vecTuple[0]] = vecTuple[1];
  }

  return params;
}

