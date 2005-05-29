
#include "stdafx.h"
#include "musicinfotagloaderflac.h"

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
    tag.SetURL(strFileName);
    CFlacTag myTag;
    if (myTag.ReadTag(strFileName))
    {
      myTag.GetMusicInfoTag(tag);
      return true;
    }
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader flac: exception in file %s", strFileName.c_str());
  }

  tag.SetLoaded(false);
  return false;
}
