/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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
#include "DVDDemuxPVRClient.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"

using namespace std;

CDVDDemux* CDVDFactoryDemuxer::CreateDemuxer(CDVDInputStream* pInputStream)
{
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
      PVR_ADDON_CAPABILITIES *pProps = CPVRManager::GetClients()->GetCurrentClientProperties();
      if (pProps && pProps->bHandlesDemuxing)
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

