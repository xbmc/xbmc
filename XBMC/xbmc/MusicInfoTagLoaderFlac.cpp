
#include "stdafx.h"
#include "musicinfotagloaderflac.h"
#include "FlacTag.h"


using namespace MUSIC_INFO;

CMusicInfoTagLoaderFlac::CMusicInfoTagLoaderFlac(void)
{}

CMusicInfoTagLoaderFlac::~CMusicInfoTagLoaderFlac()
{}

bool CMusicInfoTagLoaderFlac::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  try
  {
    // retrieve the Flac Tag info from strFileName
    // and put it in tag
    CFlacTag myTag;
    if (myTag.Read(strFileName))
    {
      myTag.GetMusicInfoTag(tag);
    }
    return tag.Loaded();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader flac: exception in file %s", strFileName.c_str());
  }

  tag.SetLoaded(false);
  return false;
}
