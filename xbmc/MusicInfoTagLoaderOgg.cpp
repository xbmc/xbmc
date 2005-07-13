
#include "stdafx.h"
#include "musicinfotagloaderogg.h"
#include "oggtag.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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
    tag.SetURL(strFileName);
    COggTag myTag;
    if (myTag.ReadTag(strFileName))
    {
      myTag.GetMusicInfoTag(tag);
      return true;
    }
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader ogg: exception in file %s", strFileName.c_str());
  }

  tag.SetLoaded(false);
  return false;
}
