
#include "stdafx.h"
#include "musicinfotagloaderogg.h"
#include "oggtag.h"


using namespace MUSIC_INFO;

CMusicInfoTagLoaderOgg::CMusicInfoTagLoaderOgg(void)
{}

CMusicInfoTagLoaderOgg::~CMusicInfoTagLoaderOgg()
{}

bool CMusicInfoTagLoaderOgg::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  try
  {
    // retrieve the OGG Tag info from strFileName
    // and put it in tag
    COggTag myTag;
    if (myTag.Read(strFileName))
    {
      myTag.GetMusicInfoTag(tag);
    }
    return tag.Loaded();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader ogg: exception in file %s", strFileName.c_str());
  }

  tag.SetLoaded(false);
  return false;
}
