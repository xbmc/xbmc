#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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

#include <map>
#include <set>
#include <string>

typedef std::string MediaType;

#define MediaTypeNone             ""
#define MediaTypeMusic            "music"
#define MediaTypeArtist           "artist"
#define MediaTypeAlbum            "album"
#define MediaTypeSong             "song"
#define MediaTypeVideo            "video"
#define MediaTypeVideoCollection  "set"
#define MediaTypeMusicVideo       "musicvideo"
#define MediaTypeMovie            "movie"
#define MediaTypeTvShow           "tvshow"
#define MediaTypeSeason           "season"
#define MediaTypeEpisode          "episode"

class MediaTypes
{
public:
  static bool IsValidMediaType(const MediaType &mediaType);
  static bool IsMediaType(const std::string &strMediaType, const MediaType &mediaType);
  static MediaType FromString(const std::string &strMediaType);
  static MediaType ToPlural(const MediaType &mediaType);

  static bool IsContainer(const MediaType &mediaType);

  static std::string GetLocalization(const MediaType &mediaType);
  static std::string GetPluralLocalization(const MediaType &mediaType);
  static std::string GetCapitalLocalization(const MediaType &mediaType);
  static std::string GetCapitalPluralLocalization(const MediaType &mediaType);
  
  typedef struct MediaTypeInfo {
    MediaTypeInfo(const MediaType &mediaType, const std::string &plural, bool container,
                  int localizationSingular, int localizationPlural,
                  int localizationSingularCapital, int localizationPluralCapital)
      : mediaType(mediaType),
        plural(plural),
        container(container),
        localizationSingular(localizationSingular),
        localizationPlural(localizationPlural),
        localizationSingularCapital(localizationSingularCapital),
        localizationPluralCapital(localizationPluralCapital)
    { }

    MediaType mediaType;
    std::string plural;
    bool container;
    int localizationSingular;
    int localizationPlural;
    int localizationSingularCapital;
    int localizationPluralCapital;
  } MediaTypeInfo;

private:
  static std::map<std::string, MediaTypeInfo>::const_iterator findMediaType(const std::string &mediaType);

  static std::map<std::string, MediaTypeInfo> m_mediaTypes;
};
