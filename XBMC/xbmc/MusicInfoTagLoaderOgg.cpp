
#include "stdafx.h"
#include "musicinfotagloaderogg.h"

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
    bool bResult = false;
    tag.SetURL(strFileName);
    CFile file;
    if ( file.Open( strFileName.c_str() ) )
    {
      COggTag myTag;
      myTag.ReadTag( &file );

      SYSTEMTIME dateTime;
      CStdString strYear = myTag.GetYear();
      CStdString strTitle = myTag.GetTitle();
      CStdString strArtist = myTag.GetArtist();
      CStdString strAlbum = myTag.GetAlbum();
      CStdString strGenre = myTag.GetGenre();
      CStdString strMBTID = myTag.GetMusicBrainzTrackID();
      CStdString strMBAID = myTag.GetMusicBrainzArtistID();
      CStdString strMBBID = myTag.GetMusicBrainzAlbumID();
      CStdString strMBAAID = myTag.GetMusicBrainzAlbumArtistID();
      CStdString strMBTRMID = myTag.GetMusicBrainzTRMID();

      tag.SetTrackNumber(myTag.GetTrackNum());
      tag.SetDuration(myTag.GetDuration() / 75); // GetDuration() returns duration in frames

      if (!strGenre.IsEmpty())
      {
        tag.SetGenre(strGenre);
      }
      if (!strTitle.IsEmpty())
      {
        bResult = true;
        tag.SetTitle(strTitle);
        tag.SetLoaded(true);
      }
      if (!strArtist.IsEmpty())
      {
        tag.SetArtist(strArtist);
      }
      if (!strAlbum.IsEmpty())
      {
        tag.SetAlbum(strAlbum);
      }
      if (!strYear.IsEmpty())
      {
        dateTime.wYear = atoi(strYear);
        tag.SetReleaseDate(dateTime);
      }
      if (!strMBTID.IsEmpty())
      {
        tag.SetMusicBrainzTrackID(strMBTID);
      }
      if (!strMBAID.IsEmpty())
      {
        tag.SetMusicBrainzArtistID(strMBAID);
      }
      if (!strMBBID.IsEmpty())
      {
        tag.SetMusicBrainzAlbumID(strMBBID);
      }
      if (!strMBAAID.IsEmpty())
      {
        tag.SetMusicBrainzAlbumArtistID(strMBAAID);
      }
      if (!strMBTRMID.IsEmpty())
      {
        tag.SetMusicBrainzTRMID(strMBTRMID);
      }

      file.Close();
    }
    return bResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Tag loader ogg: exception in file %s", strFileName.c_str());
  }

  tag.SetLoaded(false);
  return false;
}
