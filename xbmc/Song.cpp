#include "stdafx.h"
#include "Song.h"

CSong::CSong(CMusicInfoTag& tag)
{
	SYSTEMTIME stTime;
	tag.GetReleaseDate(stTime);
	strTitle		= tag.GetTitle();
	strGenre		= tag.GetGenre();
	strFileName	= tag.GetURL();
	strArtist		= tag.GetArtist();
	strAlbum		= tag.GetAlbum();
	iYear				=	stTime.wYear;
	iTrack			= tag.GetTrackNumber();
	iDuration		= tag.GetDuration();
  strThumb    = "";
	iStartOffset	= 0;
	iEndOffset		= 0;
}

CSong::CSong()
{
	Clear();
}

void CSong::Clear()
{
	strFileName="";
	strTitle="";
	strArtist="";
	strAlbum="";
	strGenre="";
  strThumb="";
	iTrack=0;
	iDuration=0;
	iYear=0;
	iStartOffset=0;
	iEndOffset=0;
}

