/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "music/Artist.h"
#include "music/tags/MatroskaTagMapping.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"

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

CMusicInfoTag Parse(const std::string& key,
                    const std::string& value,
                    MatroskaTagMapping::TagLevel level = MatroskaTagMapping::TagLevel::Track)
{
  CMusicInfoTag tag;
  MatroskaTagMapping::MapTag(key, value, level, Separators, MusicSep, tag);
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
  EXPECT_EQ(
      Parse("DATE_RELEASED", "1959-08-17", MatroskaTagMapping::TagLevel::Album).GetReleaseDate(),
      "1959-08-17");
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
  for (const char* key : {"ALBUMARTIST", "ALBUM_ARTIST", "ALBUM ARTIST"})
    EXPECT_EQ(Parse(key, "Miles Davis").GetAlbumArtistString(), "Miles Davis") << key;

  for (const char* key : {"ALBUMARTISTSORT", "SORT_ALBUM_ARTIST", "ALBUM ARTIST SORT"})
    EXPECT_EQ(Parse(key, "Davis, Miles").GetAlbumArtistSort(), "Davis, Miles") << key;

  for (const char* key : {"ARTISTSORT", "ARTIST-SORT", "ARTIST SORT"})
    EXPECT_EQ(Parse(key, "Davis, Miles").GetArtistSort(), "Davis, Miles") << key;

  for (const char* key : {"ALBUMARTISTS", "ALBUM_ARTISTS", "ALBUM ARTISTS"})
    EXPECT_EQ(Parse(key, "Miles Davis").GetAlbumArtistString(), "Miles Davis") << key;

  /*!
   * A file carrying both spellings is applied one after the other, in an order neither the file
   * nor the caller chose - std::map walks its keys. They must therefore agree on the value, which
   * means both normalise the separator rather than one keeping what the file wrote.
   */
  const std::string joined =
      Parse("ALBUMARTIST", "Miles Davis;John Coltrane").GetAlbumArtistString();
  EXPECT_EQ(joined, Parse("ALBUMARTISTS", "Miles Davis;John Coltrane").GetAlbumArtistString());
  EXPECT_EQ("Miles Davis / John Coltrane", joined);
}

/*!
 * An album level ARTIST names the album only when the album named no artist of its own. A caller
 * applies the names in the order a std::map walks them, which puts ARTIST after every ALBUM ARTIST
 * spelling, so taking it whatever the file said would file a compilation under one of its
 * performers.
 */
TEST(TestMatroskaTagMapping, KeepsTheAlbumArtistTheFileStatedOverAnAlbumLevelArtist)
{
  constexpr auto album = MatroskaTagMapping::TagLevel::Album;

  for (const char* stated : {"ALBUMARTIST", "ALBUM_ARTIST", "ALBUM ARTIST", "ALBUMARTISTS",
                             "ALBUM_ARTISTS", "ALBUM ARTISTS"})
  {
    // The order the map walk produces: the stated album artist is applied first.
    CMusicInfoTag statedFirst;
    MatroskaTagMapping::MapTag(stated, "Various Artists", album, Separators, MusicSep, statedFirst);
    MatroskaTagMapping::MapTag("ARTIST", "Miles Davis", album, Separators, MusicSep, statedFirst);
    EXPECT_EQ("Various Artists", statedFirst.GetAlbumArtistString()) << stated;

    // and the other way about, so the field is decided by the file rather than by the walk.
    CMusicInfoTag artistFirst;
    MatroskaTagMapping::MapTag("ARTIST", "Miles Davis", album, Separators, MusicSep, artistFirst);
    MatroskaTagMapping::MapTag(stated, "Various Artists", album, Separators, MusicSep, artistFirst);
    EXPECT_EQ("Various Artists", artistFirst.GetAlbumArtistString()) << stated;

    // The song still takes the album's ARTIST; it is only the album artist that is left alone.
    EXPECT_EQ("Miles Davis", statedFirst.GetArtistString()) << stated;
    EXPECT_EQ("Miles Davis", artistFirst.GetArtistString()) << stated;
  }

  // With nothing else naming it, an album level ARTIST is still what names the album artist.
  EXPECT_EQ("Miles Davis", Parse("ARTIST", "Miles Davis", album).GetAlbumArtistString());
}

/*!
 * The separator list holds "/", and in an artist a slash is as likely inside a name as between
 * two. One album level ARTIST names the album artist and the song artist, so both have to read it
 * the same way - splitting just one of them on the list made AC/DC two album artists and one song
 * artist out of a single tag.
 */
TEST(TestMatroskaTagMapping, ReadsAnAlbumLevelArtistIntoBothFieldsTheSameWay)
{
  constexpr auto album = MatroskaTagMapping::TagLevel::Album;
  const auto advanced = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  const std::string restore = advanced->m_musicItemSeparator;
  advanced->m_musicItemSeparator = MusicSep;

  CMusicInfoTag oneName;
  MatroskaTagMapping::MapTag("ARTIST", "AC/DC", album, Separators, MusicSep, oneName);

  // Repeats the reader joined are still two artists, so the delimiter itself is still read.
  const std::string repeated =
      std::string("AC/DC") + MatroskaTagMapping::MultiValueSeparator + "Free";
  CMusicInfoTag twoNames;
  MatroskaTagMapping::MapTag("ARTIST", repeated, album, Separators, MusicSep, twoNames);

  advanced->m_musicItemSeparator = restore;

  EXPECT_EQ((std::vector<std::string>{"AC/DC"}), oneName.GetAlbumArtist());
  EXPECT_EQ(oneName.GetAlbumArtist(), oneName.GetArtist());

  EXPECT_EQ((std::vector<std::string>{"AC/DC", "Free"}), twoNames.GetAlbumArtist());
  EXPECT_EQ(twoNames.GetAlbumArtist(), twoNames.GetArtist());
}

/*!
 * The same list, and the same reason, for every field that holds names rather than free text. A
 * delimiter that can sit inside a name - "/", "&", ":", "#", "|" - is not one this may split on,
 * or an album by AC/DC is filed under two artists that do not exist while the song it holds is
 * filed under the one that does. What a tagger really does delimit with is still read.
 */
TEST(TestMatroskaTagMapping, DoesNotBreakUpANameThatHoldsASeparator)
{
  const auto advanced = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  const std::string restore = advanced->m_musicItemSeparator;
  advanced->m_musicItemSeparator = MusicSep;

  for (const char* key : {"ALBUMARTIST", "ALBUM_ARTIST", "ALBUM ARTIST", "ALBUMARTISTS",
                          "ALBUM_ARTISTS", "ALBUM ARTISTS"})
  {
    EXPECT_EQ((std::vector<std::string>{"AC/DC"}), Parse(key, "AC/DC").GetAlbumArtist()) << key;
    EXPECT_EQ((std::vector<std::string>{"Simon & Garfunkel"}),
              Parse(key, "Simon & Garfunkel").GetAlbumArtist())
        << key;

    // and the album artist a file states still reads the way its ARTIST does.
    EXPECT_EQ(Parse("ARTIST", "AC/DC").GetArtist(), Parse(key, "AC/DC").GetAlbumArtist()) << key;

    // What a tagger does delimit with is untouched, whichever spelling of the key it wrote.
    EXPECT_EQ((std::vector<std::string>{"AC/DC", "Free"}),
              Parse(key, "AC/DC;Free").GetAlbumArtist())
        << key;
    EXPECT_EQ((std::vector<std::string>{"AC/DC", "Free"}),
              Parse(key, std::string("AC/DC") + MatroskaTagMapping::MultiValueSeparator + "Free")
                  .GetAlbumArtist())
        << key;
  }

  advanced->m_musicItemSeparator = restore;

  EXPECT_EQ("AC/DC", Parse("ARTISTSORT", "AC/DC").GetArtistSort());
  EXPECT_EQ("AC/DC", Parse("ALBUMARTISTSORT", "AC/DC").GetAlbumArtistSort());
  EXPECT_EQ((std::vector<std::string>{"AC/DC"}),
            Parse("ARTISTS", "AC/DC").GetMusicBrainzArtistHints());
  EXPECT_TRUE(HasRole(Parse("BAND", "AC/DC"), "Band", "AC/DC"));
  EXPECT_TRUE(HasRole(Parse("COMPOSER", "Simon & Garfunkel"), "Composer", "Simon & Garfunkel"));

  // A role still takes the several a tagger packed into one tag.
  EXPECT_TRUE(HasRole(Parse("COMPOSER", "AC/DC;Free"), "Composer", "AC/DC"));
  EXPECT_TRUE(HasRole(Parse("COMPOSER", "AC/DC;Free"), "Composer", "Free"));
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

/*!
 * A reader joins repeated SimpleTags with MultiValueSeparator, which is spaced, and the separator
 * list holds "/" on its own - so a split on the list lands inside the delimiter. What comes out
 * has to be the values the file carried, not those values with the delimiter's spaces on them: an
 * identifier with a space in it matches nothing at MusicBrainz.
 */
TEST(TestMatroskaTagMapping, KeepsNoWhitespaceFromTheMultiValueDelimiter)
{
  const std::string first = "1e2f3a4b-5c6d-7e8f-9a0b-1c2d3e4f5a6b";
  const std::string second = "2f3a4b5c-6d7e-8f9a-0b1c-2d3e4f5a6b7c";
  const std::string joined = first + MatroskaTagMapping::MultiValueSeparator + second;

  EXPECT_EQ(Parse("MUSICBRAINZ_ARTISTID", joined).GetMusicBrainzArtistID(),
            (std::vector<std::string>{first, second}));
  EXPECT_EQ(Parse("MUSICBRAINZ_ALBUMARTISTID", joined).GetMusicBrainzAlbumArtistID(),
            (std::vector<std::string>{first, second}));

  EXPECT_EQ("Miles Davis / John Coltrane",
            Parse("ALBUMARTIST", "Miles Davis / John Coltrane").GetAlbumArtistString());
  EXPECT_EQ((std::vector<std::string>{"Jazz", "Modal"}), Parse("GENRE", "Jazz / Modal").GetGenre());

  // AddRole() trimmed the role it capitalised and left the person as the split gave it.
  EXPECT_TRUE(HasRole(Parse("PERFORMER", "guitar;Eric Clapton / bass;Nathan East"), "Guitar",
                      "Eric Clapton"));
  EXPECT_TRUE(
      HasRole(Parse("PERFORMER", "guitar;Eric Clapton / bass;Nathan East"), "Bass", "Nathan East"));
}

/*!
 * SetArtist() splits on the user's separator, so an album level ARTIST has to be handed over
 * joined with that one - not with the delimiter a reader joined repeats with. The two agreed only
 * at the default setting, where they are the same string.
 */
TEST(TestMatroskaTagMapping, JoinsAlbumLevelArtistsWithTheUsersSeparator)
{
  const auto advanced = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  const std::string restore = advanced->m_musicItemSeparator;
  advanced->m_musicItemSeparator = ";";

  CMusicInfoTag album;
  MatroskaTagMapping::MapTag("ARTIST", "Miles Davis / John Coltrane",
                             MatroskaTagMapping::TagLevel::Album, Separators, ";", album);

  CMusicInfoTag track;
  MatroskaTagMapping::MapTag("ARTIST", "Miles Davis / John Coltrane",
                             MatroskaTagMapping::TagLevel::Track, Separators, ";", track);

  advanced->m_musicItemSeparator = restore;

  const std::vector<std::string> expected{"Miles Davis", "John Coltrane"};
  EXPECT_EQ(expected, album.GetArtist());
  // The level decides which fields a name reaches, never how its values are spelled.
  EXPECT_EQ(track.GetArtist(), album.GetArtist());
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
 * A name whose field the level decides means one thing at album level and another at track level.
 * Every other name means the same at both.
 */
TEST(TestMatroskaTagMapping, ReadsANameAgainstItsLevel)
{
  constexpr auto album = MatroskaTagMapping::TagLevel::Album;
  EXPECT_TRUE(HasRole(Parse("COMPOSER", "Bill Evans", album), "Composer", "Bill Evans"));
  EXPECT_EQ(Parse("GENRE", "Jazz", album).GetGenre(), (std::vector<std::string>{"Jazz"}));
  EXPECT_EQ(Parse("MUSICBRAINZ_ALBUMID", "abc", album).GetMusicBrainzAlbumID(), "abc");

  // but not where the level is what tells two fields apart
  EXPECT_EQ(Parse("TITLE", "Kind of Blue", album).GetAlbum(), "Kind of Blue");
  EXPECT_EQ(Parse("ARTIST", "Miles Davis", album).GetAlbumArtistString(), "Miles Davis");
}

TEST(TestMatroskaTagMapping, LeavesTheTagUntouchedForUnknownKeys)
{
  CMusicInfoTag tag;
  tag.SetAlbum("Kind of Blue");

  MatroskaTagMapping::MapTag("NOT_A_MATROSKA_TAG", "junk", MatroskaTagMapping::TagLevel::Track,
                             Separators, MusicSep, tag);

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
