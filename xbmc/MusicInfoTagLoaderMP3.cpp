#include "musicinfotagloadermp3.h"
#include "stdstring.h"
#include "sectionloader.h"
#include "filesystem/file.h"

#define HAVE_CONFIG
#define ID3LIB_LINKOPTION 1
#include "lib/libID3/id3.h"
#include "lib/libID3/config.h"
#include "lib/libID3/tag.h"
#include "lib/libID3/utils.h"
#include "lib/libID3/misc_support.h"
#include "lib/libID3/readers.h"
#include "lib/libID3/io_helpers.h"
#include "XIStreamReader.h"

using namespace MUSIC_INFO;
using namespace XFILE;

CMusicInfoTagLoaderMP3::CMusicInfoTagLoaderMP3(void)
{
}

CMusicInfoTagLoaderMP3::~CMusicInfoTagLoaderMP3()
{
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

		ID3_XIStreamReader reader( file );
		ID3_Tag myTag;
		if ( myTag.Link(reader, ID3TT_ALL) >= 0)
		{
				bResult=true;
				SYSTEMTIME dateTime;
				char *pYear=ID3_GetYear( &myTag );
				char *pTitle=ID3_GetTitle( &myTag );
				char *pArtist=ID3_GetArtist( &myTag );
				char *pAlbum=ID3_GetAlbum( &myTag );
				char *pGenre=ID3_GetGenre( &myTag );
				int nTrackNum=ID3_GetTrackNum( &myTag );
				
				tag.SetTrackNumber(nTrackNum);

				if (pGenre)
				{
					tag.SetTitle(pGenre);
					delete [] pGenre;
				}
				if (pTitle)
				{
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
		}
		file.Close();
	}
	CSectionLoader::Unload("LIBID3");
	return bResult;
}
