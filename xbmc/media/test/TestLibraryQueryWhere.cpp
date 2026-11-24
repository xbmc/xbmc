/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "dbwrappers/test/DatabaseTestUtils.h"
#include "media/LibraryQuery.h"
#include "media/MediaType.h"
#include "music/MusicDatabase.h"
#include "video/VideoDatabase.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

using namespace KODI;

namespace
{
struct WhereCase
{
  std::string_view type;
  //! One rule, or a group of them
  std::string_view rules;
  std::string_view where;
  bool useOriginalDate{false};
  //! The items the clause is for, when not the first the type lists
  std::optional<MEDIA::TYPE> itemType{};
};

void PrintTo(const WhereCase& c, std::ostream* os)
{
  *os << c.type << ' ' << c.rules << (c.useOriginalDate ? " (original dates)" : "");
  if (c.itemType)
  {
    *os << " for " << MEDIA::NameOf(*c.itemType);
  }
}

constexpr const char* MUSIC_DB_NAME = "TestLibraryQueryWhereMusic";
constexpr const char* VIDEO_DB_NAME = "TestLibraryQueryWhereVideo";

//! The clauses are formatted by a database, so each library has one, with nothing in it.
class TestLibraryQueryWhere : public ::testing::TestWithParam<WhereCase>
{
protected:
  static void SetUpTestSuite()
  {
    s_music = std::make_unique<CConnectedTestDatabase<CMusicDatabase>>(MUSIC_DB_NAME);
    s_video = std::make_unique<CConnectedTestDatabase<CVideoDatabase>>(VIDEO_DB_NAME);
  }

  static void TearDownTestSuite()
  {
    s_music.reset();
    s_video.reset();
  }

  static std::optional<std::string> WhereOf(const WhereCase& c)
  {
    const bool group{c.rules.starts_with("{\"and\"") || c.rules.starts_with("{\"or\"")};
    std::string json{"{\"type\":\""};
    json += c.type;
    json += "\",\"rules\":";
    json += group ? std::string{c.rules} : "{\"and\":[" + std::string{c.rules} + "]}";
    json += "}";

    MEDIA::CLibraryQuery playlist;
    if (!playlist.LoadFromJson(json))
    {
      ADD_FAILURE() << "could not load " << json;
      return std::nullopt;
    }
    const auto sources{MEDIA::LIBRARY_QUERY::SourcesOf(playlist.GetType())};
    const auto source{std::ranges::find_if(sources, [&c](const MEDIA::LIBRARY_QUERY::Source& s)
                                           { return !c.itemType || s.itemType == *c.itemType; })};
    if (source == sources.end())
    {
      ADD_FAILURE() << c.type << " lists no " << MEDIA::NameOf(*c.itemType);
      return std::nullopt;
    }
    const bool music{source->section == MEDIA::MediaSection::MUSIC};
    return playlist.GetWhereClause(music ? static_cast<const CDatabase&>(s_music->Get())
                                         : static_cast<const CDatabase&>(s_video->Get()),
                                   source->itemType, c.useOriginalDate);
  }

  static std::unique_ptr<CConnectedTestDatabase<CMusicDatabase>> s_music;
  static std::unique_ptr<CConnectedTestDatabase<CVideoDatabase>> s_video;
};

std::unique_ptr<CConnectedTestDatabase<CMusicDatabase>> TestLibraryQueryWhere::s_music;
std::unique_ptr<CConnectedTestDatabase<CVideoDatabase>> TestLibraryQueryWhere::s_video;

// clang-format off
const WhereCase SONGS_RULES[] = {
    {"songs", R"({"field":"title","operator":"contains","value":"love"})",
     R"sql(((songview.strTitle LIKE '%love%')))sql"},
    {"songs", R"({"field":"title","operator":"is","value":["a","b"]})",
     R"sql(((songview.strTitle LIKE 'a') OR (songview.strTitle LIKE 'b')))sql"},
    {"songs", R"({"field":"title","operator":"isnot","value":["a","b"]})",
     R"sql(((songview.strTitle NOT  LIKE 'a' OR songview.strTitle IS NULL) AND (songview.strTitle NOT  LIKE 'b' OR songview.strTitle IS NULL)))sql"},
    {"songs", R"({"field":"title","operator":"is","value":""})",
     R"sql(((songview.strTitle LIKE '' OR songview.strTitle IS NULL)))sql"},
    {"songs", R"({"field":"genre","operator":"is","value":"Rock"})",
     R"sql((( EXISTS (SELECT 1 FROM song_genre, genre WHERE song_genre.idSong = songview.idSong AND song_genre.idGenre = genre.idGenre AND genre.strGenre LIKE 'Rock'))))sql"},
    {"songs", R"({"field":"genre","operator":"isnot","value":"Rock"})",
     R"sql((( NOT  EXISTS (SELECT 1 FROM song_genre, genre WHERE song_genre.idSong = songview.idSong AND song_genre.idGenre = genre.idGenre AND genre.strGenre LIKE 'Rock'))))sql"},
    {"songs", R"({"field":"artist","operator":"is","value":"Queen"})",
     R"sql((( EXISTS (SELECT 1 FROM song_artist, artist WHERE song_artist.idSong = songview.idSong AND song_artist.idArtist = artist.idArtist AND artist.strArtist LIKE 'Queen'))))sql"},
    {"songs", R"({"field":"albumartist","operator":"doesnotcontain","value":"Queen"})",
     R"sql((( NOT  EXISTS (SELECT 1 FROM album_artist, artist WHERE album_artist.idAlbum = songview.idAlbum AND album_artist.idArtist = artist.idArtist AND artist.strArtist LIKE '%Queen%'))))sql"},
    {"songs", R"({"field":"album","operator":"startswith","value":"A"})",
     R"sql(((songview.strAlbum LIKE 'A%')))sql"},
    {"songs", R"({"field":"disctitle","operator":"endswith","value":"B"})",
     R"sql(((songview.strDiscSubtitle LIKE '%B')))sql"},
    {"songs", R"({"field":"lastplayed","operator":"before","value":"2020-01-01"})",
     R"sql(((songview.lastPlayed IS NULL OR songview.lastPlayed < '2020-01-01')))sql"},
    {"songs", R"({"field":"lastplayed","operator":"after","value":"2020-01-01"})",
     R"sql(((songview.lastPlayed IS NOT NULL AND songview.lastPlayed > '2020-01-01')))sql"},
    {"songs", R"({"field":"source","operator":"is","value":"Local"})",
     R"sql((( EXISTS (SELECT 1 FROM album_source, source WHERE album_source.idAlbum = songview.idAlbum AND album_source.idSource = source.idSource AND source.strName LIKE 'Local'))))sql"},
    {"songs", R"({"field":"year","operator":"is","value":"1999"})",
     R"sql(((CAST(songview.strReleaseDate as INTEGER) = 1999)))sql"},
    {"songs", R"({"field":"year","operator":"is","value":"0"})",
     R"sql((((TRIM(songview.strReleaseDate) = '' OR songview.strReleaseDate IS NULL))))sql"},
    {"songs", R"({"field":"year","operator":"isnot","value":"0"})",
     R"sql((((TRIM(songview.strReleaseDate) <> '' AND songview.strReleaseDate IS NOT NULL))))sql"},
    {"songs", R"({"field":"year","operator":"lessthan","value":"1980"})",
     R"sql((((TRIM(songview.strReleaseDate) = '' OR songview.strReleaseDate IS NULL OR CAST(songview.strReleaseDate as INTEGER) < 1980))))sql"},
    {"songs", R"({"field":"year","operator":"between","value":["1980","1989"]})",
     R"sql((CAST(songview.strReleaseDate as DECIMAL(5,1)) BETWEEN 1980 AND 1989))sql"},
    {"songs", R"({"field":"originalyear","operator":"greaterthan","value":"1970"})",
     R"sql(((CAST(songview.strOrigReleaseDate as INTEGER) > 1970)))sql"},
    {"songs", R"({"field":"year","operator":"is","value":"1999"})",
     R"sql(((CAST(songview.strOrigReleaseDate as INTEGER) = 1999)))sql", true},
    {"songs", R"({"field":"originalyear","operator":"is","value":"1999"})",
     R"sql(((CAST(songview.strOrigReleaseDate as INTEGER) = 1999)))sql", true},
    {"songs", R"({"field":"time","operator":"greaterthan","value":"3:30"})",
     R"sql(((CAST(songview.iDuration as INTEGER) > 210)))sql"},
    {"songs", R"({"field":"time","operator":"between","value":["60","120"]})",
     R"sql((CAST(songview.iDuration as INTEGER) BETWEEN 60 AND 120))sql"},
    {"songs", R"({"field":"tracknumber","operator":"lessthan","value":"3"})",
     R"sql(((CAST(songview.iTrack as DECIMAL(6,1)) < 3)))sql"},
    {"songs", R"({"field":"playcount","operator":"is","value":"0"})",
     R"sql(((CAST(songview.iTimesPlayed as DECIMAL(6,1)) = 0)))sql"},
    {"songs", R"({"field":"rating","operator":"between","value":["3","8"]})",
     R"sql((songview.rating BETWEEN 3 AND 8))sql"},
    {"songs", R"({"field":"userrating","operator":"greaterthan","value":"5"})",
     R"sql(((songview.userrating IS NOT NULL AND songview.userrating > 5)))sql"},
    {"songs", R"({"field":"userrating","operator":"is","value":"0"})",
     R"sql(((songview.userrating IS NULL OR songview.userrating = 0)))sql"},
    {"songs", R"({"field":"comment","operator":"doesnotcontain","value":"live"})",
     R"sql(((songview.comment NOT  LIKE '%live%' OR songview.comment IS NULL)))sql"},
    {"songs", R"({"field":"filename","operator":"endswith","value":".flac"})",
     R"sql(((songview.strFilename LIKE '%.flac')))sql"},
    {"songs", R"({"field":"path","operator":"startswith","value":"/music/"})",
     R"sql(((songview.strPath LIKE '/music/%')))sql"},
    {"songs", R"({"field":"dateadded","operator":"after","value":"2020-01-01"})",
     R"sql(((songview.dateAdded > '2020-01-01')))sql"},
    {"songs", R"({"field":"bpm","operator":"greaterthan","value":"120"})",
     R"sql(((CAST(songview.iBPM as DECIMAL(6,1)) > 120)))sql"},
    {"songs", R"({"field":"title","operator":"true"})",
     R"sql(('1'))sql"},
    {"songs", R"({"field":"title","operator":"between","value":["a"]})",
     R"sql(('1'))sql"},
    {"songs", R"({"or":[{"field":"title","operator":"between","value":["a"]},{"field":"year","operator":"is","value":"1999"}]})",
     R"sql(('0') OR ((CAST(songview.strReleaseDate as INTEGER) = 1999)))sql"},
    {"songs", R"({"and":[{"or":[{"field":"genre","operator":"is","value":"Rock"},{"field":"genre","operator":"is","value":"Pop"}]},{"and":[{"field":"year","operator":"greaterthan","value":"1990"},{"field":"artist","operator":"contains","value":"x"}]},{"field":"playcount","operator":"lessthan","value":"5"}]})",
     R"sql(((( EXISTS (SELECT 1 FROM song_genre, genre WHERE song_genre.idSong = songview.idSong AND song_genre.idGenre = genre.idGenre AND genre.strGenre LIKE 'Rock'))) OR (( EXISTS (SELECT 1 FROM song_genre, genre WHERE song_genre.idSong = songview.idSong AND song_genre.idGenre = genre.idGenre AND genre.strGenre LIKE 'Pop')))) AND (((CAST(songview.strReleaseDate as INTEGER) > 1990)) AND (( EXISTS (SELECT 1 FROM song_artist, artist WHERE song_artist.idSong = songview.idSong AND song_artist.idArtist = artist.idArtist AND artist.strArtist LIKE '%x%')))) AND ((CAST(songview.iTimesPlayed as DECIMAL(6,1)) < 5)))sql"},
    {"songs", R"({"and":[]})",
     R"sql()sql"},
};

const WhereCase ALBUMS_RULES[] = {
    {"albums", R"({"field":"album","operator":"contains","value":"Hits"})",
     R"sql(((albumview.strAlbum LIKE '%Hits%')))sql"},
    {"albums", R"({"field":"genre","operator":"contains","value":"Rock"})",
     R"sql((( EXISTS (SELECT 1 FROM song, song_genre, genre WHERE song.idAlbum = albumview.idAlbum AND song.idSong = song_genre.idSong AND song_genre.idGenre = genre.idGenre AND genre.strGenre LIKE '%Rock%'))))sql"},
    {"albums", R"({"field":"artist","operator":"is","value":"Queen"})",
     R"sql((( EXISTS (SELECT 1 FROM song, song_artist, artist WHERE song.idAlbum = albumview.idAlbum AND song.idSong = song_artist.idSong AND song_artist.idArtist = artist.idArtist AND artist.strArtist LIKE 'Queen'))))sql"},
    {"albums", R"({"field":"albumartist","operator":"is","value":"Queen"})",
     R"sql((( EXISTS (SELECT 1 FROM album_artist, artist WHERE album_artist.idAlbum = albumview.idAlbum AND album_artist.idArtist = artist.idArtist AND artist.strArtist LIKE 'Queen'))))sql"},
    {"albums", R"({"field":"path","operator":"contains","value":"/music/"})",
     R"sql((( EXISTS (SELECT 1 FROM song JOIN path on song.idpath = path.idpath WHERE song.idAlbum = albumview.idAlbum AND path.strPath LIKE '%/music/%'))))sql"},
    {"albums", R"({"field":"lastplayed","operator":"before","value":"2020-01-01"})",
     R"sql(((albumview.lastPlayed IS NULL OR albumview.lastPlayed < '2020-01-01')))sql"},
    {"albums", R"({"field":"source","operator":"is","value":"Local"})",
     R"sql((( EXISTS (SELECT 1 FROM album_source, source WHERE album_source.idAlbum = albumview.idAlbum AND album_source.idSource = source.idSource AND source.strName LIKE 'Local'))))sql"},
    {"albums", R"({"field":"disctitle","operator":"contains","value":"Live"})",
     R"sql((( EXISTS (SELECT 1 FROM song WHERE song.idAlbum = albumview.idAlbum AND song.strDiscSubtitle LIKE '%Live%'))))sql"},
    {"albums", R"({"field":"year","operator":"is","value":"2001"})",
     R"sql(((CAST(albumview.strReleaseDate as INTEGER) = 2001)))sql"},
    {"albums", R"({"field":"year","operator":"is","value":"2001"})",
     R"sql(((CAST(albumview.strOrigReleaseDate as INTEGER) = 2001)))sql", true},
    {"albums", R"({"field":"originalyear","operator":"isnot","value":"0"})",
     R"sql((((TRIM(albumview.strOrigReleaseDate) <> '' AND albumview.strOrigReleaseDate IS NOT NULL))))sql"},
    {"albums", R"({"field":"compilation","operator":"true"})",
     R"sql((albumview.bCompilation))sql"},
    {"albums", R"({"field":"compilation","operator":"false"})",
     R"sql(( NOT albumview.bCompilation))sql"},
    {"albums", R"({"field":"compilation","operator":"is","value":"false"})",
     R"sql(( NOT albumview.bCompilation))sql"},
    {"albums", R"({"field":"isboxset","operator":"true"})",
     R"sql((albumview.bBoxedSet = 1))sql"},
    {"albums", R"({"field":"isboxset","operator":"false"})",
     R"sql(( NOT albumview.bBoxedSet = 1))sql"},
    {"albums", R"({"field":"albumduration","operator":"lessthan","value":"40:00"})",
     R"sql(((CAST(albumview.iAlbumDuration as INTEGER) < 2400)))sql"},
    {"albums", R"({"field":"type","operator":"is","value":"album"})",
     R"sql(((albumview.strType LIKE 'album')))sql"},
    {"albums", R"({"field":"label","operator":"is","value":"EMI"})",
     R"sql(((albumview.strLabel LIKE 'EMI')))sql"},
    {"albums", R"({"field":"playcount","operator":"greaterthan","value":"2"})",
     R"sql(((CAST(albumview.iTimesPlayed as DECIMAL(6,1)) > 2)))sql"},
    {"albums", R"({"field":"userrating","operator":"is","value":"0"})",
     R"sql(((albumview.iUserrating IS NULL OR albumview.iUserrating = 0)))sql"},
    {"albums", R"({"field":"totaldiscs","operator":"greaterthan","value":"1"})",
     R"sql(((CAST(albumview.iDiscTotal as DECIMAL(6,1)) > 1)))sql"},
    {"albums", R"({"field":"albumstatus","operator":"is","value":"official"})",
     R"sql(((albumview.strReleaseStatus LIKE 'official')))sql"},
};

const WhereCase ARTISTS_RULES[] = {
    {"artists", R"({"field":"artist","operator":"contains","value":"Queen"})",
     R"sql(((artistview.strArtist LIKE '%Queen%')))sql"},
    {"artists", R"({"field":"genre","operator":"is","value":"Rock"})",
     R"sql((( (EXISTS (SELECT DISTINCT song_artist.idArtist FROM song_artist, song_genre, genre WHERE song_artist.idArtist = artistview.idArtist AND song_artist.idSong = song_genre.idSong AND song_genre.idGenre = genre.idGenre AND genre.strGenre LIKE 'Rock') OR EXISTS (SELECT DISTINCT album_artist.idArtist FROM album_artist, song, song_genre, genre WHERE album_artist.idArtist = artistview.idArtist AND song.idAlbum = album_artist.idAlbum AND song.idSong = song_genre.idSong AND song_genre.idGenre = genre.idGenre AND genre.strGenre LIKE 'Rock')))))sql"},
    {"artists", R"({"field":"role","operator":"is","value":"Composer"})",
     R"sql((( (EXISTS (SELECT DISTINCT song_artist.idArtist FROM song_artist, role WHERE song_artist.idArtist = artistview.idArtist AND song_artist.idRole = role.idRole AND role.strRole LIKE 'Composer')))))sql"},
    {"artists", R"({"field":"path","operator":"contains","value":"/music/"})",
     R"sql((( (EXISTS (SELECT DISTINCT song_artist.idArtist FROM song_artist JOIN song ON song.idSong = song_artist.idSong JOIN path ON song.idpath = path.idpath WHERE song_artist.idArtist = artistview.idArtist AND path.strPath LIKE '%/music/%')))))sql"},
    {"artists", R"({"field":"source","operator":"is","value":"Local"})",
     R"sql((( (EXISTS(SELECT 1 FROM song_artist, song, album_source, source WHERE song_artist.idArtist = artistview.idArtist AND song.idSong = song_artist.idSong AND song_artist.idRole = 1 AND album_source.idAlbum = song.idAlbum AND album_source.idSource = source.idSource AND source.strName LIKE 'Local') OR  EXISTS (SELECT 1 FROM album_artist, album_source, source WHERE album_artist.idArtist = artistview.idArtist AND album_source.idAlbum = album_artist.idAlbum AND album_source.idSource = source.idSource AND source.strName LIKE 'Local')))))sql"},
    {"artists", R"({"field":"gender","operator":"is","value":"female"})",
     R"sql(((artistview.strGender LIKE 'female')))sql"},
    {"artists", R"({"field":"born","operator":"contains","value":"1970"})",
     R"sql(((artistview.strBorn LIKE '%1970%')))sql"},
    {"artists", R"({"field":"dateadded","operator":"after","value":"2020-01-01"})",
     R"sql(((artistview.dateAdded > '2020-01-01')))sql"},
};

const WhereCase MOVIES_RULES[] = {
    {"movies", R"({"field":"title","operator":"contains","value":"Star"})",
     R"sql(((movie_view.c00 LIKE '%Star%')))sql"},
    {"movies", R"({"field":"originaltitle","operator":"isnot","value":"Star"})",
     R"sql(((movie_view.c16 NOT  LIKE 'Star' OR movie_view.c16 IS NULL)))sql"},
    {"movies", R"({"field":"genre","operator":"is","value":"Drama"})",
     R"sql((( EXISTS (SELECT 1 FROM genre_link         JOIN genre ON genre.genre_id=genre_link.genre_id         WHERE genre_link.media_id=movie_view.idMovie AND genre.name  LIKE 'Drama' AND genre_link.media_type = 'movie'))))sql"},
    {"movies", R"({"field":"genre","operator":"isnot","value":["Drama","Comedy"]})",
     R"sql((( NOT  EXISTS (SELECT 1 FROM genre_link         JOIN genre ON genre.genre_id=genre_link.genre_id         WHERE genre_link.media_id=movie_view.idMovie AND genre.name  LIKE 'Drama' AND genre_link.media_type = 'movie')) AND ( NOT  EXISTS (SELECT 1 FROM genre_link         JOIN genre ON genre.genre_id=genre_link.genre_id         WHERE genre_link.media_id=movie_view.idMovie AND genre.name  LIKE 'Comedy' AND genre_link.media_type = 'movie'))))sql"},
    {"movies", R"({"field":"director","operator":"is","value":"Jane"})",
     R"sql((( EXISTS (SELECT 1 FROM director_link         JOIN actor ON actor.actor_id=director_link.actor_id         WHERE director_link.media_id=movie_view.idMovie AND actor.name  LIKE 'Jane' AND director_link.media_type = 'movie'))))sql"},
    {"movies", R"({"field":"actor","operator":"contains","value":"Joe"})",
     R"sql((( EXISTS (SELECT 1 FROM actor_link         JOIN actor ON actor.actor_id=actor_link.actor_id         WHERE actor_link.media_id=movie_view.idMovie AND actor.name  LIKE '%Joe%' AND actor_link.media_type = 'movie'))))sql"},
    {"movies", R"({"field":"writers","operator":"is","value":"Jim"})",
     R"sql((( EXISTS (SELECT 1 FROM writer_link         JOIN actor ON actor.actor_id=writer_link.actor_id         WHERE writer_link.media_id=movie_view.idMovie AND actor.name  LIKE 'Jim' AND writer_link.media_type = 'movie'))))sql"},
    {"movies", R"({"field":"studio","operator":"is","value":"Acme"})",
     R"sql((( EXISTS (SELECT 1 FROM studio_link         JOIN studio ON studio.studio_id=studio_link.studio_id         WHERE studio_link.media_id=movie_view.idMovie AND studio.name  LIKE 'Acme' AND studio_link.media_type = 'movie'))))sql"},
    {"movies", R"({"field":"country","operator":"is","value":"UK"})",
     R"sql((( EXISTS (SELECT 1 FROM country_link         JOIN country ON country.country_id=country_link.country_id         WHERE country_link.media_id=movie_view.idMovie AND country.name  LIKE 'UK' AND country_link.media_type = 'movie'))))sql"},
    {"movies", R"({"field":"lastplayed","operator":"before","value":"2020-01-01"})",
     R"sql(((movie_view.lastPlayed IS NULL OR movie_view.lastPlayed < '2020-01-01')))sql"},
    {"movies", R"({"field":"dateadded","operator":"after","value":"2020-01-01"})",
     R"sql(((movie_view.dateAdded IS NOT NULL AND movie_view.dateAdded > '2020-01-01')))sql"},
    {"movies", R"({"field":"tag","operator":"is","value":"Favourite"})",
     R"sql((( EXISTS (SELECT 1 FROM tag_link         JOIN tag ON tag.tag_id=tag_link.tag_id         WHERE tag_link.media_id=movie_view.idMovie AND tag.name  LIKE 'Favourite' AND tag_link.media_type = 'movie'))))sql"},
    {"movies", R"({"field":"set","operator":"is","value":"Trilogy"})",
     R"sql(((movie_view.strSet LIKE 'Trilogy')))sql"},
    {"movies", R"({"field":"inprogress","operator":"true"})",
     R"sql((movie_view.idFile  IN (SELECT DISTINCT idFile FROM bookmark WHERE type = 1)))sql"},
    {"movies", R"({"field":"inprogress","operator":"false"})",
     R"sql((movie_view.idFile  NOT  IN (SELECT DISTINCT idFile FROM bookmark WHERE type = 1)))sql"},
    {"movies", R"({"field":"hastrailer","operator":"true"})",
     R"sql((movie_view.c19!= ''))sql"},
    {"movies", R"({"field":"hasversions","operator":"true"})",
     R"sql((movie_view.hasVideoVersions))sql"},
    {"movies", R"({"field":"hasextras","operator":"false"})",
     R"sql(( NOT movie_view.hasVideoExtras))sql"},
    {"movies", R"({"field":"videoresolution","operator":"is","value":"1080"})",
     R"sql(((movie_view.idFile IN (SELECT DISTINCT idFile FROM streamdetails WHERE iVideoWidth >= 1281 AND iVideoWidth <= 1920))))sql"},
    {"movies", R"({"field":"videoresolution","operator":"isnot","value":"2160"})",
     R"sql(((movie_view.idFile IN (SELECT DISTINCT idFile FROM streamdetails WHERE iVideoWidth < 1921 OR iVideoWidth > 2147483647))))sql"},
    {"movies", R"({"field":"videoresolution","operator":"greaterthan","value":"720"})",
     R"sql(((movie_view.idFile IN (SELECT DISTINCT idFile FROM streamdetails WHERE iVideoWidth > 1280))))sql"},
    {"movies", R"({"field":"videoresolution","operator":"lessthan","value":"540"})",
     R"sql(((movie_view.idFile IN (SELECT DISTINCT idFile FROM streamdetails WHERE iVideoWidth < 721))))sql"},
    {"movies", R"({"field":"audiochannels","operator":"is","value":"6"})",
     R"sql((( EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = movie_view.idFile AND iAudioChannels  = 6))))sql"},
    {"movies", R"({"field":"videocodec","operator":"is","value":"h264,hevc"})",
     R"sql((( EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = movie_view.idFile AND strVideoCodec  IN ('h264','hevc')))))sql"},
    {"movies", R"({"field":"audiocodec","operator":"isnot","value":"dts"})",
     R"sql((( NOT  EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = movie_view.idFile AND strAudioCodec  IN ('dts')))))sql"},
    {"movies", R"({"field":"audiolanguage","operator":"is","value":"eng"})",
     R"sql((( EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = movie_view.idFile AND strAudioLanguage  IN ('eng')))))sql"},
    {"movies", R"({"field":"subtitlelanguage","operator":"is","value":"eng"})",
     R"sql((( EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = movie_view.idFile AND strSubtitleLanguage  IN ('eng')))))sql"},
    {"movies", R"({"field":"videoaspect","operator":"greaterthan","value":"2"})",
     R"sql((( EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = movie_view.idFile AND fVideoAspect  > 2))))sql"},
    {"movies", R"({"field":"audiocount","operator":"greaterthan","value":"1"})",
     R"sql((( EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = movie_view.idFile AND streamdetails.iStreamtype = 1 GROUP BY streamdetails.idFile HAVING COUNT(streamdetails.iStreamType)  > 1))))sql"},
    {"movies", R"({"field":"subtitlecount","operator":"is","value":"0"})",
     R"sql((( EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = movie_view.idFile AND streamdetails.iStreamType = 2 GROUP BY streamdetails.idFile HAVING COUNT(streamdetails.iStreamType)  = 0))))sql"},
    {"movies", R"({"field":"hdrtype","operator":"is","value":"dolbyvision"})",
     R"sql((( EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = movie_view.idFile AND strHdrType  IN ('dolbyvision')))))sql"},
    {"movies", R"({"field":"hdrdetail","operator":"is","value":"profile5"})",
     R"sql((( EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = movie_view.idFile AND strHdrDetail  IN ('profile5')))))sql"},
    {"movies", R"({"field":"playcount","operator":"is","value":"0"})",
     R"sql(((movie_view.playCount IS NULL OR movie_view.playCount = 0)))sql"},
    {"movies", R"({"field":"playcount","operator":"greaterthan","value":"1"})",
     R"sql(((movie_view.playCount IS NOT NULL AND movie_view.playCount > 1)))sql"},
    {"movies", R"({"field":"userrating","operator":"isnot","value":"0"})",
     R"sql(((movie_view.userrating IS NOT NULL AND movie_view.userrating != 0)))sql"},
    {"movies", R"({"field":"year","operator":"is","value":"1999"})",
     R"sql(((CAST(movie_view.premiered as DECIMAL(6,1)) = 1999)))sql"},
    {"movies", R"({"field":"rating","operator":"greaterthan","value":"7.5"})",
     R"sql(((movie_view.rating > 7.5)))sql"},
    {"movies", R"({"field":"top250","operator":"lessthan","value":"100"})",
     R"sql(((CAST(movie_view.c13 as DECIMAL(6,1)) < 100)))sql"},
    {"movies", R"({"field":"time","operator":"greaterthan","value":"1:30:00"})",
     R"sql(((CAST(movie_view.c11 as INTEGER) > 5400)))sql"},
    {"movies", R"({"field":"mpaarating","operator":"contains","value":"PG"})",
     R"sql(((movie_view.c12 LIKE '%PG%')))sql"},
    {"movies", R"({"field":"filename","operator":"contains","value":"1080p"})",
     R"sql(((movie_view.strFilename LIKE '%1080p%')))sql"},
    {"movies", R"({"field":"path","operator":"startswith","value":"/videos/"})",
     R"sql(((movie_view.strPath LIKE '/videos/%')))sql"},
    {"movies", R"({"field":"votes","operator":"greaterthan","value":"1000"})",
     R"sql(((movie_view.votes > 1000)))sql"},
    {"movies", R"({"field":"plot","operator":"contains","value":"ship"})",
     R"sql(((movie_view.c01 LIKE '%ship%')))sql"},
};

const WhereCase TVSHOWS_RULES[] = {
    {"tvshows", R"({"field":"title","operator":"contains","value":"Star"})",
     R"sql(((tvshow_view.c00 LIKE '%Star%')))sql"},
    {"tvshows", R"({"field":"genre","operator":"is","value":"Drama"})",
     R"sql((( EXISTS (SELECT 1 FROM genre_link         JOIN genre ON genre.genre_id=genre_link.genre_id         WHERE genre_link.media_id=tvshow_view.idShow AND genre.name  LIKE 'Drama' AND genre_link.media_type = 'tvshow'))))sql"},
    {"tvshows", R"({"field":"director","operator":"is","value":"Jane"})",
     R"sql((( EXISTS (SELECT 1 FROM director_link         JOIN actor ON actor.actor_id=director_link.actor_id         WHERE director_link.media_id=tvshow_view.idShow AND actor.name  LIKE 'Jane' AND director_link.media_type = 'tvshow'))))sql"},
    {"tvshows", R"({"field":"actor","operator":"is","value":"Joe"})",
     R"sql((( EXISTS (SELECT 1 FROM actor_link         JOIN actor ON actor.actor_id=actor_link.actor_id         WHERE actor_link.media_id=tvshow_view.idShow AND actor.name  LIKE 'Joe' AND actor_link.media_type = 'tvshow'))))sql"},
    {"tvshows", R"({"field":"studio","operator":"is","value":"Acme"})",
     R"sql((( EXISTS (SELECT 1 FROM studio_link         JOIN studio ON studio.studio_id=studio_link.studio_id         WHERE studio_link.media_id=tvshow_view.idShow AND studio.name  LIKE 'Acme' AND studio_link.media_type = 'tvshow'))))sql"},
    {"tvshows", R"({"field":"mpaarating","operator":"is","value":"TV-14"})",
     R"sql((( (tvshow_view.c13 LIKE 'TV-14'))))sql"},
    {"tvshows", R"({"field":"lastplayed","operator":"after","value":"2020-01-01"})",
     R"sql(((tvshow_view.lastPlayed IS NOT NULL AND tvshow_view.lastPlayed > '2020-01-01')))sql"},
    {"tvshows", R"({"field":"dateadded","operator":"before","value":"2020-01-01"})",
     R"sql(((tvshow_view.dateAdded IS NULL OR tvshow_view.dateAdded < '2020-01-01')))sql"},
    {"tvshows", R"({"field":"playcount","operator":"is","value":"0"})",
     R"sql(((CASE WHEN COALESCE(tvshow_view.totalCount - tvshow_view.watchedcount, 0) > 0 THEN 0 ELSE 1 END  = 0)))sql"},
    {"tvshows", R"({"field":"tag","operator":"is","value":"Favourite"})",
     R"sql((( EXISTS (SELECT 1 FROM tag_link         JOIN tag ON tag.tag_id=tag_link.tag_id         WHERE tag_link.media_id=tvshow_view.idShow AND tag.name  LIKE 'Favourite' AND tag_link.media_type = 'tvshow'))))sql"},
    {"tvshows", R"({"field":"inprogress","operator":"true"})",
     R"sql(( ((tvshow_view.watchedcount > 0 AND tvshow_view.watchedcount < tvshow_view.totalCount) OR (tvshow_view.watchedcount = 0 AND EXISTS (SELECT 1 FROM episode_view WHERE episode_view.idShow = tvshow_view.idShow AND episode_view.resumeTimeInSeconds > 0)))))sql"},
    {"tvshows", R"({"field":"inprogress","operator":"false"})",
     R"sql(( NOT  ((tvshow_view.watchedcount > 0 AND tvshow_view.watchedcount < tvshow_view.totalCount) OR (tvshow_view.watchedcount = 0 AND EXISTS (SELECT 1 FROM episode_view WHERE episode_view.idShow = tvshow_view.idShow AND episode_view.resumeTimeInSeconds > 0)))))sql"},
    {"tvshows", R"({"field":"hastrailer","operator":"false"})",
     R"sql(( NOT tvshow_view.c16!= ''))sql"},
    {"tvshows", R"({"field":"year","operator":"is","value":"2010"})",
     R"sql(((CAST(tvshow_view.c05 as DECIMAL(6,1)) LIKE '%2010%')))sql"},
    {"tvshows", R"({"field":"year","operator":"isnot","value":"2010"})",
     R"sql(((CAST(tvshow_view.c05 as DECIMAL(6,1)) NOT  LIKE '%2010%' OR tvshow_view.c05 IS NULL)))sql"},
    {"tvshows", R"({"field":"year","operator":"greaterthan","value":"2010"})",
     R"sql(((CAST(tvshow_view.c05 as DECIMAL(6,1)) > 2010)))sql"},
    {"tvshows", R"({"field":"numepisodes","operator":"greaterthan","value":"10"})",
     R"sql(((tvshow_view.totalCount > 10)))sql"},
    {"tvshows", R"({"field":"numwatched","operator":"is","value":"0"})",
     R"sql(((tvshow_view.watchedcount = 0)))sql"},
    {"tvshows", R"({"field":"status","operator":"is","value":"Ended"})",
     R"sql(((tvshow_view.c02 LIKE 'Ended')))sql"},
    {"tvshows", R"({"field":"userrating","operator":"greaterthan","value":"7"})",
     R"sql(((tvshow_view.userrating IS NOT NULL AND tvshow_view.userrating > 7)))sql"},
};

const WhereCase EPISODES_RULES[] = {
    {"episodes", R"({"field":"title","operator":"contains","value":"Pilot"})",
     R"sql(((episode_view.c00 LIKE '%Pilot%')))sql"},
    {"episodes", R"({"field":"tvshow","operator":"is","value":"Show"})",
     R"sql(((episode_view.strTitle LIKE 'Show')))sql"},
    {"episodes", R"({"field":"genre","operator":"is","value":"Drama"})",
     R"sql((( EXISTS (SELECT 1 FROM genre_link         JOIN genre ON genre.genre_id=genre_link.genre_id         WHERE genre_link.media_id=episode_view.idShow AND genre.name  LIKE 'Drama' AND genre_link.media_type = 'tvshow'))))sql"},
    {"episodes", R"({"field":"tag","operator":"is","value":"Favourite"})",
     R"sql((( EXISTS (SELECT 1 FROM tag_link         JOIN tag ON tag.tag_id=tag_link.tag_id         WHERE tag_link.media_id=episode_view.idShow AND tag.name  LIKE 'Favourite' AND tag_link.media_type = 'tvshow'))))sql"},
    {"episodes", R"({"field":"director","operator":"is","value":"Jane"})",
     R"sql((( EXISTS (SELECT 1 FROM director_link         JOIN actor ON actor.actor_id=director_link.actor_id         WHERE director_link.media_id=episode_view.idEpisode AND actor.name  LIKE 'Jane' AND director_link.media_type = 'episode'))))sql"},
    {"episodes", R"({"field":"actor","operator":"is","value":"Joe"})",
     R"sql((( EXISTS (SELECT 1 FROM actor_link         JOIN actor ON actor.actor_id=actor_link.actor_id         WHERE actor_link.media_id=episode_view.idEpisode AND actor.name  LIKE 'Joe' AND actor_link.media_type = 'episode'))))sql"},
    {"episodes", R"({"field":"writers","operator":"is","value":"Jim"})",
     R"sql((( EXISTS (SELECT 1 FROM writer_link         JOIN actor ON actor.actor_id=writer_link.actor_id         WHERE writer_link.media_id=episode_view.idEpisode AND actor.name  LIKE 'Jim' AND writer_link.media_type = 'episode'))))sql"},
    {"episodes", R"({"field":"studio","operator":"is","value":"Acme"})",
     R"sql((( EXISTS (SELECT 1 FROM studio_link         JOIN studio ON studio.studio_id=studio_link.studio_id         WHERE studio_link.media_id=episode_view.idShow AND studio.name  LIKE 'Acme' AND studio_link.media_type = 'tvshow'))))sql"},
    {"episodes", R"({"field":"mpaarating","operator":"isnot","value":"TV-14"})",
     R"sql((( NOT  (episode_view.mpaa LIKE 'TV-14'))))sql"},
    {"episodes", R"({"field":"lastplayed","operator":"before","value":"2020-01-01"})",
     R"sql(((episode_view.lastPlayed IS NULL OR episode_view.lastPlayed < '2020-01-01')))sql"},
    {"episodes", R"({"field":"dateadded","operator":"after","value":"2020-01-01"})",
     R"sql(((episode_view.dateAdded IS NOT NULL AND episode_view.dateAdded > '2020-01-01')))sql"},
    {"episodes", R"({"field":"airdate","operator":"after","value":"2020-01-01"})",
     R"sql(((episode_view.c05 > '2020-01-01')))sql"},
    {"episodes", R"({"field":"year","operator":"is","value":"2010"})",
     R"sql(((CAST(episode_view.premiered as DECIMAL(6,1)) LIKE '%2010%')))sql"},
    {"episodes", R"({"field":"inprogress","operator":"true"})",
     R"sql((episode_view.idFile  IN (SELECT DISTINCT idFile FROM bookmark WHERE type = 1)))sql"},
    {"episodes", R"({"field":"inprogress","operator":"false"})",
     R"sql((episode_view.idFile  NOT  IN (SELECT DISTINCT idFile FROM bookmark WHERE type = 1)))sql"},
    {"episodes", R"({"field":"episode","operator":"is","value":"3"})",
     R"sql(((CAST(episode_view.c13 as DECIMAL(6,1)) = 3)))sql"},
    {"episodes", R"({"field":"season","operator":"greaterthan","value":"1"})",
     R"sql(((CAST(episode_view.c12 as DECIMAL(6,1)) > 1)))sql"},
    {"episodes", R"({"field":"playcount","operator":"is","value":"0"})",
     R"sql(((episode_view.playCount IS NULL OR episode_view.playCount = 0)))sql"},
    {"episodes", R"({"field":"userrating","operator":"lessthan","value":"5"})",
     R"sql(((episode_view.userrating IS NULL OR episode_view.userrating < 5)))sql"},
    {"episodes", R"({"field":"videoresolution","operator":"lessthan","value":"720"})",
     R"sql(((episode_view.idFile IN (SELECT DISTINCT idFile FROM streamdetails WHERE iVideoWidth < 961))))sql"},
    {"episodes", R"({"field":"audiocount","operator":"is","value":"2"})",
     R"sql((( EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = episode_view.idFile AND streamdetails.iStreamtype = 1 GROUP BY streamdetails.idFile HAVING COUNT(streamdetails.iStreamType)  = 2))))sql"},
    {"episodes", R"({"field":"time","operator":"lessthan","value":"30:00"})",
     R"sql(((CAST(episode_view.c09 as INTEGER) < 1800)))sql"},
};

const WhereCase MUSICVIDEOS_RULES[] = {
    {"musicvideos", R"({"field":"title","operator":"contains","value":"Live"})",
     R"sql(((musicvideo_view.c00 LIKE '%Live%')))sql"},
    {"musicvideos", R"({"field":"genre","operator":"is","value":"Rock"})",
     R"sql((( EXISTS (SELECT 1 FROM genre_link         JOIN genre ON genre.genre_id=genre_link.genre_id         WHERE genre_link.media_id=musicvideo_view.idMVideo AND genre.name  LIKE 'Rock' AND genre_link.media_type = 'musicvideo'))))sql"},
    {"musicvideos", R"({"field":"artist","operator":"is","value":"Queen"})",
     R"sql((( EXISTS (SELECT 1 FROM actor_link         JOIN actor ON actor.actor_id=actor_link.actor_id         WHERE actor_link.media_id=musicvideo_view.idMVideo AND actor.name  LIKE 'Queen' AND actor_link.media_type = 'musicvideo'))))sql"},
    {"musicvideos", R"({"field":"album","operator":"is","value":"Hits"})",
     R"sql(((musicvideo_view.c09 LIKE 'Hits')))sql"},
    {"musicvideos", R"({"field":"studio","operator":"is","value":"EMI"})",
     R"sql((( EXISTS (SELECT 1 FROM studio_link         JOIN studio ON studio.studio_id=studio_link.studio_id         WHERE studio_link.media_id=musicvideo_view.idMVideo AND studio.name  LIKE 'EMI' AND studio_link.media_type = 'musicvideo'))))sql"},
    {"musicvideos", R"({"field":"director","operator":"is","value":"Jane"})",
     R"sql((( EXISTS (SELECT 1 FROM director_link         JOIN actor ON actor.actor_id=director_link.actor_id         WHERE director_link.media_id=musicvideo_view.idMVideo AND actor.name  LIKE 'Jane' AND director_link.media_type = 'musicvideo'))))sql"},
    {"musicvideos", R"({"field":"lastplayed","operator":"after","value":"2020-01-01"})",
     R"sql(((musicvideo_view.lastPlayed IS NOT NULL AND musicvideo_view.lastPlayed > '2020-01-01')))sql"},
    {"musicvideos", R"({"field":"dateadded","operator":"before","value":"2020-01-01"})",
     R"sql(((musicvideo_view.dateAdded IS NULL OR musicvideo_view.dateAdded < '2020-01-01')))sql"},
    {"musicvideos", R"({"field":"tag","operator":"is","value":"Favourite"})",
     R"sql((( EXISTS (SELECT 1 FROM tag_link         JOIN tag ON tag.tag_id=tag_link.tag_id         WHERE tag_link.media_id=musicvideo_view.idMVideo AND tag.name  LIKE 'Favourite' AND tag_link.media_type = 'musicvideo'))))sql"},
    {"musicvideos", R"({"field":"playcount","operator":"greaterthan","value":"1"})",
     R"sql(((musicvideo_view.playCount IS NOT NULL AND musicvideo_view.playCount > 1)))sql"},
    {"musicvideos", R"({"field":"userrating","operator":"is","value":"0"})",
     R"sql(((musicvideo_view.userrating IS NULL OR musicvideo_view.userrating = 0)))sql"},
    {"musicvideos", R"({"field":"year","operator":"is","value":"1999"})",
     R"sql(((CAST(musicvideo_view.premiered as DECIMAL(6,1)) = 1999)))sql"},
    {"musicvideos", R"({"field":"time","operator":"greaterthan","value":"3:30"})",
     R"sql(((CAST(musicvideo_view.c04 as INTEGER) > 210)))sql"},
    {"musicvideos", R"({"field":"videocodec","operator":"is","value":"h264"})",
     R"sql((( EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = musicvideo_view.idFile AND strVideoCodec  IN ('h264')))))sql"},
    {"musicvideos", R"({"field":"plot","operator":"contains","value":"x"})",
     R"sql(((musicvideo_view.c08 LIKE '%x%')))sql"},
};

const WhereCase MIXED_RULES[] = {
    {"mixed", R"({"field":"albumartist","operator":"is","value":"Queen"})",
     R"sql((( EXISTS (SELECT 1 FROM album_artist, artist WHERE album_artist.idAlbum = songview.idAlbum AND album_artist.idArtist = artist.idArtist AND artist.strArtist LIKE 'Queen'))))sql",
     false, MEDIA::TYPE::SONG},
    {"mixed", R"({"field":"albumartist","operator":"is","value":"Queen"})",
     R"sql((( EXISTS (SELECT 1 FROM actor_link         JOIN actor ON actor.actor_id=actor_link.actor_id         WHERE actor_link.media_id=musicvideo_view.idMVideo AND actor.name  LIKE 'Queen' AND actor_link.media_type = 'musicvideo'))))sql",
     false, MEDIA::TYPE::MUSIC_VIDEO},
    {"mixed", R"({"field":"tracknumber","operator":"is","value":"1"})",
     R"sql(((CAST(songview.iTrack as DECIMAL(6,1)) = 1)))sql",
     false, MEDIA::TYPE::SONG},
    {"mixed", R"({"field":"tracknumber","operator":"is","value":"1"})",
     R"sql(((CAST(musicvideo_view.c12 as DECIMAL(6,1)) = 1)))sql",
     false, MEDIA::TYPE::MUSIC_VIDEO},
};

//! Rules on a field the type has none of, and number rules whose value is not a number
const WhereCase REFUSED_RULES[] = {
    {"musicvideos", R"({"field":"albumartist","operator":"is","value":"Queen"})", ""},
    {"musicvideos", R"({"field":"tracknumber","operator":"is","value":"1"})", ""},
    {"musicvideos", R"({"field":"originaltitle","operator":"is","value":"x"})", ""},
    {"songs", R"({"field":"videoresolution","operator":"is","value":"1080"})", ""},
    {"tvshows", R"({"field":"videocodec","operator":"is","value":"h264"})", ""},
    {"mixed", R"({"field":"originaltitle","operator":"is","value":"x"})", ""},
    {"movies", R"({"field":"nosuchfield","operator":"is","value":"x"})", ""},
    {"movies", R"({"field":"rating","operator":"greaterthan","value":"0 OR 1=1"})", ""},
    {"movies", R"({"field":"year","operator":"is","value":"2000 OR 1=1"})", ""},
    {"movies", R"({"field":"playcount","operator":"lessthan","value":"1 OR 1=1"})", ""},
    {"movies", R"({"field":"rating","operator":"between","value":["0","10 OR 1=1"]})", ""},
    {"movies", R"({"field":"time","operator":"between","value":["0 OR 1=1","60"]})", ""},
};
// clang-format on
} // namespace

TEST_P(TestLibraryQueryWhere, ARuleGivesItsClause)
{
  const std::optional<std::string> where{WhereOf(GetParam())};
  ASSERT_TRUE(where);
  EXPECT_EQ(GetParam().where, *where);
}

INSTANTIATE_TEST_SUITE_P(Songs, TestLibraryQueryWhere, ::testing::ValuesIn(SONGS_RULES));
INSTANTIATE_TEST_SUITE_P(Albums, TestLibraryQueryWhere, ::testing::ValuesIn(ALBUMS_RULES));
INSTANTIATE_TEST_SUITE_P(Artists, TestLibraryQueryWhere, ::testing::ValuesIn(ARTISTS_RULES));
INSTANTIATE_TEST_SUITE_P(Movies, TestLibraryQueryWhere, ::testing::ValuesIn(MOVIES_RULES));
INSTANTIATE_TEST_SUITE_P(TvShows, TestLibraryQueryWhere, ::testing::ValuesIn(TVSHOWS_RULES));
INSTANTIATE_TEST_SUITE_P(Episodes, TestLibraryQueryWhere, ::testing::ValuesIn(EPISODES_RULES));
INSTANTIATE_TEST_SUITE_P(MusicVideos,
                         TestLibraryQueryWhere,
                         ::testing::ValuesIn(MUSICVIDEOS_RULES));
INSTANTIATE_TEST_SUITE_P(Mixed, TestLibraryQueryWhere, ::testing::ValuesIn(MIXED_RULES));

namespace
{
class TestLibraryQueryRefused : public TestLibraryQueryWhere
{
};
} // namespace

TEST_P(TestLibraryQueryRefused, ARuleOnAFieldItsTypeDoesNotHaveIsRefused)
{
  EXPECT_FALSE(WhereOf(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(Fields, TestLibraryQueryRefused, ::testing::ValuesIn(REFUSED_RULES));
