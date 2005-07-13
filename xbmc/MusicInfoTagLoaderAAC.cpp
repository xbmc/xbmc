
#include "stdafx.h"
#include "musicinfotagloaderaac.h"
#include "cores/paplayer/AACCodec.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace MUSIC_INFO;

CMusicInfoTagLoaderAAC::CMusicInfoTagLoaderAAC(void)
{}

CMusicInfoTagLoaderAAC::~CMusicInfoTagLoaderAAC()
{}

int CMusicInfoTagLoaderAAC::ReadDuration(const CStdString& strFileName)
{
  AACCodec codec;
  if (codec.Init(strFileName, 4096))
  {
    return (int)((codec.m_TotalTime + 500) / 1000);
  }
  return 0;
}