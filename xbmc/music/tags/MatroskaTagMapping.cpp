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
#include <vector>

using namespace MUSIC_INFO;

namespace
{
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
  /*!
  * Matroska Tag spec does not allow storing multi values in a single tag, but some tools
  * do it anyway using a delimiter. So we need to split the value using the separator and
  * then join it back using the music item separator from as.xml if needed.
  */
  if (key == "ALBUM")
    tag.SetAlbum(value);
  else if (key == "ARTIST")
    // tag.SetArtist(StringUtils::Join(StringUtils::Split(value, separators), musicsep));
    tag.SetArtist(value);
  else if (key == "ARTISTS")
    tag.SetMusicBrainzArtistHints(StringUtils::Split(value, separators));
  else if (key == "ALBUMARTISTS" || key == "ALBUM/ARTISTS")
    tag.SetAlbumArtist(value);
  else if (key == "ALBUMARTIST" || key == "ALBUM/ARTIST")
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
  else if (key == "ARTIST-SORT" || key == "ARTISTSORT")
    tag.SetArtistSort(StringUtils::Join(StringUtils::Split(value, separators), musicsep));
  else if (key == "ALBUMARTISTSORT" || key == "SORT_ALBUM/ARTIST")
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
}

namespace
{
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
