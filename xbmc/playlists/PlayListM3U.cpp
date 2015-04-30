/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "PlayListM3U.h"
#include "filesystem/File.h"
#include "URL.h"
#include "Util.h"
#include "utils/StringUtils.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"
#include "music/tags/MusicInfoTag.h"

using namespace PLAYLIST;
using namespace XFILE;

#define M3U_START_MARKER "#EXTM3U"
#define M3U_INFO_MARKER  "#EXTINF"
#define M3U_ARTIST_MARKER  "#EXTART"
#define M3U_ALBUM_MARKER  "#EXTALB"
#define M3U_STREAM_MARKER  "#EXT-X-STREAM-INF"
#define M3U_BANDWIDTH_MARKER  "BANDWIDTH"
#define M3U_OFFSET_MARKER  "#EXT-KX-OFFSET"

// example m3u file:
//   #EXTM3U
//   #EXTART:Demo Artist
//   #EXTALB:Demo Album
//   #EXTINF:5,demo
//   E:\Program Files\Winamp3\demo.mp3
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

    if (StringUtils::StartsWith(strLine, M3U_INFO_MARKER))
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
    else if (StringUtils::StartsWith(strLine, M3U_OFFSET_MARKER))
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
    else if (strLine != M3U_START_MARKER &&
             !StringUtils::StartsWith(strLine, M3U_ARTIST_MARKER) &&
             !StringUtils::StartsWith(strLine, M3U_ALBUM_MARKER))
    {
      std::string strFileName = strLine;

      if (strFileName.size() > 0 && strFileName[0] == '#')
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
        Add(newItem);

        // Reset the values just in case there part of the file have the extended marker
        // and part don't
        strInfo = "";
        lDuration = 0;
        iStartOffset = 0;
        iEndOffset = 0;
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
  std::string strLine = StringUtils::Format("%s\n",M3U_START_MARKER);
  if (file.Write(strLine.c_str(), strLine.size()) != static_cast<ssize_t>(strLine.size()))
    return; // error

  for (int i = 0; i < (int)m_vecItems.size(); ++i)
  {
    CFileItemPtr item = m_vecItems[i];
    std::string strDescription=item->GetLabel();
    g_charsetConverter.utf8ToStringCharset(strDescription);
    strLine = StringUtils::Format( "%s:%i,%s\n", M3U_INFO_MARKER, item->GetMusicInfoTag()->GetDuration() / 1000, strDescription.c_str() );
    if (file.Write(strLine.c_str(), strLine.size()) != static_cast<ssize_t>(strLine.size()))
      return; // error
    if (item->m_lStartOffset != 0 || item->m_lEndOffset != 0)
    {
      strLine = StringUtils::Format("%s:%i,%i\n", M3U_OFFSET_MARKER, item->m_lStartOffset, item->m_lEndOffset);
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

std::string CPlayListM3U::GetBestBandwidthStream(const std::string &strFileName, size_t bandwidth)
{
  // we may be passed a playlist that does not contain playlists of different
  // bitrates (eg: this playlist is really the HLS video). So, default the
  // return to the filename so it can be played
  char szLine[4096];
  std::string strLine;
  size_t maxBandwidth = 0;

  // open the file, and if it fails, return
  CFile file;
  if (!file.Open(strFileName) )
  {
    file.Close();
    return strFileName;
  }

  // get protocol options if they were set, so we can restore them again at the end
  CURL playlistUrl(strFileName);
  
  // and set the fallback value
  CURL subStreamUrl = CURL(strFileName);
  
  // determine the base
  CURL basePlaylistUrl(URIUtils::GetParentPath(strFileName));
  basePlaylistUrl.SetOptions("");
  basePlaylistUrl.SetProtocolOptions("");
  std::string basePart = basePlaylistUrl.Get();

  // convert bandwidth specified in kbps to bps used by the m3u8
  bandwidth *= 1000;

  while (file.ReadString(szLine, 1024))
  {
    // read and trim a line
    strLine = szLine;
    StringUtils::Trim(strLine);

    // skip the first line
    if (strLine == M3U_START_MARKER)
        continue;
    else if (StringUtils::StartsWith(strLine, M3U_STREAM_MARKER))
    {
      // parse the line so we can pull out the bandwidth
      std::map< std::string, std::string > params = ParseStreamLine(strLine);
      std::map< std::string, std::string >::iterator it = params.find(M3U_BANDWIDTH_MARKER);

      if (it != params.end())
      {
        size_t streamBandwidth = atoi(it->second.c_str());
        if ((maxBandwidth < streamBandwidth) && (streamBandwidth <= bandwidth))
        {
          // read the next line
          if (!file.ReadString(szLine, 1024))
            continue;

          strLine = szLine;
          StringUtils::Trim(strLine);

          // this line was empty
          if (strLine.empty())
            continue;

          // store the max bandwidth
          maxBandwidth = streamBandwidth;

          // if the path is absolute just use it
          if (CURL::IsFullPath(strLine))
            subStreamUrl = CURL(strLine);
          else
            subStreamUrl = CURL(basePart + strLine);
        }
      }
    }
  }

  // if any protocol options were set, restore them
  subStreamUrl.SetProtocolOptions(playlistUrl.GetProtocolOptions());
  return subStreamUrl.Get();
}

std::map< std::string, std::string > CPlayListM3U::ParseStreamLine(const std::string &streamLine)
{
  std::map< std::string, std::string > params;

  // ensure the line has something beyond the stream marker and ':'
  if (streamLine.size() < strlen(M3U_STREAM_MARKER) + 2)
    return params;

  // get the actual params following the :
  std::string strParams(streamLine.substr(strlen(M3U_STREAM_MARKER) + 1));

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

