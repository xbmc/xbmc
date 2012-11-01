/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
/*!
 \file Song.h
\brief
*/
#pragma once

#include "utils/StdString.h"
#include "utils/ISerializable.h"
#include "XBDateTime.h"
#include "music/tags/MusicInfoTag.h" // for EmbeddedArt

#include <map>
#include <vector>

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
class CSong: public ISerializable
{
public:
  CSong() ;
  CSong(MUSIC_INFO::CMusicInfoTag& tag);
  virtual ~CSong(){};
  void Clear() ;
  virtual void Serialize(CVariant& value) const;

  bool operator<(const CSong &song) const
  {
    if (strFileName < song.strFileName) return true;
    if (strFileName > song.strFileName) return false;
    if (iTrack < song.iTrack) return true;
    return false;
  }

  /*! \brief whether this song has art associated with it
   Tests both the strThumb and embeddedArt members.
   */
  bool HasArt() const;

  /*! \brief whether the art from this song matches the art from another
   Tests both the strThumb and embeddedArt members.
   */
  bool ArtMatches(const CSong &right) const;

  long idSong;
  CStdString strFileName;
  CStdString strTitle;
  std::vector<std::string> artist;
  CStdString strAlbum;
  std::vector<std::string> albumArtist;
  std::vector<std::string> genre;
  CStdString strThumb;
  MUSIC_INFO::EmbeddedArtInfo embeddedArt;
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
  CDateTime lastPlayed;
  int iStartOffset;
  int iEndOffset;
  int iAlbumId;
  bool bCompilation;

  // Karaoke-specific information
  long       iKaraokeNumber;        //! Karaoke song number to "select by number". 0 for non-karaoke
  CStdString strKaraokeLyrEncoding; //! Karaoke song lyrics encoding if known. Empty if unknown.
  int        iKaraokeDelay;         //! Karaoke song lyrics-music delay in 1/10 seconds.
};

/*!
 \ingroup music
 \brief A map of CSong objects, used for CMusicDatabase
 */
class CSongMap
{
public:
  CSongMap();

  std::map<CStdString, CSong>::const_iterator Begin();
  std::map<CStdString, CSong>::const_iterator End();
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
