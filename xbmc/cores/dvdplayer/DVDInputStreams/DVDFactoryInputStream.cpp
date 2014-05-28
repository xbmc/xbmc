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

#ifndef DVDINPUTSTREAMS_SYSTEM_H_INCLUDED
#define DVDINPUTSTREAMS_SYSTEM_H_INCLUDED
#include "system.h"
#endif

#ifndef DVDINPUTSTREAMS_DVDFACTORYINPUTSTREAM_H_INCLUDED
#define DVDINPUTSTREAMS_DVDFACTORYINPUTSTREAM_H_INCLUDED
#include "DVDFactoryInputStream.h"
#endif

#ifndef DVDINPUTSTREAMS_DVDINPUTSTREAM_H_INCLUDED
#define DVDINPUTSTREAMS_DVDINPUTSTREAM_H_INCLUDED
#include "DVDInputStream.h"
#endif

#ifndef DVDINPUTSTREAMS_DVDINPUTSTREAMFILE_H_INCLUDED
#define DVDINPUTSTREAMS_DVDINPUTSTREAMFILE_H_INCLUDED
#include "DVDInputStreamFile.h"
#endif

#ifndef DVDINPUTSTREAMS_DVDINPUTSTREAMNAVIGATOR_H_INCLUDED
#define DVDINPUTSTREAMS_DVDINPUTSTREAMNAVIGATOR_H_INCLUDED
#include "DVDInputStreamNavigator.h"
#endif

#ifndef DVDINPUTSTREAMS_DVDINPUTSTREAMHTTP_H_INCLUDED
#define DVDINPUTSTREAMS_DVDINPUTSTREAMHTTP_H_INCLUDED
#include "DVDInputStreamHttp.h"
#endif

#ifndef DVDINPUTSTREAMS_DVDINPUTSTREAMFFMPEG_H_INCLUDED
#define DVDINPUTSTREAMS_DVDINPUTSTREAMFFMPEG_H_INCLUDED
#include "DVDInputStreamFFmpeg.h"
#endif

#ifndef DVDINPUTSTREAMS_DVDINPUTSTREAMPVRMANAGER_H_INCLUDED
#define DVDINPUTSTREAMS_DVDINPUTSTREAMPVRMANAGER_H_INCLUDED
#include "DVDInputStreamPVRManager.h"
#endif

#ifndef DVDINPUTSTREAMS_DVDINPUTSTREAMTV_H_INCLUDED
#define DVDINPUTSTREAMS_DVDINPUTSTREAMTV_H_INCLUDED
#include "DVDInputStreamTV.h"
#endif

#ifndef DVDINPUTSTREAMS_DVDINPUTSTREAMRTMP_H_INCLUDED
#define DVDINPUTSTREAMS_DVDINPUTSTREAMRTMP_H_INCLUDED
#include "DVDInputStreamRTMP.h"
#endif

#ifdef HAVE_LIBBLURAY
#include "DVDInputStreamBluray.h"
#endif
#ifdef HAS_FILESYSTEM_HTSP
#include "DVDInputStreamHTSP.h"
#endif
#ifdef ENABLE_DVDINPUTSTREAM_STACK
#include "DVDInputStreamStack.h"
#endif
#ifndef DVDINPUTSTREAMS_FILEITEM_H_INCLUDED
#define DVDINPUTSTREAMS_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif

#ifndef DVDINPUTSTREAMS_STORAGE_MEDIAMANAGER_H_INCLUDED
#define DVDINPUTSTREAMS_STORAGE_MEDIAMANAGER_H_INCLUDED
#include "storage/MediaManager.h"
#endif

#ifndef DVDINPUTSTREAMS_URL_H_INCLUDED
#define DVDINPUTSTREAMS_URL_H_INCLUDED
#include "URL.h"
#endif

#ifndef DVDINPUTSTREAMS_FILESYSTEM_FILE_H_INCLUDED
#define DVDINPUTSTREAMS_FILESYSTEM_FILE_H_INCLUDED
#include "filesystem/File.h"
#endif

#ifndef DVDINPUTSTREAMS_UTILS_URIUTILS_H_INCLUDED
#define DVDINPUTSTREAMS_UTILS_URIUTILS_H_INCLUDED
#include "utils/URIUtils.h"
#endif



CDVDInputStream* CDVDFactoryInputStream::CreateInputStream(IDVDPlayer* pPlayer, const std::string& file, const std::string& content)
{
  CFileItem item(file.c_str(), false);

  if(item.IsDVDImage())
  {
#ifdef HAVE_LIBBLURAY
    CURL url("udf://");
    url.SetHostName(file);
    url.SetFileName("BDMV/index.bdmv");
    if(XFILE::CFile::Exists(url.Get()))
        return new CDVDInputStreamBluray(pPlayer);
#endif

    return new CDVDInputStreamNavigator(pPlayer);
  }

#ifdef HAS_DVD_DRIVE
  if(file.compare(g_mediaManager.TranslateDevicePath("")) == 0)
  {
#ifdef HAVE_LIBBLURAY
    if(XFILE::CFile::Exists(URIUtils::AddFileToFolder(file, "BDMV/index.bdmv")))
        return new CDVDInputStreamBluray(pPlayer);
#endif

    return new CDVDInputStreamNavigator(pPlayer);
  }
#endif

  if (item.IsDVDFile(false, true))
    return (new CDVDInputStreamNavigator(pPlayer));
  else if(file.substr(0, 6) == "pvr://")
    return new CDVDInputStreamPVRManager(pPlayer);
#ifdef HAVE_LIBBLURAY
  else if (item.IsType(".bdmv") || item.IsType(".mpls") || file.substr(0, 7) == "bluray:")
    return new CDVDInputStreamBluray(pPlayer);
#endif
  else if(file.substr(0, 6) == "rtp://"
       || file.substr(0, 7) == "rtsp://"
       || file.substr(0, 6) == "sdp://"
       || file.substr(0, 6) == "udp://"
       || file.substr(0, 6) == "tcp://"
       || file.substr(0, 6) == "mms://"
       || file.substr(0, 7) == "mmst://"
       || file.substr(0, 7) == "mmsh://"
       || (item.IsInternetStream() && item.IsType(".m3u8")))
    return new CDVDInputStreamFFmpeg();
  else if(file.substr(0, 8) == "sling://"
       || file.substr(0, 7) == "myth://"
       || file.substr(0, 8) == "cmyth://"
       || file.substr(0, 8) == "gmyth://"
       || file.substr(0, 6) == "vtp://")
    return new CDVDInputStreamTV();
#ifdef ENABLE_DVDINPUTSTREAM_STACK
  else if(file.substr(0, 8) == "stack://")
    return new CDVDInputStreamStack();
#endif
#ifdef HAS_LIBRTMP
  else if(file.substr(0, 7) == "rtmp://"
       || file.substr(0, 8) == "rtmpt://"
       || file.substr(0, 8) == "rtmpe://"
       || file.substr(0, 9) == "rtmpte://"
       || file.substr(0, 8) == "rtmps://")
    return new CDVDInputStreamRTMP();
#endif
#ifdef HAS_FILESYSTEM_HTSP
  else if(file.substr(0, 7) == "htsp://")
    return new CDVDInputStreamHTSP();
#endif

  // our file interface handles all these types of streams
  return (new CDVDInputStreamFile());
}
