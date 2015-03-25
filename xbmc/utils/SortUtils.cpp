/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "SortUtils.h"
#include "LangInfo.h"
#include "URL.h"
#include "Util.h"
#include "XBDateTime.h"
#include "settings/AdvancedSettings.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <algorithm>

using namespace std;

string ArrayToString(SortAttribute attributes, const CVariant &variant, const string &seperator = " / ")
{
  vector<string> strArray;
  if (variant.isArray())
  {
    for (CVariant::const_iterator_array it = variant.begin_array(); it != variant.end_array(); it++)
    {
      if (attributes & SortAttributeIgnoreArticle)
        strArray.push_back(SortUtils::RemoveArticles(it->asString()));
      else
        strArray.push_back(it->asString());
    }

    return StringUtils::Join(strArray, seperator);
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

string ByLabel(SortAttribute attributes, const SortItem &values)
{
  if (attributes & SortAttributeIgnoreArticle)
    return SortUtils::RemoveArticles(values.at(FieldLabel).asString());

  return values.at(FieldLabel).asString();
}

string ByFile(SortAttribute attributes, const SortItem &values)
{
  CURL url(values.at(FieldPath).asString());
  
  return StringUtils::Format("%s %" PRId64, url.GetFileNameWithoutPath().c_str(), values.at(FieldStartOffset).asInteger());
}

string ByPath(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%s %" PRId64, values.at(FieldPath).asString().c_str(), values.at(FieldStartOffset).asInteger());
}

string ByLastPlayed(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%s %s", values.at(FieldLastPlayed).asString().c_str(), ByLabel(attributes, values).c_str());
}

string ByPlaycount(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%i %s", (int)values.at(FieldPlaycount).asInteger(), ByLabel(attributes, values).c_str());
}

string ByDate(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldDate).asString() + " " + ByLabel(attributes, values);
}

string ByDateAdded(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%s %d", values.at(FieldDateAdded).asString().c_str(), (int)values.at(FieldId).asInteger());;
}

string BySize(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%" PRId64, values.at(FieldSize).asInteger());
}

string ByDriveType(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%d %s", (int)values.at(FieldDriveType).asInteger(), ByLabel(attributes, values).c_str());
}

string ByTitle(SortAttribute attributes, const SortItem &values)
{
  if (attributes & SortAttributeIgnoreArticle)
    return SortUtils::RemoveArticles(values.at(FieldTitle).asString());

  return values.at(FieldTitle).asString();
}

string ByAlbum(SortAttribute attributes, const SortItem &values)
{
  string album = values.at(FieldAlbum).asString();
  if (attributes & SortAttributeIgnoreArticle)
    album = SortUtils::RemoveArticles(album);

  std::string label = StringUtils::Format("%s %s", album.c_str(), ArrayToString(attributes, values.at(FieldArtist)).c_str());

  const CVariant &track = values.at(FieldTrackNumber);
  if (!track.isNull())
    label += StringUtils::Format(" %i", (int)track.asInteger());

  return label;
}

string ByAlbumType(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldAlbumType).asString() + " " + ByLabel(attributes, values);
}

string ByArtist(SortAttribute attributes, const SortItem &values)
{
  std::string label = ArrayToString(attributes, values.at(FieldArtist));

  const CVariant &year = values.at(FieldYear);
  if (g_advancedSettings.m_bMusicLibraryAlbumsSortByArtistThenYear &&
      !year.isNull())
    label += StringUtils::Format(" %i", (int)year.asInteger());

  const CVariant &album = values.at(FieldAlbum);
  if (!album.isNull())
    label += " " + SortUtils::RemoveArticles(album.asString());

  const CVariant &track = values.at(FieldTrackNumber);
  if (!track.isNull())
    label += StringUtils::Format(" %i", (int)track.asInteger());

  return label;
}

string ByTrackNumber(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%i", (int)values.at(FieldTrackNumber).asInteger());
}

string ByTime(SortAttribute attributes, const SortItem &values)
{
  std::string label;
  const CVariant &time = values.at(FieldTime);
  if (time.isInteger())
    label = StringUtils::Format("%i", (int)time.asInteger());
  else
    label = StringUtils::Format("%s", time.asString().c_str());
  return label;
}

string ByProgramCount(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%i", (int)values.at(FieldProgramCount).asInteger());
}

string ByPlaylistOrder(SortAttribute attributes, const SortItem &values)
{
  // TODO: Playlist order is hacked into program count variable (not nice, but ok until 2.0)
  return ByProgramCount(attributes, values);
}

string ByGenre(SortAttribute attributes, const SortItem &values)
{
  return ArrayToString(attributes, values.at(FieldGenre));
}

string ByCountry(SortAttribute attributes, const SortItem &values)
{
  return ArrayToString(attributes, values.at(FieldCountry));
}

string ByYear(SortAttribute attributes, const SortItem &values)
{
  std::string label;
  const CVariant &airDate = values.at(FieldAirDate);
  if (!airDate.isNull() && !airDate.asString().empty())
    label = airDate.asString() + " ";

  label += StringUtils::Format("%i", (int)values.at(FieldYear).asInteger());

  const CVariant &album = values.at(FieldAlbum);
  if (!album.isNull())
    label += " " + SortUtils::RemoveArticles(album.asString());

  const CVariant &track = values.at(FieldTrackNumber);
  if (!track.isNull())
    label += StringUtils::Format(" %i", (int)track.asInteger());

  label += " " + ByLabel(attributes, values);
 
  return label;
}

string BySortTitle(SortAttribute attributes, const SortItem &values)
{
  string title = values.at(FieldSortTitle).asString();
  if (title.empty())
    title = values.at(FieldTitle).asString();

  if (attributes & SortAttributeIgnoreArticle)
    title = SortUtils::RemoveArticles(title);

  return title;
}

string ByRating(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%f %s", values.at(FieldRating).asFloat(), ByLabel(attributes, values).c_str());
}

string ByVotes(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%d %s", (int)values.at(FieldVotes).asInteger(), ByLabel(attributes, values).c_str());
}

string ByTop250(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%d %s", (int)values.at(FieldTop250).asInteger(), ByLabel(attributes, values).c_str());
}

string ByMPAA(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldMPAA).asString() + " " + ByLabel(attributes, values);
}

string ByStudio(SortAttribute attributes, const SortItem &values)
{
  return ArrayToString(attributes, values.at(FieldStudio));
}

string ByEpisodeNumber(SortAttribute attributes, const SortItem &values)
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

  return StringUtils::Format("%" PRIu64" %s", num, title.c_str());
}

string BySeason(SortAttribute attributes, const SortItem &values)
{
  int season = (int)values.at(FieldSeason).asInteger();
  const CVariant &specialSeason = values.at(FieldSeasonSpecialSort);
  if (!specialSeason.isNull())
    season = (int)specialSeason.asInteger();

  return StringUtils::Format("%i %s", season, ByLabel(attributes, values).c_str());
}

string ByNumberOfEpisodes(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%i %s", (int)values.at(FieldNumberOfEpisodes).asInteger(), ByLabel(attributes, values).c_str());
}

string ByNumberOfWatchedEpisodes(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%i %s", (int)values.at(FieldNumberOfWatchedEpisodes).asInteger(), ByLabel(attributes, values).c_str());
}

string ByTvShowStatus(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldTvShowStatus).asString() + " " + ByLabel(attributes, values);
}

string ByTvShowTitle(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldTvShowTitle).asString() + " " + ByLabel(attributes, values);
}

string ByProductionCode(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldProductionCode).asString();
}

string ByVideoResolution(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%i %s", (int)values.at(FieldVideoResolution).asInteger(), ByLabel(attributes, values).c_str());
}

string ByVideoCodec(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%s %s", values.at(FieldVideoCodec).asString().c_str(), ByLabel(attributes, values).c_str());
}

string ByVideoAspectRatio(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%.03f %s", values.at(FieldVideoAspectRatio).asFloat(), ByLabel(attributes, values).c_str());
}

string ByAudioChannels(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%i %s", (int)values.at(FieldAudioChannels).asInteger(), ByLabel(attributes, values).c_str());
}

string ByAudioCodec(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%s %s", values.at(FieldAudioCodec).asString().c_str(), ByLabel(attributes, values).c_str());
}

string ByAudioLanguage(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%s %s", values.at(FieldAudioLanguage).asString().c_str(), ByLabel(attributes, values).c_str());;
}

string BySubtitleLanguage(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%s %s", values.at(FieldSubtitleLanguage).asString().c_str(), ByLabel(attributes, values).c_str());
}

string ByBitrate(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%" PRId64, values.at(FieldBitrate).asInteger());
}

string ByListeners(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%" PRId64, values.at(FieldListeners).asInteger());;
}

string ByRandom(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%i", CUtil::GetRandomNumber());;
}

string ByChannel(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldChannelName).asString();
}

string ByChannelNumber(SortAttribute attributes, const SortItem &values)
{
  return StringUtils::Format("%i", (int)values.at(FieldChannelNumber).asInteger());
}

string ByDateTaken(SortAttribute attributes, const SortItem &values)
{
  return values.at(FieldDateTaken).asString();
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
  else if (leftSortSpecial != SortSpecialNone && leftSortSpecial == rightSortSpecial)
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

map<SortBy, SortUtils::SortPreparator> fillPreparators()
{
  map<SortBy, SortUtils::SortPreparator> preparators;

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
  preparators[SortByAlbum]                    = ByAlbum;
  preparators[SortByAlbumType]                = ByAlbumType;
  preparators[SortByGenre]                    = ByGenre;
  preparators[SortByCountry]                  = ByCountry;
  preparators[SortByYear]                     = ByYear;
  preparators[SortByRating]                   = ByRating;
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
  preparators[SortByDateTaken]                = ByDateTaken;

  return preparators;
}

map<SortBy, Fields> fillSortingFields()
{
  map<SortBy, Fields> sortingFields;

  sortingFields.insert(pair<SortBy, Fields>(SortByNone, Fields()));

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
  sortingFields[SortByArtist].insert(FieldYear);
  sortingFields[SortByArtist].insert(FieldAlbum);
  sortingFields[SortByArtist].insert(FieldTrackNumber);
  sortingFields[SortByAlbum].insert(FieldAlbum);
  sortingFields[SortByAlbum].insert(FieldArtist);
  sortingFields[SortByAlbum].insert(FieldTrackNumber);
  sortingFields[SortByAlbumType].insert(FieldAlbumType);
  sortingFields[SortByGenre].insert(FieldGenre);
  sortingFields[SortByCountry].insert(FieldCountry);
  sortingFields[SortByYear].insert(FieldYear);
  sortingFields[SortByYear].insert(FieldAirDate);
  sortingFields[SortByYear].insert(FieldAlbum);
  sortingFields[SortByYear].insert(FieldTrackNumber);
  sortingFields[SortByRating].insert(FieldRating);
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
  sortingFields[SortByDateTaken].insert(FieldDateTaken);
  sortingFields.insert(pair<SortBy, Fields>(SortByRandom, Fields()));

  return sortingFields;
}

map<SortBy, SortUtils::SortPreparator> SortUtils::m_preparators = fillPreparators();
map<SortBy, Fields> SortUtils::m_sortingFields = fillSortingFields();

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
            item->insert(pair<Field, CVariant>(*field, CVariant::ConstNullVariant));
        }

        std::wstring sortLabel;
        g_charsetConverter.utf8ToW(preparator(attributes, *item), sortLabel, false);
        item->insert(pair<Field, CVariant>(FieldSort, CVariant(sortLabel)));
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
            (*item)->insert(pair<Field, CVariant>(*field, CVariant::ConstNullVariant));
        }

        std::wstring sortLabel;
        g_charsetConverter.utf8ToW(preparator(attributes, **item), sortLabel, false);
        (*item)->insert(pair<Field, CVariant>(FieldSort, CVariant(sortLabel)));
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
  map<SortBy, SortPreparator>::const_iterator it = m_preparators.find(sortBy);
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
  map<SortBy, Fields>::const_iterator it = m_sortingFields.find(sortBy);
  if (it != m_sortingFields.end())
    return it->second;

  return m_sortingFields[SortByNone];
}

string SortUtils::RemoveArticles(const string &label)
{
  std::set<std::string> sortTokens = g_langInfo.GetSortTokens();
  for (std::set<std::string>::const_iterator token = sortTokens.begin(); token != sortTokens.end(); ++token)
  {
    if (token->size() < label.size() && StringUtils::StartsWith(label, *token))
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
  { SortByArtist,                   SORT_METHOD_ARTIST_IGNORE_THE,            SortAttributeIgnoreArticle, 557 },
  { SortByAlbum,                    SORT_METHOD_ALBUM,                        SortAttributeNone,          558 },
  { SortByAlbum,                    SORT_METHOD_ALBUM_IGNORE_THE,             SortAttributeIgnoreArticle, 558 },
  { SortByGenre,                    SORT_METHOD_GENRE,                        SortAttributeNone,          515 },
  { SortByCountry,                  SORT_METHOD_COUNTRY,                      SortAttributeNone,          574 },
  { SortByDateAdded,                SORT_METHOD_DATEADDED,                    SortAttributeIgnoreFolders, 570 },
  { SortByFile,                     SORT_METHOD_FILE,                         SortAttributeIgnoreFolders, 561 },
  { SortByRating,                   SORT_METHOD_SONG_RATING,                  SortAttributeNone,          563 },
  { SortByRating,                   SORT_METHOD_VIDEO_RATING,                 SortAttributeIgnoreFolders, 563 },
  { SortBySortTitle,                SORT_METHOD_VIDEO_SORT_TITLE,             SortAttributeIgnoreFolders, 171 },
  { SortBySortTitle,                SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE,  (SortAttribute)(SortAttributeIgnoreFolders | SortAttributeIgnoreArticle), 171 },
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
  { SortByDateTaken,                SORT_METHOD_DATE_TAKEN,                   SortAttributeIgnoreFolders, 577 },
  { SortByNone,                     SORT_METHOD_NONE,                         SortAttributeNone,          16018 },
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

SORT_METHOD SortUtils::TranslateOldSortMethod(SortBy sortBy, bool ignoreArticle)
{
  for (size_t i = 0; i < sizeof(table) / sizeof(sort_map); i++)
  {
    if (table[i].sort == sortBy)
    {
      if (ignoreArticle == ((table[i].flags & SortAttributeIgnoreArticle) == SortAttributeIgnoreArticle))
        return table[i].old;
    }
  }
  for (size_t i = 0; i < sizeof(table) / sizeof(sort_map); i++)
  {
    if (table[i].sort == sortBy)
      return table[i].old;
  }
  return SORT_METHOD_NONE;
}

SortDescription SortUtils::TranslateOldSortMethod(SORT_METHOD sortBy)
{
  SortDescription description;
  for (size_t i = 0; i < sizeof(table) / sizeof(sort_map); i++)
  {
    if (table[i].old == sortBy)
    {
      description.sortBy = table[i].sort;
      description.sortAttributes = table[i].flags;
      break;
    }
  }
  return description;
}

int SortUtils::GetSortLabel(SortBy sortBy)
{
  for (size_t i = 0; i < sizeof(table) / sizeof(sort_map); i++)
  {
    if (table[i].sort == sortBy)
      return table[i].label;
  }
  return 16018; // None
}
