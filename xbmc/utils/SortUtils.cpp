/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SortUtils.h"

#include "LangInfo.h"
#include "URL.h"
#include "Util.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <algorithm>
#include <inttypes.h>

std::string ArrayToString(SortAttribute attributes, const CVariant &variant, const std::string &separator = " / ")
{
  std::vector<std::string> strArray;
  if (variant.isArray())
  {
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
  return StringUtils::Format("{} {}", (int)values.at(FieldPlaycount).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByDate(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldDate).asString() + " " + ByLabel(attributes, values);
}

std::string ByDateAdded(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", values.at(FieldDateAdded).asString(),
                             (int)values.at(FieldId).asInteger());
}

std::string BySize(SortAttribute attributes, const SortItem &values)
{
  return std::to_string(values.at(FieldSize).asInteger());
}

std::string ByDriveType(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", (int)values.at(FieldDriveType).asInteger(),
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
    label += StringUtils::Format(" {}", (int)track.asInteger());

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
    label += StringUtils::Format(" {}", (int)track.asInteger());

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
    label += StringUtils::Format(" {}", static_cast<int>(year.asInteger()));

  const CVariant &album = values.at(FieldAlbum);
  if (!album.isNull())
    label += " " + SortUtils::RemoveArticles(album.asString());

  const CVariant &track = values.at(FieldTrackNumber);
  if (!track.isNull())
    label += StringUtils::Format(" {}", (int)track.asInteger());

  return label;
}

std::string ByTrackNumber(SortAttribute attributes, const SortItem &values)
{
  return std::to_string((int)values.at(FieldTrackNumber).asInteger());
}

std::string ByTotalDiscs(SortAttribute attributes, const SortItem& values)
{
  return StringUtils::Format("{} {}", static_cast<int>(values.at(FieldTotalDiscs).asInteger()),
                             ByLabel(attributes, values));
}
std::string ByTime(SortAttribute attributes, const SortItem &values)
{
  std::string label;
  const CVariant &time = values.at(FieldTime);
  if (time.isInteger())
    label = std::to_string((int)time.asInteger());
  else
    label = time.asString();
  return label;
}

std::string ByProgramCount(SortAttribute attributes, const SortItem &values)
{
  return std::to_string((int)values.at(FieldProgramCount).asInteger());
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

  label += std::to_string((int)values.at(FieldYear).asInteger());

  const CVariant &album = values.at(FieldAlbum);
  if (!album.isNull())
    label += " " + SortUtils::RemoveArticles(album.asString());

  const CVariant &track = values.at(FieldTrackNumber);
  if (!track.isNull())
    label += StringUtils::Format(" {}", (int)track.asInteger());

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
    label += StringUtils::Format(" {}", static_cast<int>(track.asInteger()));

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
  return StringUtils::Format("{} {}", static_cast<int>(values.at(FieldUserRating).asInteger()),
                             ByLabel(attributes, values));
}

std::string ByVotes(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", (int)values.at(FieldVotes).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByTop250(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", (int)values.at(FieldTop250).asInteger(),
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
    num = ((uint64_t)seasonSpecial.asInteger() << 32) + (episodeSpecial.asInteger() << 16) - ((2 << 15) - values.at(FieldEpisodeNumber).asInteger());
  else
    num = ((uint64_t)values.at(FieldSeason).asInteger() << 32) + (values.at(FieldEpisodeNumber).asInteger() << 16);

  std::string title;
  if (values.find(FieldMediaType) != values.end() && values.at(FieldMediaType).asString() == MediaTypeMovie)
    title = BySortTitle(attributes, values);
  if (title.empty())
    title = ByLabel(attributes, values);

  return StringUtils::Format("{} {}", num, title);
}

std::string BySeason(SortAttribute attributes, const SortItem &values)
{
  int season = (int)values.at(FieldSeason).asInteger();
  const CVariant &specialSeason = values.at(FieldSeasonSpecialSort);
  if (!specialSeason.isNull())
    season = (int)specialSeason.asInteger();

  return StringUtils::Format("{} {}", season, ByLabel(attributes, values));
}

std::string ByNumberOfEpisodes(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", (int)values.at(FieldNumberOfEpisodes).asInteger(),
                             ByLabel(attributes, values));
}

std::string ByNumberOfWatchedEpisodes(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("{} {}", (int)values.at(FieldNumberOfWatchedEpisodes).asInteger(),
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
  return StringUtils::Format("{} {}", (int)values.at(FieldVideoResolution).asInteger(),
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
  return StringUtils::Format("{} {}", (int)values.at(FieldAudioChannels).asInteger(),
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
  return std::to_string((int)values.at(FieldRelevance).asInteger());
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
  return StringUtils::Format("{} {}", static_cast<int>(values.at(FieldBPM).asInteger()),
                             ByLabel(attributes, values));
}

bool preliminarySort(const SortItem &left, const SortItem &right, bool handleFolder, bool &result, std::wstring &labelLeft, std::wstring &labelRight)
{
  // make sure both items have the necessary data to do the sorting
  SortItem::const_iterator itLeftSort, itRightSort;
  if ((itLeftSort = left.find(FieldSort)) == left.end())
  {
    result = false;
    return true;
  }
  if ((itRightSort = right.find(FieldSort)) == right.end())
  {
    result = true;
    return true;
  }

  // look at special sorting behaviour
  SortItem::const_iterator itLeft, itRight;
  SortSpecial leftSortSpecial = SortSpecialNone;
  SortSpecial rightSortSpecial = SortSpecialNone;
  if ((itLeft = left.find(FieldSortSpecial)) != left.end() && itLeft->second.asInteger() <= (int64_t)SortSpecialOnBottom)
    leftSortSpecial = (SortSpecial)itLeft->second.asInteger();
  if ((itRight = right.find(FieldSortSpecial)) != right.end() && itRight->second.asInteger() <= (int64_t)SortSpecialOnBottom)
    rightSortSpecial = (SortSpecial)itRight->second.asInteger();

  // one has a special sort
  if (leftSortSpecial != rightSortSpecial)
  {
    // left should be sorted on top
    // or right should be sorted on bottom
    // => left is sorted above right
    if (leftSortSpecial == SortSpecialOnTop ||
        rightSortSpecial == SortSpecialOnBottom)
    {
      result = true;
      return true;
    }

    // otherwise right is sorted above left
    result = false;
    return true;
  }
  // both have either sort on top or sort on bottom -> leave as-is
  else if (leftSortSpecial != SortSpecialNone)
  {
    result = false;
    return true;
  }

  if (handleFolder)
  {
    itLeft = left.find(FieldFolder);
    itRight = right.find(FieldFolder);
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

  return StringUtils::AlphaNumericCompare(labelLeft.c_str(), labelRight.c_str()) < 0;
}

bool SorterDescending(const SortItem &left, const SortItem &right)
{
  bool result;
  std::wstring labelLeft, labelRight;
  if (preliminarySort(left, right, true, result, labelLeft, labelRight))
    return result;

  return StringUtils::AlphaNumericCompare(labelLeft.c_str(), labelRight.c_str()) > 0;
}

bool SorterIgnoreFoldersAscending(const SortItem &left, const SortItem &right)
{
  bool result;
  std::wstring labelLeft, labelRight;
  if (preliminarySort(left, right, false, result, labelLeft, labelRight))
    return result;

  return StringUtils::AlphaNumericCompare(labelLeft.c_str(), labelRight.c_str()) < 0;
}

bool SorterIgnoreFoldersDescending(const SortItem &left, const SortItem &right)
{
  bool result;
  std::wstring labelLeft, labelRight;
  if (preliminarySort(left, right, false, result, labelLeft, labelRight))
    return result;

  return StringUtils::AlphaNumericCompare(labelLeft.c_str(), labelRight.c_str()) > 0;
}

bool SorterIndirectAscending(const SortItemPtr &left, const SortItemPtr &right)
{
  return SorterAscending(*left, *right);
}

bool SorterIndirectDescending(const SortItemPtr &left, const SortItemPtr &right)
{
  return SorterDescending(*left, *right);
}

bool SorterIndirectIgnoreFoldersAscending(const SortItemPtr &left, const SortItemPtr &right)
{
  return SorterIgnoreFoldersAscending(*left, *right);
}

bool SorterIndirectIgnoreFoldersDescending(const SortItemPtr &left, const SortItemPtr &right)
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
    if (preparator != NULL)
    {
      Fields sortingFields = GetFieldsForSorting(sortBy);

      // Prepare the string used for sorting and store it under FieldSort
      for (DatabaseResults::iterator item = items.begin(); item != items.end(); ++item)
      {
        // add all fields to the item that are required for sorting if they are currently missing
        for (Fields::const_iterator field = sortingFields.begin(); field != sortingFields.end(); ++field)
        {
          if (item->find(*field) == item->end())
            item->insert(std::pair<Field, CVariant>(*field, CVariant::ConstNullVariant));
        }

        std::wstring sortLabel;
        g_charsetConverter.utf8ToW(preparator(attributes, *item), sortLabel, false);
        item->insert(std::pair<Field, CVariant>(FieldSort, CVariant(sortLabel)));
      }

      // Do the sorting
      std::stable_sort(items.begin(), items.end(), getSorter(sortOrder, attributes));
    }
  }

  if (limitStart > 0 && (size_t)limitStart < items.size())
  {
    items.erase(items.begin(), items.begin() + limitStart);
    limitEnd -= limitStart;
  }
  if (limitEnd > 0 && (size_t)limitEnd < items.size())
    items.erase(items.begin() + limitEnd, items.end());
}

void SortUtils::Sort(SortBy sortBy, SortOrder sortOrder, SortAttribute attributes, SortItems& items, int limitEnd /* = -1 */, int limitStart /* = 0 */)
{
  if (sortBy != SortByNone)
  {
    // get the matching SortPreparator
    SortPreparator preparator = getPreparator(sortBy);
    if (preparator != NULL)
    {
      Fields sortingFields = GetFieldsForSorting(sortBy);

      // Prepare the string used for sorting and store it under FieldSort
      for (SortItems::iterator item = items.begin(); item != items.end(); ++item)
      {
        // add all fields to the item that are required for sorting if they are currently missing
        for (Fields::const_iterator field = sortingFields.begin(); field != sortingFields.end(); ++field)
        {
          if ((*item)->find(*field) == (*item)->end())
            (*item)->insert(std::pair<Field, CVariant>(*field, CVariant::ConstNullVariant));
        }

        std::wstring sortLabel;
        g_charsetConverter.utf8ToW(preparator(attributes, **item), sortLabel, false);
        (*item)->insert(std::pair<Field, CVariant>(FieldSort, CVariant(sortLabel)));
      }

      // Do the sorting
      std::stable_sort(items.begin(), items.end(), getSorterIndirect(sortOrder, attributes));
    }
  }

  if (limitStart > 0 && (size_t)limitStart < items.size())
  {
    items.erase(items.begin(), items.begin() + limitStart);
    limitEnd -= limitStart;
  }
  if (limitEnd > 0 && (size_t)limitEnd < items.size())
    items.erase(items.begin() + limitEnd, items.end());
}

void SortUtils::Sort(const SortDescription &sortDescription, DatabaseResults& items)
{
  Sort(sortDescription.sortBy, sortDescription.sortOrder, sortDescription.sortAttributes, items, sortDescription.limitEnd, sortDescription.limitStart);
}

void SortUtils::Sort(const SortDescription &sortDescription, SortItems& items)
{
  Sort(sortDescription.sortBy, sortDescription.sortOrder, sortDescription.sortAttributes, items, sortDescription.limitEnd, sortDescription.limitStart);
}

bool SortUtils::SortFromDataset(const SortDescription &sortDescription, const MediaType &mediaType, const std::unique_ptr<dbiplus::Dataset> &dataset, DatabaseResults &results)
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
  std::map<SortBy, SortPreparator>::const_iterator it = m_preparators.find(sortBy);
  if (it != m_preparators.end())
    return it->second;

  return m_preparators[SortByNone];
}

SortUtils::Sorter SortUtils::getSorter(SortOrder sortOrder, SortAttribute attributes)
{
  if (attributes & SortAttributeIgnoreFolders)
    return sortOrder == SortOrderDescending ? SorterIgnoreFoldersDescending : SorterIgnoreFoldersAscending;

  return sortOrder == SortOrderDescending ? SorterDescending : SorterAscending;
}

SortUtils::SorterIndirect SortUtils::getSorterIndirect(SortOrder sortOrder, SortAttribute attributes)
{
  if (attributes & SortAttributeIgnoreFolders)
    return sortOrder == SortOrderDescending ? SorterIndirectIgnoreFoldersDescending : SorterIndirectIgnoreFoldersAscending;

  return sortOrder == SortOrderDescending ? SorterIndirectDescending : SorterIndirectAscending;
}

const Fields& SortUtils::GetFieldsForSorting(SortBy sortBy)
{
  std::map<SortBy, Fields>::const_iterator it = m_sortingFields.find(sortBy);
  if (it != m_sortingFields.end())
    return it->second;

  return m_sortingFields[SortByNone];
}

std::string SortUtils::RemoveArticles(const std::string &label)
{
  std::set<std::string> sortTokens = g_langInfo.GetSortTokens();
  for (std::set<std::string>::const_iterator token = sortTokens.begin(); token != sortTokens.end(); ++token)
  {
    if (token->size() < label.size() && StringUtils::StartsWithNoCase(label, *token))
      return label.substr(token->size());
  }

  return label;
}

typedef struct
{
  SortBy        sort;
  SORT_METHOD   old;
  SortAttribute flags;
  int           label;
} sort_map;

// clang-format off
const sort_map table[] = {
  { SortByLabel,                    SORT_METHOD_LABEL,                        SortAttributeNone,          551 },
  { SortByLabel,                    SORT_METHOD_LABEL_IGNORE_THE,             SortAttributeIgnoreArticle, 551 },
  { SortByLabel,                    SORT_METHOD_LABEL_IGNORE_FOLDERS,         SortAttributeIgnoreFolders, 551 },
  { SortByDate,                     SORT_METHOD_DATE,                         SortAttributeNone,          552 },
  { SortBySize,                     SORT_METHOD_SIZE,                         SortAttributeNone,          553 },
  { SortByBitrate,                  SORT_METHOD_BITRATE,                      SortAttributeNone,          623 },
  { SortByDriveType,                SORT_METHOD_DRIVE_TYPE,                   SortAttributeNone,          564 },
  { SortByTrackNumber,              SORT_METHOD_TRACKNUM,                     SortAttributeNone,          554 },
  { SortByEpisodeNumber,            SORT_METHOD_EPISODE,                      SortAttributeNone,          20359 },// 20360 "Episodes" used for SORT_METHOD_EPISODE for sorting tvshows by episode count
  { SortByTime,                     SORT_METHOD_DURATION,                     SortAttributeNone,          180 },
  { SortByTime,                     SORT_METHOD_VIDEO_RUNTIME,                SortAttributeNone,          180 },
  { SortByTitle,                    SORT_METHOD_TITLE,                        SortAttributeNone,          556 },
  { SortByTitle,                    SORT_METHOD_TITLE_IGNORE_THE,             SortAttributeIgnoreArticle, 556 },
  { SortByTitle,                    SORT_METHOD_VIDEO_TITLE,                  SortAttributeNone,          556 },
  { SortByArtist,                   SORT_METHOD_ARTIST,                       SortAttributeNone,          557 },
  { SortByArtistThenYear,           SORT_METHOD_ARTIST_AND_YEAR,              SortAttributeNone,          578 },
  { SortByArtist,                   SORT_METHOD_ARTIST_IGNORE_THE,            SortAttributeIgnoreArticle, 557 },
  { SortByAlbum,                    SORT_METHOD_ALBUM,                        SortAttributeNone,          558 },
  { SortByAlbum,                    SORT_METHOD_ALBUM_IGNORE_THE,             SortAttributeIgnoreArticle, 558 },
  { SortByGenre,                    SORT_METHOD_GENRE,                        SortAttributeNone,          515 },
  { SortByCountry,                  SORT_METHOD_COUNTRY,                      SortAttributeNone,          574 },
  { SortByDateAdded,                SORT_METHOD_DATEADDED,                    SortAttributeIgnoreFolders, 570 },
  { SortByFile,                     SORT_METHOD_FILE,                         SortAttributeIgnoreFolders, 561 },
  { SortByRating,                   SORT_METHOD_SONG_RATING,                  SortAttributeNone,          563 },
  { SortByRating,                   SORT_METHOD_VIDEO_RATING,                 SortAttributeIgnoreFolders, 563 },
  { SortByUserRating,               SORT_METHOD_SONG_USER_RATING,             SortAttributeIgnoreFolders, 38018 },
  { SortByUserRating,               SORT_METHOD_VIDEO_USER_RATING,            SortAttributeIgnoreFolders, 38018 },
  { SortBySortTitle,                SORT_METHOD_VIDEO_SORT_TITLE,             SortAttributeIgnoreFolders, 171 },
  { SortBySortTitle,                SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE,  (SortAttribute)(SortAttributeIgnoreFolders | SortAttributeIgnoreArticle), 171 },
  { SortByOriginalTitle,            SORT_METHOD_VIDEO_ORIGINAL_TITLE,         SortAttributeIgnoreFolders, 20376 },
  { SortByOriginalTitle,            SORT_METHOD_VIDEO_ORIGINAL_TITLE_IGNORE_THE,  (SortAttribute)(SortAttributeIgnoreFolders | SortAttributeIgnoreArticle), 20376 },
  { SortByYear,                     SORT_METHOD_YEAR,                         SortAttributeIgnoreFolders, 562 },
  { SortByProductionCode,           SORT_METHOD_PRODUCTIONCODE,               SortAttributeNone,          20368 },
  { SortByProgramCount,             SORT_METHOD_PROGRAM_COUNT,                SortAttributeNone,          567 }, // label is "play count"
  { SortByPlaylistOrder,            SORT_METHOD_PLAYLIST_ORDER,               SortAttributeIgnoreFolders, 559 },
  { SortByMPAA,                     SORT_METHOD_MPAA_RATING,                  SortAttributeNone,          20074 },
  { SortByStudio,                   SORT_METHOD_STUDIO,                       SortAttributeNone,          572 },
  { SortByStudio,                   SORT_METHOD_STUDIO_IGNORE_THE,            SortAttributeIgnoreArticle, 572 },
  { SortByPath,                     SORT_METHOD_FULLPATH,                     SortAttributeNone,          573 },
  { SortByLastPlayed,               SORT_METHOD_LASTPLAYED,                   SortAttributeIgnoreFolders, 568 },
  { SortByPlaycount,                SORT_METHOD_PLAYCOUNT,                    SortAttributeIgnoreFolders, 567 },
  { SortByListeners,                SORT_METHOD_LISTENERS,                    SortAttributeNone,          20455 },
  { SortByChannel,                  SORT_METHOD_CHANNEL,                      SortAttributeNone,          19029 },
  { SortByChannel,                  SORT_METHOD_CHANNEL_NUMBER,               SortAttributeNone,          549 },
  { SortByChannel,                  SORT_METHOD_CLIENT_CHANNEL_ORDER,         SortAttributeNone,          19315 },
  { SortByProvider,                 SORT_METHOD_PROVIDER,                     SortAttributeNone,          19348 },
  { SortByUserPreference,           SORT_METHOD_USER_PREFERENCE,              SortAttributeNone,          19349 },
  { SortByDateTaken,                SORT_METHOD_DATE_TAKEN,                   SortAttributeIgnoreFolders, 577 },
  { SortByNone,                     SORT_METHOD_NONE,                         SortAttributeNone,          16018 },
  { SortByTotalDiscs,               SORT_METHOD_TOTAL_DISCS,                  SortAttributeNone,          38077 },
  { SortByOrigDate,                 SORT_METHOD_ORIG_DATE,                    SortAttributeNone,          38079 },
  { SortByBPM,                      SORT_METHOD_BPM,                          SortAttributeNone,          38080 },

  // the following have no corresponding SORT_METHOD_*
  { SortByAlbumType,                SORT_METHOD_NONE,                         SortAttributeNone,          564 },
  { SortByVotes,                    SORT_METHOD_NONE,                         SortAttributeNone,          205 },
  { SortByTop250,                   SORT_METHOD_NONE,                         SortAttributeNone,          13409 },
  { SortByMPAA,                     SORT_METHOD_NONE,                         SortAttributeNone,          20074 },
  { SortByDateAdded,                SORT_METHOD_NONE,                         SortAttributeNone,          570 },
  { SortByTvShowTitle,              SORT_METHOD_NONE,                         SortAttributeNone,          20364 },
  { SortByTvShowStatus,             SORT_METHOD_NONE,                         SortAttributeNone,          126 },
  { SortBySeason,                   SORT_METHOD_NONE,                         SortAttributeNone,          20373 },
  { SortByNumberOfEpisodes,         SORT_METHOD_NONE,                         SortAttributeNone,          20360 },
  { SortByNumberOfWatchedEpisodes,  SORT_METHOD_NONE,                         SortAttributeNone,          21441 },
  { SortByVideoResolution,          SORT_METHOD_NONE,                         SortAttributeNone,          21443 },
  { SortByVideoCodec,               SORT_METHOD_NONE,                         SortAttributeNone,          21445 },
  { SortByVideoAspectRatio,         SORT_METHOD_NONE,                         SortAttributeNone,          21374 },
  { SortByAudioChannels,            SORT_METHOD_NONE,                         SortAttributeNone,          21444 },
  { SortByAudioCodec,               SORT_METHOD_NONE,                         SortAttributeNone,          21446 },
  { SortByAudioLanguage,            SORT_METHOD_NONE,                         SortAttributeNone,          21447 },
  { SortBySubtitleLanguage,         SORT_METHOD_NONE,                         SortAttributeNone,          21448 },
  { SortByRandom,                   SORT_METHOD_NONE,                         SortAttributeNone,          590 }
};
// clang-format on

SORT_METHOD SortUtils::TranslateOldSortMethod(SortBy sortBy, bool ignoreArticle)
{
  for (const sort_map& t : table)
  {
    if (t.sort == sortBy)
    {
      if (ignoreArticle == ((t.flags & SortAttributeIgnoreArticle) == SortAttributeIgnoreArticle))
        return t.old;
    }
  }
  for (const sort_map& t : table)
  {
    if (t.sort == sortBy)
      return t.old;
  }
  return SORT_METHOD_NONE;
}

SortDescription SortUtils::TranslateOldSortMethod(SORT_METHOD sortBy)
{
  SortDescription description;
  for (const sort_map& t : table)
  {
    if (t.old == sortBy)
    {
      description.sortBy = t.sort;
      description.sortAttributes = t.flags;
      break;
    }
  }
  return description;
}

int SortUtils::GetSortLabel(SortBy sortBy)
{
  for (const sort_map& t : table)
  {
    if (t.sort == sortBy)
      return t.label;
  }
  return 16018; // None
}

template<typename T>
T TypeFromString(const std::map<std::string, T>& typeMap, const std::string& name, const T& defaultType)
{
  auto it = typeMap.find(name);
  if (it == typeMap.end())
    return defaultType;

  return it->second;
}

template<typename T>
const std::string& TypeToString(const std::map<std::string, T>& typeMap, const T& value)
{
  auto it = std::find_if(typeMap.begin(), typeMap.end(),
    [&value](const std::pair<std::string, T>& pair)
  {
    return pair.second == value;
  });

  if (it == typeMap.end())
    return StringUtils::Empty;

  return it->first;
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

const std::map<std::string, SortOrder> sortOrders = {
  { "ascending", SortOrderAscending },
  { "descending", SortOrderDescending }
};

SortOrder SortUtils::SortOrderFromString(const std::string& sortOrder)
{
  return TypeFromString<SortOrder>(sortOrders, sortOrder, SortOrderNone);
}

const std::string& SortUtils::SortOrderToString(SortOrder sortOrder)
{
  return TypeToString<SortOrder>(sortOrders, sortOrder);
}
