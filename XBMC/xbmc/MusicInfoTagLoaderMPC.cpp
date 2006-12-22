
#include "stdafx.h"
#include "musicinfotagloadermpc.h"
#include "cores/paplayer/MPCCodec.h"


using namespace MUSIC_INFO;

CMusicInfoTagLoaderMPC::CMusicInfoTagLoaderMPC(void)
{}

CMusicInfoTagLoaderMPC::~CMusicInfoTagLoaderMPC()
{}

int CMusicInfoTagLoaderMPC::ReadDuration(const CStdString &strFileName)
{
  MPCCodec codec;
  if (codec.Init(strFileName, 4096))
  {
    return (int)((codec.m_TotalTime + 500) / 1000);
  }
  return 0;
}