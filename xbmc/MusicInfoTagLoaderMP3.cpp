#include "musicinfotagloadermp3.h"
#include "stdstring.h"
#include "sectionloader.h"
#include "Util.h"

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
	auto_ptr<char>pYear  (ID3_GetYear( &id3tag  ));
	auto_ptr<char>pTitle (ID3_GetTitle( &id3tag ));
	auto_ptr<char>pArtist(ID3_GetArtist( &id3tag));
	auto_ptr<char>pAlbum (ID3_GetAlbum( &id3tag ));
	auto_ptr<char>pGenre (ID3_GetGenre( &id3tag ));
	int nTrackNum=ID3_GetTrackNum( &id3tag );

	tag.SetTrackNumber(nTrackNum);

	if (NULL != pGenre.get())
	{
		tag.SetGenre(pGenre.get());
	}
	if (NULL != pTitle.get())
	{
		bResult = true;
		tag.SetTitle(pTitle.get());
	}
	if (NULL != pArtist.get())
	{
		tag.SetArtist(pArtist.get());
	}
	if (NULL != pAlbum.get())
	{
		tag.SetAlbum(pAlbum.get());
	}
	if (NULL != pYear.get())
	{
		dateTime.wYear=atoi(pYear.get());
		tag.SetReleaseDate(dateTime);
	}

	//	extract Cover Art and save as album thumb
	if (ID3_HasPicture(&id3tag))
	{
		ID3_PictureType nPicTyp=ID3PT_COVERFRONT;
		bool bFound=false;
		auto_ptr<char>pMimeTyp (ID3_GetMimeTypeOfPicType(&id3tag, nPicTyp));
		if (pMimeTyp.get() == NULL)
		{
			nPicTyp=ID3PT_OTHER;
			auto_ptr<char>pMimeTyp (ID3_GetMimeTypeOfPicType(&id3tag, nPicTyp));
			if (pMimeTyp.get() != NULL)
				bFound=true;
		}
		else
			bFound=true;

		if (bFound)
		{
			CStdString strCoverArt;
			CUtil::GetAlbumThumb(tag.GetAlbum(), strCoverArt);
			if (!CUtil::FileExists(strCoverArt))
				ID3_GetPictureDataOfPicType(&id3tag, strCoverArt, nPicTyp);
		}
	}
	const Mp3_Headerinfo* mp3info = id3tag.GetMp3HeaderInfo();
	if ( mp3info != NULL )
		tag.SetDuration( mp3info->time );
	return bResult;
}

bool CMusicInfoTagLoaderMP3::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
	// retrieve the ID3 Tag info from strFileName
	// and put it in tag
	bool bResult=false;
//	CSectionLoader::Load("LIBID3");
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
//	CSectionLoader::Unload("LIBID3");
	return bResult;
}
