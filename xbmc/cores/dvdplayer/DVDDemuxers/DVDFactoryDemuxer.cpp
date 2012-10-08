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
#include "DVDFactoryDemuxer.h"

#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDInputStreamHttp.h"
#include "DVDInputStreams/DVDInputStreamPVRManager.h"

#include "DVDDemuxFFmpeg.h"
#include "DVDDemuxShoutcast.h"
#ifdef HAS_FILESYSTEM_HTSP
#include "DVDDemuxHTSP.h"
#endif
#include "DVDDemuxBXA.h"
#include "DVDDemuxPVRClient.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"

using namespace std;
using namespace PVR;

CDVDDemux* CDVDFactoryDemuxer::CreateDemuxer(CDVDInputStream* pInputStream)
{
  // Try to open the AirTunes demuxer
  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_FILE) && pInputStream->GetContent().compare("audio/x-xbmc-pcm") == 0 )
  {
    // audio/x-xbmc-pcm this is the used codec for AirTunes
    // (apples audio only streaming)
    auto_ptr<CDVDDemuxBXA> demuxer(new CDVDDemuxBXA());
    if(demuxer->Open(pInputStream))
      return demuxer.release();
    else
      return NULL;
  }

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_HTTP))
  {
    CDVDInputStreamHttp* pHttpStream = (CDVDInputStreamHttp*)pInputStream;
    CHttpHeader *header = pHttpStream->GetHttpHeader();

    /* check so we got the meta information as requested in our http header */
    if( header->GetValue("icy-metaint").length() > 0 )
    {
      auto_ptr<CDVDDemuxShoutcast> demuxer(new CDVDDemuxShoutcast());
      if(demuxer->Open(pInputStream))
        return demuxer.release();
      else
        return NULL;
    }
  }

#ifdef HAS_FILESYSTEM_HTSP
  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_HTSP))
  {
    auto_ptr<CDVDDemuxHTSP> demuxer(new CDVDDemuxHTSP());
    if(demuxer->Open(pInputStream))
      return demuxer.release();
    else
      return NULL;
  }
#endif

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER))
  {
    CDVDInputStreamPVRManager* pInputStreamPVR = (CDVDInputStreamPVRManager*)pInputStream;
    CDVDInputStream* pOtherStream = pInputStreamPVR->GetOtherStream();
    if(pOtherStream)
    {
      /* Used for MediaPortal PVR addon (uses PVR otherstream for playback of rtsp streams) */
      if (pOtherStream->IsStreamType(DVDSTREAM_TYPE_FFMPEG))
      {
        auto_ptr<CDVDDemuxFFmpeg> demuxer(new CDVDDemuxFFmpeg());
        if(demuxer->Open(pOtherStream))
          return demuxer.release();
        else
          return NULL;
      }
    }

    std::string filename = pInputStream->GetFileName();
    /* Use PVR demuxer only for live streams */
    if (filename.substr(0, 14) == "pvr://channels")
    {
      boost::shared_ptr<CPVRClient> client;
      if (g_PVRClients->GetPlayingClient(client) &&
          client->HandlesDemuxing())
      {
        auto_ptr<CDVDDemuxPVRClient> demuxer(new CDVDDemuxPVRClient());
        if(demuxer->Open(pInputStream))
          return demuxer.release();
        else
          return NULL;
      }
    }
  }

  auto_ptr<CDVDDemuxFFmpeg> demuxer(new CDVDDemuxFFmpeg());
  if(demuxer->Open(pInputStream))
    return demuxer.release();
  else
    return NULL;
}

