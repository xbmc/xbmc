/*!
 \file MusicDatabase.h
\brief
*/
#pragma once

#include <map>
#include <vector>

#include "musicInfoTag.h"

/*!
 \ingroup music
 \brief Class to store and read album information from CMusicDatabase
 \sa CSong, CMusicDatabase
 */

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
  CSong(MUSIC_INFO::CMusicInfoTag& tag);
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
  CStdString strAlbumArtist;
  CStdString strGenre;
  CStdString strThumb;
  CStdString strMusicBrainzTrackID;
  CStdString strMusicBrainzArtistID;
  CStdString strMusicBrainzAlbumID;
  CStdString strMusicBrainzAlbumArtistID;
  CStdString strMusicBrainzTRMID;
  CStdString strComment;
  char rating;
  int iTrack;
  int iDuration;
  int iYear;
  int iTimesPlayed;
  CStdString lastPlayed;
  int iStartOffset;
  int iEndOffset;
};

/*!
 \ingroup music
 \brief A map of CSong objects, used for CMusicDatabase
 */
class CSongMap
{
public:
  CSongMap();

  CSong *Find(const CStdString &file);
  void Add(const CStdString &file, const CSong &song);
  void Clear();
  int Size();

private:
  std::map<CStdString, CSong> m_map;
};

/*!
 \ingroup music
 \brief A vector of CSong objects, used for CMusicDatabase
 \sa CMusicDatabase
 */
typedef std::vector<CSong> VECSONGS;

/*!
 \ingroup music
 \brief A vector of CStdString objects, used for CMusicDatabase
 \sa CMusicDatabase
 */
typedef std::vector<CGenre> VECGENRES;
