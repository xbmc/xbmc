/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#ifndef MUSIC_UTILS_STDSTRING_H_INCLUDED
#define MUSIC_UTILS_STDSTRING_H_INCLUDED
#include "utils/StdString.h"
#endif

#ifndef MUSIC_UTILS_ISERIALIZABLE_H_INCLUDED
#define MUSIC_UTILS_ISERIALIZABLE_H_INCLUDED
#include "utils/ISerializable.h"
#endif

#ifndef MUSIC_XBDATETIME_H_INCLUDED
#define MUSIC_XBDATETIME_H_INCLUDED
#include "XBDateTime.h"
#endif

#ifndef MUSIC_MUSIC_TAGS_MUSICINFOTAG_H_INCLUDED
#define MUSIC_MUSIC_TAGS_MUSICINFOTAG_H_INCLUDED
#include "music/tags/MusicInfoTag.h" // for EmbeddedArt
#endif

#ifndef MUSIC_ARTIST_H_INCLUDED
#define MUSIC_ARTIST_H_INCLUDED
#include "Artist.h"
#endif

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

class CFileItem;

/*!
 \ingroup music
 \brief Class to store and read song information from CMusicDatabase
 \sa CAlbum, CMusicDatabase
 */
class CSong: public ISerializable
{
public:
  CSong() ;
  CSong(CFileItem& item);
  virtual ~CSong(){};
  void Clear() ;
  void MergeScrapedSong(const CSong& source, bool override);
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
  int idAlbum;
  CStdString strFileName;
  CStdString strTitle;
  std::vector<std::string> artist;
  VECARTISTCREDITS artistCredits;
  CStdString strAlbum;
  std::vector<std::string> albumArtist;
  std::vector<std::string> genre;
  CStdString strThumb;
  MUSIC_INFO::EmbeddedArtInfo embeddedArt;
  CStdString strMusicBrainzTrackID;
  CStdString strComment;
  char rating;
  int iTrack;
  int iDuration;
  int iYear;
  int iTimesPlayed;
  CDateTime lastPlayed;
  int iStartOffset;
  int iEndOffset;
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
typedef std::map<std::string, CSong> MAPSONGS;

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
