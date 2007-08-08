
#include "stdafx.h"
#include "DVDFactoryDemuxer.h"

#include "../DVDInputStreams/DVDInputStream.h"
#include "../DVDInputStreams/DVDInputStreamHttp.h"

#include "DVDDemuxFFmpeg.h"
#include "DVDDemuxShoutcast.h"

CDVDDemux* CDVDFactoryDemuxer::CreateDemuxer(CDVDInputStream* pInputStream)
{
  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_HTTP))
  {
    CDVDInputStreamHttp* pHttpStream = (CDVDInputStreamHttp*)pInputStream;
    CHttpHeader *header = pHttpStream->GetHttpHeader();

    /* check so we got the meta information as requested in our http header */
    if( header->GetValue("icy-metaint").length() > 0 )
      return new CDVDDemuxShoutcast();
  }
  
  return new CDVDDemuxFFmpeg();
}

