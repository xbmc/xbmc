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

#include "PlayListFactory.h"
#include "PlayListM3U.h"
#include "PlayListPLS.h"
#include "PlayListB4S.h"
#include "PlayListWPL.h"
#include "PlayListURL.h"
#include "PlayListXML.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"

using namespace PLAYLIST;

CPlayList* CPlayListFactory::Create(const std::string& filename)
{
  CFileItem item(filename,false);
  return Create(item);
}

CPlayList* CPlayListFactory::Create(const CFileItem& item)
{
  if( item.IsInternetStream() )
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
  }

  std::string extension = URIUtils::GetExtension(item.GetPath());
  StringUtils::ToLower(extension);

  if (extension == ".m3u" || extension == ".strm")
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

  return NULL;

}

bool CPlayListFactory::IsPlaylist(const CFileItem& item)
{
  std::string strMimeType = item.GetMimeType();
  StringUtils::ToLower(strMimeType);

/* These are abit uncertain 
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

  return IsPlaylist(item.GetPath());
}

bool CPlayListFactory::IsPlaylist(const CURL& url)
{
  return URIUtils::HasExtension(url,
                                ".m3u|.b4s|.pls|.strm|.wpl|.asx|.ram|.url|.pxml");
}

bool CPlayListFactory::IsPlaylist(const std::string& filename)
{
  return URIUtils::HasExtension(filename,
                     ".m3u|.b4s|.pls|.strm|.wpl|.asx|.ram|.url|.pxml");
}

