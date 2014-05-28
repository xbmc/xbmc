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

#ifndef DVDDEMUXERS_SYSTEM_H_INCLUDED
#define DVDDEMUXERS_SYSTEM_H_INCLUDED
#include "system.h"
#endif

#ifndef DVDDEMUXERS_DVDFACTORYDEMUXER_H_INCLUDED
#define DVDDEMUXERS_DVDFACTORYDEMUXER_H_INCLUDED
#include "DVDFactoryDemuxer.h"
#endif


#ifndef DVDDEMUXERS_DVDINPUTSTREAMS_DVDINPUTSTREAM_H_INCLUDED
#define DVDDEMUXERS_DVDINPUTSTREAMS_DVDINPUTSTREAM_H_INCLUDED
#include "DVDInputStreams/DVDInputStream.h"
#endif

#ifndef DVDDEMUXERS_DVDINPUTSTREAMS_DVDINPUTSTREAMHTTP_H_INCLUDED
#define DVDDEMUXERS_DVDINPUTSTREAMS_DVDINPUTSTREAMHTTP_H_INCLUDED
#include "DVDInputStreams/DVDInputStreamHttp.h"
#endif

#ifndef DVDDEMUXERS_DVDINPUTSTREAMS_DVDINPUTSTREAMPVRMANAGER_H_INCLUDED
#define DVDDEMUXERS_DVDINPUTSTREAMS_DVDINPUTSTREAMPVRMANAGER_H_INCLUDED
#include "DVDInputStreams/DVDInputStreamPVRManager.h"
#endif


#ifndef DVDDEMUXERS_DVDDEMUXFFMPEG_H_INCLUDED
#define DVDDEMUXERS_DVDDEMUXFFMPEG_H_INCLUDED
#include "DVDDemuxFFmpeg.h"
#endif

#ifndef DVDDEMUXERS_DVDDEMUXSHOUTCAST_H_INCLUDED
#define DVDDEMUXERS_DVDDEMUXSHOUTCAST_H_INCLUDED
#include "DVDDemuxShoutcast.h"
#endif

#ifdef HAS_FILESYSTEM_HTSP
#include "DVDDemuxHTSP.h"
#endif
#ifndef DVDDEMUXERS_DVDDEMUXBXA_H_INCLUDED
#define DVDDEMUXERS_DVDDEMUXBXA_H_INCLUDED
#include "DVDDemuxBXA.h"
#endif

#ifndef DVDDEMUXERS_DVDDEMUXCDDA_H_INCLUDED
#define DVDDEMUXERS_DVDDEMUXCDDA_H_INCLUDED
#include "DVDDemuxCDDA.h"
#endif

#ifndef DVDDEMUXERS_DVDDEMUXPVRCLIENT_H_INCLUDED
#define DVDDEMUXERS_DVDDEMUXPVRCLIENT_H_INCLUDED
#include "DVDDemuxPVRClient.h"
#endif

#ifndef DVDDEMUXERS_PVR_PVRMANAGER_H_INCLUDED
#define DVDDEMUXERS_PVR_PVRMANAGER_H_INCLUDED
#include "pvr/PVRManager.h"
#endif

#ifndef DVDDEMUXERS_PVR_ADDONS_PVRCLIENTS_H_INCLUDED
#define DVDDEMUXERS_PVR_ADDONS_PVRCLIENTS_H_INCLUDED
#include "pvr/addons/PVRClients.h"
#endif


using namespace std;
using namespace PVR;

CDVDDemux* CDVDFactoryDemuxer::CreateDemuxer(CDVDInputStream* pInputStream)
{
  if (!pInputStream)
    return NULL;

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
  
  // Try to open CDDA demuxer
  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_FILE) && pInputStream->GetContent().compare("application/octet-stream") == 0)
  {
    std::string filename = pInputStream->GetFileName();
    if (filename.substr(0, 7) == "cdda://")
    {
      CLog::Log(LOGDEBUG, "DVDFactoryDemuxer: Stream is probably CD audio. Creating CDDA demuxer.");

      auto_ptr<CDVDDemuxCDDA> demuxer(new CDVDDemuxCDDA());
      if (demuxer->Open(pInputStream))
      {
        return demuxer.release();
      }
    }
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

