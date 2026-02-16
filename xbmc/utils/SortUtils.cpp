/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SortUtils.h"

#include "LangInfo.h"
#include "SortFileItem.h"
#include "URL.h"
#include "Util.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <algorithm>
#include <array>
#include <limits>

std::string ArrayToString(SortAttribute attributes, const CVariant &variant, const std::string &separator = " / ")
{
  if (variant.isArray())
  {
    std::vector<std::string> strArray;
    for (CVariant::const_iterator_array it = variant.begin_array(); it != variant.end_array(); ++it)
    {
      if (attributes & SortAttributeIgnoreArticle)
        strArray.push_back(SortUtils::RemoveArticles(it->asString()));
      else
        strArray.push_back(it->asString());
    }

    return StringUtils::Join(strArray, separator);
  }
  else if (variant.isString())
  {
    if (attributes & SortAttributeIgnoreArticle)
      return SortUtils::RemoveArticles(variant.asString());
    else
      return variant.asString();
  }

  return "";
}

std::string ByLabel(SortAttribute attributes, const SortItem &values)
{
  if (attributes & SortAttributeIgnoreArticle)
    return SortUtils::RemoveArticles(values.at(Field::LABEL).asString());

  return values.at(Field::LABEL).asString();
}

std::string ByFile(SortAttribute attributes, const SortItem &values)
{
  CURL url(values.at(Field::PATH).asString());

  return StringUtils::Format("{} {}", url.GetFileNameWithoutPath(),
                             values.at(Field::START_OFFSET).asInteger());
}

std::string ByPath(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(Field::PATH).asString(),
                             values.at(Field::START_OFFSET).asInteger());
}

std::string ByLastPlayed(SortAttribute attributes, const SortItem &values)
{
  if (attributes & SortAttributeIgnoreLabel)
    return values.at(Field::LAST_PLAYED).asString();

  return StringUtils::Format("{} {}", values.at(Field::LAST_PLAYED).asString(),
                             ByLabel(attributes, values));
}

std::string ByPlaycount(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(Field::PLAYCOUNT).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByDate(SortAttribute attributes, const SortItem &values)
{
  return values.at(Field::DATE).asString() + " " + ByLabel(attributes, values);
}

std::string ByDateAdded(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(Field::DATE_ADDED).asString(),
                             values.at(Field::ID).asInteger());
}

std::string BySize(SortAttribute attributes, const SortItem &values)
{
  return std::to_string(values.at(Field::SIZE).asInteger());
}

std::string ByDriveType(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(Field::DRIVE_TYPE).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByTitle(SortAttribute attributes, const SortItem &values)
{
  if (attributes & SortAttributeIgnoreArticle)
    return SortUtils::RemoveArticles(values.at(Field::TITLE).asString());

  return values.at(Field::TITLE).asString();
}

std::string ByAlbum(SortAttribute attributes, const SortItem &values)
{
  std::string album = values.at(Field::ALBUM).asString();
  if (attributes & SortAttributeIgnoreArticle)
    album = SortUtils::RemoveArticles(album);

  std::string label =
      StringUtils::Format("{} {}", album, ArrayToString(attributes, values.at(Field::ARTIST)));

  const CVariant& track = values.at(Field::TRACK_NUMBER);
  if (!track.isNull())
    label += StringUtils::Format(" {}", track.asInteger());

  return label;
}

std::string ByAlbumType(SortAttribute attributes, const SortItem &values)
{
  return values.at(Field::ALBUM_TYPE).asString() + " " + ByLabel(attributes, values);
}

std::string ByArtist(SortAttribute attributes, const SortItem &values)
{
  std::string label;
  if (attributes & SortAttributeUseArtistSortName)
  {
    const CVariant& artistsort = values.at(Field::ARTIST_SORT);
    if (!artistsort.isNull())
      label = artistsort.asString();
  }
  if (label.empty())
    label = ArrayToString(attributes, values.at(Field::ARTIST));

  const CVariant& album = values.at(Field::ALBUM);
  if (!album.isNull())
    label += " " + SortUtils::RemoveArticles(album.asString());

  const CVariant& track = values.at(Field::TRACK_NUMBER);
  if (!track.isNull())
    label += StringUtils::Format(" {}", track.asInteger());

  return label;
}

std::string ByArtistThenYear(SortAttribute attributes, const SortItem &values)
{
  std::string label;
  if (attributes & SortAttributeUseArtistSortName)
  {
    const CVariant& artistsort = values.at(Field::ARTIST_SORT);
    if (!artistsort.isNull())
      label = artistsort.asString();
  }
  if (label.empty())
    label = ArrayToString(attributes, values.at(Field::ARTIST));

  const CVariant& year = values.at(Field::YEAR);
  if (!year.isNull())
    label += StringUtils::Format(" {}", year.asInteger());

  const CVariant& album = values.at(Field::ALBUM);
  if (!album.isNull())
    label += " " + SortUtils::RemoveArticles(album.asString());

  const CVariant& track = values.at(Field::TRACK_NUMBER);
  if (!track.isNull())
    label += StringUtils::Format(" {}", track.asInteger());

  return label;
}

std::string ByTrackNumber(SortAttribute attributes, const SortItem &values)
{
  return std::to_string(values.at(Field::TRACK_NUMBER).asInteger());
}

std::string ByTotalDiscs(SortAttribute attributes, const SortItem& values)
{
  return StringUtils::Format("{} {}", values.at(Field::TOTAL_DISCS).asInteger(),
                             ByLabel(attributes, values));
}
std::string ByTime(SortAttribute attributes, const SortItem &values)
{
  std::string label;
  const CVariant& time = values.at(Field::TIME);
  if (time.isInteger())
    label = std::to_string(time.asInteger());
  else
    label = time.asString();
  return label;
}

std::string ByProgramCount(SortAttribute attributes, const SortItem &values)
{
  return std::to_string(values.at(Field::PROGRAM_COUNT).asInteger());
}

std::string ByPlaylistOrder(SortAttribute attributes, const SortItem &values)
{
  //! @todo Playlist order is hacked into program count variable (not nice, but ok until 2.0)
  return ByProgramCount(attributes, values);
}

std::string ByGenre(SortAttribute attributes, const SortItem &values)
{
  return ArrayToString(attributes, values.at(Field::GENRE));
}

std::string ByCountry(SortAttribute attributes, const SortItem &values)
{
  return ArrayToString(attributes, values.at(Field::COUNTRY));
}

std::string ByYear(SortAttribute attributes, const SortItem &values)
{
  std::string label;
  const CVariant& airDate = values.at(Field::AIR_DATE);
  if (!airDate.isNull() && !airDate.asString().empty())
    label = airDate.asString() + " ";

  label += std::to_string(values.at(Field::YEAR).asInteger());

  const CVariant& album = values.at(Field::ALBUM);
  if (!album.isNull())
    label += " " + SortUtils::RemoveArticles(album.asString());

  const CVariant& track = values.at(Field::TRACK_NUMBER);
  if (!track.isNull())
    label += StringUtils::Format(" {}", track.asInteger());

  label += " " + ByLabel(attributes, values);

  return label;
}

std::string ByOrigDate(SortAttribute attributes, const SortItem& values)
{
  std::string label;
  label = values.at(Field::ORIG_DATE).asString();

  const CVariant& album = values.at(Field::ALBUM);
  if (!album.isNull())
    label += " " + SortUtils::RemoveArticles(album.asString());

  const CVariant& track = values.at(Field::TRACK_NUMBER);
  if (!track.isNull())
    label += StringUtils::Format(" {}", track.asInteger());

  label += " " + ByLabel(attributes, values);

  return label;
}

std::string BySortTitle(SortAttribute attributes, const SortItem &values)
{
  std::string title = values.at(Field::SORT_TITLE).asString();
  if (title.empty())
    title = values.at(Field::TITLE).asString();

  if (attributes & SortAttributeIgnoreArticle)
    title = SortUtils::RemoveArticles(title);

  return title;
}

std::string ByOriginalTitle(SortAttribute attributes, const SortItem& values)
{
  std::string title = values.at(Field::ORIGINAL_TITLE).asString();
  if (title.empty())
    title = values.at(Field::SORT_TITLE).asString();

  if (title.empty())
    title = values.at(Field::TITLE).asString();

  if (attributes & SortAttributeIgnoreArticle)
    title = SortUtils::RemoveArticles(title);

  return title;
}

std::string ByRating(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{:f} {}", values.at(Field::RATING).asFloat(),
                             ByLabel(attributes, values));
}

std::string ByUserRating(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(Field::USER_RATING).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByVotes(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(Field::VOTES).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByTop250(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(Field::TOP250).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByMPAA(SortAttribute attributes, const SortItem &values)
{
  return values.at(Field::MPAA).asString() + " " + ByLabel(attributes, values);
}

std::string ByStudio(SortAttribute attributes, const SortItem &values)
{
  return ArrayToString(attributes, values.at(Field::STUDIO));
}

std::string ByEpisodeNumber(SortAttribute attributes, const SortItem &values)
{
  // we calculate an offset number based on the episode's
  // sort season and episode values. in addition
  // we include specials 'episode' numbers to get proper
  // sorting of multiple specials in a row. each
  // of these are given their particular ranges to semi-ensure uniqueness.
  // theoretical problem: if a show has > 2^15 specials and two of these are placed
  // after each other they will sort backwards. if a show has > 2^32-1 seasons
  // or if a season has > 2^16-1 episodes strange things will happen (overflow)
  uint64_t num;
  const CVariant& episodeSpecial = values.at(Field::EPISODE_NUMBER_SPECIAL_SORT);
  const CVariant& seasonSpecial = values.at(Field::SEASON_SPECIAL_SORT);
  if (!episodeSpecial.isNull() && !seasonSpecial.isNull() &&
     (episodeSpecial.asInteger() > 0 || seasonSpecial.asInteger() > 0))
    num = (static_cast<uint64_t>(seasonSpecial.asInteger()) << 32) +
          (static_cast<uint64_t>(episodeSpecial.asInteger()) << 16) -
          ((2 << 15) - values.at(Field::EPISODE_NUMBER).asInteger());
  else
    num = (static_cast<uint64_t>(values.at(Field::SEASON).asInteger()) << 32) +
          (static_cast<uint64_t>(values.at(Field::EPISODE_NUMBER).asInteger()) << 16);

  std::string title;
  if (values.contains(Field::MEDIA_TYPE) &&
      values.at(Field::MEDIA_TYPE).asString() == MediaTypeMovie)
    title = BySortTitle(attributes, values);
  if (title.empty())
    title = ByLabel(attributes, values);

  return StringUtils::Format("{} {}", num, title);
}

std::string BySeason(SortAttribute attributes, const SortItem &values)
{
  auto season = values.at(Field::SEASON).asInteger();

  if (season == 0)
    season = std::numeric_limits<int>::max();

  const CVariant& specialSeason = values.at(Field::SEASON_SPECIAL_SORT);
  if (!specialSeason.isNull() && specialSeason.asInteger() > 0)
    season = specialSeason.asInteger();

  return StringUtils::Format("{} {}", season, ByLabel(attributes, values));
}

std::string ByNumberOfEpisodes(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(Field::NUMBER_OF_EPISODES).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByNumberOfWatchedEpisodes(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(Field::NUMBER_OF_WATCHED_EPISODES).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByTvShowStatus(SortAttribute attributes, const SortItem &values)
{
  return values.at(Field::TVSHOW_STATUS).asString() + " " + ByLabel(attributes, values);
}

std::string ByTvShowTitle(SortAttribute attributes, const SortItem &values)
{
  return values.at(Field::TVSHOW_TITLE).asString() + " " + ByLabel(attributes, values);
}

std::string ByProductionCode(SortAttribute attributes, const SortItem &values)
{
  return values.at(Field::PRODUCTION_CODE).asString();
}

std::string ByVideoResolution(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(Field::VIDEO_RESOLUTION).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByVideoCodec(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(Field::VIDEO_CODEC).asString(),
                             ByLabel(attributes, values));
}

std::string ByVideoAspectRatio(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{:.3f} {}", values.at(Field::VIDEO_ASPECT_RATIO).asFloat(),
                             ByLabel(attributes, values));
}

std::string ByAudioChannels(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(Field::AUDIO_CHANNELS).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByAudioCodec(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(Field::AUDIO_CODEC).asString(),
                             ByLabel(attributes, values));
}

std::string ByAudioLanguage(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(Field::AUDIO_LANGUAGE).asString(),
                             ByLabel(attributes, values));
}

std::string BySubtitleLanguage(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(Field::SUBTITLE_LANGUAGE).asString(),
                             ByLabel(attributes, values));
}

std::string ByBitrate(SortAttribute attributes, const SortItem &values)
{
  return std::to_string(values.at(Field::BITRATE).asInteger());
}

std::string ByListeners(SortAttribute attributes, const SortItem &values)
{
  return std::to_string(values.at(Field::LISTENERS).asInteger());
}

std::string ByRandom(SortAttribute attributes, const SortItem &values)
{
  return std::to_string(CUtil::GetRandomNumber());
}

std::string ByChannel(SortAttribute attributes, const SortItem &values)
{
  return values.at(Field::CHANNEL_NAME).asString();
}

std::string ByChannelNumber(SortAttribute attributes, const SortItem &values)
{
  return values.at(Field::CHANNEL_NUMBER).asString();
}

std::string ByClientChannelOrder(SortAttribute attributes, const SortItem& values)
{
  return values.at(Field::CLIENT_CHANNEL_ORDER).asString();
}

std::string ByProvider(SortAttribute attributes, const SortItem& values)
{
  return values.at(Field::PROVIDER).asString();
}

std::string ByUserPreference(SortAttribute attributes, const SortItem& values)
{
  return values.at(Field::USER_PREFERENCE).asString();
}

std::string ByDateTaken(SortAttribute attributes, const SortItem &values)
{
  return values.at(Field::DATE_TAKEN).asString();
}

std::string ByRelevance(SortAttribute attributes, const SortItem &values)
{
  return std::to_string(values.at(Field::RELEVANCE).asInteger());
}

std::string ByInstallDate(SortAttribute attributes, const SortItem &values)
{
  return values.at(Field::INSTALL_DATE).asString();
}

std::string ByLastUpdated(SortAttribute attributes, const SortItem &values)
{
  return values.at(Field::LAST_UPDATED).asString();
}

std::string ByLastUsed(SortAttribute attributes, const SortItem &values)
{
  return values.at(Field::LAST_USED).asString();
}

std::string ByBPM(SortAttribute attributes, const SortItem& values)
{
  return StringUtils::Format("{} {}", values.at(Field::BPM).asInteger(),
                             ByLabel(attributes, values));
}

bool preliminarySort(const SortItem& left,
                     const SortItem& right,
                     bool handleFolder,
                     bool& result,
                     std::wstring& labelLeft,
                     std::wstring& labelRight)
{
  // make sure both items have the necessary data to do the sorting
  const auto itLeftSort = left.find(Field::SORT);
  if (itLeftSort == left.end())
  {
    result = false;
    return true;
  }
  const auto itRightSort = right.find(Field::SORT);
  if (itRightSort == right.end())
  {
    result = true;
    return true;
  }

  // look at special sorting behaviour
  SortSpecial leftSortSpecial = SortSpecial::NONE;
  SortSpecial rightSortSpecial = SortSpecial::NONE;
  if (const auto itLeft = left.find(Field::SORT_SPECIAL);
      itLeft != left.end() &&
      itLeft->second.asInteger() <= static_cast<int64_t>(SortSpecial::BOTTOM))
    leftSortSpecial = static_cast<SortSpecial>(itLeft->second.asInteger());
  if (const auto itRight = right.find(Field::SORT_SPECIAL);
      itRight != right.end() &&
      itRight->second.asInteger() <= static_cast<int64_t>(SortSpecial::BOTTOM))
    rightSortSpecial = static_cast<SortSpecial>(itRight->second.asInteger());

  // one has a special sort
  if (leftSortSpecial != rightSortSpecial)
  {
    // left should be sorted on top
    // or right should be sorted on bottom
    // => left is sorted above right
    if (leftSortSpecial == SortSpecial::TOP || rightSortSpecial == SortSpecial::BOTTOM)
    {
      result = true;
      return true;
    }

    // otherwise right is sorted above left
    result = false;
    return true;
  }
  // both have either sort on top or sort on bottom -> leave as-is
  else if (leftSortSpecial != SortSpecial::NONE)
  {
    result = false;
    return true;
  }

  if (handleFolder)
  {
    const auto itLeft = left.find(Field::FOLDER);
    const auto itRight = right.find(Field::FOLDER);
    if (itLeft != left.end() && itRight != right.end() &&
        itLeft->second.asBoolean() != itRight->second.asBoolean())
    {
      result = itLeft->second.asBoolean();
      return true;
    }
  }

  labelLeft = itLeftSort->second.asWideString();
  labelRight = itRightSort->second.asWideString();

  return false;
}

bool SorterAscending(const SortItem &left, const SortItem &right)
{
  bool result;
  std::wstring labelLeft, labelRight;
  if (preliminarySort(left, right, true, result, labelLeft, labelRight))
    return result;

  return StringUtils::AlphaNumericCompare(labelLeft, labelRight) < 0;
}

bool SorterDescending(const SortItem &left, const SortItem &right)
{
  bool result;
  std::wstring labelLeft, labelRight;
  if (preliminarySort(left, right, true, result, labelLeft, labelRight))
    return result;

  return StringUtils::AlphaNumericCompare(labelLeft, labelRight) > 0;
}

bool SorterIgnoreFoldersAscending(const SortItem &left, const SortItem &right)
{
  bool result;
  std::wstring labelLeft, labelRight;
  if (preliminarySort(left, right, false, result, labelLeft, labelRight))
    return result;

  return StringUtils::AlphaNumericCompare(labelLeft, labelRight) < 0;
}

bool SorterIgnoreFoldersDescending(const SortItem &left, const SortItem &right)
{
  bool result;
  std::wstring labelLeft, labelRight;
  if (preliminarySort(left, right, false, result, labelLeft, labelRight))
    return result;

  return StringUtils::AlphaNumericCompare(labelLeft, labelRight) > 0;
}

bool SorterIndirectAscending(const std::shared_ptr<SortItem>& left,
                             const std::shared_ptr<SortItem>& right)
{
  return SorterAscending(*left, *right);
}

bool SorterIndirectDescending(const std::shared_ptr<SortItem>& left,
                              const std::shared_ptr<SortItem>& right)
{
  return SorterDescending(*left, *right);
}

bool SorterIndirectIgnoreFoldersAscending(const std::shared_ptr<SortItem>& left,
                                          const std::shared_ptr<SortItem>& right)
{
  return SorterIgnoreFoldersAscending(*left, *right);
}

bool SorterIndirectIgnoreFoldersDescending(const std::shared_ptr<SortItem>& left,
                                           const std::shared_ptr<SortItem>& right)
{
  return SorterIgnoreFoldersDescending(*left, *right);
}

// clang-format off
std::map<SortBy, SortUtils::SortPreparator> fillPreparators()
{
  return std::map<SortBy, SortUtils::SortPreparator>{
    {SortBy::NONE,                       nullptr},
    {SortBy::LABEL,                      ByLabel},
    {SortBy::DATE,                       ByDate},
    {SortBy::SIZE,                       BySize},
    {SortBy::FILE,                       ByFile},
    {SortBy::PATH,                       ByPath},
    {SortBy::DRIVE_TYPE,                 ByDriveType},
    {SortBy::TITLE,                      ByTitle},
    {SortBy::TRACK_NUMBER,               ByTrackNumber},
    {SortBy::TIME,                       ByTime},
    {SortBy::ARTIST,                     ByArtist},
    {SortBy::ARTIST_THEN_YEAR,           ByArtistThenYear},
    {SortBy::ALBUM,                      ByAlbum},
    {SortBy::ALBUM_TYPE,                 ByAlbumType},
    {SortBy::GENRE,                      ByGenre},
    {SortBy::COUNTRY,                    ByCountry},
    {SortBy::YEAR,                       ByYear},
    {SortBy::RATING,                     ByRating},
    {SortBy::USER_RATING,                ByUserRating},
    {SortBy::VOTES,                      ByVotes},
    {SortBy::TOP250,                     ByTop250},
    {SortBy::PROGRAM_COUNT,              ByProgramCount},
    {SortBy::PLAYLIST_ORDER,             ByPlaylistOrder},
    {SortBy::EPISODE_NUMBER,             ByEpisodeNumber},
    {SortBy::SEASON,                     BySeason},
    {SortBy::NUMBER_OF_EPISODES,         ByNumberOfEpisodes},
    {SortBy::NUMBER_OF_WATCHED_EPISODES, ByNumberOfWatchedEpisodes},
    {SortBy::TVSHOW_STATUS,              ByTvShowStatus},
    {SortBy::TVSHOW_TITLE,               ByTvShowTitle},
    {SortBy::SORT_TITLE,                 BySortTitle},
    {SortBy::PRODUCTION_CODE,            ByProductionCode},
    {SortBy::MPAA,                       ByMPAA},
    {SortBy::VIDEO_RESOLUTION,           ByVideoResolution},
    {SortBy::VIDEO_CODEC,                ByVideoCodec},
    {SortBy::VIDEO_ASPECT_RATIO,         ByVideoAspectRatio},
    {SortBy::AUDIO_CHANNELS,             ByAudioChannels},
    {SortBy::AUDIO_CODEC,                ByAudioCodec},
    {SortBy::AUDIO_LANGUAGE,             ByAudioLanguage},
    {SortBy::SUBTITLE_LANGUAGE,          BySubtitleLanguage},
    {SortBy::STUDIO,                     ByStudio},
    {SortBy::DATE_ADDED,                 ByDateAdded},
    {SortBy::LAST_PLAYED,                ByLastPlayed},
    {SortBy::PLAYCOUNT,                  ByPlaycount},
    {SortBy::LISTENERS,                  ByListeners},
    {SortBy::BITRATE,                    ByBitrate},
    {SortBy::RANDOM,                     ByRandom},
    {SortBy::CHANNEL,                    ByChannel},
    {SortBy::CHANNEL_NUMBER,             ByChannelNumber},
    {SortBy::CLIENT_CHANNEL_ORDER,       ByClientChannelOrder},
    {SortBy::PROVIDER,                   ByProvider},
    {SortBy::USER_PREFERENCE,            ByUserPreference},
    {SortBy::DATE_TAKEN,                 ByDateTaken},
    {SortBy::RELEVANCE,                  ByRelevance},
    {SortBy::INSTALL_DATE,               ByInstallDate},
    {SortBy::LAST_UPDATED,               ByLastUpdated},
    {SortBy::LAST_USED,                  ByLastUsed},
    {SortBy::TOTAL_DISCS,                ByTotalDiscs},
    {SortBy::ORIG_DATE,                  ByOrigDate},
    {SortBy::BPM,                        ByBPM},
    {SortBy::ORIGINAL_TITLE,             ByOriginalTitle},
  };
}

std::map<SortBy, Fields> fillSortingFields()
{
  return std::map<SortBy, Fields>{
    {SortBy::NONE,                       {}},
    {SortBy::RANDOM,                     {}},
    {SortBy::LABEL,                      {Field::LABEL}},
    {SortBy::DATE,                       {Field::DATE}},
    {SortBy::SIZE,                       {Field::SIZE}},
    {SortBy::FILE,                       {Field::PATH, Field::START_OFFSET}},
    {SortBy::PATH,                       {Field::PATH, Field::START_OFFSET}},
    {SortBy::DRIVE_TYPE,                 {Field::DRIVE_TYPE}},
    {SortBy::TITLE,                      {Field::TITLE}},
    {SortBy::TRACK_NUMBER,               {Field::TRACK_NUMBER}},
    {SortBy::TIME,                       {Field::TIME}},
    {SortBy::ARTIST,                     {Field::ARTIST, Field::ARTIST_SORT, Field::YEAR, Field::ALBUM, Field::TRACK_NUMBER}},
    {SortBy::ARTIST_THEN_YEAR,           {Field::ARTIST, Field::ARTIST_SORT, Field::YEAR, Field::ORIG_DATE,
                                          Field::ALBUM, Field::TRACK_NUMBER}},
    {SortBy::ALBUM,                      {Field::ALBUM, Field::ARTIST, Field::ARTIST_SORT, Field::TRACK_NUMBER}},
    {SortBy::ALBUM_TYPE,                 {Field::ALBUM_TYPE}},
    {SortBy::GENRE,                      {Field::GENRE}},
    {SortBy::COUNTRY,                    {Field::COUNTRY}},
    {SortBy::YEAR,                       {Field::YEAR, Field::AIR_DATE, Field::ALBUM,
                                          Field::TRACK_NUMBER, Field::ORIG_DATE}},
    {SortBy::RATING,                     {Field::RATING}},
    {SortBy::USER_RATING,                {Field::USER_RATING}},
    {SortBy::VOTES,                      {Field::VOTES}},
    {SortBy::TOP250,                     {Field::TOP250}},
    {SortBy::PROGRAM_COUNT,              {Field::PROGRAM_COUNT}},
    {SortBy::PLAYLIST_ORDER,             {Field::PROGRAM_COUNT}},
    {SortBy::EPISODE_NUMBER,             {Field::EPISODE_NUMBER, Field::SEASON, Field::EPISODE_NUMBER_SPECIAL_SORT,
                                          Field::SEASON_SPECIAL_SORT, Field::TITLE, Field::SORT_TITLE}},
    {SortBy::SEASON,                     {Field::SEASON, Field::SEASON_SPECIAL_SORT}},
    {SortBy::NUMBER_OF_EPISODES,         {Field::NUMBER_OF_EPISODES}},
    {SortBy::NUMBER_OF_WATCHED_EPISODES, {Field::NUMBER_OF_WATCHED_EPISODES}},
    {SortBy::TVSHOW_STATUS,              {Field::TVSHOW_STATUS}},
    {SortBy::TVSHOW_TITLE,               {Field::TVSHOW_TITLE}},
    {SortBy::SORT_TITLE,                 {Field::SORT_TITLE, Field::TITLE}},
    {SortBy::PRODUCTION_CODE,            {Field::PRODUCTION_CODE}},
    {SortBy::MPAA,                       {Field::MPAA}},
    {SortBy::VIDEO_RESOLUTION,           {Field::VIDEO_RESOLUTION}},
    {SortBy::VIDEO_CODEC,                {Field::VIDEO_CODEC}},
    {SortBy::VIDEO_ASPECT_RATIO,         {Field::VIDEO_ASPECT_RATIO}},
    {SortBy::AUDIO_CHANNELS,             {Field::AUDIO_CHANNELS}},
    {SortBy::AUDIO_CODEC,                {Field::AUDIO_CODEC}},
    {SortBy::AUDIO_LANGUAGE,             {Field::AUDIO_LANGUAGE}},
    {SortBy::SUBTITLE_LANGUAGE,          {Field::SUBTITLE_LANGUAGE}},
    {SortBy::STUDIO,                     {Field::STUDIO}},
    {SortBy::DATE_ADDED,                 {Field::DATE_ADDED, Field::ID}},
    {SortBy::LAST_PLAYED,                {Field::LAST_PLAYED}},
    {SortBy::PLAYCOUNT,                  {Field::PLAYCOUNT}},
    {SortBy::LISTENERS,                  {Field::LISTENERS}},
    {SortBy::BITRATE,                    {Field::BITRATE}},
    {SortBy::CHANNEL,                    {Field::CHANNEL_NAME}},
    {SortBy::CHANNEL_NUMBER,             {Field::CHANNEL_NUMBER}},
    {SortBy::CLIENT_CHANNEL_ORDER,       {Field::CLIENT_CHANNEL_ORDER}},
    {SortBy::PROVIDER,                   {Field::PROVIDER}},
    {SortBy::USER_PREFERENCE,            {Field::USER_PREFERENCE}},
    {SortBy::DATE_TAKEN,                 {Field::DATE_TAKEN}},
    {SortBy::RELEVANCE,                  {Field::RELEVANCE}},
    {SortBy::INSTALL_DATE,               {Field::INSTALL_DATE}},
    {SortBy::LAST_UPDATED,               {Field::LAST_UPDATED}},
    {SortBy::LAST_USED,                  {Field::LAST_USED}},
    {SortBy::TOTAL_DISCS,                {Field::TOTAL_DISCS}},
    {SortBy::ORIG_DATE,                  {Field::ORIG_DATE, Field::ALBUM, Field::TRACK_NUMBER}},
    {SortBy::BPM,                        {Field::BPM}},
    {SortBy::ORIGINAL_TITLE,             {Field::ORIGINAL_TITLE, Field::TITLE, Field::SORT_TITLE}},
  };
}

// clang-format on

std::map<SortBy, SortUtils::SortPreparator> SortUtils::m_preparators = fillPreparators();
std::map<SortBy, Fields> SortUtils::m_sortingFields = fillSortingFields();

void SortUtils::GetFieldsForSQLSort(const MediaType& mediaType,
                                    SortBy sortMethod,
                                    FieldList& fields)
{
  fields.clear();
  if (mediaType == MediaTypeNone)
    return;

  if (mediaType == MediaTypeAlbum)
  {
    if (sortMethod == SortBy::LABEL || sortMethod == SortBy::ALBUM || sortMethod == SortBy::TITLE)
    {
      fields.emplace_back(Field::ALBUM);
      fields.emplace_back(Field::ARTIST);
    }
    else if (sortMethod == SortBy::ALBUM_TYPE)
    {
      fields.emplace_back(Field::ALBUM_TYPE);
      fields.emplace_back(Field::ALBUM);
      fields.emplace_back(Field::ARTIST);
    }
    else if (sortMethod == SortBy::ARTIST)
    {
      fields.emplace_back(Field::ARTIST);
      fields.emplace_back(Field::ALBUM);
    }
    else if (sortMethod == SortBy::ARTIST_THEN_YEAR)
    {
      fields.emplace_back(Field::ARTIST);
      fields.emplace_back(Field::YEAR);
      fields.emplace_back(Field::ALBUM);
    }
    else if (sortMethod == SortBy::YEAR)
    {
      fields.emplace_back(Field::YEAR);
      fields.emplace_back(Field::ALBUM);
    }
    else if (sortMethod == SortBy::GENRE)
    {
      fields.emplace_back(Field::GENRE);
      fields.emplace_back(Field::ALBUM);
    }
    else if (sortMethod == SortBy::DATE_ADDED)
      fields.emplace_back(Field::DATE_ADDED);
    else if (sortMethod == SortBy::PLAYCOUNT)
    {
      fields.emplace_back(Field::PLAYCOUNT);
      fields.emplace_back(Field::ALBUM);
    }
    else if (sortMethod == SortBy::LAST_PLAYED)
    {
      fields.emplace_back(Field::LAST_PLAYED);
      fields.emplace_back(Field::ALBUM);
    }
    else if (sortMethod == SortBy::RATING)
    {
      fields.emplace_back(Field::RATING);
      fields.emplace_back(Field::ALBUM);
    }
    else if (sortMethod == SortBy::VOTES)
    {
      fields.emplace_back(Field::VOTES);
      fields.emplace_back(Field::ALBUM);
    }
    else if (sortMethod == SortBy::USER_RATING)
    {
      fields.emplace_back(Field::USER_RATING);
      fields.emplace_back(Field::ALBUM);
    }
    else if (sortMethod == SortBy::TOTAL_DISCS)
    {
      fields.emplace_back(Field::TOTAL_DISCS);
      fields.emplace_back(Field::ALBUM);
    }
    else if (sortMethod == SortBy::ORIG_DATE)
    {
      fields.emplace_back(Field::ORIG_DATE);
      fields.emplace_back(Field::ALBUM);
    }
  }
  else if (mediaType == MediaTypeSong)
  {
    if (sortMethod == SortBy::LABEL || sortMethod == SortBy::TRACK_NUMBER)
      fields.emplace_back(Field::TRACK_NUMBER);
    else if (sortMethod == SortBy::TITLE)
      fields.emplace_back(Field::TITLE);
    else if (sortMethod == SortBy::ALBUM)
    {
      fields.emplace_back(Field::ALBUM);
      fields.emplace_back(Field::ALBUM_ARTIST);
      fields.emplace_back(Field::TRACK_NUMBER);
    }
    else if (sortMethod == SortBy::ARTIST)
    {
      fields.emplace_back(Field::ARTIST);
      fields.emplace_back(Field::ALBUM);
      fields.emplace_back(Field::TRACK_NUMBER);
    }
    else if (sortMethod == SortBy::ARTIST_THEN_YEAR)
    {
      fields.emplace_back(Field::ARTIST);
      fields.emplace_back(Field::YEAR);
      fields.emplace_back(Field::ALBUM);
      fields.emplace_back(Field::TRACK_NUMBER);
    }
    else if (sortMethod == SortBy::YEAR)
    {
      fields.emplace_back(Field::YEAR);
      fields.emplace_back(Field::ALBUM);
      fields.emplace_back(Field::TRACK_NUMBER);
    }
    else if (sortMethod == SortBy::GENRE)
    {
      fields.emplace_back(Field::GENRE);
      fields.emplace_back(Field::ALBUM);
    }
    else if (sortMethod == SortBy::DATE_ADDED)
      fields.emplace_back(Field::DATE_ADDED);
    else if (sortMethod == SortBy::PLAYCOUNT)
    {
      fields.emplace_back(Field::PLAYCOUNT);
      fields.emplace_back(Field::TRACK_NUMBER);
    }
    else if (sortMethod == SortBy::LAST_PLAYED)
    {
      fields.emplace_back(Field::LAST_PLAYED);
      fields.emplace_back(Field::TRACK_NUMBER);
    }
    else if (sortMethod == SortBy::RATING)
    {
      fields.emplace_back(Field::RATING);
      fields.emplace_back(Field::TRACK_NUMBER);
    }
    else if (sortMethod == SortBy::VOTES)
    {
      fields.emplace_back(Field::VOTES);
      fields.emplace_back(Field::TRACK_NUMBER);
    }
    else if (sortMethod == SortBy::USER_RATING)
    {
      fields.emplace_back(Field::USER_RATING);
      fields.emplace_back(Field::TRACK_NUMBER);
    }
    else if (sortMethod == SortBy::FILE)
    {
      fields.emplace_back(Field::PATH);
      fields.emplace_back(Field::FILENAME);
      fields.emplace_back(Field::START_OFFSET);
    }
    else if (sortMethod == SortBy::TIME)
      fields.emplace_back(Field::TIME);
    else if (sortMethod == SortBy::ALBUM_TYPE)
    {
      fields.emplace_back(Field::ALBUM_TYPE);
      fields.emplace_back(Field::ALBUM);
      fields.emplace_back(Field::TRACK_NUMBER);
    }
    else if (sortMethod == SortBy::ORIG_DATE)
    {
      fields.emplace_back(Field::ORIG_DATE);
      fields.emplace_back(Field::ALBUM);
      fields.emplace_back(Field::TRACK_NUMBER);
    }
    else if (sortMethod == SortBy::BPM)
      fields.emplace_back(Field::BPM);
  }
  else if (mediaType == MediaTypeArtist)
  {
    if (sortMethod == SortBy::LABEL || sortMethod == SortBy::TITLE || sortMethod == SortBy::ARTIST)
      fields.emplace_back(Field::ARTIST);
    else if (sortMethod == SortBy::GENRE)
      fields.emplace_back(Field::GENRE);
    else if (sortMethod == SortBy::DATE_ADDED)
      fields.emplace_back(Field::DATE_ADDED);
  }

  // Add sort by id to define order when other fields same or sort none
  fields.emplace_back(Field::ID);
  return;
}

void SortUtils::Sort(SortBy sortBy, SortOrder sortOrder, SortAttribute attributes, DatabaseResults& items, int limitEnd /* = -1 */, int limitStart /* = 0 */)
{
  if (sortBy != SortBy::NONE)
  {
    // get the matching SortPreparator
    SortPreparator preparator = getPreparator(sortBy);
    if (preparator)
    {
      Fields sortingFields = GetFieldsForSorting(sortBy);

      // Prepare the string used for sorting and store it under FieldSort
      for (auto& item : items)
      {
        // add all fields to the item that are required for sorting if they are currently missing
        for (const auto& field : sortingFields)
        {
          if (!item.contains(field))
            item.emplace(field, CVariant::ConstNullVariant);
        }

        std::wstring sortLabel;
        g_charsetConverter.utf8ToW(preparator(attributes, item), sortLabel, false);
        item.emplace(Field::SORT, CVariant(sortLabel));
      }

      // Do the sorting
      std::stable_sort(items.begin(), items.end(), getSorter(sortOrder, attributes));
    }
  }

  if (limitStart > 0 && static_cast<size_t>(limitStart) < items.size())
  {
    items.erase(items.begin(), items.begin() + limitStart);
    limitEnd -= limitStart;
  }
  if (limitEnd > 0 && static_cast<size_t>(limitEnd) < items.size())
    items.erase(items.begin() + limitEnd, items.end());
}

void SortUtils::Sort(SortBy sortBy, SortOrder sortOrder, SortAttribute attributes, SortItems& items, int limitEnd /* = -1 */, int limitStart /* = 0 */)
{
  if (sortBy != SortBy::NONE)
  {
    // get the matching SortPreparator
    SortPreparator preparator = getPreparator(sortBy);
    if (preparator)
    {
      Fields sortingFields = GetFieldsForSorting(sortBy);

      // Prepare the string used for sorting and store it under FieldSort
      for (auto& item : items)
      {
        // add all fields to the item that are required for sorting if they are currently missing
        for (const auto& field : sortingFields)
        {
          if (!item->contains(field))
            item->emplace(field, CVariant::ConstNullVariant);
        }

        std::wstring sortLabel;
        g_charsetConverter.utf8ToW(preparator(attributes, *item), sortLabel, false);
        item->emplace(Field::SORT, CVariant(sortLabel));
      }

      // Do the sorting
      std::stable_sort(items.begin(), items.end(), getSorterIndirect(sortOrder, attributes));
    }
  }

  if (limitStart > 0 && static_cast<size_t>(limitStart) < items.size())
  {
    items.erase(items.begin(), items.begin() + limitStart);
    limitEnd -= limitStart;
  }
  if (limitEnd > 0 && static_cast<size_t>(limitEnd) < items.size())
    items.erase(items.begin() + limitEnd, items.end());
}

void SortUtils::Sort(const SortDescription &sortDescription, DatabaseResults& items)
{
  Sort(sortDescription.sortBy, sortDescription.sortOrder, sortDescription.sortAttributes, items,
       sortDescription.limitEnd, sortDescription.limitStart);
}

void SortUtils::Sort(const SortDescription &sortDescription, SortItems& items)
{
  Sort(sortDescription.sortBy, sortDescription.sortOrder, sortDescription.sortAttributes, items,
       sortDescription.limitEnd, sortDescription.limitStart);
}

bool SortUtils::SortFromDataset(const SortDescription& sortDescription,
                                const MediaType& mediaType,
                                dbiplus::Dataset& dataset,
                                DatabaseResults& results)
{
  FieldList fields;
  if (!DatabaseUtils::GetSelectFields(SortUtils::GetFieldsForSorting(sortDescription.sortBy), mediaType, fields))
    fields.clear();

  if (!DatabaseUtils::GetDatabaseResults(mediaType, fields, dataset, results))
    return false;

  SortDescription sorting = sortDescription;
  if (sortDescription.sortBy == SortBy::NONE)
  {
    sorting.limitStart = 0;
    sorting.limitEnd = -1;
  }

  Sort(sorting, results);

  return true;
}

const SortUtils::SortPreparator& SortUtils::getPreparator(SortBy sortBy)
{
  const auto it = m_preparators.find(sortBy);
  return it == m_preparators.end() ? m_preparators[SortBy::NONE] : it->second;
}

SortUtils::Sorter SortUtils::getSorter(SortOrder sortOrder, SortAttribute attributes)
{
  if (attributes & SortAttributeIgnoreFolders)
    return sortOrder == SortOrder::DESCENDING ? SorterIgnoreFoldersDescending
                                              : SorterIgnoreFoldersAscending;

  return sortOrder == SortOrder::DESCENDING ? SorterDescending : SorterAscending;
}

SortUtils::SorterIndirect SortUtils::getSorterIndirect(SortOrder sortOrder, SortAttribute attributes)
{
  if (attributes & SortAttributeIgnoreFolders)
    return sortOrder == SortOrder::DESCENDING ? SorterIndirectIgnoreFoldersDescending
                                              : SorterIndirectIgnoreFoldersAscending;

  return sortOrder == SortOrder::DESCENDING ? SorterIndirectDescending : SorterIndirectAscending;
}

const Fields& SortUtils::GetFieldsForSorting(SortBy sortBy)
{
  const auto it = m_sortingFields.find(sortBy);
  return it == m_sortingFields.end() ? m_sortingFields[SortBy::NONE] : it->second;
}

std::string SortUtils::RemoveArticles(const std::string &label)
{
  const CLangInfo::Tokens sortTokens = g_langInfo.GetSortTokens();
  const auto match = std::ranges::find_if(sortTokens, [&label](const auto& token)
                                          { return StringUtils::StartsWithNoCase(label, token); });
  return match == sortTokens.end() ? label : label.substr(match->size());
}

struct sort_map
{
  SortBy        sort;
  SortMethod old;
  SortAttribute flags;
  int           label;
};

// clang-format off
const auto table = std::array{
  sort_map{SortBy::LABEL,                       SortMethod::LABEL,                           SortAttributeNone,          551 },
  sort_map{SortBy::LABEL,                       SortMethod::LABEL_IGNORE_THE,                SortAttributeIgnoreArticle, 551 },
  sort_map{SortBy::LABEL,                       SortMethod::LABEL_IGNORE_FOLDERS,            SortAttributeIgnoreFolders, 551 },
  sort_map{SortBy::DATE,                        SortMethod::DATE,                            SortAttributeNone,          552 },
  sort_map{SortBy::SIZE,                        SortMethod::SIZE,                            SortAttributeNone,          553 },
  sort_map{SortBy::BITRATE,                     SortMethod::BITRATE,                         SortAttributeNone,          623 },
  sort_map{SortBy::DRIVE_TYPE,                  SortMethod::DRIVE_TYPE,                      SortAttributeNone,          564 },
  sort_map{SortBy::TRACK_NUMBER,                SortMethod::TRACKNUM,                        SortAttributeNone,          554 },
  sort_map{SortBy::EPISODE_NUMBER,              SortMethod::EPISODE,                         SortAttributeNone,          20359 },// 20360 "Episodes" used for SORT_METHOD_EPISODE for sorting tvshows by episode count
  sort_map{SortBy::TIME,                        SortMethod::DURATION,                        SortAttributeNone,          180 },
  sort_map{SortBy::TIME,                        SortMethod::VIDEO_RUNTIME,                   SortAttributeNone,          180 },
  sort_map{SortBy::TITLE,                       SortMethod::TITLE,                           SortAttributeNone,          556 },
  sort_map{SortBy::TITLE,                       SortMethod::TITLE_IGNORE_THE,                SortAttributeIgnoreArticle, 556 },
  sort_map{SortBy::TITLE,                       SortMethod::VIDEO_TITLE,                     SortAttributeNone,          556 },
  sort_map{SortBy::ARTIST,                      SortMethod::ARTIST,                          SortAttributeNone,          557 },
  sort_map{SortBy::ARTIST_THEN_YEAR,            SortMethod::ARTIST_AND_YEAR,                 SortAttributeNone,          578 },
  sort_map{SortBy::ARTIST,                      SortMethod::ARTIST_IGNORE_THE,               SortAttributeIgnoreArticle, 557 },
  sort_map{SortBy::ALBUM,                       SortMethod::ALBUM,                           SortAttributeNone,          558 },
  sort_map{SortBy::ALBUM,                       SortMethod::ALBUM_IGNORE_THE,                SortAttributeIgnoreArticle, 558 },
  sort_map{SortBy::GENRE,                       SortMethod::GENRE,                           SortAttributeNone,          515 },
  sort_map{SortBy::COUNTRY,                     SortMethod::COUNTRY,                         SortAttributeNone,          574 },
  sort_map{SortBy::DATE_ADDED,                  SortMethod::DATEADDED,                       SortAttributeIgnoreFolders, 570 },
  sort_map{SortBy::FILE,                        SortMethod::FILE,                            SortAttributeIgnoreFolders, 561 },
  sort_map{SortBy::RATING,                      SortMethod::SONG_RATING,                     SortAttributeNone,          563 },
  sort_map{SortBy::RATING,                      SortMethod::VIDEO_RATING,                    SortAttributeIgnoreFolders, 563 },
  sort_map{SortBy::USER_RATING,                 SortMethod::SONG_USER_RATING,                SortAttributeIgnoreFolders, 38018 },
  sort_map{SortBy::USER_RATING,                 SortMethod::VIDEO_USER_RATING,               SortAttributeIgnoreFolders, 38018 },
  sort_map{SortBy::SORT_TITLE,                  SortMethod::VIDEO_SORT_TITLE,                SortAttributeIgnoreFolders, 171 },
  sort_map{SortBy::SORT_TITLE,                  SortMethod::VIDEO_SORT_TITLE_IGNORE_THE,     SortAttribute(SortAttributeIgnoreFolders | SortAttributeIgnoreArticle), 171 },
  sort_map{SortBy::ORIGINAL_TITLE,              SortMethod::VIDEO_ORIGINAL_TITLE,            SortAttributeIgnoreFolders, 20376 },
  sort_map{SortBy::ORIGINAL_TITLE,              SortMethod::VIDEO_ORIGINAL_TITLE_IGNORE_THE, SortAttribute(SortAttributeIgnoreFolders | SortAttributeIgnoreArticle), 20376 },
  sort_map{SortBy::YEAR,                        SortMethod::YEAR,                            SortAttributeIgnoreFolders, 562 },
  sort_map{SortBy::PRODUCTION_CODE,             SortMethod::PRODUCTIONCODE,                  SortAttributeNone,          20368 },
  sort_map{SortBy::PROGRAM_COUNT,               SortMethod::PROGRAM_COUNT,                   SortAttributeNone,          567 }, // label is "play count"
  sort_map{SortBy::PLAYLIST_ORDER,              SortMethod::PLAYLIST_ORDER,                  SortAttributeIgnoreFolders, 559 },
  sort_map{SortBy::MPAA,                        SortMethod::MPAA_RATING,                     SortAttributeNone,          20074 },
  sort_map{SortBy::STUDIO,                      SortMethod::STUDIO,                          SortAttributeNone,          572 },
  sort_map{SortBy::STUDIO,                      SortMethod::STUDIO_IGNORE_THE,               SortAttributeIgnoreArticle, 572 },
  sort_map{SortBy::PATH,                        SortMethod::FULLPATH,                        SortAttributeNone,          573 },
  sort_map{SortBy::LAST_PLAYED,                 SortMethod::LASTPLAYED,                      SortAttributeIgnoreFolders, 568 },
  sort_map{SortBy::PLAYCOUNT,                   SortMethod::PLAYCOUNT,                       SortAttributeIgnoreFolders, 567 },
  sort_map{SortBy::LISTENERS,                   SortMethod::LISTENERS,                       SortAttributeNone,          20455 },
  sort_map{SortBy::CHANNEL,                     SortMethod::CHANNEL,                         SortAttributeNone,          19029 },
  sort_map{SortBy::CHANNEL,                     SortMethod::CHANNEL_NUMBER,                  SortAttributeNone,          549 },
  sort_map{SortBy::CHANNEL,                     SortMethod::CLIENT_CHANNEL_ORDER,            SortAttributeNone,          19315 },
  sort_map{SortBy::PROVIDER,                    SortMethod::PROVIDER,                        SortAttributeNone,          19348 },
  sort_map{SortBy::USER_PREFERENCE,             SortMethod::USER_PREFERENCE,                 SortAttributeNone,          19349 },
  sort_map{SortBy::DATE_TAKEN,                  SortMethod::DATE_TAKEN,                      SortAttributeIgnoreFolders, 577 },
  sort_map{SortBy::NONE,                        SortMethod::NONE,                            SortAttributeNone,          16018 },
  sort_map{SortBy::TOTAL_DISCS,                 SortMethod::TOTAL_DISCS,                     SortAttributeNone,          38077 },
  sort_map{SortBy::ORIG_DATE,                   SortMethod::ORIG_DATE,                       SortAttributeNone,          38079 },
  sort_map{SortBy::BPM,                         SortMethod::BPM,                             SortAttributeNone,          38080 },

  // the following have no corresponding SortMetho d::*
  sort_map{SortBy::ALBUM_TYPE,                  SortMethod::NONE,                            SortAttributeNone,          564 },
  sort_map{SortBy::VOTES,                       SortMethod::NONE,                            SortAttributeNone,          205 },
  sort_map{SortBy::TOP250,                      SortMethod::NONE,                            SortAttributeNone,          13409 },
  sort_map{SortBy::MPAA,                        SortMethod::NONE,                            SortAttributeNone,          20074 },
  sort_map{SortBy::DATE_ADDED,                  SortMethod::NONE,                            SortAttributeNone,          570 },
  sort_map{SortBy::TVSHOW_TITLE,                SortMethod::NONE,                            SortAttributeNone,          20364 },
  sort_map{SortBy::TVSHOW_STATUS,               SortMethod::NONE,                            SortAttributeNone,          126 },
  sort_map{SortBy::SEASON,                      SortMethod::NONE,                            SortAttributeNone,          20373 },
  sort_map{SortBy::NUMBER_OF_EPISODES,          SortMethod::NONE,                            SortAttributeNone,          20360 },
  sort_map{SortBy::NUMBER_OF_WATCHED_EPISODES,  SortMethod::NONE,                            SortAttributeNone,          21441 },
  sort_map{SortBy::VIDEO_RESOLUTION,            SortMethod::NONE,                            SortAttributeNone,          21443 },
  sort_map{SortBy::VIDEO_CODEC,                 SortMethod::NONE,                            SortAttributeNone,          21445 },
  sort_map{SortBy::VIDEO_ASPECT_RATIO,          SortMethod::NONE,                            SortAttributeNone,          21374 },
  sort_map{SortBy::AUDIO_CHANNELS,              SortMethod::NONE,                            SortAttributeNone,          21444 },
  sort_map{SortBy::AUDIO_CODEC,                 SortMethod::NONE,                            SortAttributeNone,          21446 },
  sort_map{SortBy::AUDIO_LANGUAGE,              SortMethod::NONE,                            SortAttributeNone,          21447 },
  sort_map{SortBy::SUBTITLE_LANGUAGE,           SortMethod::NONE,                            SortAttributeNone,          21448 },
  sort_map{SortBy::RANDOM,                      SortMethod::NONE,                            SortAttributeNone,          590 }
};
// clang-format on

SortMethod SortUtils::TranslateOldSortMethod(SortBy sortBy, bool ignoreArticle)
{
  const auto ign_match = std::ranges::find_if(
      table,
      [sortBy, ignoreArticle](const auto& t)
      {
        return t.sort == sortBy && ignoreArticle == ((t.flags & SortAttributeIgnoreArticle) ==
                                                     SortAttributeIgnoreArticle);
      });

  if (ign_match != table.end())
    return ign_match->old;

  const auto match =
      std::ranges::find_if(table, [sortBy](const auto& t) { return t.sort == sortBy; });

  return match == table.end() ? SortMethod::NONE : match->old;
}

SortDescription SortUtils::TranslateOldSortMethod(SortMethod sortBy)
{
  SortDescription description;
  const auto match =
      std::ranges::find_if(table, [sortBy](const auto& t) { return t.old == sortBy; });

  if (match != table.end())
  {
    description.sortBy = match->sort;
    description.sortAttributes = match->flags;
  }
  return description;
}

int SortUtils::GetSortLabel(SortBy sortBy)
{
  const auto match =
      std::ranges::find_if(table, [sortBy](const auto& t) { return t.sort == sortBy; });

  return match == table.end() ? 16018 : match->label; // 16018 = None
}

template<typename T>
T TypeFromString(const std::map<std::string, T>& typeMap, const std::string& name, const T& defaultType)
{
  const auto it = typeMap.find(name);
  return it == typeMap.end() ? defaultType : it->second;
}

template<typename T>
const std::string& TypeToString(const std::map<std::string, T>& typeMap, const T& value)
{
  const auto it =
      std::ranges::find_if(typeMap, [&value](const auto& pair) { return pair.second == value; });
  return it == typeMap.end() ? StringUtils::Empty : it->first;
}

/**
 * @brief Sort methods to translate string values to enum values.
 *
 * @warning On string changes, edit __SortBy__ enumerator to have strings right
 * for documentation!
 */
const std::map<std::string, SortBy> sortMethods{
    {"label", SortBy::LABEL},
    {"date", SortBy::DATE},
    {"size", SortBy::SIZE},
    {"file", SortBy::FILE},
    {"path", SortBy::PATH},
    {"drivetype", SortBy::DRIVE_TYPE},
    {"title", SortBy::TITLE},
    {"track", SortBy::TRACK_NUMBER},
    {"time", SortBy::TIME},
    {"artist", SortBy::ARTIST},
    {"artistyear", SortBy::ARTIST_THEN_YEAR},
    {"album", SortBy::ALBUM},
    {"albumtype", SortBy::ALBUM_TYPE},
    {"genre", SortBy::GENRE},
    {"country", SortBy::COUNTRY},
    {"year", SortBy::YEAR},
    {"rating", SortBy::RATING},
    {"votes", SortBy::VOTES},
    {"top250", SortBy::TOP250},
    {"programcount", SortBy::PROGRAM_COUNT},
    {"playlist", SortBy::PLAYLIST_ORDER},
    {"episode", SortBy::EPISODE_NUMBER},
    {"season", SortBy::SEASON},
    {"totalepisodes", SortBy::NUMBER_OF_EPISODES},
    {"watchedepisodes", SortBy::NUMBER_OF_WATCHED_EPISODES},
    {"tvshowstatus", SortBy::TVSHOW_STATUS},
    {"tvshowtitle", SortBy::TVSHOW_TITLE},
    {"sorttitle", SortBy::SORT_TITLE},
    {"productioncode", SortBy::PRODUCTION_CODE},
    {"mpaa", SortBy::MPAA},
    {"videoresolution", SortBy::VIDEO_RESOLUTION},
    {"videocodec", SortBy::VIDEO_CODEC},
    {"videoaspectratio", SortBy::VIDEO_ASPECT_RATIO},
    {"audiochannels", SortBy::AUDIO_CHANNELS},
    {"audiocodec", SortBy::AUDIO_CODEC},
    {"audiolanguage", SortBy::AUDIO_LANGUAGE},
    {"subtitlelanguage", SortBy::SUBTITLE_LANGUAGE},
    {"studio", SortBy::STUDIO},
    {"dateadded", SortBy::DATE_ADDED},
    {"lastplayed", SortBy::LAST_PLAYED},
    {"playcount", SortBy::PLAYCOUNT},
    {"listeners", SortBy::LISTENERS},
    {"bitrate", SortBy::BITRATE},
    {"random", SortBy::RANDOM},
    {"channel", SortBy::CHANNEL},
    {"channelnumber", SortBy::CHANNEL_NUMBER},
    {"clientchannelorder", SortBy::CLIENT_CHANNEL_ORDER},
    {"provider", SortBy::PROVIDER},
    {"userpreference", SortBy::USER_PREFERENCE},
    {"datetaken", SortBy::DATE_TAKEN},
    {"userrating", SortBy::USER_RATING},
    {"installdate", SortBy::INSTALL_DATE},
    {"lastupdated", SortBy::LAST_UPDATED},
    {"lastused", SortBy::LAST_USED},
    {"totaldiscs", SortBy::TOTAL_DISCS},
    {"originaldate", SortBy::ORIG_DATE},
    {"bpm", SortBy::BPM},
    {"originaltitle", SortBy::ORIGINAL_TITLE},
};

SortBy SortUtils::SortMethodFromString(const std::string& sortMethod)
{
  return TypeFromString<SortBy>(sortMethods, sortMethod, SortBy::NONE);
}

const std::string& SortUtils::SortMethodToString(SortBy sortMethod)
{
  return TypeToString<SortBy>(sortMethods, sortMethod);
}

const std::map<std::string, SortOrder> sortOrders = {{"ascending", SortOrder::ASCENDING},
                                                     {"descending", SortOrder::DESCENDING}};

SortOrder SortUtils::SortOrderFromString(const std::string& sortOrder)
{
  return TypeFromString<SortOrder>(sortOrders, sortOrder, SortOrder::NONE);
}

const std::string& SortUtils::SortOrderToString(SortOrder sortOrder)
{
  return TypeToString<SortOrder>(sortOrders, sortOrder);
}
