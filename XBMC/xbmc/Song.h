/*!
  \file MusicDatabase.h
	\brief
	*/
#pragma once
#include "MusicInfotag.h"

using namespace MUSIC_INFO;

/*!
	\ingroup music
	\brief Class to store and read song information from CMusicDatabase
	\sa CAlbum, CMusicDatabase
	*/
class CSong
{
public:
	CSong() ;
	CSong(CMusicInfoTag& tag);
	virtual ~CSong(){};
	void Clear() ;

	bool operator<(const CSong &song) const
	{
		if (strFileName < song.strFileName) return true;
		if (strFileName > song.strFileName) return false;
		if (iTrack < song.iTrack) return true;
		return false;
	}
	CStdString strFileName;
	CStdString strTitle;
	CStdString strArtist;
	CStdString strAlbum;
	CStdString strGenre;
  CStdString strThumb;
	int iTrack;
	int iDuration;
	int iYear;
	int iTimedPlayed;
	int iStartOffset;
	int iEndOffset;
};

/*!
	\ingroup music
	\brief Class to store and read album information from CMusicDatabase
	\sa CSong, CMusicDatabase
	*/
class CAlbum
{
public:
	bool operator<(const CAlbum &a) const
	{
		return strAlbum+strPath<a.strAlbum+a.strPath;
	}
	CStdString strAlbum;
	CStdString strPath;
	CStdString strArtist;
	CStdString strGenre;
	CStdString strTones ;
	CStdString strStyles ;
	CStdString strReview ;
	CStdString strImage ;
	int    iRating ;
	int		 iYear;
};

/*!
	\ingroup music
	\brief A vector of CSong objects, used for CMusicDatabase
	\sa CMusicDatabase
	*/
typedef vector<CSong>  VECSONGS;

/*!
	\ingroup music
	\brief A map of CSong objects, used for CMusicDatabase
	\sa IMAPSONGS, CMusicDatabase
	*/
typedef map<CStdString,CSong>  MAPSONGS;

/*!
	\ingroup music
	\brief The MAPSONGS iterator
	\sa MAPSONGS, CMusicDatabase
	*/
typedef map<CStdString,CSong>::iterator  IMAPSONGS;

/*!
	\ingroup music
	\brief A vector of CStdString objects, used for CMusicDatabase
	*/
typedef vector<CStdString> VECARTISTS;

/*!
	\ingroup music
	\brief A vector of CStdString objects, used for CMusicDatabase
	\sa CMusicDatabase
	*/
typedef vector<CStdString> VECGENRES;

/*!
	\ingroup music
	\brief A vector of CAlbum objects, used for CMusicDatabase
	\sa CMusicDatabase
	*/
typedef vector<CAlbum> VECALBUMS;
