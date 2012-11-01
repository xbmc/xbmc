/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
#include "utils/RegExp.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "settings/AdvancedSettings.h"
#include "music/tags/MusicInfoTag.h"

using namespace PLAYLIST;
using namespace XFILE;

#define M3U_START_MARKER "#EXTM3U"
#define M3U_INFO_MARKER  "#EXTINF"
#define M3U_ARTIST_MARKER  "#EXTART"
#define M3U_ALBUM_MARKER  "#EXTALB"
#define M3U_STREAM_MARKER  "#EXT-X-STREAM-INF"
#define M3U_BANDWIDTH_MARKER  "BANDWIDTH"

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


bool CPlayListM3U::Load(const CStdString& strFileName)
{
  char szLine[4096];
  CStdString strLine;
  CStdString strInfo = "";
  long lDuration = 0;

  Clear();

  m_strPlayListName = URIUtils::GetFileName(strFileName);
  URIUtils::GetParentPath(strFileName, m_strBasePath);

  CFile file;
  if (!file.Open(strFileName) )
  {
    file.Close();
    return false;
  }

  while (file.ReadString(szLine, 1024))
  {
    strLine = szLine;
    strLine.TrimRight(" \t\r\n");
    strLine.TrimLeft(" \t");

    if (strLine.Left( (int)strlen(M3U_INFO_MARKER) ) == M3U_INFO_MARKER)
    {
      // start of info
      int iColon = (int)strLine.find(":");
      int iComma = (int)strLine.find(",");
      if (iColon >= 0 && iComma >= 0 && iComma > iColon)
      {
        // Read the info and duration
        iColon++;
        CStdString strLength = strLine.Mid(iColon, iComma - iColon);
        lDuration = atoi(strLength.c_str());
        iComma++;
        strInfo = strLine.Right((int)strLine.size() - iComma);
        g_charsetConverter.unknownToUTF8(strInfo);
      }
    }
    else if (strLine != M3U_START_MARKER && strLine.Left(strlen(M3U_ARTIST_MARKER)) != M3U_ARTIST_MARKER && strLine.Left(strlen(M3U_ALBUM_MARKER)) != M3U_ALBUM_MARKER )
    {
      CStdString strFileName = strLine;

      if (strFileName.size() > 0 && strFileName[0] == '#')
        continue; // assume a comment or something else we don't support

      // Skip self - do not load playlist recursively
      if (URIUtils::GetFileName(strFileName).Equals(m_strPlayListName))
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
        if (lDuration && newItem->IsAudio())
          newItem->GetMusicInfoTag()->SetDuration(lDuration);
        Add(newItem);

        // Reset the values just in case there part of the file have the extended marker
        // and part don't
        strInfo = "";
        lDuration = 0;
      }
    }
  }

  file.Close();
  return true;
}

void CPlayListM3U::Save(const CStdString& strFileName) const
{
  if (!m_vecItems.size())
    return;
  CStdString strPlaylist = CUtil::MakeLegalPath(strFileName);
  CFile file;
  if (!file.OpenForWrite(strPlaylist,true))
  {
    CLog::Log(LOGERROR, "Could not save M3U playlist: [%s]", strPlaylist.c_str());
    return;
  }
  CStdString strLine;
  strLine.Format("%s\n",M3U_START_MARKER);
  file.Write(strLine.c_str(),strLine.size());
  for (int i = 0; i < (int)m_vecItems.size(); ++i)
  {
    CFileItemPtr item = m_vecItems[i];
    CStdString strDescription=item->GetLabel();
    g_charsetConverter.utf8ToStringCharset(strDescription);
    strLine.Format( "%s:%i,%s\n", M3U_INFO_MARKER, item->GetMusicInfoTag()->GetDuration() / 1000, strDescription.c_str() );
    file.Write(strLine.c_str(),strLine.size());
    CStdString strFileName = ResolveURL(item);
    g_charsetConverter.utf8ToStringCharset(strFileName);
    strLine.Format("%s\n",strFileName.c_str());
    file.Write(strLine.c_str(),strLine.size());
  }
  file.Close();
}

CStdString CPlayListM3U::GetBestBandwidthStream(const CStdString &strFileName, size_t bandwidth)
{
  // we may be passed a playlist that does not contain playlists of different
  // bitrates (eg: this playlist is really the HLS video). So, default the
  // return to the filename so it can be played
  char szLine[4096];
  CStdString strLine;
  CStdString strPlaylist = strFileName;
  size_t maxBandwidth = 0;

  // if we cannot get the last / we wont be able to determine the sub-playlists
  size_t baseEnd = strPlaylist.rfind('/');
  if (baseEnd == std::string::npos)
    return strPlaylist;

  // store the base path (the path without the filename)
  CStdString basePath = strPlaylist.substr(0, baseEnd + 1);

  // open the file, and if it fails, return
  CFile file;
  if (!file.Open(strFileName) )
  {
    file.Close();
    return strPlaylist;
  }

  // convert bandwidth specified in kbps to bps used by the m3u8
  bandwidth *= 1000;

  while (file.ReadString(szLine, 1024))
  {
    // read and trim a line
    strLine = szLine;
    strLine.TrimRight(" \t\r\n");
    strLine.TrimLeft(" \t");

    // skip the first line
    if (strLine == M3U_START_MARKER)
        continue;
    else if (strLine.Left(strlen(M3U_STREAM_MARKER)) == M3U_STREAM_MARKER)
    {
      // parse the line so we can pull out the bandwidth
      std::map< CStdString, CStdString > params = ParseStreamLine(strLine);
      std::map< CStdString, CStdString >::iterator it = params.find(M3U_BANDWIDTH_MARKER);

      if (it != params.end())
      {
        size_t streamBandwidth = atoi(it->second.c_str());
        if ((maxBandwidth < streamBandwidth) && (streamBandwidth <= bandwidth))
        {
          // read the next line
          if (!file.ReadString(szLine, 1024))
            continue;

          strLine = szLine;
          strLine.TrimRight(" \t\r\n");
          strLine.TrimLeft(" \t");

          // this line was empty
          if (strLine.empty())
            continue;

          // store the max bandwidth
          maxBandwidth = streamBandwidth;

          // if the path is absolute just use it
          if (CURL::IsFullPath(strLine))
            strPlaylist = strLine;
          else
            strPlaylist = basePath + strLine;
        }
      }
    }
  }

  CLog::Log(LOGINFO, "Auto-selecting %s based on configured bandwidth.", strPlaylist.c_str());

  return strPlaylist;
}

std::map< CStdString, CStdString > CPlayListM3U::ParseStreamLine(const CStdString &streamLine)
{
  std::map< CStdString, CStdString > params;

  // ensure the line has something beyond the stream marker and ':'
  if (strlen(streamLine) < strlen(M3U_STREAM_MARKER) + 2)
    return params;

  // get the actual params following the :
  CStdString strParams(streamLine.substr(strlen(M3U_STREAM_MARKER) + 1));

  // separate the parameters
  CStdStringArray vecParams = StringUtils::SplitString(strParams, ",");
  for (size_t i = 0; i < vecParams.size(); i++)
  {
    // split the param, ensure there was an =
    CStdStringArray vecTuple = StringUtils::SplitString(vecParams[i].Trim(), "=");
    if (vecTuple.size() < 2)
      continue;

    // remove white space from name and value and store it in the dictionary
    params[vecTuple[0].Trim()] = vecTuple[1].Trim();
  }

  return params;
}

