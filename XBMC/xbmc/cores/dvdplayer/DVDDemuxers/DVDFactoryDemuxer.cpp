
#include "stdafx.h"
#include "DVDFactoryDemuxer.h"

#include "../DVDInputStreams/DVDInputStream.h"
#include "../DVDInputStreams/DVDInputStreamHttp.h"

#include "DVDDemuxFFmpeg.h"
#include "DVDDemuxShoutcast.h"

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
  auto_ptr<CDVDDemuxFFmpeg> demuxer(new CDVDDemuxFFmpeg());
  if(demuxer->Open(pInputStream))
    return demuxer.release();
  else
    return NULL;
}

