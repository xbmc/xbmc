/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CSong;
class CArtist;
class CVariant;

#include "ReplayGain.h"
#include "XBDateTime.h"
#include "music/Album.h"
#include "utils/IArchivable.h"
#include "utils/ISerializable.h"
#include "utils/ISortable.h"

#include <string>
#include <vector>

namespace MUSIC_INFO
{
class CMusicInfoTag final : public IArchivable, public ISerializable, public ISortable
{
public:
  CMusicInfoTag(void);
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
  int GetTotalDiscs() const;
  int GetDuration() const;  // may be set even if Loaded() returns false
  int GetYear() const;
  const std::string& GetReleaseDate() const;
  const std::string GetReleaseYear() const;
  const std::string& GetOriginalDate() const;
  const std::string GetOriginalYear() const;
  int GetDatabaseId() const;
  const std::string &GetType() const;
  const std::string& GetDiscSubtitle() const;
  int GetBPM() const;
  std::string GetYearString() const;
  const std::string& GetMusicBrainzTrackID() const;
  const std::vector<std::string>& GetMusicBrainzArtistID() const;
  const std::vector<std::string>& GetMusicBrainzArtistHints() const;
  const std::string& GetMusicBrainzAlbumID() const;
  const std::string& GetMusicBrainzReleaseGroupID() const;
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
  bool GetBoxset() const;
  float GetRating() const;
  int GetUserrating() const;
  int GetVotes() const;
  int GetListeners() const;
  int GetPlayCount() const;
  int GetBitRate() const;
  int GetNoOfChannels() const;
  int GetSampleRate() const;
  const std::string& GetAlbumReleaseStatus() const;
  const std::string& GetStationName() const;
  const std::string& GetStationArt() const;
  const std::string& GetSongVideoURL() const;
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
  void SetGenre(const std::string& strGenre, bool bTrim = false);
  void SetGenre(const std::vector<std::string>& genres, bool bTrim = false);
  void SetYear(int year);
  void SetOriginalDate(const std::string& strOriginalDate);
  void SetReleaseDate(const std::string& strReleaseDate);
  void SetDatabaseId(int id, const std::string &type);
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
  void SetMusicBrainzReleaseGroupID(const std::string& strReleaseGroupID);
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
  void SetDateAdded(const CDateTime& dateAdded);
  void SetDateUpdated(const std::string& strDateUpdated);
  void SetDateUpdated(const CDateTime& dateUpdated);
  void SetDateNew(const std::string& strDateNew);
  void SetDateNew(const CDateTime& dateNew);
  void SetCompilation(bool compilation);
  void SetBoxset(bool boxset);
  void SetCoverArtInfo(size_t size, const std::string &mimeType);
  void SetReplayGain(const ReplayGain& aGain);
  void SetAlbumReleaseType(CAlbum::ReleaseType releaseType);
  void SetType(const MediaType& mediaType);
  void SetDiscSubtitle(const std::string& strDiscSubtitle);
  void SetTotalDiscs(int iDiscTotal);
  void SetBPM(int iBPM);
  void SetBitRate(int bitrate);
  void SetNoOfChannels(int channels);
  void SetSampleRate(int samplerate);
  void SetAlbumReleaseStatus(const std::string& strReleaseStatus);
  void SetStationName(const std::string& strStationName); // name of online radio station
  void SetStationArt(const std::string& strStationArt);
  void SetSongVideoURL(const std::string& songVideoURL); // link to video of song

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
  void AddOriginalDate(const std::string& strDateYear);
  void AddReleaseDate(const std::string& strDateYear, bool isMonth = false);

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
  std::string m_strMusicBrainzReleaseGroupID;
  std::string m_strMusicBrainzReleaseType;
  VECMUSICROLES m_musicRoles; //Artists contributing to the recording and role (from tags other than ARTIST or ALBUMARTIST)
  std::string m_strComment;
  std::string m_strMood;
  std::string m_strRecordLabel;
  std::string m_strLyrics;
  std::string m_cuesheet;
  std::string m_strDiscSubtitle;
  std::string m_strReleaseDate; //ISO8601 date YYYY, YYYY-MM or YYYY-MM-DD
  std::string m_strOriginalDate; //ISO8601 date YYYY, YYYY-MM or YYYY-MM-DD
  CDateTime m_lastPlayed;
  CDateTime m_dateNew;
  CDateTime m_dateAdded;
  CDateTime m_dateUpdated;
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
  int m_iDiscTotal;
  bool m_bBoxset;
  int m_iBPM;
  CAlbum::ReleaseType m_albumReleaseType;
  std::string m_strReleaseStatus;
  int m_samplerate;
  int m_channels;
  int m_bitrate;
  std::string m_stationName;
  std::string m_stationArt; // Used to fetch thumb URL for Shoutcasts
  std::string m_songVideoURL; // link to a video for a song

  EmbeddedArtInfo m_coverArt; ///< art information

  ReplayGain m_replayGain; ///< ReplayGain information
};
}
