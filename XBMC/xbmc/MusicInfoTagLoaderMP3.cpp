#include "musicinfotagloadermp3.h"
#include "stdstring.h"
#include "sectionloader.h"

using namespace MUSIC_INFO;
using namespace XFILE;

CMusicInfoTagLoaderMP3::CMusicInfoTagLoaderMP3(void)
{
}

CMusicInfoTagLoaderMP3::~CMusicInfoTagLoaderMP3()
{
}

bool CMusicInfoTagLoaderMP3::ReadTag( ID3_Tag& id3tag, CMusicInfoTag& tag )
{
	bool bResult= false;

	SYSTEMTIME dateTime;
	char *pYear=ID3_GetYear( &id3tag );
	char *pTitle=ID3_GetTitle( &id3tag );
	char *pArtist=ID3_GetArtist( &id3tag );
	char *pAlbum=ID3_GetAlbum( &id3tag );
	char *pGenre=ID3_GetGenre( &id3tag );
	int nTrackNum=ID3_GetTrackNum( &id3tag );

	tag.SetTrackNumber(nTrackNum);

	if (pGenre)
	{
		tag.SetGenre(pGenre);
		delete [] pGenre;
	}
	if (pTitle)
	{
		bResult = true;
		tag.SetTitle(pTitle);
		delete [] pTitle;
	}
	if (pArtist)
	{
		tag.SetArtist(pArtist);
		delete [] pArtist;
	}
	if (pAlbum)
	{
		tag.SetAlbum(pAlbum);
		delete [] pAlbum;
	}
	if (pYear)
	{
		dateTime.wYear=atoi(pYear);
		tag.SetReleaseDate(dateTime);
		delete pYear;
	}

	return bResult;
}

bool CMusicInfoTagLoaderMP3::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
	// retrieve the ID3 Tag info from strFileName
	// and put it in tag
	bool bResult=false;
	CSectionLoader::Load("LIBID3");
	tag.SetURL(strFileName);
	tag.SetLoaded(true);
	CFile file;
	if ( file.Open( strFileName.c_str() ) ) 
	{
		//	Do not use ID3TT_ALL, because
		//	id3lib reads the ID3V1 tag first
		//	then ID3V2 tag is blocked.
		ID3_XIStreamReader reader( file );
		ID3_Tag myTag;
		if ( myTag.Link(reader, ID3TT_ID3V2) >= 0)
		{
			if ( !(bResult = ReadTag( myTag, tag )) ) {
				myTag.Clear();
				if ( myTag.Link(reader, ID3TT_ID3V1 ) >= 0 ) {
					bResult = ReadTag( myTag, tag );
				}
			}
		}
		file.Close();
	}
	CSectionLoader::Unload("LIBID3");
	return bResult;
}
