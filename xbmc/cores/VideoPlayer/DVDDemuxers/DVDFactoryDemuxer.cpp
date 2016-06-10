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

#include "system.h"
#include "DVDFactoryDemuxer.h"

#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDInputStreamPVRManager.h"

#include "DVDDemuxFFmpeg.h"
#include "DVDDemuxBXA.h"
#include "DVDDemuxCDDA.h"
#include "DVDDemuxClient.h"
#include "DemuxMultiSource.h"
#include "pvr/PVRManager.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace PVR;

CDVDDemux* CDVDFactoryDemuxer::CreateDemuxer(CDVDInputStream* pInputStream, bool fileinfo)
{
  if (!pInputStream)
    return NULL;

  // Try to open the AirTunes demuxer
  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_FILE) && pInputStream->GetContent().compare("audio/x-xbmc-pcm") == 0 )
  {
    // audio/x-xbmc-pcm this is the used codec for AirTunes
    // (apples audio only streaming)
    std::unique_ptr<CDVDDemuxBXA> demuxer(new CDVDDemuxBXA());
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

      std::unique_ptr<CDVDDemuxCDDA> demuxer(new CDVDDemuxCDDA());
      if (demuxer->Open(pInputStream))
      {
        return demuxer.release();
      }
    }
  }

  // Input stream handles demuxing
  if (pInputStream->GetIDemux())
  {
    std::unique_ptr<CDVDDemuxClient> demuxer(new CDVDDemuxClient());
    if(demuxer->Open(pInputStream))
      return demuxer.release();
    else
      return nullptr;
  }

  bool streaminfo = true; /* Look for streams before playback */
  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER))
  {
    CDVDInputStreamPVRManager* pInputStreamPVR = (CDVDInputStreamPVRManager*)pInputStream;
    CDVDInputStream* pOtherStream = pInputStreamPVR->GetOtherStream();

    /* Don't parse the streaminfo for some cases of streams to reduce the channel switch time */
    bool useFastswitch = URIUtils::IsUsingFastSwitch(pInputStream->GetFileName());
    streaminfo = !useFastswitch;

    if (pOtherStream)
    {
      /* Used for MediaPortal PVR addon (uses PVR otherstream for playback of rtsp streams) */
      if (pOtherStream->IsStreamType(DVDSTREAM_TYPE_FFMPEG))
      {
        std::unique_ptr<CDVDDemuxFFmpeg> demuxer(new CDVDDemuxFFmpeg());
        if(demuxer->Open(pOtherStream, streaminfo))
          return demuxer.release();
        else
          return nullptr;
      }
    }
  }

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_FFMPEG))
  {
    bool useFastswitch = URIUtils::IsUsingFastSwitch(pInputStream->GetFileName());
    streaminfo = !useFastswitch;
  }

  // Try to open the MultiFiles demuxer
  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_MULTIFILES))
  {
    std::unique_ptr<CDemuxMultiSource> demuxer(new CDemuxMultiSource());
    if (demuxer->Open(pInputStream))
      return demuxer.release();
    else
      return NULL;
  }

  std::unique_ptr<CDVDDemuxFFmpeg> demuxer(new CDVDDemuxFFmpeg());
  if(demuxer->Open(pInputStream, streaminfo, fileinfo))
    return demuxer.release();
  else
    return NULL;
}

