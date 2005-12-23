/*!
 \file MusicDatabase.h
\brief
*/
#pragma once

#include <map>
#include <vector>

#include "MusicInfotag.h"

using namespace MUSIC_INFO;

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
    return strAlbum + strPath < a.strAlbum + a.strPath;
  }
  long idAlbum;
  CStdString strAlbum;
  CStdString strPath;
  CStdString strArtist;
  CStdString strGenre;
  CStdString strThumb;
  CStdString strTones ;
  CStdString strStyles ;
  CStdString strReview ;
  CStdString strImage ;
  int iRating ;
  int iYear;
};

class CArtist
{
public:
  long idArtist;
  CStdString strArtist;
};

class CGenre
{
public:
  long idGenre;
  CStdString strGenre;
};

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
  long idSong;
  CStdString strFileName;
  CStdString strTitle;
  CStdString strArtist;
  CStdString strAlbum;
  CStdString strGenre;
  CStdString strThumb;
  CStdString strMusicBrainzTrackID;
  CStdString strMusicBrainzArtistID;
  CStdString strMusicBrainzAlbumID;
  CStdString strMusicBrainzAlbumArtistID;
  CStdString strMusicBrainzTRMID;
  int iTrack;
  int iDuration;
  int iYear;
  int iTimedPlayed;
  int iStartOffset;
  int iEndOffset;
};

/*!
 \ingroup music
 \brief A vector of CSong objects, used for CMusicDatabase
 \sa CMusicDatabase
 */
typedef std::vector<CSong> VECSONGS;

/*!
 \ingroup music
 \brief A map of CSong objects, used for CMusicDatabase
 \sa IMAPSONGS, CMusicDatabase
 */
typedef std::map<CStdString, CSong> MAPSONGS;

/*!
 \ingroup music
 \brief The MAPSONGS iterator
 \sa MAPSONGS, CMusicDatabase
 */
typedef std::map<CStdString, CSong>::iterator IMAPSONGS;

/*!
 \ingroup music
 \brief A vector of CStdString objects, used for CMusicDatabase
 */
typedef std::vector<CArtist> VECARTISTS;

/*!
 \ingroup music
 \brief A vector of CStdString objects, used for CMusicDatabase
 \sa CMusicDatabase
 */
typedef std::vector<CGenre> VECGENRES;

/*!
 \ingroup music
 \brief A vector of CAlbum objects, used for CMusicDatabase
 \sa CMusicDatabase
 */
typedef std::vector<CAlbum> VECALBUMS;
