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

#include "system.h"
#include "DVDFactoryInputStream.h"
#include "DVDInputStream.h"
#include "DVDInputStreamFile.h"
#include "DVDInputStreamNavigator.h"
#include "DVDInputStreamHttp.h"
#include "DVDInputStreamFFmpeg.h"
#include "DVDInputStreamPVRManager.h"
#include "DVDInputStreamTV.h"
#include "DVDInputStreamRTMP.h"
#ifdef HAVE_LIBBLURAY
#include "DVDInputStreamBluray.h"
#endif
#ifdef HAS_FILESYSTEM_HTSP
#include "DVDInputStreamHTSP.h"
#endif
#ifdef ENABLE_DVDINPUTSTREAM_STACK
#include "DVDInputStreamStack.h"
#endif
#include "FileItem.h"
#include "storage/MediaManager.h"

CDVDInputStream* CDVDFactoryInputStream::CreateInputStream(IDVDPlayer* pPlayer, const std::string& file, const std::string& content)
{
  CFileItem item(file.c_str(), false);
  if (content != "bluray/iso" && (item.IsDVDFile(false, true) || item.IsDVDImage() ||
#ifdef HAS_DVD_DRIVE
    file.compare(g_mediaManager.TranslateDevicePath("")) == 0 ))
#else
  0 ))
#endif
  {
    return (new CDVDInputStreamNavigator(pPlayer));
  }
  else if(file.substr(0, 6) == "pvr://")
    return new CDVDInputStreamPVRManager(pPlayer);
#ifdef HAVE_LIBBLURAY
  else if (item.IsType(".bdmv") || item.IsType(".mpls") || content == "bluray/iso" || file.substr(0, 7) == "bluray:")
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
