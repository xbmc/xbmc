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

#include "utils/ISerializable.h"
#include "XBDateTime.h"
#include "music/EmbeddedArt.h"
#include "music/tags/ReplayGain.h"
#include "Artist.h"
#include <map>
#include <string>
#include <vector>

class CVariant;

/*!
 \ingroup music
 \brief Class to store and read album information from CMusicDatabase
 \sa CSong, CMusicDatabase
 */

class CGenre
{
public:
  long idGenre;
  std::string strGenre;
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
  ~CSong() override{};
  void Clear() ;
  void MergeScrapedSong(const CSong& source, bool override);
  void Serialize(CVariant& value) const override;

  bool operator<(const CSong &song) const
  {
    if (strFileName < song.strFileName) return true;
    if (strFileName > song.strFileName) return false;
    if (iTrack < song.iTrack) return true;
    return false;
  }

  /*! \brief Get artist names from the vector of artistcredits objects
  \return artist names as a vector of strings
  */
  const std::vector<std::string> GetArtist() const;
  
  /*! \brief Get artist sort name string 
  \return artist sort name as a single string
  */
  const std::string GetArtistSort() const;

  /*! \brief Get artist MusicBrainz IDs from the vector of artistcredits objects
  \return artist MusicBrainz IDs as a vector of strings
  */
  const std::vector<std::string> GetMusicBrainzArtistID() const;

  /*! \brief Get artist names from the artist description string (if it exists)
  or concatenated from the vector of artistcredits objects
  \return artist names as a single string
  */
  const std::string GetArtistString() const;

  /*! \brief Get song artist IDs (for json rpc) from the vector of artistcredits objects
  \return album artist IDs as a vector of integers
  */
  const std::vector<int> GetArtistIDArray() const;

  /*! \brief Get album artist names associated with song from tag data
   Note for initial album processing only, normalised album artist data belongs to album 
   and is stored in album artist credits
  \return album artist names as a vector of strings
  */
  const std::vector<std::string> GetAlbumArtist() const { return m_albumArtist; }
  
  /*! \brief Get album artist sort name string
  \return album artist sort name as a single string
  */
  const std::string GetAlbumArtistSort() const { return m_strAlbumArtistSort; }

  /*! \brief Get composer sort name string
  \return composer sort name as a single string
  */
  const std::string GetComposerSort() const { return m_strComposerSort; }

  /*! \brief Get the full list of artist names and the role each played for those
    that contributed to the recording. Given in music file tags other than ARTIST
    or ALBUMARTIST, e.g. COMPOSER or CONDUCTOR etc.
  \return a vector of all contributing artist names and their roles
  */
  const VECMUSICROLES& GetContributors() const { return m_musicRoles; };
  //void AddArtistRole(const int &role, const std::string &artist);
  void AppendArtistRole(const CMusicRole& musicRole);

  /*! \brief Set album artist vector. 
   Album artist is held local to song until album created for inital processing only.
   Normalised album artist data belongs to album and is stored in album artist credits
  \param album artist names as a vector of strings
  */
  void SetAlbumArtist(const std::vector<std::string>& albumartists) { m_albumArtist = albumartists; }
  
  /*! \brief Whether this song has any artists in artist credits vector
    Tests if artist credits has been populated yet, during processing there can be
    artists in the artist description but not yet in the credits
  */
  bool HasArtistCredits() const { return !artistCredits.empty(); }

  /*! \brief Whether this song has any artists in music roles (contributors) vector
  Tests if contributors has been populated yet, there may be none.
  */
  bool HasContributors() const { return !m_musicRoles.empty(); }

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
  std::string strFileName;
  std::string strTitle;
  std::string strArtistSort;
  std::string strArtistDesc;
  VECARTISTCREDITS artistCredits;
  std::string strAlbum;
  std::vector<std::string> genre;
  std::string strThumb;
  MUSIC_INFO::EmbeddedArtInfo embeddedArt;
  std::string strMusicBrainzTrackID;
  std::string strComment;
  std::string strMood;
  std::string strCueSheet;
  float rating;
  int userrating;
  int votes;
  int iTrack;
  int iDuration;
  int iYear;
  int iTimesPlayed;
  CDateTime lastPlayed;
  CDateTime dateAdded;
  int iStartOffset;
  int iEndOffset;
  bool bCompilation;
  std::string strRecordLabel; // Record label from tag for album processing by CMusicInfoScanner::FileItemsToAlbums
  std::string strAlbumType; // (Musicbrainz release type) album type from tag for album processing by CMusicInfoScanner::FileItemsToAlbums

  ReplayGain replayGain;
private:
  std::vector<std::string> m_albumArtist; // Album artist from tag for album processing, no desc or MBID
  std::string m_strAlbumArtistSort; // Albumartist sort string from tag for album processing by CMusicInfoScanner::FileItemsToAlbums
  std::string m_strComposerSort;
  VECMUSICROLES m_musicRoles;
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
 \brief A vector of std::string objects, used for CMusicDatabase
 \sa CMusicDatabase
 */
typedef std::vector<CGenre> VECGENRES;
