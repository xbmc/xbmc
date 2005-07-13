
#include "stdafx.h"
#include "musicinfotagloaderwavpack.h"
#include "cores/paplayer/WavPackCodec.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace MUSIC_INFO;

CMusicInfoTagLoaderWAVPack::CMusicInfoTagLoaderWAVPack(void)
{}

CMusicInfoTagLoaderWAVPack::~CMusicInfoTagLoaderWAVPack()
{}

int CMusicInfoTagLoaderWAVPack::ReadDuration(const CStdString& strFileName)
{
  WAVPackCodec codec;
  if (codec.Init(strFileName, 4096))
  {
    return (int)((codec.m_TotalTime + 500) / 1000);
  }
  return 0;
}