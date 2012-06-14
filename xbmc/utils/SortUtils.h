#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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
#include <string>

#include "DatabaseUtils.h"

typedef enum {
  SortOrderNone = 0,
  SortOrderAscending,
  SortOrderDescending
} SortOrder;

typedef enum {
  SortAttributeNone           = 0x0,
  SortAttributeIgnoreArticle  = 0x1,
  SortAttributeIgnoreFolders  = 0x2
} SortAttribute;

typedef enum {
  SortSpecialNone     = 0,
  SortSpecialOnTop    = 1,
  SortSpecialOnBottom = 2
} SortSpecial;

typedef enum {
  SortByNone = 0,
  SortByLabel,
  SortByDate,
  SortBySize,
  SortByFile,
  SortByPath,
  SortByDriveType,
  SortByTitle,
  SortByTrackNumber,
  SortByTime,
  SortByArtist,
  SortByAlbum,
  SortByAlbumType,
  SortByGenre,
  SortByCountry,
  SortByYear,
  SortByRating,
  SortByVotes,
  SortByTop250,
  SortByProgramCount,
  SortByPlaylistOrder,
  SortByEpisodeNumber,
  SortBySeason,
  SortByNumberOfEpisodes,
  SortByNumberOfWatchedEpisodes,
  SortByTvShowStatus,
  SortByTvShowTitle,
  SortBySortTitle,
  SortByProductionCode,
  SortByMPAA,
  SortByVideoResolution,
  SortByVideoCodec,
  SortByVideoAspectRatio,
  SortByAudioChannels,
  SortByAudioCodec,
  SortByAudioLanguage,
  SortBySubtitleLanguage,
  SortByStudio,
  SortByDateAdded,
  SortByLastPlayed,
  SortByPlaycount,
  SortByListeners,
  SortByBitrate,
  SortByRandom,
  SortByChannel
} SortBy;

typedef struct SortDescription {
  SortBy sortBy;
  SortOrder sortOrder;
  SortAttribute sortAttributes;
  int limitStart;
  int limitEnd;

  SortDescription()
    : sortBy(SortByNone), sortOrder(SortOrderAscending), sortAttributes(SortAttributeNone),
      limitStart(0), limitEnd(-1)
  { }
} SortDescription;

typedef DatabaseResult SortItem;
typedef DatabaseResults SortItems;

class SortUtils
{
public:
  static void Sort(SortBy sortBy, SortOrder sortOrder, SortAttribute attributes, SortItems& items, int limitEnd = -1, int limitStart = 0);
  static void Sort(const SortDescription &sortDescription, SortItems& items);
  static bool SortFromDataset(const SortDescription &sortDescription, MediaType mediaType, const std::auto_ptr<dbiplus::Dataset> &dataset, DatabaseResults &results);
  
  static const Fields& GetFieldsForSorting(SortBy sortBy);
  static std::string RemoveArticles(const std::string &label);
  
  typedef std::string (*SortPreparator) (SortAttribute, const SortItem&);
  typedef bool (*Sorter) (const SortItem&, const SortItem&);
  
private:
  static const SortPreparator& getPreparator(SortBy sortBy);
  static Sorter getSorter(SortOrder sortOrder, SortAttribute attributes);

  static std::map<SortBy, SortPreparator> m_preparators;
  static std::map<SortBy, Fields> m_sortingFields;
};
