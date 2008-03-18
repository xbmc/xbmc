/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "PlayListFactory.h"
#include "PlayListM3U.h"
#include "PlayListPLS.h"
#include "PlayListB4S.h"
#include "PlayListWPL.h"
#include "Util.h"


using namespace PLAYLIST;


CPlayList* CPlayListFactory::Create(const CStdString& filename)
{
  CFileItem item(filename,false);
  return Create(item);
}

CPlayList* CPlayListFactory::Create(const CFileItem& item)
{
  if(item.IsLastFM()) //lastfm is always a stream, and just silly to check content
    return NULL;

  if( item.IsInternetStream() )
  {
    CStdString strContentType = item.GetContentType();
    strContentType.MakeLower();

    if (strContentType == "video/x-ms-asf"
    || strContentType == "video/x-ms-asx"
    || strContentType == "video/x-ms-wmv"
    || strContentType == "video/x-ms-wma"
    || strContentType == "video/x-ms-wfs"
    || strContentType == "video/x-ms-wvx"
    || strContentType == "video/x-ms-wax")
      return new CPlayListASX();

    if (strContentType == "audio/x-pn-realaudio")
      return new CPlayListRAM();

    if (strContentType == "audio/x-scpls"
    || strContentType == "playlist"
    || strContentType == "text/html")
      return new CPlayListPLS();

    if (strContentType == "audio/x-mpegurl")
      return new CPlayListM3U();

    if (strContentType == "application/vnd.ms-wpl")
      return new CPlayListWPL();
  }

  CStdString extension = CUtil::GetExtension(item.m_strPath);
  extension.MakeLower();

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

  return NULL;

}

bool CPlayListFactory::IsPlaylist(const CStdString& filename)
{
  CStdString extension = CUtil::GetExtension(filename);
  extension.ToLower();

  if (extension == ".m3u") return true;
  if (extension == ".b4s") return true;
  if (extension == ".pls") return true;
  if (extension == ".strm") return true;
  if (extension == ".wpl") return true;
  if (extension == ".asx") return true;
  if (extension == ".ram") return true;
  if (extension == ".xsp") return true;
  return false;
}

