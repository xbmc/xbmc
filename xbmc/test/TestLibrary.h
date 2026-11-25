/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "dbwrappers/test/DatabaseTestUtils.h"
#include "music/MusicDatabase.h"
#include "utils/Artwork.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace KODI::TEST
{
/*!
 * \brief The details of the video at \p fileAndPath as a scan gives them, its paths added to \p db
 * as a scan adds them. It is titled \p title, or with its file name.
 */
inline CVideoInfoTag VideoTag(CVideoDatabase& db,
                              const std::string& fileAndPath,
                              const std::string& title = "")
{
  CVideoInfoTag tag;
  tag.m_strTitle = title.empty() ? URIUtils::GetFileName(fileAndPath) : title;
  tag.m_strFileNameAndPath = fileAndPath;
  tag.m_strPath = URIUtils::GetDirectory(fileAndPath);
  tag.m_basePath = CFileItem(fileAndPath, false).GetBaseMoviePath(false);
  tag.m_parentPathID = db.AddPath(URIUtils::GetParentPath(tag.m_basePath));
  return tag;
}

//! \return the movie's id; it is in \p set unless that is empty
inline int AddMovie(CVideoDatabase& db,
                    const std::string& fileAndPath,
                    const std::string& title = "",
                    const std::string& set = "")
{
  CVideoInfoTag tag{VideoTag(db, fileAndPath, title)};
  if (!set.empty())
    tag.SetSet(set);
  return db.SetDetailsForMovie(tag, ART::Artwork{});
}

//! \return the id of the show in \p folders, the first its own, with whatever else \p details holds
inline int AddTvShow(CVideoDatabase& db,
                     const std::string& title,
                     const std::vector<std::string>& folders,
                     CVideoInfoTag details = {})
{
  details.m_strTitle = title;
  details.m_strPath = folders.front();
  return db.SetDetailsForTvShow(folders, details, ART::Artwork{}, ART::SeasonsArtwork{});
}

//! \return the id of the show's episode \p episode of season 1
inline int AddEpisode(CVideoDatabase& db,
                      int idShow,
                      const std::string& fileAndPath,
                      int episode,
                      const std::string& title = "")
{
  CVideoInfoTag tag{VideoTag(db, fileAndPath, title)};
  tag.m_iSeason = 1;
  tag.m_iEpisode = episode;
  return db.SetDetailsForEpisode(tag, ART::Artwork{}, idShow);
}
} // namespace KODI::TEST

/*!
 * \brief A music and a video library of their own for a test, which code that opens the
 * libraries itself then reads.
 */
class CTestLibrary
{
public:
  explicit CTestLibrary(const std::string& name)
    : m_music(&CAdvancedSettings::m_databaseMusic, name + "Music"),
      m_video(&CAdvancedSettings::m_databaseVideo, name + "Video")
  {
  }

  CMusicDatabase& Music() { return m_music.Get(); }
  CVideoDatabase& Video() { return m_video.Get(); }

  int AddMovie(const std::string& title, const std::string& set = "")
  {
    return KODI::TEST::AddMovie(Video(), "/videos/movies/" + title + ".mkv", title, set);
  }

  int AddTvShow(const std::string& title)
  {
    return KODI::TEST::AddTvShow(Video(), title, {ShowPath(title)});
  }

  int AddEpisode(int idShow, const std::string& show, const std::string& title, int episode)
  {
    return KODI::TEST::AddEpisode(Video(), idShow, ShowPath(show) + title + ".mkv", episode, title);
  }

  int AddMusicVideo(const std::string& title, const std::string& artist)
  {
    CVideoInfoTag tag{KODI::TEST::VideoTag(Video(), "/videos/clips/" + title + ".mkv", title)};
    tag.m_artist = {artist};
    return Video().SetDetailsForMusicVideo(tag, KODI::ART::Artwork{});
  }

  //! \return the song's id; its album and artist are added when the library lacks them
  int AddSong(const std::string& title,
              const std::string& album,
              const std::string& artist,
              int track = 1)
  {
    CMusicDatabase& db{Music()};
    const int idArtist{db.AddArtist(artist, "")};
    int idAlbum{
        Value(db.PrepareSQL("SELECT idAlbum FROM album WHERE strAlbum = '%s'", album.c_str()))};
    if (idAlbum <= 0)
    {
      EXPECT_TRUE(db.ExecuteQuery(
          db.PrepareSQL("INSERT INTO album (strAlbum, strArtistDisp, strReleaseType) "
                        "VALUES ('%s', '%s', 'album')",
                        album.c_str(), artist.c_str())));
      idAlbum = Value("SELECT MAX(idAlbum) FROM album");
      EXPECT_TRUE(db.ExecuteQuery(
          db.PrepareSQL("INSERT INTO album_artist (idArtist, idAlbum, iOrder, strArtist) "
                        "VALUES (%i, %i, 0, '%s')",
                        idArtist, idAlbum, artist.c_str())));
    }
    int idPath{Value("SELECT idPath FROM path WHERE strPath = '/music/'")};
    if (idPath <= 0)
    {
      EXPECT_TRUE(db.ExecuteQuery("INSERT INTO path (strPath) VALUES ('/music/')"));
      idPath = Value("SELECT MAX(idPath) FROM path");
    }
    EXPECT_TRUE(db.ExecuteQuery(db.PrepareSQL(
        "INSERT INTO song (idAlbum, idPath, strFileName, iTrack, strTitle, strArtistDisp) "
        "VALUES (%i, %i, '%s.flac', %i, '%s', '%s')",
        idAlbum, idPath, title.c_str(), track, title.c_str(), artist.c_str())));
    const int idSong{Value("SELECT MAX(idSong) FROM song")};
    EXPECT_TRUE(db.ExecuteQuery(
        db.PrepareSQL("INSERT INTO song_artist (idArtist, idSong, idRole, iOrder, strArtist) "
                      "VALUES (%i, %i, 1, 0, '%s')",
                      idArtist, idSong, artist.c_str())));
    return idSong;
  }

private:
  static std::string ShowPath(const std::string& show) { return "/videos/tv/" + show + "/"; }

  int Value(const std::string& sql)
  {
    const std::string value{Music().GetSingleValue(sql)};
    return value.empty() ? -1 : std::stoi(value);
  }

  CScopedTestDatabase<CMusicDatabase> m_music;
  CScopedTestDatabase<CVideoDatabase> m_video;
};
