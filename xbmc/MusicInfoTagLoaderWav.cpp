
#include "stdafx.h"
#include "MusicInfoTagLoaderWav.h"
#include "cores/paplayer/WAVCodec.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace MUSIC_INFO;

CMusicInfoTagLoaderWAV::CMusicInfoTagLoaderWAV(void)
{}

CMusicInfoTagLoaderWAV::~CMusicInfoTagLoaderWAV()
{}

bool CMusicInfoTagLoaderWAV::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  try
  {
    // WAV has no tag information other than the duration.

    // Load our codec class
    WAVCodec codec;
    if (codec.Init(strFileName, 4096))
    {
      tag.SetURL(strFileName);
      tag.SetDuration((int)(codec.m_TotalTime/1000));
      tag.SetLoaded(false);
      codec.DeInit();
      return true;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader wav: exception in file %s", strFileName.c_str());
  }

  return false;
}
