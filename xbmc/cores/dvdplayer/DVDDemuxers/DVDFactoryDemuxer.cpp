
#include "stdafx.h"
#include "DVDFactoryDemuxer.h"

#include "DVDDemuxFFmpeg.h"
#include "..\DVDInputStreams\DVDInputStream.h"

CDVDDemux* CDVDFactoryDemuxer::CreateDemuxer(CDVDInputStream* pInputStream)
{
  return new CDVDDemuxFFmpeg();
}
