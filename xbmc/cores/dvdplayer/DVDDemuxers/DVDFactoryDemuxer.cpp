
#include "../../../stdafx.h"
#include "DVDFactoryDemuxer.h"
#include "..\DVDCodecs\DVDCodecs.h"

#include "..\DVDInputStreams\DVDInputStream.h"
#include "..\DVDInputStreams\DVDInputStreamHttp.h"

#include "DVDDemuxFFmpeg.h"
#include "DVDDemuxShoutcast.h"

CDVDDemux* CDVDFactoryDemuxer::CreateDemuxer(CDVDInputStream* pInputStream)
{
  if (pInputStream->m_streamType == DVDSTREAM_TYPE_HTTP)
  {
    CDVDInputStreamHttp* pHttpStream = (CDVDInputStreamHttp*)pInputStream;
    // we can always check header info in (pHttpStream->GetHttpHeader())
    // for now assume it is always shoutcast
    return new CDVDDemuxShoutcast();
  }
  
  return new CDVDDemuxFFmpeg();
}

