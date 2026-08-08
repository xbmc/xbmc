/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "music/Artist.h"
#include "music/tags/MatroskaTagMapping.h"
#include "music/tags/MusicInfoTag.h"

#include <algorithm>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace MUSIC_INFO;

namespace
{
// The separator set CAudioBookFileDirectory and CMusicInfoTagLoaderMatroska both build.
const std::vector<std::string> Separators{" feat. ", " ft. ", " Feat. ", " Ft. ",  ";", ":",
                                          "|",       "#",     "/",       " with ", "&"};
constexpr const char* MusicSep = " / ";

CMusicInfoTag Parse(const std::string& key, const std::string& value)
{
  CMusicInfoTag tag;
  MatroskaTagMapping::MapTag(key, value, Separators, MusicSep, tag);
  return tag;
}

bool HasRole(const CMusicInfoTag& tag, const std::string& role, const std::string& artist)
{
  const auto& contributors = tag.GetContributors();
  return std::any_of(contributors.begin(), contributors.end(), [&](const CMusicRole& c)
                     { return c.GetRoleDesc() == role && c.GetArtist() == artist; });
}
} // namespace

/*!
 * Every tag name the FFmpeg reader in CAudioBookFileDirectory produces has to land in the same
 * field as it does for the TagLib reader. Both call MapTag, so this table is the whole contract
 * between them.
 */
TEST(TestMatroskaTagMapping, MapsFileLevelTagsToTheirFields)
{
  EXPECT_EQ(Parse("ALBUM", "Kind of Blue").GetAlbum(), "Kind of Blue");
  EXPECT_EQ(Parse("ALBUM/TITLE", "Kind of Blue").GetAlbum(), "Kind of Blue");
  EXPECT_EQ(Parse("TITLE", "So What").GetTitle(), "So What");
  EXPECT_EQ(Parse("ARTIST", "Miles Davis").GetArtistString(), "Miles Davis");
  EXPECT_EQ(Parse("PUBLISHER", "Columbia").GetRecordLabel(), "Columbia");
  EXPECT_EQ(Parse("LABEL", "Columbia").GetRecordLabel(), "Columbia");
  EXPECT_EQ(Parse("COMMENT", "remastered").GetComment(), "remastered");
  EXPECT_EQ(Parse("MOOD", "Cool").GetMood(), "Cool");
  EXPECT_EQ(Parse("SUBTITLE", "Disc One").GetDiscSubtitle(), "Disc One");
  EXPECT_EQ(Parse("SETSUBTITLE", "Disc One").GetDiscSubtitle(), "Disc One");
  EXPECT_EQ(Parse("PART_NUMBER", "3").GetTrackNumber(), 3);
  EXPECT_EQ(Parse("TRACK", "3").GetTrackNumber(), 3);
  EXPECT_EQ(Parse("DISC", "2").GetDiscNumber(), 2);
  EXPECT_EQ(Parse("DISCNUMBER", "2").GetDiscNumber(), 2);
  EXPECT_TRUE(Parse("COMPILATION", "1").GetCompilation());
}

TEST(TestMatroskaTagMapping, MapsDatesToReleaseAndOriginal)
{
  EXPECT_EQ(Parse("DATE", "1959-08-17").GetReleaseDate(), "1959-08-17");
  EXPECT_EQ(Parse("DATE_RELEASED", "1959-08-17").GetReleaseDate(), "1959-08-17");
  EXPECT_EQ(Parse("ALBUM/DATE_RELEASED", "1959-08-17").GetReleaseDate(), "1959-08-17");
  EXPECT_EQ(Parse("YEAR", "1959").GetReleaseDate(), "1959");
  EXPECT_EQ(Parse("DATE_RECORDED", "1959-03-02").GetOriginalDate(), "1959-03-02");
  EXPECT_EQ(Parse("ORIGYEAR", "1959").GetOriginalDate(), "1959");
  EXPECT_EQ(Parse("ORIGINALDATE", "1959-03-02").GetOriginalDate(), "1959-03-02");
}

TEST(TestMatroskaTagMapping, MapsMusicBrainzIdentifiers)
{
  const std::string mbid = "1e2f3a4b-5c6d-7e8f-9a0b-1c2d3e4f5a6b";
  EXPECT_EQ(Parse("MUSICBRAINZ_TRACKID", mbid).GetMusicBrainzTrackID(), mbid);
  EXPECT_EQ(Parse("MUSICBRAINZ_ALBUMID", mbid).GetMusicBrainzAlbumID(), mbid);
  EXPECT_EQ(Parse("MUSICBRAINZ_RELEASEGROUPID", mbid).GetMusicBrainzReleaseGroupID(), mbid);
  EXPECT_EQ(Parse("MUSICBRAINZ_ALBUMTYPE", "album").GetMusicBrainzReleaseType(), "album");
  EXPECT_EQ(Parse("MUSICBRAINZ_ALBUMSTATUS", "official").GetAlbumReleaseStatus(), "official");
  EXPECT_EQ(Parse("MUSICBRAINZ_ARTISTID", mbid).GetMusicBrainzArtistID(),
            std::vector<std::string>{mbid});
  EXPECT_EQ(Parse("MUSICBRAINZ_ALBUMARTISTID", mbid).GetMusicBrainzAlbumArtistID(),
            std::vector<std::string>{mbid});
}

/*!
 * mp3tag writes the spaced spellings; the spec and most other taggers write the underscored or
 * run-together ones. All of them have to reach the same field.
 */
TEST(TestMatroskaTagMapping, AcceptsEverySpellingOfTheAlbumArtistKeys)
{
  for (const char* key : {"ALBUMARTIST", "ALBUM/ARTIST", "ALBUM ARTIST", "ALBUM/ARTIST"})
    EXPECT_EQ(Parse(key, "Miles Davis").GetAlbumArtistString(), "Miles Davis") << key;

  for (const char* key : {"ALBUMARTISTSORT", "SORT_ALBUM/ARTIST", "ALBUM ARTIST SORT"})
    EXPECT_EQ(Parse(key, "Davis, Miles").GetAlbumArtistSort(), "Davis, Miles") << key;

  for (const char* key : {"ARTISTSORT", "ARTIST-SORT", "ARTIST SORT"})
    EXPECT_EQ(Parse(key, "Davis, Miles").GetArtistSort(), "Davis, Miles") << key;

  for (const char* key : {"ALBUMARTISTS", "ALBUM/ARTISTS", "ALBUM ARTISTS"})
    EXPECT_EQ(Parse(key, "Miles Davis").GetAlbumArtistString(), "Miles Davis") << key;
}

TEST(TestMatroskaTagMapping, MapsRolesToContributors)
{
  EXPECT_TRUE(HasRole(Parse("COMPOSER", "Bill Evans"), "Composer", "Bill Evans"));
  EXPECT_TRUE(HasRole(Parse("CONDUCTOR", "Gil Evans"), "Conductor", "Gil Evans"));
  EXPECT_TRUE(HasRole(Parse("LYRICIST", "Jon Hendricks"), "Lyricist", "Jon Hendricks"));
  EXPECT_TRUE(HasRole(Parse("ENGINEER", "Fred Plaut"), "Engineer", "Fred Plaut"));
  EXPECT_TRUE(HasRole(Parse("PRODUCER", "Teo Macero"), "Producer", "Teo Macero"));
  EXPECT_TRUE(HasRole(Parse("WRITER", "Miles Davis"), "Writer", "Miles Davis"));
  EXPECT_TRUE(HasRole(Parse("BAND", "The Sextet"), "Band", "The Sextet"));
  EXPECT_TRUE(HasRole(Parse("REMIXED_BY", "Someone"), "Remixer", "Someone"));
  EXPECT_TRUE(HasRole(Parse("MIXED_BY", "Someone"), "Mixer", "Someone"));
}

/*!
 * The Matroska spec wants one SimpleTag per value, but FFmpeg returns only the last of a repeated
 * set (https://trac.ffmpeg.org/ticket/9641), so taggers pack role/person pairs into one comma
 * delimited value. Role names are capitalised on the way in.
 */
TEST(TestMatroskaTagMapping, SplitsCommaDelimitedRolePairs)
{
  const CMusicInfoTag tag = Parse("INVOLVEDPEOPLE", "producer,Teo Macero,engineer,Fred Plaut");

  EXPECT_TRUE(HasRole(tag, "Producer", "Teo Macero"));
  EXPECT_TRUE(HasRole(tag, "Engineer", "Fred Plaut"));
}

TEST(TestMatroskaTagMapping, IgnoresATrailingRoleWithNoPerson)
{
  const CMusicInfoTag tag = Parse("INVOLVEDPEOPLE", "producer,Teo Macero,engineer");

  EXPECT_TRUE(HasRole(tag, "Producer", "Teo Macero"));
  EXPECT_EQ(tag.GetContributors().size(), 1u);
}

TEST(TestMatroskaTagMapping, SplitsMultiValueTagsOnTheSeparatorList)
{
  EXPECT_EQ(Parse("MUSICBRAINZ_ARTISTID", "aaa;bbb").GetMusicBrainzArtistID(),
            (std::vector<std::string>{"aaa", "bbb"}));
  EXPECT_TRUE(HasRole(Parse("COMPOSER", "Bill Evans;Miles Davis"), "Composer", "Bill Evans"));
  EXPECT_TRUE(HasRole(Parse("COMPOSER", "Bill Evans;Miles Davis"), "Composer", "Miles Davis"));
}

TEST(TestMatroskaTagMapping, SplitsGenreOnTheMusicItemSeparator)
{
  EXPECT_EQ(Parse("GENRE", "Jazz / Modal").GetGenre(), (std::vector<std::string>{"Jazz", "Modal"}));
}

/*!
 * Keys arrive upper-cased - CAudioBookFileDirectory upper-cases what FFmpeg hands it, and TagLib
 * yields the names as written, which the Matroska spec defines as upper case. A lower-cased key
 * must not match, or dropping that normalisation would silently lose every tag.
 */
TEST(TestMatroskaTagMapping, MatchesUpperCaseKeysOnly)
{
  EXPECT_TRUE(Parse("album", "Kind of Blue").GetAlbum().empty());
  EXPECT_TRUE(Parse("Artist", "Miles Davis").GetArtistString().empty());
}

/*!
 * FFmpeg names a tag after the TargetType the file gives it, so an album level tag arrives under
 * ALBUM/. Anything the prefix does not change the meaning of maps as it would without it.
 */
TEST(TestMatroskaTagMapping, StripsFFmpegsAlbumPrefix)
{
  EXPECT_TRUE(HasRole(Parse("ALBUM/COMPOSER", "Bill Evans"), "Composer", "Bill Evans"));
  EXPECT_EQ(Parse("ALBUM/GENRE", "Jazz").GetGenre(), (std::vector<std::string>{"Jazz"}));
  EXPECT_EQ(Parse("ALBUM/MUSICBRAINZ_ALBUMID", "abc").GetMusicBrainzAlbumID(), "abc");

  // but not where the prefix is what tells the two fields apart
  EXPECT_EQ(Parse("ALBUM/TITLE", "Kind of Blue").GetTitle(), "");
  EXPECT_EQ(Parse("ALBUM/ARTIST", "Miles Davis").GetArtistString(), "");
}

TEST(TestMatroskaTagMapping, LeavesTheTagUntouchedForUnknownKeys)
{
  CMusicInfoTag tag;
  tag.SetAlbum("Kind of Blue");

  MatroskaTagMapping::MapTag("NOT_A_MATROSKA_TAG", "junk", Separators, MusicSep, tag);

  EXPECT_EQ(tag.GetAlbum(), "Kind of Blue");
  EXPECT_TRUE(tag.GetTitle().empty());
  EXPECT_TRUE(tag.GetContributors().empty());
}

/*!
 * Tag values come from untrusted media files, so a non-numeric track or disc number has to be
 * dropped rather than thrown out of MapTag.
 */
TEST(TestMatroskaTagMapping, SurvivesNonNumericTrackAndDiscNumbers)
{
  EXPECT_NO_THROW({
    EXPECT_EQ(Parse("PART_NUMBER", "not a number").GetTrackNumber(), 0);
    EXPECT_EQ(Parse("DISCNUMBER", "").GetDiscNumber(), 0);
  });
}
