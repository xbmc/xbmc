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

#include <exception>
#include <string>
#include <string_view>
#include <vector>

using namespace MUSIC_INFO;

namespace
{
/*!
 * \brief Apply one tag, reporting whether the name was one this knows.
 *
 * Separate from MapTag() so that a name it does not know can be retried with FFmpeg's ALBUM/
 * prefix removed - see MapTag().
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
} // namespace

void MUSIC_INFO::MatroskaTagMapping::MapTag(const std::string& key,
                                            const std::string& value,
                                            const std::vector<std::string>& separators,
                                            const std::string& musicsep,
                                            CMusicInfoTag& tag)
{
  if (Map(key, value, separators, musicsep, tag))
    return;

  /*!
  * FFmpeg's demuxer has no TargetTypeValue to carry, so it prefixes a tag with the TargetType name
  * the file gives it and a slash: a TargetTypeValue 50 COMPOSER arrives as ALBUM/COMPOSER. Under
  * the prefix the name is the ordinary one, so strip it and try again. The two whose meaning
  * changes with the prefix, ALBUM/TITLE and ALBUM/ARTIST, are matched above and never reach here.
  *
  * ALBUM is the conventional TargetType name for TargetTypeValue 50; a file naming it otherwise is
  * not recognised.
  */
  constexpr std::string_view albumPrefix = "ALBUM/";
  if (key.starts_with(albumPrefix))
    Map(key.substr(albumPrefix.size()), value, separators, musicsep, tag);
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
  * underscored and run-together ones are what the spec and most other taggers use. The ALBUM/
  * prefixed ones come from FFmpeg's demuxer alone - see MapTag() - and mean the same tag at
  * album level; only these two change field with the prefix.
  */
  if (key == "ALBUM" || key == "ALBUM/TITLE")
    tag.SetAlbum(value);
  else if (key == "ARTIST")
    // tag.SetArtist(StringUtils::Join(StringUtils::Split(value, separators), musicsep));
    tag.SetArtist(value);
  else if (key == "ARTISTS")
    tag.SetMusicBrainzArtistHints(StringUtils::Split(value, separators));
  else if (key == "ALBUMARTISTS" || key == "ALBUM/ARTISTS" || key == "ALBUM ARTISTS" ||
           key == "ALBUM/ARTISTS")
    tag.SetAlbumArtist(value);
  else if (key == "ALBUMARTIST" || key == "ALBUM/ARTIST" || key == "ALBUM ARTIST" ||
           key == "ALBUM/ARTIST")
    tag.SetAlbumArtist(StringUtils::Join(StringUtils::Split(value, separators), musicsep));
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
    tag.SetGenre(StringUtils::Split(value, musicsep), true);
  else if (key == "COMPILATION")
    tag.SetCompilation(true);
  else if (key == "DATE" || key == "DATE_RELEASED" || key == "YEAR")
    tag.SetReleaseDate(value);
  else if (key == "DATE_RECORDED" || key == "ORIGINALDATE" || key == "ORIGINALYEAR" ||
           key == "ORIGYEAR")
    tag.SetOriginalDate(value);
  else if (key == "MOOD")
    tag.SetMood(StringUtils::Join(StringUtils::Split(value, separators), musicsep));
  // genre could be comma delimited or not. Temporarily add the comma just in case.
  // true trims any whitespace around the genre(s)
  else if (key == "COMMENT")
    tag.SetComment(value);
  else if (key == "ARTIST-SORT" || key == "ARTISTSORT" || key == "ARTIST SORT")
    tag.SetArtistSort(StringUtils::Join(StringUtils::Split(value, separators), musicsep));
  else if (key == "ALBUMARTISTSORT" || key == "SORT_ALBUM/ARTIST" || key == "ALBUM ARTIST SORT")
    tag.SetAlbumArtistSort(StringUtils::Join(StringUtils::Split(value, separators), musicsep));
  else if (key == "COMPOSERSORT")
    tag.SetComposerSort(StringUtils::Join(StringUtils::Split(value, separators), musicsep));
  else if (key == "DISCSUBTITLE" || key == "SUBTITLE" || key == "SETSUBTITLE")
    tag.SetDiscSubtitle(value);
  else if (key == "MUSICBRAINZ_ARTISTID")
    tag.SetMusicBrainzArtistID(StringUtils::Split(value, separators));
  else if (key == "MUSICBRAINZ_ALBUMID")
    tag.SetMusicBrainzAlbumID(value);
  else if (key == "MUSICBRAINZ_RELEASEGROUPID")
    tag.SetMusicBrainzReleaseGroupID(value);
  else if (key == "MUSICBRAINZ_ALBUMARTISTID")
    tag.SetMusicBrainzAlbumArtistID(StringUtils::Split(value, separators));
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
    tag.AddArtistRole("Writer", StringUtils::Split(value, separators));
  else if (key == "PERFORMER")
  {
    std::vector<std::string> tagdata = StringUtils::Split(value, separators);
    AddRole(tagdata, separators, tag);
  }
  else if (key == "ARRANGER")
  {
    std::vector<std::string> tagdata = StringUtils::Split(value, separators);
    AddRole(tagdata, separators, tag);
  }
  else if (key == "REMIXED_BY" || key == "REMIXEDBY")
    tag.AddArtistRole("Remixer", StringUtils::Split(value, separators));
  else if (key == "MIXED_BY" || key == "MIXER")
    tag.AddArtistRole("Mixer", StringUtils::Split(value, separators));
  else if (key == "LYRICIST")
    tag.AddArtistRole("Lyricist", StringUtils::Split(value, separators));
  else if (key == "COMPOSER")
    tag.AddArtistRole("Composer", StringUtils::Split(value, separators));
  else if (key == "CONDUCTOR")
    tag.AddArtistRole("Conductor", StringUtils::Split(value, separators));
  else if (key == "ENGINEER")
    tag.AddArtistRole("Engineer", StringUtils::Split(value, separators));
  else if (key == "PRODUCER")
    tag.AddArtistRole("Producer", StringUtils::Split(value, separators));
  else if (key == "BAND")
    tag.AddArtistRole("Band", StringUtils::Split(value, separators));
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
      std::vector<std::string> roles = StringUtils::Split(data[i], separators);
      for (auto& role : roles)
      {
        StringUtils::Trim(role);
        StringUtils::ToCapitalize(role);
        musictag.AddArtistRole(role, StringUtils::Split(data[i + 1], separators));
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
      std::vector<std::string> roles = StringUtils::Split(data[i], separators);
      for (auto& role : roles)
      {
        StringUtils::Trim(role);
        StringUtils::ToCapitalize(role);
        musictag.AddArtistRole(role, StringUtils::Split(data[i + 1], ","));
      }
    }
  }
}
} // namespace
