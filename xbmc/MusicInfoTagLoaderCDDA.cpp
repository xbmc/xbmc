#include "stdafx.h"
#include "musicinfotagloadercdda.h"
#include "FileSystem/cddb.h"

using namespace MUSIC_INFO;
using namespace CDDB;

CMusicInfoTagLoaderCDDA::CMusicInfoTagLoaderCDDA(void)
{

}

CMusicInfoTagLoaderCDDA::~CMusicInfoTagLoaderCDDA()
{

}

bool CMusicInfoTagLoaderCDDA::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
	try
	{
		tag.SetURL(strFileName);
		bool bResult=false;

		//	Get information for the inserted disc
		CCdInfo* pCdInfo = CDetectDVDMedia::GetCdInfo();
		if (pCdInfo==NULL)
			return bResult;

		//	Prepare cddb
		Xcddb cddb;
		CStdString strDir;
		strDir.Format("%s\\cddb", g_stSettings.m_szAlbumDirectory);
		cddb.setCacheDir(strDir);

		int iTrack=atoi(strFileName.substr(13, strFileName.size() - 13 - 5).c_str()) + 1;

		//	Tracknumber and duration is always available
		tag.SetTrackNumber(iTrack);
		tag.SetDuration( ( pCdInfo->GetTrackInformation(iTrack).nMins * 60 ) 
			+ pCdInfo->GetTrackInformation(iTrack).nSecs );

		//	Only load cached cddb info in this tag loader, the internet database query is made in CCDDADirectory
		if (g_guiSettings.GetBool("MyMusic.UseCDDB") && pCdInfo->HasCDDBInfo() && cddb.isCDCached(pCdInfo))
		{
			//	get cddb information
			if (cddb.queryCDinfo(pCdInfo))
			{
				//	Fill the fileitems music tag with cddb information, if available
				CStdString strTitle=cddb.getTrackTitle(iTrack);
				if (strTitle.size() > 0)
				{
					//	Title
					tag.SetTitle(strTitle);

					//	Artist: Use track artist or disc artist
					CStdString strArtist=cddb.getTrackArtist(iTrack);
					if (strArtist.IsEmpty())
						cddb.getDiskArtist(strArtist);
					tag.SetArtist(strArtist);

					// Album
					CStdString strAlbum;
					cddb.getDiskTitle( strAlbum );
					tag.SetAlbum(strAlbum);
					
					//	Year
					SYSTEMTIME dateTime;
					dateTime.wYear=atoi(cddb.getYear().c_str());
					tag.SetReleaseDate( dateTime );

					//	Genre
					tag.SetGenre( cddb.getGenre() );

					tag.SetLoaded(true);
					bResult=true;
				}
			}
		}
		else
		{
			//	No cddb info, maybe we have CD-Text
			trackinfo ti = pCdInfo->GetTrackInformation(iTrack);

			// Fill the fileitems music tag with CD-Text information, if available
			CStdString strTitle=ti.cdtext.field[CDTEXT_TITLE];
			if (strTitle.size() > 0)
			{
				//	Title
				tag.SetTitle(strTitle);

				// Get info for track zero, as we may have and need CD-Text Album info
				cdtext_t discCDText = pCdInfo->GetDiscCDTextInformation();

				//	Artist: Use track artist or disc artist
				CStdString strArtist=ti.cdtext.field[CDTEXT_PERFORMER];
				if (strArtist.IsEmpty())
					strArtist=discCDText.field[CDTEXT_PERFORMER];
				tag.SetArtist(strArtist);

				// Album
				CStdString strAlbum;
				strAlbum=discCDText.field[CDTEXT_TITLE];
				tag.SetAlbum(strAlbum);
			
				//	Genre
				CStdString strGenre=ti.cdtext.field[CDTEXT_GENRE];
				tag.SetGenre( strGenre );

				tag.SetLoaded(true);
				bResult=true;
			}
		}
		return bResult;
	}
	catch(...)
	{
		CLog::Log(LOGERROR, "Tag loader CDDB: exception in file %s", strFileName.c_str());
	}

	tag.SetLoaded(false);
	return false;
}
