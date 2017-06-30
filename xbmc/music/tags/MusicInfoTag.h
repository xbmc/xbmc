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

class CSong;
class CArtist;
class CVariant;

#include <stdint.h>
#include <string>
#include <vector>

#include "ReplayGain.h"
#include "XBDateTime.h"
#include "music/Album.h"
#include "music/Artist.h"
#include "music/EmbeddedArt.h"
#include "utils/IArchivable.h"
#include "utils/ISerializable.h"
#include "utils/ISortable.h"


namespace MUSIC_INFO
{
class CMusicInfoTag : public IArchivable, public ISerializable, public ISortable
{
public:
  CMusicInfoTag(void);
  CMusicInfoTag(const CMusicInfoTag& tag);
  ~CMusicInfoTag() override;
  const CMusicInfoTag& operator =(const CMusicInfoTag& tag);
  bool operator !=(const CMusicInfoTag& tag) const;
  bool Loaded() const;
  const std::string& GetTitle() const;
  const std::string& GetURL() const;
  const std::vector<std::string>& GetArtist() const;
  const std::string& GetArtistSort() const;
  const std::string GetArtistString() const;
  const std::string& GetComposerSort() const;
  const std::string& GetAlbum() const;
  int GetAlbumId() const;
  const std::vector<std::string>& GetAlbumArtist() const;
  const std::string GetAlbumArtistString() const;
  const std::string& GetAlbumArtistSort() const;
  const std::vector<std::string>& GetGenre() const;
  int GetTrackNumber() const;
  int GetDiscNumber() const;
  int GetTrackAndDiscNumber() const;
  int GetDuration() const;  // may be set even if Loaded() returns false
  int GetYear() const;
  int GetDatabaseId() const;
  const std::string &GetType() const;

  void GetReleaseDate(SYSTEMTIME& dateTime) const;
  std::string GetYearString() const;
  const std::string& GetMusicBrainzTrackID() const;
  const std::vector<std::string>& GetMusicBrainzArtistID() const;
  const std::vector<std::string>& GetMusicBrainzArtistHints() const;
  const std::string& GetMusicBrainzAlbumID() const;
  const std::vector<std::string>& GetMusicBrainzAlbumArtistID() const;
  const std::vector<std::string>& GetMusicBrainzAlbumArtistHints() const;
  const std::string& GetMusicBrainzReleaseType() const;
  const std::string& GetComment() const;
  const std::string& GetMood() const;
  const std::string& GetRecordLabel() const;
  const std::string& GetLyrics() const;
  const std::string& GetCueSheet() const;
  const CDateTime& GetLastPlayed() const;
  const CDateTime& GetDateAdded() const;
  bool  GetCompilation() const;
  float GetRating() const;
  int GetUserrating() const;
  int GetVotes() const;
  int GetListeners() const;
  int GetPlayCount() const;
  const EmbeddedArtInfo &GetCoverArtInfo() const;
  const ReplayGain& GetReplayGain() const;
  CAlbum::ReleaseType GetAlbumReleaseType() const;

  void SetURL(const std::string& strURL);
  void SetTitle(const std::string& strTitle);
  void SetArtist(const std::string& strArtist);
  void SetArtist(const std::vector<std::string>& artists, bool FillDesc = false);
  void SetArtistDesc(const std::string& strArtistDesc);
  void SetArtistSort(const std::string& strArtistsort);
  void SetComposerSort(const std::string& strComposerSort);
  void SetAlbum(const std::string& strAlbum);
  void SetAlbumId(const int iAlbumId);
  void SetAlbumArtist(const std::string& strAlbumArtist);
  void SetAlbumArtist(const std::vector<std::string>& albumArtists, bool FillDesc = false);
  void SetAlbumArtistDesc(const std::string& strAlbumArtistDesc);
  void SetAlbumArtistSort(const std::string& strAlbumArtistSort);
  void SetGenre(const std::string& strGenre);
  void SetGenre(const std::vector<std::string>& genres);
  void SetYear(int year);
  void SetDatabaseId(long id, const std::string &type);
  void SetReleaseDate(SYSTEMTIME& dateTime);
  void SetTrackNumber(int iTrack);
  void SetDiscNumber(int iDiscNumber);
  void SetTrackAndDiscNumber(int iTrackAndDisc);
  void SetDuration(int iSec);
  void SetLoaded(bool bOnOff = true);
  void SetArtist(const CArtist& artist);
  void SetAlbum(const CAlbum& album);
  void SetSong(const CSong& song);  
  void SetMusicBrainzTrackID(const std::string& strTrackID);
  void SetMusicBrainzArtistID(const std::vector<std::string>& musicBrainzArtistId);
  void SetMusicBrainzArtistHints(const std::vector<std::string>& musicBrainzArtistHints);
  void SetMusicBrainzAlbumID(const std::string& strAlbumID);
  void SetMusicBrainzAlbumArtistID(const std::vector<std::string>& musicBrainzAlbumArtistId);
  void SetMusicBrainzAlbumArtistHints(const std::vector<std::string>& musicBrainzAlbumArtistHints);
  void SetMusicBrainzReleaseType(const std::string& ReleaseType);
  void SetComment(const std::string& comment);
  void SetMood(const std::string& mood);
  void SetRecordLabel(const std::string& publisher);
  void SetLyrics(const std::string& lyrics);
  void SetCueSheet(const std::string& cueSheet);
  void SetRating(float rating);
  void SetUserrating(int rating);
  void SetVotes(int votes);
  void SetListeners(int listeners);
  void SetPlayCount(int playcount);
  void SetLastPlayed(const std::string& strLastPlayed);
  void SetLastPlayed(const CDateTime& strLastPlayed);
  void SetDateAdded(const std::string& strDateAdded);
  void SetDateAdded(const CDateTime& strDateAdded);
  void SetCompilation(bool compilation);
  void SetCoverArtInfo(size_t size, const std::string &mimeType);
  void SetReplayGain(const ReplayGain& aGain);
  void SetAlbumReleaseType(CAlbum::ReleaseType releaseType);
  void SetType(const MediaType mediaType);

  /*! \brief Append a unique artist to the artist list
   Checks if we have this artist already added, and if not adds it to the songs artist list.
   \param value artist to add.
   */
  void AppendArtist(const std::string &artist);

  /*! \brief Append a unique album artist to the artist list
   Checks if we have this album artist already added, and if not adds it to the songs album artist list.
   \param albumArtist album artist to add.
   */
  void AppendAlbumArtist(const std::string &albumArtist);

  /*! \brief Append a unique genre to the genre list
   Checks if we have this genre already added, and if not adds it to the songs genre list.
   \param genre genre to add.
   */
  void AppendGenre(const std::string &genre);
  
  void AddArtistRole(const std::string& Role, const std::string& strArtist);
  void AddArtistRole(const std::string& Role, const std::vector<std::string>& artists);
  void AppendArtistRole(const CMusicRole& ArtistRole);
  const std::string GetArtistStringForRole(const std::string& strRole) const;
  const std::string GetContributorsText() const;
  const std::string GetContributorsAndRolesText() const;
  const VECMUSICROLES &GetContributors() const;
  void SetContributors(const VECMUSICROLES& contributors);
  bool HasContributors() const { return !m_musicRoles.empty(); }

  void Archive(CArchive& ar) override;
  void Serialize(CVariant& ar) const override;
  void ToSortable(SortItem& sortable, Field field) const override;

  void Clear();

protected:
  /*! \brief Trim whitespace off the given string
   \param value string to trim
   \return trimmed value, with spaces removed from left and right, as well as carriage returns from the right.
   */
  std::string Trim(const std::string &value) const;

  std::string m_strURL;
  std::string m_strTitle;
  std::vector<std::string> m_artist;
  std::string m_strArtistSort;
  std::string m_strArtistDesc;
  std::string m_strComposerSort;
  std::string m_strAlbum;
  std::vector<std::string> m_albumArtist;
  std::string m_strAlbumArtistDesc;
  std::string m_strAlbumArtistSort;
  std::vector<std::string> m_genre;
  std::string m_strMusicBrainzTrackID;
  std::vector<std::string> m_musicBrainzArtistID;
  std::vector<std::string> m_musicBrainzArtistHints;
  std::string m_strMusicBrainzAlbumID;
  std::vector<std::string> m_musicBrainzAlbumArtistID;
  std::vector<std::string> m_musicBrainzAlbumArtistHints;
  std::string m_strMusicBrainzReleaseType;
  VECMUSICROLES m_musicRoles; //Artists contributing to the recording and role (from tags other than ARTIST or ALBUMARTIST)
  std::string m_strComment;
  std::string m_strMood;
  std::string m_strRecordLabel;
  std::string m_strLyrics;
  std::string m_cuesheet;
  CDateTime m_lastPlayed;
  CDateTime m_dateAdded;
  bool m_bCompilation;
  int m_iDuration;
  int m_iTrack;     // consists of the disk number in the high 16 bits, the track number in the low 16bits
  int m_iDbId;
  MediaType m_type; ///< item type "music", "song", "album", "artist"
  bool m_bLoaded;
  float m_Rating;
  int m_Userrating;
  int m_Votes;
  int m_listeners;
  int m_iTimesPlayed;
  int m_iAlbumId;
  SYSTEMTIME m_dwReleaseDate;
  CAlbum::ReleaseType m_albumReleaseType;

  EmbeddedArtInfo m_coverArt; ///< art information

  ReplayGain m_replayGain; ///< ReplayGain information
};
}
