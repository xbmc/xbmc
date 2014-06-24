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
class CAlbum;
class CArtist;

#include <vector>
#include <string>
#include <stdint.h>

#include "utils/IArchivable.h"
#include "utils/ISerializable.h"
#include "utils/ISortable.h"
#include "XBDateTime.h"

#define REPLAY_GAIN_HAS_TRACK_INFO 1
#define REPLAY_GAIN_HAS_ALBUM_INFO 2
#define REPLAY_GAIN_HAS_TRACK_PEAK 4
#define REPLAY_GAIN_HAS_ALBUM_PEAK 8

enum ReplayGain
{
  REPLAY_GAIN_NONE  = 0,
  REPLAY_GAIN_ALBUM,
  REPLAY_GAIN_TRACK
};

namespace MUSIC_INFO
{
  class EmbeddedArtInfo : public IArchivable
  {
  public:
    EmbeddedArtInfo() {};
    EmbeddedArtInfo(size_t size, const std::string &mime);
    void set(size_t size, const std::string &mime);
    void clear();
    bool empty() const;
    bool matches(const EmbeddedArtInfo &right) const;
    virtual void Archive(CArchive& ar);
    size_t      size;
    std::string mime;
  };

  class EmbeddedArt : public EmbeddedArtInfo
  {
  public:
    EmbeddedArt() {};
    EmbeddedArt(const uint8_t *data, size_t size, const std::string &mime);
    void set(const uint8_t *data, size_t size, const std::string &mime);
    std::vector<uint8_t> data;
  };

class CMusicInfoTag : public IArchivable, public ISerializable, public ISortable
{
public:
  CMusicInfoTag(void);
  CMusicInfoTag(const CMusicInfoTag& tag);
  virtual ~CMusicInfoTag();
  const CMusicInfoTag& operator =(const CMusicInfoTag& tag);
  bool operator !=(const CMusicInfoTag& tag) const;
  bool Loaded() const;
  const std::string& GetTitle() const;
  const std::string& GetURL() const;
  const std::vector<std::string>& GetArtist() const;
  const std::string& GetAlbum() const;
  int GetAlbumId() const;
  const std::vector<std::string>& GetAlbumArtist() const;
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
  const std::string& GetMusicBrainzAlbumID() const;
  const std::vector<std::string>& GetMusicBrainzAlbumArtistID() const;
  const std::string& GetMusicBrainzTRMID() const;
  const std::string& GetComment() const;
  const std::string& GetLyrics() const;
  const CDateTime& GetLastPlayed() const;
  bool  GetCompilation() const;
  char  GetRating() const;
  int  GetListeners() const;
  int  GetPlayCount() const;
  const EmbeddedArtInfo &GetCoverArtInfo() const;
  int   GetReplayGainTrackGain() const;
  int   GetReplayGainAlbumGain() const;
  float GetReplayGainTrackPeak() const;
  float GetReplayGainAlbumPeak() const;
  int   HasReplayGainInfo() const;

  void SetURL(const std::string& strURL);
  void SetTitle(const std::string& strTitle);
  void SetArtist(const std::string& strArtist);
  void SetArtist(const std::vector<std::string>& artists);
  void SetAlbum(const std::string& strAlbum);
  void SetAlbumId(const int iAlbumId);
  void SetAlbumArtist(const std::string& strAlbumArtist);
  void SetAlbumArtist(const std::vector<std::string>& albumArtists);
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
  void SetMusicBrainzAlbumID(const std::string& strAlbumID);
  void SetMusicBrainzAlbumArtistID(const std::vector<std::string>& musicBrainzAlbumArtistId);
  void SetMusicBrainzTRMID(const std::string& strTRMID);
  void SetComment(const std::string& comment);
  void SetLyrics(const std::string& lyrics);
  void SetRating(char rating);
  void SetListeners(int listeners);
  void SetPlayCount(int playcount);
  void SetLastPlayed(const std::string& strLastPlayed);
  void SetLastPlayed(const CDateTime& strLastPlayed);
  void SetCompilation(bool compilation);
  void SetCoverArtInfo(size_t size, const std::string &mimeType);
  void SetReplayGainTrackGain(int trackGain);
  void SetReplayGainAlbumGain(int albumGain);
  void SetReplayGainTrackPeak(float trackPeak);
  void SetReplayGainAlbumPeak(float albumPeak);

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

  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& ar) const;
  virtual void ToSortable(SortItem& sortable, Field field) const;

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
  std::string m_strAlbum;
  std::vector<std::string> m_albumArtist;
  std::vector<std::string> m_genre;
  std::string m_strMusicBrainzTrackID;
  std::vector<std::string> m_musicBrainzArtistID;
  std::string m_strMusicBrainzAlbumID;
  std::vector<std::string> m_musicBrainzAlbumArtistID;
  std::string m_strMusicBrainzTRMID;
  std::string m_strComment;
  std::string m_strLyrics;
  CDateTime m_lastPlayed;
  bool m_bCompilation;
  int m_iDuration;
  int m_iTrack;     // consists of the disk number in the high 16 bits, the track number in the low 16bits
  int m_iDbId;
  MediaType m_type; ///< item type "song", "album", "artist"
  bool m_bLoaded;
  char m_rating;
  int m_listeners;
  int m_iTimesPlayed;
  int m_iAlbumId;
  SYSTEMTIME m_dwReleaseDate;

  // ReplayGain
  int m_iTrackGain; // measured in milliBels
  int m_iAlbumGain;
  float m_fTrackPeak; // 1.0 == full digital scale
  float m_fAlbumPeak;
  int m_iHasGainInfo;   // valid info
  EmbeddedArtInfo m_coverArt; ///< art information
};
}
