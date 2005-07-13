
#include "../../../stdafx.h"
#include "DVDFactoryDemuxer.h"

#include "DVDDemuxFFmpeg.h"
#include "..\DVDInputStreams\DVDInputStream.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CDVDDemux* CDVDFactoryDemuxer::CreateDemuxer(CDVDInputStream* pInputStream)
{
  return new CDVDDemuxFFmpeg();
}
