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

#include "DVDDemuxFFmpeg.h"
#include "DVDDemuxShoutcast.h"
#ifdef HAS_FILESYSTEM_HTSP
#include "DVDDemuxHTSP.h"
#endif
#include "DVDDemuxTS.h"

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

/*
  // Try our internal TS demuxer
  std::string::size_type index = pInputStream->GetFileName().find_last_of ('.');
  std::string extension;
  if (index != std::string::npos)
    extension = pInputStream->GetFileName().substr(index+1);
  else
    extension = "";
  TSTransportType tsType = TS_TYPE_UNKNOWN;
  if (!extension.compare("ts"))
    tsType = TS_TYPE_STD;
  else if(!extension.compare("m2ts") || !extension.compare("m2t"))
    tsType = TS_TYPE_M2TS;
  if (tsType != TS_TYPE_UNKNOWN)
  {
    auto_ptr<CDVDDemuxTS> demuxer(new CDVDDemuxTS());
    if(demuxer->Open(pInputStream, tsType))
      return demuxer.release();
  }
*/

  auto_ptr<CDVDDemuxFFmpeg> demuxer(new CDVDDemuxFFmpeg());
  if(demuxer->Open(pInputStream))
    return demuxer.release();
  else
    return NULL;
}

