/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayListFactory.h"

#include "FileItem.h"
#include "playlists/PlayListB4S.h"
#include "playlists/PlayListM3U.h"
#include "playlists/PlayListPLS.h"
#include "playlists/PlayListURL.h"
#include "playlists/PlayListWPL.h"
#include "playlists/PlayListXML.h"
#include "playlists/PlayListXSPF.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace PLAYLIST;

CPlayList* CPlayListFactory::Create(const std::string& filename)
{
  CFileItem item(filename,false);
  return Create(item);
}

CPlayList* CPlayListFactory::Create(const CFileItem& item)
{
  if (item.IsInternetStream())
  {
    // Ensure the MIME type has been retrieved for http:// and shout:// streams
    if (item.GetMimeType().empty())
      const_cast<CFileItem&>(item).FillInMimeType();

    std::string strMimeType = item.GetMimeType();
    StringUtils::ToLower(strMimeType);

    if (strMimeType == "video/x-ms-asf"
    || strMimeType == "video/x-ms-asx"
    || strMimeType == "video/x-ms-wmv"
    || strMimeType == "video/x-ms-wma"
    || strMimeType == "video/x-ms-wfs"
    || strMimeType == "video/x-ms-wvx"
    || strMimeType == "video/x-ms-wax")
      return new CPlayListASX();

    if (strMimeType == "audio/x-pn-realaudio")
      return new CPlayListRAM();

    if (strMimeType == "audio/x-scpls"
    || strMimeType == "playlist"
    || strMimeType == "text/html")
      return new CPlayListPLS();

    // online m3u8 files are for hls streaming -- do not treat as playlist
    if (strMimeType == "audio/x-mpegurl" && !item.IsType(".m3u8"))
      return new CPlayListM3U();

    if (strMimeType == "application/vnd.ms-wpl")
      return new CPlayListWPL();

    if (strMimeType == "application/xspf+xml")
      return new CPlayListXSPF();
  }

  std::string path = item.GetDynPath();

  std::string extension = URIUtils::GetExtension(path);
  StringUtils::ToLower(extension);

  if (extension == ".m3u" || (extension == ".m3u8" && !item.IsInternetStream()) || extension == ".strm")
    return new CPlayListM3U();

  if (extension == ".pls")
    return new CPlayListPLS();

  if (extension == ".b4s")
    return new CPlayListB4S();

  if (extension == ".wpl")
    return new CPlayListWPL();

  if (extension == ".asx")
    return new CPlayListASX();

  if (extension == ".ram")
    return new CPlayListRAM();

  if (extension == ".url")
    return new CPlayListURL();

  if (extension == ".pxml")
    return new CPlayListXML();

  if (extension == ".xspf")
    return new CPlayListXSPF();

  return NULL;

}

bool CPlayListFactory::IsPlaylist(const CFileItem& item)
{
  std::string strMimeType = item.GetMimeType();
  StringUtils::ToLower(strMimeType);

/* These are a bit uncertain
  if(strMimeType == "video/x-ms-asf"
  || strMimeType == "video/x-ms-asx"
  || strMimeType == "video/x-ms-wmv"
  || strMimeType == "video/x-ms-wma"
  || strMimeType == "video/x-ms-wfs"
  || strMimeType == "video/x-ms-wvx"
  || strMimeType == "video/x-ms-wax"
  || strMimeType == "video/x-ms-asf")
    return true;
*/

  // online m3u8 files are hls:// -- do not treat as playlist
  if (item.IsInternetStream() && item.IsType(".m3u8"))
    return false;

  if(strMimeType == "audio/x-pn-realaudio"
  || strMimeType == "playlist"
  || strMimeType == "audio/x-mpegurl")
    return true;

  return IsPlaylist(item.GetDynPath());
}

bool CPlayListFactory::IsPlaylist(const CURL& url)
{
  return URIUtils::HasExtension(url,
                                ".m3u|.m3u8|.b4s|.pls|.strm|.wpl|.asx|.ram|.url|.pxml|.xspf|.xsp");
}

bool CPlayListFactory::IsPlaylist(const std::string& filename)
{
  return URIUtils::HasExtension(filename,
                                ".m3u|.m3u8|.b4s|.pls|.strm|.wpl|.asx|.ram|.url|.pxml|.xspf|.xsp");
}

