/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MatroskaTagMapping.h"

#include "MusicInfoTag.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <array>
#include <exception>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

using namespace MUSIC_INFO;

namespace
{
/*!
 * \brief Apply one tag, reporting whether the name was one this knows.
 *
 * Separate from MapTag(), which handles the names whose field depends on the level before
 * falling through to here for the ones that do not.
 */
bool Map(const std::string& key,
         const std::string& value,
         const std::vector<std::string>& separators,
         const std::string& musicsep,
         CMusicInfoTag& tag);

void AddRole(const std::vector<std::string>& data,
             const std::vector<std::string>& separators,
             CMusicInfoTag& musictag);
void AddCommaDelimitedString(const std::vector<std::string>& data,
                             const std::vector<std::string>& separators,
                             CMusicInfoTag& musictag);

/*!
 * \brief Split a tag value that may hold several, and keep no whitespace from the split.
 *
 * A reader joins repeated SimpleTags with MultiValueSeparator, which is spaced, while the
 * separator list a caller passes holds "/" on its own. Splitting the one on the other lands inside
 * the delimiter and leaves its spaces on the values - " id2" is not the MusicBrainz identifier the
 * file carried, and no field here wants a value it could not have been given.
 */
std::vector<std::string> SplitValues(const std::string& value,
                                     const std::vector<std::string>& separators)
{
  std::vector<std::string> parts = StringUtils::Split(value, separators);
  for (auto& part : parts)
    StringUtils::Trim(part);
  std::erase_if(parts, [](const std::string& part) { return part.empty(); });
  return parts;
}

//! Delimiters as likely inside an artist as between two: AC/DC, Simon & Garfunkel.
constexpr std::string_view SeparatorsInsideNames{"/&:#|"};

/*!
 * \brief The caller's separators, less the ones that occur inside a name.
 *
 * A tagger packs several values into one tag with a delimiter the file never states, which is why
 * a caller passes the list of delimiters seen in the wild. A field holding names cannot split on
 * all of them: doing so made two album artists out of AC/DC while the same tag gave the song the
 * one artist it has.
 *
 * So it drops those and keeps the rest, and several artists still arrive as several. The spaced
 * MultiValueSeparator a reader joins repeats with is added back, because it is the bare "/" that
 * is dropped and the reader's delimiter would go with it - the spacing is what tells a delimiter
 * from a name.
 */
std::vector<std::string> NameSeparators(const std::vector<std::string>& separators)
{
  std::vector<std::string> names = separators;
  std::erase_if(
      names, [](const std::string& sep)
      { return sep.size() == 1 && SeparatorsInsideNames.find(sep[0]) != std::string_view::npos; });
  if (std::find(names.begin(), names.end(), MUSIC_INFO::MatroskaTagMapping::MultiValueSeparator) ==
      names.end())
    names.emplace_back(MUSIC_INFO::MatroskaTagMapping::MultiValueSeparator);
  return names;
}

std::vector<std::string> SplitNames(const std::string& value,
                                    const std::vector<std::string>& separators)
{
  return SplitValues(value, NameSeparators(separators));
}

//! The same, for a setter that splits the string it is handed on the user's separator itself.
std::string JoinNames(const std::string& value,
                      const std::vector<std::string>& separators,
                      const std::string& musicsep)
{
  return StringUtils::Join(SplitNames(value, separators), musicsep);
}
} // namespace

void MUSIC_INFO::MatroskaTagMapping::MapTag(const std::string& key,
                                            const std::string& value,
                                            TagLevel level,
                                            const std::vector<std::string>& separators,
                                            const std::string& musicsep,
                                            CMusicInfoTag& tag)
{
  /*!
  * The names whose field the level decides. Everything else means the same thing wherever it
  * sits, and a caller applies album level tags before track level ones so that a song naming
  * itself wins over the album that named it.
  */
  if (level == TagLevel::Album)
  {
    if (key == "TITLE")
    {
      tag.SetAlbum(value);
      // Nothing else may have named the song yet; a track level TITLE still overrides this.
      tag.SetTitle(value);
      return;
    }
    if (key == "ARTIST")
    {
      // One ARTIST names both fields, so it is read once and read the way the track level branch
      // reads it.
      const std::string artists = JoinNames(value, separators, musicsep);

      /*!
      * It names the album's artist, but only for an album that did not name one itself. A caller
      * walks a std::map, so ARTIST is always applied after ALBUM ARTIST, ALBUMARTIST and
      * ALBUM_ARTIST - taking it unconditionally would replace what the file stated, and a
      * compilation carrying ALBUM ARTIST=Various Artists beside ARTIST=Miles Davis would be filed
      * under Miles Davis. Deferring here decides it by what the file said rather than by the order
      * the names happen to arrive in.
      */
      if (tag.GetAlbumArtist().empty())
        tag.SetAlbumArtist(artists);

      // The album's artist is the song's until the song says otherwise.
      tag.SetArtist(artists);
      return;
    }
  }
  else if (level == TagLevel::File && key == "TITLE")
  {
    /*!
    * The Segment title names the file. That is the song when the file holds one, and the album
    * when nothing better named it - a file with no album level tags at all still belongs
    * somewhere.
    */
    tag.SetTitle(value);
    if (tag.GetAlbum().empty())
      tag.SetAlbum(value);
    return;
  }

  Map(key, value, separators, musicsep, tag);
}

namespace
{
bool Map(const std::string& key,
         const std::string& value,
         const std::vector<std::string>& separators,
         const std::string& musicsep,
         CMusicInfoTag& tag)
{
  /*!
  * Matroska Tag spec does not allow storing multi values in a single tag, but some tools
  * do it anyway using a delimiter. So we need to split the value using the separator and
  * then join it back using the music item separator from as.xml if needed.
  *
  * The spaced spellings (ALBUM ARTIST, ARTIST SORT, ...) are what mp3tag writes; the
  * underscored and run-together ones are what the spec and most other taggers use.
  */
  if (key == "ALBUM")
    tag.SetAlbum(value);
  else if (key == "ARTIST")
    // SetArtist() splits on musicsep, so several artists have to arrive joined with it.
    tag.SetArtist(JoinNames(value, separators, musicsep));
  else if (key == "ARTISTS")
    tag.SetMusicBrainzArtistHints(SplitNames(value, separators));
  else if (key == "ALBUMARTISTS" || key == "ALBUM_ARTISTS" || key == "ALBUM ARTISTS")
    // Split and rejoined like ALBUMARTIST below: a file carrying both spellings would otherwise
    // keep whichever the caller happened to apply last, one of them unnormalised.
    tag.SetAlbumArtist(JoinNames(value, separators, musicsep));
  else if (key == "ALBUMARTIST" || key == "ALBUM_ARTIST" || key == "ALBUM ARTIST")
    tag.SetAlbumArtist(JoinNames(value, separators, musicsep));
  else if (key == "TITLE")
    tag.SetTitle(value);
  else if (key == "PART_NUMBER" || key == "TRACK")
  {
    try
    {
      tag.SetTrackNumber(std::stoi(value));
    }
    catch (const std::exception&)
    {
    }
  }
  else if (key == "DISC" || key == "DISCNUMBER")
  {
    try
    {
      tag.SetDiscNumber(std::stoi(value));
    }
    catch (const std::exception&)
    {
    }
  }
  else if (key == "GENRE")
    // Both delimiters: the user's, which is what a single tag packing several genres is written
    // with, and the one a reader joined repeated GENRE tags with.
    tag.SetGenre(SplitValues(value, {musicsep, MatroskaTagMapping::MultiValueSeparator}), true);
  else if (key == "COMPILATION")
    tag.SetCompilation(true);
  else if (key == "DATE" || key == "DATE_RELEASED" || key == "YEAR")
    tag.SetReleaseDate(value);
  else if (key == "DATE_RECORDED" || key == "ORIGINALDATE" || key == "ORIGINALYEAR" ||
           key == "ORIGYEAR")
    tag.SetOriginalDate(value);
  else if (key == "MOOD")
    tag.SetMood(StringUtils::Join(SplitValues(value, separators), musicsep));
  // genre could be comma delimited or not. Temporarily add the comma just in case.
  // true trims any whitespace around the genre(s)
  else if (key == "COMMENT")
    tag.SetComment(value);
  else if (key == "ARTIST-SORT" || key == "ARTISTSORT" || key == "ARTIST SORT")
    tag.SetArtistSort(JoinNames(value, separators, musicsep));
  else if (key == "ALBUMARTISTSORT" || key == "SORT_ALBUM_ARTIST" || key == "ALBUM ARTIST SORT")
    tag.SetAlbumArtistSort(JoinNames(value, separators, musicsep));
  else if (key == "COMPOSERSORT")
    tag.SetComposerSort(JoinNames(value, separators, musicsep));
  else if (key == "DISCSUBTITLE" || key == "SUBTITLE" || key == "SETSUBTITLE")
    tag.SetDiscSubtitle(value);
  else if (key == "MUSICBRAINZ_ARTISTID")
    tag.SetMusicBrainzArtistID(SplitValues(value, separators));
  else if (key == "MUSICBRAINZ_ALBUMID")
    tag.SetMusicBrainzAlbumID(value);
  else if (key == "MUSICBRAINZ_RELEASEGROUPID")
    tag.SetMusicBrainzReleaseGroupID(value);
  else if (key == "MUSICBRAINZ_ALBUMARTISTID")
    tag.SetMusicBrainzAlbumArtistID(SplitValues(value, separators));
  else if (key == "MUSICBRAINZ_TRACKID")
    tag.SetMusicBrainzTrackID(value);
  else if (key == "MUSICBRAINZ_ALBUMARTIST")
  {
    // tag.SetAlbumArtist(value);
  }
  else if (key == "MUSICBRAINZ_ALBUMTYPE")
    tag.SetMusicBrainzReleaseType(value);
  else if (key == "MUSICBRAINZ_ALBUMSTATUS")
    tag.SetAlbumReleaseStatus(value);
  else if (key == "ENCODED_BY" || key == "LANGUAGE")
  {
  }
  else if (key == "LABEL" || key == "PUBLISHER")
    tag.SetRecordLabel(value);
  else if (key == "CATALOGNUMBER")
  {
  } // No database field yet
  else if (key == "COPYRIGHT")
  {
  } // Copyright message
  else if (key == "WRITER")
    tag.AddArtistRole("Writer", SplitNames(value, separators));
  else if (key == "PERFORMER")
  {
    std::vector<std::string> tagdata = SplitValues(value, separators);
    AddRole(tagdata, separators, tag);
  }
  else if (key == "ARRANGER")
  {
    std::vector<std::string> tagdata = SplitValues(value, separators);
    AddRole(tagdata, separators, tag);
  }
  else if (key == "REMIXED_BY" || key == "REMIXEDBY")
    tag.AddArtistRole("Remixer", SplitNames(value, separators));
  else if (key == "MIXED_BY" || key == "MIXER")
    tag.AddArtistRole("Mixer", SplitNames(value, separators));
  else if (key == "LYRICIST")
    tag.AddArtistRole("Lyricist", SplitNames(value, separators));
  else if (key == "COMPOSER")
    tag.AddArtistRole("Composer", SplitNames(value, separators));
  else if (key == "CONDUCTOR")
    tag.AddArtistRole("Conductor", SplitNames(value, separators));
  else if (key == "ENGINEER")
    tag.AddArtistRole("Engineer", SplitNames(value, separators));
  else if (key == "PRODUCER")
    tag.AddArtistRole("Producer", SplitNames(value, separators));
  else if (key == "BAND")
    tag.AddArtistRole("Band", SplitNames(value, separators));
  // comma separated list of role, person
  else if (key == "INVOLVEDPEOPLE" || key == "ACTOR")
  {
    std::vector<std::string> tagdata = StringUtils::Split(value, ",");

    AddCommaDelimitedString(tagdata, separators, tag);
  }
  else if (key == "INSTRUMENTS")
  {
    std::vector<std::string> tagdata = StringUtils::Split(value, ",");

    AddCommaDelimitedString(tagdata, separators, tag);
  }
  else
    return false;

  return true;
}

void AddRole(const std::vector<std::string>& data,
             const std::vector<std::string>& separators,
             CMusicInfoTag& musictag)
{
  if (!data.empty())
  {
    for (size_t i = 0; i + 1 < data.size(); i += 2)
    {
      std::vector<std::string> roles = SplitValues(data[i], separators);
      for (auto& role : roles)
      {
        StringUtils::Trim(role);
        StringUtils::ToCapitalize(role);
        musictag.AddArtistRole(role, SplitValues(data[i + 1], separators));
      }
    }
  }
}

void AddCommaDelimitedString(const std::vector<std::string>& data,
                             const std::vector<std::string>& separators,
                             CMusicInfoTag& musictag)
{
  if (!data.empty())
  {
    for (size_t i = 0; i + 1 < data.size(); i += 2)
    {
      std::vector<std::string> roles = SplitValues(data[i], separators);
      for (auto& role : roles)
      {
        StringUtils::Trim(role);
        StringUtils::ToCapitalize(role);
        musictag.AddArtistRole(role, SplitValues(data[i + 1], {","}));
      }
    }
  }
}
} // namespace

/*!
* Every spelling MapTag() accepts for a field that holds several values. The run-together, spaced
* and underscored forms of one name sit together: a tagger's choice of spelling must not decide
* whether a repeat is a second value or a replacement.
*/
bool MUSIC_INFO::MatroskaTagMapping::HoldsSeveralValues(const std::string& name)
{
  constexpr std::array<const char*, 33> names = {"ALBUM ARTIST",
                                                 "ALBUM ARTIST SORT",
                                                 "ALBUM ARTISTS",
                                                 "ALBUMARTIST",
                                                 "ALBUMARTISTS",
                                                 "ALBUMARTISTSORT",
                                                 "ALBUM_ARTIST",
                                                 "ALBUM_ARTISTS",
                                                 "ARRANGER",
                                                 "ARTIST",
                                                 "ARTIST SORT",
                                                 "ARTIST-SORT",
                                                 "ARTISTS",
                                                 "ARTISTSORT",
                                                 "BAND",
                                                 "COMPOSER",
                                                 "COMPOSERSORT",
                                                 "CONDUCTOR",
                                                 "ENGINEER",
                                                 "GENRE",
                                                 "LYRICIST",
                                                 "MIXED_BY",
                                                 "MIXER",
                                                 "MOOD",
                                                 "MUSICBRAINZ_ALBUMARTISTID",
                                                 "MUSICBRAINZ_ARTISTID",
                                                 "PERFORMER",
                                                 "PRODUCER",
                                                 "REMIXED",
                                                 "REMIXEDBY",
                                                 "REMIXED_BY",
                                                 "SORT_ALBUM_ARTIST",
                                                 "WRITER"};

  return std::find(std::begin(names), std::end(names), name) != std::end(names);
}
