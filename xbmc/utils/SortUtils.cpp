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
    return SortUtils::RemoveArticles(values.at(FieldLabel).asString());

  return values.at(FieldLabel).asString();
}

std::string ByFile(SortAttribute attributes, const SortItem &values)
{
  CURL url(values.at(FieldPath).asString());

  return StringUtils::Format("{} {}", url.GetFileNameWithoutPath(),
                             values.at(FieldStartOffset).asInteger());
}

std::string ByPath(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(FieldPath).asString(),
                             values.at(FieldStartOffset).asInteger());
}

std::string ByLastPlayed(SortAttribute attributes, const SortItem &values)
{
  if (attributes & SortAttributeIgnoreLabel)
    return values.at(FieldLastPlayed).asString();

  return StringUtils::Format("{} {}", values.at(FieldLastPlayed).asString(),
                             ByLabel(attributes, values));
}

std::string ByPlaycount(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(FieldPlaycount).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByDate(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldDate).asString() + " " + ByLabel(attributes, values);
}

std::string ByDateAdded(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(FieldDateAdded).asString(),
                             values.at(FieldId).asInteger());
}

std::string BySize(SortAttribute attributes, const SortItem &values)
{
  return std::to_string(values.at(FieldSize).asInteger());
}

std::string ByDriveType(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(FieldDriveType).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByTitle(SortAttribute attributes, const SortItem &values)
{
  if (attributes & SortAttributeIgnoreArticle)
    return SortUtils::RemoveArticles(values.at(FieldTitle).asString());

  return values.at(FieldTitle).asString();
}

std::string ByAlbum(SortAttribute attributes, const SortItem &values)
{
  std::string album = values.at(FieldAlbum).asString();
  if (attributes & SortAttributeIgnoreArticle)
    album = SortUtils::RemoveArticles(album);

  std::string label =
      StringUtils::Format("{} {}", album, ArrayToString(attributes, values.at(FieldArtist)));

  const CVariant &track = values.at(FieldTrackNumber);
  if (!track.isNull())
    label += StringUtils::Format(" {}", track.asInteger());

  return label;
}

std::string ByAlbumType(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldAlbumType).asString() + " " + ByLabel(attributes, values);
}

std::string ByArtist(SortAttribute attributes, const SortItem &values)
{
  std::string label;
  if (attributes & SortAttributeUseArtistSortName)
  {
    const CVariant &artistsort = values.at(FieldArtistSort);
    if (!artistsort.isNull())
      label = artistsort.asString();
  }
  if (label.empty())
    label = ArrayToString(attributes, values.at(FieldArtist));

  const CVariant &album = values.at(FieldAlbum);
  if (!album.isNull())
    label += " " + SortUtils::RemoveArticles(album.asString());

  const CVariant &track = values.at(FieldTrackNumber);
  if (!track.isNull())
    label += StringUtils::Format(" {}", track.asInteger());

  return label;
}

std::string ByArtistThenYear(SortAttribute attributes, const SortItem &values)
{
  std::string label;
  if (attributes & SortAttributeUseArtistSortName)
  {
    const CVariant &artistsort = values.at(FieldArtistSort);
    if (!artistsort.isNull())
      label = artistsort.asString();
  }
  if (label.empty())
    label = ArrayToString(attributes, values.at(FieldArtist));

  const CVariant &year = values.at(FieldYear);
  if (!year.isNull())
    label += StringUtils::Format(" {}", year.asInteger());

  const CVariant &album = values.at(FieldAlbum);
  if (!album.isNull())
    label += " " + SortUtils::RemoveArticles(album.asString());

  const CVariant &track = values.at(FieldTrackNumber);
  if (!track.isNull())
    label += StringUtils::Format(" {}", track.asInteger());

  return label;
}

std::string ByTrackNumber(SortAttribute attributes, const SortItem &values)
{
  return std::to_string(values.at(FieldTrackNumber).asInteger());
}

std::string ByTotalDiscs(SortAttribute attributes, const SortItem& values)
{
  return StringUtils::Format("{} {}", values.at(FieldTotalDiscs).asInteger(),
                             ByLabel(attributes, values));
}
std::string ByTime(SortAttribute attributes, const SortItem &values)
{
  std::string label;
  const CVariant &time = values.at(FieldTime);
  if (time.isInteger())
    label = std::to_string(time.asInteger());
  else
    label = time.asString();
  return label;
}

std::string ByProgramCount(SortAttribute attributes, const SortItem &values)
{
  return std::to_string(values.at(FieldProgramCount).asInteger());
}

std::string ByPlaylistOrder(SortAttribute attributes, const SortItem &values)
{
  //! @todo Playlist order is hacked into program count variable (not nice, but ok until 2.0)
  return ByProgramCount(attributes, values);
}

std::string ByGenre(SortAttribute attributes, const SortItem &values)
{
  return ArrayToString(attributes, values.at(FieldGenre));
}

std::string ByCountry(SortAttribute attributes, const SortItem &values)
{
  return ArrayToString(attributes, values.at(FieldCountry));
}

std::string ByYear(SortAttribute attributes, const SortItem &values)
{
  std::string label;
  const CVariant &airDate = values.at(FieldAirDate);
  if (!airDate.isNull() && !airDate.asString().empty())
    label = airDate.asString() + " ";

  label += std::to_string(values.at(FieldYear).asInteger());

  const CVariant &album = values.at(FieldAlbum);
  if (!album.isNull())
    label += " " + SortUtils::RemoveArticles(album.asString());

  const CVariant &track = values.at(FieldTrackNumber);
  if (!track.isNull())
    label += StringUtils::Format(" {}", track.asInteger());

  label += " " + ByLabel(attributes, values);

  return label;
}

std::string ByOrigDate(SortAttribute attributes, const SortItem& values)
{
  std::string label;
  label = values.at(FieldOrigDate).asString();

  const CVariant& album = values.at(FieldAlbum);
  if (!album.isNull())
    label += " " + SortUtils::RemoveArticles(album.asString());

  const CVariant &track = values.at(FieldTrackNumber);
  if (!track.isNull())
    label += StringUtils::Format(" {}", track.asInteger());

  label += " " + ByLabel(attributes, values);

  return label;
}

std::string BySortTitle(SortAttribute attributes, const SortItem &values)
{
  std::string title = values.at(FieldSortTitle).asString();
  if (title.empty())
    title = values.at(FieldTitle).asString();

  if (attributes & SortAttributeIgnoreArticle)
    title = SortUtils::RemoveArticles(title);

  return title;
}

std::string ByOriginalTitle(SortAttribute attributes, const SortItem& values)
{
  std::string title = values.at(FieldOriginalTitle).asString();
  if (title.empty())
    title = values.at(FieldSortTitle).asString();

  if (title.empty())
    title = values.at(FieldTitle).asString();

  if (attributes & SortAttributeIgnoreArticle)
    title = SortUtils::RemoveArticles(title);

  return title;
}

std::string ByRating(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{:f} {}", values.at(FieldRating).asFloat(),
                             ByLabel(attributes, values));
}

std::string ByUserRating(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(FieldUserRating).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByVotes(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(FieldVotes).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByTop250(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(FieldTop250).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByMPAA(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldMPAA).asString() + " " + ByLabel(attributes, values);
}

std::string ByStudio(SortAttribute attributes, const SortItem &values)
{
  return ArrayToString(attributes, values.at(FieldStudio));
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
  const CVariant &episodeSpecial = values.at(FieldEpisodeNumberSpecialSort);
  const CVariant &seasonSpecial = values.at(FieldSeasonSpecialSort);
  if (!episodeSpecial.isNull() && !seasonSpecial.isNull() &&
     (episodeSpecial.asInteger() > 0 || seasonSpecial.asInteger() > 0))
    num = (static_cast<uint64_t>(seasonSpecial.asInteger()) << 32) +
          (static_cast<uint64_t>(episodeSpecial.asInteger()) << 16) -
          ((2 << 15) - values.at(FieldEpisodeNumber).asInteger());
  else
    num = (static_cast<uint64_t>(values.at(FieldSeason).asInteger()) << 32) +
          (static_cast<uint64_t>(values.at(FieldEpisodeNumber).asInteger()) << 16);

  std::string title;
  if (values.contains(FieldMediaType) && values.at(FieldMediaType).asString() == MediaTypeMovie)
    title = BySortTitle(attributes, values);
  if (title.empty())
    title = ByLabel(attributes, values);

  return StringUtils::Format("{} {}", num, title);
}

std::string BySeason(SortAttribute attributes, const SortItem &values)
{
  auto season = values.at(FieldSeason).asInteger();

  if (season == 0)
    season = std::numeric_limits<int>::max();

  const CVariant &specialSeason = values.at(FieldSeasonSpecialSort);
  if (!specialSeason.isNull() && specialSeason.asInteger() > 0)
    season = specialSeason.asInteger();

  return StringUtils::Format("{} {}", season, ByLabel(attributes, values));
}

std::string ByNumberOfEpisodes(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(FieldNumberOfEpisodes).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByNumberOfWatchedEpisodes(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(FieldNumberOfWatchedEpisodes).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByTvShowStatus(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldTvShowStatus).asString() + " " + ByLabel(attributes, values);
}

std::string ByTvShowTitle(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldTvShowTitle).asString() + " " + ByLabel(attributes, values);
}

std::string ByProductionCode(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldProductionCode).asString();
}

std::string ByVideoResolution(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(FieldVideoResolution).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByVideoCodec(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(FieldVideoCodec).asString(),
                             ByLabel(attributes, values));
}

std::string ByVideoAspectRatio(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{:.3f} {}", values.at(FieldVideoAspectRatio).asFloat(),
                             ByLabel(attributes, values));
}

std::string ByAudioChannels(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(FieldAudioChannels).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByAudioCodec(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(FieldAudioCodec).asString(),
                             ByLabel(attributes, values));
}

std::string ByAudioLanguage(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(FieldAudioLanguage).asString(),
                             ByLabel(attributes, values));
}

std::string BySubtitleLanguage(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(FieldSubtitleLanguage).asString(),
                             ByLabel(attributes, values));
}

std::string ByBitrate(SortAttribute attributes, const SortItem &values)
{
  return std::to_string(values.at(FieldBitrate).asInteger());
}

std::string ByListeners(SortAttribute attributes, const SortItem &values)
{
  return std::to_string(values.at(FieldListeners).asInteger());
}

std::string ByRandom(SortAttribute attributes, const SortItem &values)
{
  return std::to_string(CUtil::GetRandomNumber());
}

std::string ByChannel(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldChannelName).asString();
}

std::string ByChannelNumber(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldChannelNumber).asString();
}

std::string ByClientChannelOrder(SortAttribute attributes, const SortItem& values)
{
  return values.at(FieldClientChannelOrder).asString();
}

std::string ByProvider(SortAttribute attributes, const SortItem& values)
{
  return values.at(FieldProvider).asString();
}

std::string ByUserPreference(SortAttribute attributes, const SortItem& values)
{
  return values.at(FieldUserPreference).asString();
}

std::string ByDateTaken(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldDateTaken).asString();
}

std::string ByRelevance(SortAttribute attributes, const SortItem &values)
{
  return std::to_string(values.at(FieldRelevance).asInteger());
}

std::string ByInstallDate(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldInstallDate).asString();
}

std::string ByLastUpdated(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldLastUpdated).asString();
}

std::string ByLastUsed(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldLastUsed).asString();
}

std::string ByBPM(SortAttribute attributes, const SortItem& values)
{
  return StringUtils::Format("{} {}", values.at(FieldBPM).asInteger(), ByLabel(attributes, values));
}

bool preliminarySort(const SortItem& left,
                     const SortItem& right,
                     bool handleFolder,
                     bool& result,
                     std::wstring& labelLeft,
                     std::wstring& labelRight)
{
  // make sure both items have the necessary data to do the sorting
  const auto itLeftSort = left.find(FieldSort);
  if (itLeftSort == left.end())
  {
    result = false;
    return true;
  }
  const auto itRightSort = right.find(FieldSort);
  if (itRightSort == right.end())
  {
    result = true;
    return true;
  }

  // look at special sorting behaviour
  SortSpecial leftSortSpecial = SortSpecial::NONE;
  SortSpecial rightSortSpecial = SortSpecial::NONE;
  if (const auto itLeft = left.find(FieldSortSpecial);
      itLeft != left.end() &&
      itLeft->second.asInteger() <= static_cast<int64_t>(SortSpecial::BOTTOM))
    leftSortSpecial = static_cast<SortSpecial>(itLeft->second.asInteger());
  if (const auto itRight = right.find(FieldSortSpecial);
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
    const auto itLeft = left.find(FieldFolder);
    const auto itRight = right.find(FieldFolder);
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
  std::map<SortBy, SortUtils::SortPreparator> preparators;

  preparators[SortByNone]                     = NULL;
  preparators[SortByLabel]                    = ByLabel;
  preparators[SortByDate]                     = ByDate;
  preparators[SortBySize]                     = BySize;
  preparators[SortByFile]                     = ByFile;
  preparators[SortByPath]                     = ByPath;
  preparators[SortByDriveType]                = ByDriveType;
  preparators[SortByTitle]                    = ByTitle;
  preparators[SortByTrackNumber]              = ByTrackNumber;
  preparators[SortByTime]                     = ByTime;
  preparators[SortByArtist]                   = ByArtist;
  preparators[SortByArtistThenYear]           = ByArtistThenYear;
  preparators[SortByAlbum]                    = ByAlbum;
  preparators[SortByAlbumType]                = ByAlbumType;
  preparators[SortByGenre]                    = ByGenre;
  preparators[SortByCountry]                  = ByCountry;
  preparators[SortByYear]                     = ByYear;
  preparators[SortByRating]                   = ByRating;
  preparators[SortByUserRating]               = ByUserRating;
  preparators[SortByVotes]                    = ByVotes;
  preparators[SortByTop250]                   = ByTop250;
  preparators[SortByProgramCount]             = ByProgramCount;
  preparators[SortByPlaylistOrder]            = ByPlaylistOrder;
  preparators[SortByEpisodeNumber]            = ByEpisodeNumber;
  preparators[SortBySeason]                   = BySeason;
  preparators[SortByNumberOfEpisodes]         = ByNumberOfEpisodes;
  preparators[SortByNumberOfWatchedEpisodes]  = ByNumberOfWatchedEpisodes;
  preparators[SortByTvShowStatus]             = ByTvShowStatus;
  preparators[SortByTvShowTitle]              = ByTvShowTitle;
  preparators[SortBySortTitle]                = BySortTitle;
  preparators[SortByProductionCode]           = ByProductionCode;
  preparators[SortByMPAA]                     = ByMPAA;
  preparators[SortByVideoResolution]          = ByVideoResolution;
  preparators[SortByVideoCodec]               = ByVideoCodec;
  preparators[SortByVideoAspectRatio]         = ByVideoAspectRatio;
  preparators[SortByAudioChannels]            = ByAudioChannels;
  preparators[SortByAudioCodec]               = ByAudioCodec;
  preparators[SortByAudioLanguage]            = ByAudioLanguage;
  preparators[SortBySubtitleLanguage]         = BySubtitleLanguage;
  preparators[SortByStudio]                   = ByStudio;
  preparators[SortByDateAdded]                = ByDateAdded;
  preparators[SortByLastPlayed]               = ByLastPlayed;
  preparators[SortByPlaycount]                = ByPlaycount;
  preparators[SortByListeners]                = ByListeners;
  preparators[SortByBitrate]                  = ByBitrate;
  preparators[SortByRandom]                   = ByRandom;
  preparators[SortByChannel]                  = ByChannel;
  preparators[SortByChannelNumber]            = ByChannelNumber;
  preparators[SortByClientChannelOrder]       = ByClientChannelOrder;
  preparators[SortByProvider]                 = ByProvider;
  preparators[SortByUserPreference]           = ByUserPreference;
  preparators[SortByDateTaken]                = ByDateTaken;
  preparators[SortByRelevance]                = ByRelevance;
  preparators[SortByInstallDate]              = ByInstallDate;
  preparators[SortByLastUpdated]              = ByLastUpdated;
  preparators[SortByLastUsed]                 = ByLastUsed;
  preparators[SortByTotalDiscs]               = ByTotalDiscs;
  preparators[SortByOrigDate]                 = ByOrigDate;
  preparators[SortByBPM]                      = ByBPM;
  preparators[SortByOriginalTitle]            = ByOriginalTitle;

  return preparators;
}
// clang-format on

std::map<SortBy, Fields> fillSortingFields()
{
  std::map<SortBy, Fields> sortingFields;

  sortingFields.insert(std::pair<SortBy, Fields>(SortByNone, Fields()));

  sortingFields[SortByLabel].insert(FieldLabel);
  sortingFields[SortByDate].insert(FieldDate);
  sortingFields[SortBySize].insert(FieldSize);
  sortingFields[SortByFile].insert(FieldPath);
  sortingFields[SortByFile].insert(FieldStartOffset);
  sortingFields[SortByPath].insert(FieldPath);
  sortingFields[SortByPath].insert(FieldStartOffset);
  sortingFields[SortByDriveType].insert(FieldDriveType);
  sortingFields[SortByTitle].insert(FieldTitle);
  sortingFields[SortByTrackNumber].insert(FieldTrackNumber);
  sortingFields[SortByTime].insert(FieldTime);
  sortingFields[SortByArtist].insert(FieldArtist);
  sortingFields[SortByArtist].insert(FieldArtistSort);
  sortingFields[SortByArtist].insert(FieldYear);
  sortingFields[SortByArtist].insert(FieldAlbum);
  sortingFields[SortByArtist].insert(FieldTrackNumber);
  sortingFields[SortByArtistThenYear].insert(FieldArtist);
  sortingFields[SortByArtistThenYear].insert(FieldArtistSort);
  sortingFields[SortByArtistThenYear].insert(FieldYear);
  sortingFields[SortByArtistThenYear].insert(FieldOrigDate);
  sortingFields[SortByArtistThenYear].insert(FieldAlbum);
  sortingFields[SortByArtistThenYear].insert(FieldTrackNumber);
  sortingFields[SortByAlbum].insert(FieldAlbum);
  sortingFields[SortByAlbum].insert(FieldArtist);
  sortingFields[SortByAlbum].insert(FieldArtistSort);
  sortingFields[SortByAlbum].insert(FieldTrackNumber);
  sortingFields[SortByAlbumType].insert(FieldAlbumType);
  sortingFields[SortByGenre].insert(FieldGenre);
  sortingFields[SortByCountry].insert(FieldCountry);
  sortingFields[SortByYear].insert(FieldYear);
  sortingFields[SortByYear].insert(FieldAirDate);
  sortingFields[SortByYear].insert(FieldAlbum);
  sortingFields[SortByYear].insert(FieldTrackNumber);
  sortingFields[SortByYear].insert(FieldOrigDate);
  sortingFields[SortByRating].insert(FieldRating);
  sortingFields[SortByUserRating].insert(FieldUserRating);
  sortingFields[SortByVotes].insert(FieldVotes);
  sortingFields[SortByTop250].insert(FieldTop250);
  sortingFields[SortByProgramCount].insert(FieldProgramCount);
  sortingFields[SortByPlaylistOrder].insert(FieldProgramCount);
  sortingFields[SortByEpisodeNumber].insert(FieldEpisodeNumber);
  sortingFields[SortByEpisodeNumber].insert(FieldSeason);
  sortingFields[SortByEpisodeNumber].insert(FieldEpisodeNumberSpecialSort);
  sortingFields[SortByEpisodeNumber].insert(FieldSeasonSpecialSort);
  sortingFields[SortByEpisodeNumber].insert(FieldTitle);
  sortingFields[SortByEpisodeNumber].insert(FieldSortTitle);
  sortingFields[SortBySeason].insert(FieldSeason);
  sortingFields[SortBySeason].insert(FieldSeasonSpecialSort);
  sortingFields[SortByNumberOfEpisodes].insert(FieldNumberOfEpisodes);
  sortingFields[SortByNumberOfWatchedEpisodes].insert(FieldNumberOfWatchedEpisodes);
  sortingFields[SortByTvShowStatus].insert(FieldTvShowStatus);
  sortingFields[SortByTvShowTitle].insert(FieldTvShowTitle);
  sortingFields[SortBySortTitle].insert(FieldSortTitle);
  sortingFields[SortBySortTitle].insert(FieldTitle);
  sortingFields[SortByProductionCode].insert(FieldProductionCode);
  sortingFields[SortByMPAA].insert(FieldMPAA);
  sortingFields[SortByVideoResolution].insert(FieldVideoResolution);
  sortingFields[SortByVideoCodec].insert(FieldVideoCodec);
  sortingFields[SortByVideoAspectRatio].insert(FieldVideoAspectRatio);
  sortingFields[SortByAudioChannels].insert(FieldAudioChannels);
  sortingFields[SortByAudioCodec].insert(FieldAudioCodec);
  sortingFields[SortByAudioLanguage].insert(FieldAudioLanguage);
  sortingFields[SortBySubtitleLanguage].insert(FieldSubtitleLanguage);
  sortingFields[SortByStudio].insert(FieldStudio);
  sortingFields[SortByDateAdded].insert(FieldDateAdded);
  sortingFields[SortByDateAdded].insert(FieldId);
  sortingFields[SortByLastPlayed].insert(FieldLastPlayed);
  sortingFields[SortByPlaycount].insert(FieldPlaycount);
  sortingFields[SortByListeners].insert(FieldListeners);
  sortingFields[SortByBitrate].insert(FieldBitrate);
  sortingFields[SortByChannel].insert(FieldChannelName);
  sortingFields[SortByChannelNumber].insert(FieldChannelNumber);
  sortingFields[SortByClientChannelOrder].insert(FieldClientChannelOrder);
  sortingFields[SortByProvider].insert(FieldProvider);
  sortingFields[SortByUserPreference].insert(FieldUserPreference);
  sortingFields[SortByDateTaken].insert(FieldDateTaken);
  sortingFields[SortByRelevance].insert(FieldRelevance);
  sortingFields[SortByInstallDate].insert(FieldInstallDate);
  sortingFields[SortByLastUpdated].insert(FieldLastUpdated);
  sortingFields[SortByLastUsed].insert(FieldLastUsed);
  sortingFields[SortByTotalDiscs].insert(FieldTotalDiscs);
  sortingFields[SortByOrigDate].insert(FieldOrigDate);
  sortingFields[SortByOrigDate].insert(FieldAlbum);
  sortingFields[SortByOrigDate].insert(FieldTrackNumber);
  sortingFields[SortByBPM].insert(FieldBPM);
  sortingFields[SortByOriginalTitle].insert(FieldOriginalTitle);
  sortingFields[SortByOriginalTitle].insert(FieldTitle);
  sortingFields[SortByOriginalTitle].insert(FieldSortTitle);
  sortingFields.insert(std::pair<SortBy, Fields>(SortByRandom, Fields()));

  return sortingFields;
}

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
    if (sortMethod == SortByLabel || sortMethod == SortByAlbum || sortMethod == SortByTitle)
    {
      fields.emplace_back(FieldAlbum);
      fields.emplace_back(FieldArtist);
    }
    else if (sortMethod == SortByAlbumType)
    {
      fields.emplace_back(FieldAlbumType);
      fields.emplace_back(FieldAlbum);
      fields.emplace_back(FieldArtist);
    }
    else if (sortMethod == SortByArtist)
    {
      fields.emplace_back(FieldArtist);
      fields.emplace_back(FieldAlbum);
    }
    else if (sortMethod == SortByArtistThenYear)
    {
      fields.emplace_back(FieldArtist);
      fields.emplace_back(FieldYear);
      fields.emplace_back(FieldAlbum);
    }
    else if (sortMethod == SortByYear)
    {
      fields.emplace_back(FieldYear);
      fields.emplace_back(FieldAlbum);
    }
    else if (sortMethod == SortByGenre)
    {
      fields.emplace_back(FieldGenre);
      fields.emplace_back(FieldAlbum);
    }
    else if (sortMethod == SortByDateAdded)
      fields.emplace_back(FieldDateAdded);
    else if (sortMethod == SortByPlaycount)
    {
      fields.emplace_back(FieldPlaycount);
      fields.emplace_back(FieldAlbum);
    }
    else if (sortMethod == SortByLastPlayed)
    {
      fields.emplace_back(FieldLastPlayed);
      fields.emplace_back(FieldAlbum);
    }
    else if (sortMethod == SortByRating)
    {
      fields.emplace_back(FieldRating);
      fields.emplace_back(FieldAlbum);
    }
    else if (sortMethod == SortByVotes)
    {
      fields.emplace_back(FieldVotes);
      fields.emplace_back(FieldAlbum);
    }
    else if (sortMethod == SortByUserRating)
    {
      fields.emplace_back(FieldUserRating);
      fields.emplace_back(FieldAlbum);
    }
    else if (sortMethod == SortByTotalDiscs)
    {
      fields.emplace_back(FieldTotalDiscs);
      fields.emplace_back(FieldAlbum);
    }
    else if (sortMethod == SortByOrigDate)
    {
      fields.emplace_back(FieldOrigDate);
      fields.emplace_back(FieldAlbum);
    }
  }
  else if (mediaType == MediaTypeSong)
  {
    if (sortMethod == SortByLabel || sortMethod == SortByTrackNumber)
      fields.emplace_back(FieldTrackNumber);
    else if (sortMethod == SortByTitle)
      fields.emplace_back(FieldTitle);
    else if (sortMethod == SortByAlbum)
    {
      fields.emplace_back(FieldAlbum);
      fields.emplace_back(FieldAlbumArtist);
      fields.emplace_back(FieldTrackNumber);
    }
    else if (sortMethod == SortByArtist)
    {
      fields.emplace_back(FieldArtist);
      fields.emplace_back(FieldAlbum);
      fields.emplace_back(FieldTrackNumber);
    }
    else if (sortMethod == SortByArtistThenYear)
    {
      fields.emplace_back(FieldArtist);
      fields.emplace_back(FieldYear);
      fields.emplace_back(FieldAlbum);
      fields.emplace_back(FieldTrackNumber);
    }
    else if (sortMethod == SortByYear)
    {
      fields.emplace_back(FieldYear);
      fields.emplace_back(FieldAlbum);
      fields.emplace_back(FieldTrackNumber);
    }
    else if (sortMethod == SortByGenre)
    {
      fields.emplace_back(FieldGenre);
      fields.emplace_back(FieldAlbum);
    }
    else if (sortMethod == SortByDateAdded)
      fields.emplace_back(FieldDateAdded);
    else if (sortMethod == SortByPlaycount)
    {
      fields.emplace_back(FieldPlaycount);
      fields.emplace_back(FieldTrackNumber);
    }
    else if (sortMethod == SortByLastPlayed)
    {
      fields.emplace_back(FieldLastPlayed);
      fields.emplace_back(FieldTrackNumber);
    }
    else if (sortMethod == SortByRating)
    {
      fields.emplace_back(FieldRating);
      fields.emplace_back(FieldTrackNumber);
    }
    else if (sortMethod == SortByVotes)
    {
      fields.emplace_back(FieldVotes);
      fields.emplace_back(FieldTrackNumber);
    }
    else if (sortMethod == SortByUserRating)
    {
      fields.emplace_back(FieldUserRating);
      fields.emplace_back(FieldTrackNumber);
    }
    else if (sortMethod == SortByFile)
    {
      fields.emplace_back(FieldPath);
      fields.emplace_back(FieldFilename);
      fields.emplace_back(FieldStartOffset);
    }
    else if (sortMethod == SortByTime)
      fields.emplace_back(FieldTime);
    else if (sortMethod == SortByAlbumType)
    {
      fields.emplace_back(FieldAlbumType);
      fields.emplace_back(FieldAlbum);
      fields.emplace_back(FieldTrackNumber);
    }
    else if (sortMethod == SortByOrigDate)
    {
      fields.emplace_back(FieldOrigDate);
      fields.emplace_back(FieldAlbum);
      fields.emplace_back(FieldTrackNumber);
    }
    else if (sortMethod == SortByBPM)
      fields.emplace_back(FieldBPM);
  }
  else if (mediaType == MediaTypeArtist)
  {
    if (sortMethod == SortByLabel || sortMethod == SortByTitle || sortMethod == SortByArtist)
      fields.emplace_back(FieldArtist);
    else if (sortMethod == SortByGenre)
      fields.emplace_back(FieldGenre);
    else if (sortMethod == SortByDateAdded)
      fields.emplace_back(FieldDateAdded);
  }

  // Add sort by id to define order when other fields same or sort none
  fields.emplace_back(FieldId);
  return;
}

void SortUtils::Sort(SortBy sortBy, SortOrder sortOrder, SortAttribute attributes, DatabaseResults& items, int limitEnd /* = -1 */, int limitStart /* = 0 */)
{
  if (sortBy != SortByNone)
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
        item.emplace(FieldSort, CVariant(sortLabel));
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
  if (sortBy != SortByNone)
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
        item->emplace(FieldSort, CVariant(sortLabel));
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
  if (sortDescription.sortBy == SortByNone)
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
  return it == m_preparators.end() ? m_preparators[SortByNone] : it->second;
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
  return it == m_sortingFields.end() ? m_sortingFields[SortByNone] : it->second;
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
const auto table = std::array {
  sort_map{ SortByLabel,                    SortMethod::LABEL,                        SortAttributeNone,          551 },
  sort_map{ SortByLabel,                    SortMethod::LABEL_IGNORE_THE,             SortAttributeIgnoreArticle, 551 },
  sort_map{ SortByLabel,                    SortMethod::LABEL_IGNORE_FOLDERS,         SortAttributeIgnoreFolders, 551 },
  sort_map{ SortByDate,                     SortMethod::DATE,                         SortAttributeNone,          552 },
  sort_map{ SortBySize,                     SortMethod::SIZE,                         SortAttributeNone,          553 },
  sort_map{ SortByBitrate,                  SortMethod::BITRATE,                      SortAttributeNone,          623 },
  sort_map{ SortByDriveType,                SortMethod::DRIVE_TYPE,                   SortAttributeNone,          564 },
  sort_map{ SortByTrackNumber,              SortMethod::TRACKNUM,                     SortAttributeNone,          554 },
  sort_map{ SortByEpisodeNumber,            SortMethod::EPISODE,                      SortAttributeNone,          20359 },// 20360 "Episodes" used for SORT_METHOD_EPISODE for sorting tvshows by episode count
  sort_map{ SortByTime,                     SortMethod::DURATION,                     SortAttributeNone,          180 },
  sort_map{ SortByTime,                     SortMethod::VIDEO_RUNTIME,                SortAttributeNone,          180 },
  sort_map{ SortByTitle,                    SortMethod::TITLE,                        SortAttributeNone,          556 },
  sort_map{ SortByTitle,                    SortMethod::TITLE_IGNORE_THE,             SortAttributeIgnoreArticle, 556 },
  sort_map{ SortByTitle,                    SortMethod::VIDEO_TITLE,                  SortAttributeNone,          556 },
  sort_map{ SortByArtist,                   SortMethod::ARTIST,                       SortAttributeNone,          557 },
  sort_map{ SortByArtistThenYear,           SortMethod::ARTIST_AND_YEAR,              SortAttributeNone,          578 },
  sort_map{ SortByArtist,                   SortMethod::ARTIST_IGNORE_THE,            SortAttributeIgnoreArticle, 557 },
  sort_map{ SortByAlbum,                    SortMethod::ALBUM,                        SortAttributeNone,          558 },
  sort_map{ SortByAlbum,                    SortMethod::ALBUM_IGNORE_THE,             SortAttributeIgnoreArticle, 558 },
  sort_map{ SortByGenre,                    SortMethod::GENRE,                        SortAttributeNone,          515 },
  sort_map{ SortByCountry,                  SortMethod::COUNTRY,                      SortAttributeNone,          574 },
  sort_map{ SortByDateAdded,                SortMethod::DATEADDED,                    SortAttributeIgnoreFolders, 570 },
  sort_map{ SortByFile,                     SortMethod::FILE,                         SortAttributeIgnoreFolders, 561 },
  sort_map{ SortByRating,                   SortMethod::SONG_RATING,                  SortAttributeNone,          563 },
  sort_map{ SortByRating,                   SortMethod::VIDEO_RATING,                 SortAttributeIgnoreFolders, 563 },
  sort_map{ SortByUserRating,               SortMethod::SONG_USER_RATING,             SortAttributeIgnoreFolders, 38018 },
  sort_map{ SortByUserRating,               SortMethod::VIDEO_USER_RATING,            SortAttributeIgnoreFolders, 38018 },
  sort_map{ SortBySortTitle,                SortMethod::VIDEO_SORT_TITLE,             SortAttributeIgnoreFolders, 171 },
  sort_map{ SortBySortTitle,                SortMethod::VIDEO_SORT_TITLE_IGNORE_THE,  SortAttribute(SortAttributeIgnoreFolders | SortAttributeIgnoreArticle), 171 },
  sort_map{ SortByOriginalTitle,            SortMethod::VIDEO_ORIGINAL_TITLE,         SortAttributeIgnoreFolders, 20376 },
  sort_map{ SortByOriginalTitle,            SortMethod::VIDEO_ORIGINAL_TITLE_IGNORE_THE, SortAttribute(SortAttributeIgnoreFolders | SortAttributeIgnoreArticle), 20376 },
  sort_map{ SortByYear,                     SortMethod::YEAR,                         SortAttributeIgnoreFolders, 562 },
  sort_map{ SortByProductionCode,           SortMethod::PRODUCTIONCODE,               SortAttributeNone,          20368 },
  sort_map{ SortByProgramCount,             SortMethod::PROGRAM_COUNT,                SortAttributeNone,          567 }, // label is "play count"
  sort_map{ SortByPlaylistOrder,            SortMethod::PLAYLIST_ORDER,               SortAttributeIgnoreFolders, 559 },
  sort_map{ SortByMPAA,                     SortMethod::MPAA_RATING,                  SortAttributeNone,          20074 },
  sort_map{ SortByStudio,                   SortMethod::STUDIO,                       SortAttributeNone,          572 },
  sort_map{ SortByStudio,                   SortMethod::STUDIO_IGNORE_THE,            SortAttributeIgnoreArticle, 572 },
  sort_map{ SortByPath,                     SortMethod::FULLPATH,                     SortAttributeNone,          573 },
  sort_map{ SortByLastPlayed,               SortMethod::LASTPLAYED,                   SortAttributeIgnoreFolders, 568 },
  sort_map{ SortByPlaycount,                SortMethod::PLAYCOUNT,                    SortAttributeIgnoreFolders, 567 },
  sort_map{ SortByListeners,                SortMethod::LISTENERS,                    SortAttributeNone,          20455 },
  sort_map{ SortByChannel,                  SortMethod::CHANNEL,                      SortAttributeNone,          19029 },
  sort_map{ SortByChannel,                  SortMethod::CHANNEL_NUMBER,               SortAttributeNone,          549 },
  sort_map{ SortByChannel,                  SortMethod::CLIENT_CHANNEL_ORDER,         SortAttributeNone,          19315 },
  sort_map{ SortByProvider,                 SortMethod::PROVIDER,                     SortAttributeNone,          19348 },
  sort_map{ SortByUserPreference,           SortMethod::USER_PREFERENCE,              SortAttributeNone,          19349 },
  sort_map{ SortByDateTaken,                SortMethod::DATE_TAKEN,                   SortAttributeIgnoreFolders, 577 },
  sort_map{ SortByNone,                     SortMethod::NONE,                         SortAttributeNone,          16018 },
  sort_map{ SortByTotalDiscs,               SortMethod::TOTAL_DISCS,                  SortAttributeNone,          38077 },
  sort_map{ SortByOrigDate,                 SortMethod::ORIG_DATE,                    SortAttributeNone,          38079 },
  sort_map{ SortByBPM,                      SortMethod::BPM,                          SortAttributeNone,          38080 },

  // the following have no corresponding SortMethod::*
  sort_map{ SortByAlbumType,                SortMethod::NONE,                         SortAttributeNone,          564 },
  sort_map{ SortByVotes,                    SortMethod::NONE,                         SortAttributeNone,          205 },
  sort_map{ SortByTop250,                   SortMethod::NONE,                         SortAttributeNone,          13409 },
  sort_map{ SortByMPAA,                     SortMethod::NONE,                         SortAttributeNone,          20074 },
  sort_map{ SortByDateAdded,                SortMethod::NONE,                         SortAttributeNone,          570 },
  sort_map{ SortByTvShowTitle,              SortMethod::NONE,                         SortAttributeNone,          20364 },
  sort_map{ SortByTvShowStatus,             SortMethod::NONE,                         SortAttributeNone,          126 },
  sort_map{ SortBySeason,                   SortMethod::NONE,                         SortAttributeNone,          20373 },
  sort_map{ SortByNumberOfEpisodes,         SortMethod::NONE,                         SortAttributeNone,          20360 },
  sort_map{ SortByNumberOfWatchedEpisodes,  SortMethod::NONE,                         SortAttributeNone,          21441 },
  sort_map{ SortByVideoResolution,          SortMethod::NONE,                         SortAttributeNone,          21443 },
  sort_map{ SortByVideoCodec,               SortMethod::NONE,                         SortAttributeNone,          21445 },
  sort_map{ SortByVideoAspectRatio,         SortMethod::NONE,                         SortAttributeNone,          21374 },
  sort_map{ SortByAudioChannels,            SortMethod::NONE,                         SortAttributeNone,          21444 },
  sort_map{ SortByAudioCodec,               SortMethod::NONE,                         SortAttributeNone,          21446 },
  sort_map{ SortByAudioLanguage,            SortMethod::NONE,                         SortAttributeNone,          21447 },
  sort_map{ SortBySubtitleLanguage,         SortMethod::NONE,                         SortAttributeNone,          21448 },
  sort_map{ SortByRandom,                   SortMethod::NONE,                         SortAttributeNone,          590 }
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
const std::map<std::string, SortBy> sortMethods = {
    {"label", SortByLabel},
    {"date", SortByDate},
    {"size", SortBySize},
    {"file", SortByFile},
    {"path", SortByPath},
    {"drivetype", SortByDriveType},
    {"title", SortByTitle},
    {"track", SortByTrackNumber},
    {"time", SortByTime},
    {"artist", SortByArtist},
    {"artistyear", SortByArtistThenYear},
    {"album", SortByAlbum},
    {"albumtype", SortByAlbumType},
    {"genre", SortByGenre},
    {"country", SortByCountry},
    {"year", SortByYear},
    {"rating", SortByRating},
    {"votes", SortByVotes},
    {"top250", SortByTop250},
    {"programcount", SortByProgramCount},
    {"playlist", SortByPlaylistOrder},
    {"episode", SortByEpisodeNumber},
    {"season", SortBySeason},
    {"totalepisodes", SortByNumberOfEpisodes},
    {"watchedepisodes", SortByNumberOfWatchedEpisodes},
    {"tvshowstatus", SortByTvShowStatus},
    {"tvshowtitle", SortByTvShowTitle},
    {"sorttitle", SortBySortTitle},
    {"productioncode", SortByProductionCode},
    {"mpaa", SortByMPAA},
    {"videoresolution", SortByVideoResolution},
    {"videocodec", SortByVideoCodec},
    {"videoaspectratio", SortByVideoAspectRatio},
    {"audiochannels", SortByAudioChannels},
    {"audiocodec", SortByAudioCodec},
    {"audiolanguage", SortByAudioLanguage},
    {"subtitlelanguage", SortBySubtitleLanguage},
    {"studio", SortByStudio},
    {"dateadded", SortByDateAdded},
    {"lastplayed", SortByLastPlayed},
    {"playcount", SortByPlaycount},
    {"listeners", SortByListeners},
    {"bitrate", SortByBitrate},
    {"random", SortByRandom},
    {"channel", SortByChannel},
    {"channelnumber", SortByChannelNumber},
    {"clientchannelorder", SortByClientChannelOrder},
    {"provider", SortByProvider},
    {"userpreference", SortByUserPreference},
    {"datetaken", SortByDateTaken},
    {"userrating", SortByUserRating},
    {"installdate", SortByInstallDate},
    {"lastupdated", SortByLastUpdated},
    {"lastused", SortByLastUsed},
    {"totaldiscs", SortByTotalDiscs},
    {"originaldate", SortByOrigDate},
    {"bpm", SortByBPM},
    {"originaltitle", SortByOriginalTitle},
};

SortBy SortUtils::SortMethodFromString(const std::string& sortMethod)
{
  return TypeFromString<SortBy>(sortMethods, sortMethod, SortByNone);
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
