/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

/*!
 \file Album.h
\brief
*/

#include <map>
#include <vector>
#include "Artist.h"
#include "Song.h"
#include "XBDateTime.h"
#include "utils/ScraperUrl.h"

class TiXmlNode;
class CFileItem;
class CAlbum
{
public:
  explicit CAlbum(const CFileItem& item);
  CAlbum() = default;
  bool operator<(const CAlbum &a) const;
  void MergeScrapedAlbum(const CAlbum& album, bool override = true);

  void Reset()
  {
    idAlbum = -1;
    strAlbum.clear();
    strMusicBrainzAlbumID.clear();
    strReleaseGroupMBID.clear();
    artistCredits.clear();
    strArtistDesc.clear();
    strArtistSort.clear();
    genre.clear();
    thumbURL.Clear();
    moods.clear();
    styles.clear();
    themes.clear();
    art.clear();
    strReview.clear();
    strLabel.clear();
    strType.clear();
    strPath.clear();
    m_strDateOfRelease.clear();
    fRating = -1;
    iUserrating = -1;
    iVotes = -1;
    iYear = -1;
    bCompilation = false;
    iTimesPlayed = 0;
    dateAdded.Reset();
    lastPlayed.Reset();
    songs.clear();
    releaseType = Album;
    strLastScraped.clear();
    bScrapedMBID = false;
    bArtistSongMerge = false;
  }

  /*! \brief Get album artist names from the vector of artistcredits objects
  \return album artist names as a vector of strings
  */
  const std::vector<std::string> GetAlbumArtist() const;

  /*! \brief Get album artist MusicBrainz IDs from the vector of artistcredits objects
  \return album artist MusicBrainz IDs as a vector of strings
  */
  const std::vector<std::string> GetMusicBrainzAlbumArtistID() const;
  std::string GetGenreString() const;

  /*! \brief Get album artist names from the artist description string (if it exists)
             or concatenated from the vector of artistcredits objects
  \return album artist names as a single string
  */
  const std::string GetAlbumArtistString() const;

  /*! \brief Get album artist sort name from the artist sort string (if it exists)
  or concatenated from the vector of artistcredits objects
  \return album artist sort names as a single string
  */
  const std::string GetAlbumArtistSort() const;

  /*! \brief Get album artist IDs (for json rpc) from the vector of artistcredits objects
  \return album artist IDs as a vector of integers
  */
  const std::vector<int> GetArtistIDArray() const;

  typedef enum ReleaseType {
    Album = 0,
    Single
  } ReleaseType;

  std::string GetReleaseType() const;
  void SetReleaseType(const std::string& strReleaseType);
  void SetDateAdded(const std::string& strDateAdded);
  void SetLastPlayed(const std::string& strLastPlayed);

  static std::string ReleaseTypeToString(ReleaseType releaseType);
  static ReleaseType ReleaseTypeFromString(const std::string& strReleaseType);

  /*! \brief Set album artist credits using the arrays of tag values.
   If strArtistSort (as from ALBUMARTISTSORT tag) is already set then individual
   artist sort names are also processed.
   \param names       String vector of albumartist names (as from ALBUMARTIST tag)
   \param hints       String vector of albumartist name hints (as from ALBUMARTISTS tag)
   \param mbids       String vector of albumartist Musicbrainz IDs (as from MUSICBRAINZABUMARTISTID tag)
   \param artistnames String vector of artist names (as from ARTIST tag)
   \param artisthints String vector of artist name hints (as from ARTISTS tag)
   \param artistmbids String vector of artist Musicbrainz IDs (as from MUSICBRAINZARTISTID tag)
  */
  void SetArtistCredits(const std::vector<std::string>& names, const std::vector<std::string>& hints,
                        const std::vector<std::string>& mbids,
                        const std::vector<std::string>& artistnames = std::vector<std::string>(),
                        const std::vector<std::string>& artisthints = std::vector<std::string>(),
                        const std::vector<std::string>& artistmbids = std::vector<std::string>());

  /*! \brief Load album information from an XML file.
   See CVideoInfoTag::Load for a description of the types of elements we load.
   \param element    the root XML element to parse.
   \param append     whether information should be added to the existing tag, or whether it should be reset first.
   \param prioritise if appending, whether additive tags should be prioritised (i.e. replace or prepend) over existing values. Defaults to false.
   \sa CVideoInfoTag::Load
   */
  bool Load(const TiXmlElement *element, bool append = false, bool prioritise = false);
  bool Save(TiXmlNode *node, const std::string &tag, const std::string& strPath);

  long idAlbum = -1;
  std::string strAlbum;
  std::string strMusicBrainzAlbumID;
  std::string strReleaseGroupMBID;
  std::string strArtistDesc;
  std::string strArtistSort;
  VECARTISTCREDITS artistCredits;
  std::vector<std::string> genre;
  CScraperUrl thumbURL;
  std::vector<std::string> moods;
  std::vector<std::string> styles;
  std::vector<std::string> themes;
  std::map<std::string, std::string> art;
  std::string strReview;
  std::string strLabel;
  std::string strType;
  std::string strPath;
  std::string m_strDateOfRelease;
  float fRating = -1;
  int iUserrating = -1;
  int iVotes = -1;
  int iYear = -1;
  bool bCompilation = false;
  int iTimesPlayed = 0;
  CDateTime dateAdded;
  CDateTime lastPlayed;
  VECSONGS songs;     ///< Local songs
  ReleaseType releaseType = Album;
  std::string strLastScraped;
  bool bScrapedMBID = false;
  bool bArtistSongMerge = false;
};

typedef std::vector<CAlbum> VECALBUMS;
