/*!
 \file Album.h
\brief
*/
#pragma once

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

#include <map>
#include <vector>
#include "Artist.h"
#include "Song.h"
#include "utils/ScraperUrl.h"

class TiXmlNode;
class CFileItem;
class CAlbum
{
public:
  CAlbum(const CFileItem& item);
  CAlbum() { idAlbum = 0; iRating = 0; iYear = 0; iTimesPlayed = 0; releaseType = Album; };
  bool operator<(const CAlbum &a) const;
  void MergeScrapedAlbum(const CAlbum& album, bool override = true);

  void Reset()
  {
    idAlbum = -1;
    strAlbum.clear();
    strMusicBrainzAlbumID.clear();
    artist.clear();
    artistCredits.clear();
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
    iRating=-1;
    iYear=-1;
    bCompilation = false;
    iTimesPlayed = 0;
    songs.clear();
    infoSongs.clear();
    releaseType = Album;
  }

  std::string GetArtistString() const;
  std::string GetGenreString() const;

  typedef enum ReleaseType {
    Album = 0,
    Single
  } ReleaseType;

  std::string GetReleaseType() const;
  void SetReleaseType(const std::string& strReleaseType);

  static std::string ReleaseTypeToString(ReleaseType releaseType);
  static ReleaseType ReleaseTypeFromString(const std::string& strReleaseType);

  /*! \brief Load album information from an XML file.
   See CVideoInfoTag::Load for a description of the types of elements we load.
   \param element    the root XML element to parse.
   \param append     whether information should be added to the existing tag, or whether it should be reset first.
   \param prioritise if appending, whether additive tags should be prioritised (i.e. replace or prepend) over existing values. Defaults to false.
   \sa CVideoInfoTag::Load
   */
  bool Load(const TiXmlElement *element, bool append = false, bool prioritise = false);
  bool Save(TiXmlNode *node, const std::string &tag, const std::string& strPath);

  long idAlbum;
  std::string strAlbum;
  std::string strMusicBrainzAlbumID;
  std::vector<std::string> artist;
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
  int iRating;
  int iYear;
  bool bCompilation;
  int iTimesPlayed;
  VECSONGS songs;     ///< Local songs
  VECSONGS infoSongs; ///< Scraped songs
  ReleaseType releaseType;
};

typedef std::vector<CAlbum> VECALBUMS;
