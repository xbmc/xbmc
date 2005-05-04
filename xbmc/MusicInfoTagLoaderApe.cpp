
#include "stdafx.h"
#include "musicinfotagloaderape.h"

using namespace MUSIC_INFO;

CMusicInfoTagLoaderApe::CMusicInfoTagLoaderApe(void)
{}

CMusicInfoTagLoaderApe::~CMusicInfoTagLoaderApe()
{}

bool CMusicInfoTagLoaderApe::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  try
  {
    // retrieve the APE Tag info from strFileName
    // and put it in tag
    bool bResult = false;
    tag.SetURL(strFileName);
    CAPEv2Tag myTag;
    if (myTag.ReadTag(strFileName.c_str()))
    {
      tag.SetTitle(myTag.GetTitle());
      tag.SetAlbum(myTag.GetAlbum());
      tag.SetArtist(myTag.GetArtist());
      tag.SetGenre(myTag.GetGenre());
      tag.SetTrackNumber(myTag.GetTrackNum());
      SYSTEMTIME dateTime;
      ZeroMemory(&dateTime, sizeof(SYSTEMTIME));
      dateTime.wYear = atoi(myTag.GetYear());
      tag.SetReleaseDate(dateTime);
      tag.SetLoaded();
      // Find duration - we must read the info from the ape file for this
      tag.SetDuration((int)(myTag.ReadDuration(strFileName.c_str()) / 1000));
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
