
#include "stdafx.h"
#include "musicinfotagloaderflac.h"

using namespace MUSIC_INFO;

CMusicInfoTagLoaderFlac::CMusicInfoTagLoaderFlac(void)
{
}

CMusicInfoTagLoaderFlac::~CMusicInfoTagLoaderFlac()
{
}

bool CMusicInfoTagLoaderFlac::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
	try
	{
		// retrieve the Flac Tag info from strFileName
		// and put it in tag
		bool bResult= false;
		tag.SetURL(strFileName);
		CFile file;
		if ( file.Open( strFileName.c_str() ) )  {
			CFlacTag myTag;
			myTag.ReadTag( &file );

			SYSTEMTIME dateTime;
			CStdString strYear=myTag.GetYear();
			CStdString strTitle=myTag.GetTitle();
			CStdString strArtist=myTag.GetArtist();
			CStdString strAlbum=myTag.GetAlbum();
			CStdString strGenre=myTag.GetGenre();

			tag.SetTrackNumber(myTag.GetTrackNum());
			tag.SetDuration(myTag.GetDuration()/75);	// GetDuration() returns duration in frames

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
				dateTime.wYear=atoi(strYear);
				tag.SetReleaseDate(dateTime);
			}

			file.Close();
		}
		return bResult;
	}
	catch(...)
	{
		CLog::Log(LOGERROR, "Tag loader flac: exception in file %s", strFileName.c_str());
	}

	tag.SetLoaded(false);
	return false;
}
