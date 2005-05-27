
#include "stdafx.h"
#include "MusicInfoTagLoaderShn.h"
#include "cores/paplayer/SHNCodec.h"

using namespace MUSIC_INFO;

CMusicInfoTagLoaderSHN::CMusicInfoTagLoaderSHN(void)
{}

CMusicInfoTagLoaderSHN::~CMusicInfoTagLoaderSHN()
{}

bool CMusicInfoTagLoaderSHN::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  try
  {
    // SHN has no tag information other than the duration.

    // Load our codec class
    SHNCodec codec;
    if (codec.Init(strFileName, 65536))
    {
      tag.SetDuration((int)((codec.m_TotalTime + 500)/ 1000));
      tag.SetLoaded(false);
      codec.DeInit();
      return true;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader ape: exception in file %s", strFileName.c_str());
  }

  tag.SetLoaded(false);
  return false;
}
