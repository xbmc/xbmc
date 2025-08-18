/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DatabaseUtils.h"
#include "LabelFormatter.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

enum class SortMethod;

typedef enum {
  SortOrderNone = 0,
  SortOrderAscending,
  SortOrderDescending
} SortOrder;

typedef enum
{
  SortAttributeNone = 0x0,
  SortAttributeIgnoreArticle = 0x1,
  SortAttributeIgnoreFolders = 0x2,
  SortAttributeUseArtistSortName = 0x4,
  SortAttributeIgnoreLabel = 0x8,
  SortAttributeForceConsiderFolders = 0x10, // overrides SortAttributeIgnoreFolders
} SortAttribute;

typedef enum {
  SortSpecialNone     = 0,
  SortSpecialOnTop    = 1,
  SortSpecialOnBottom = 2
} SortSpecial;

///
/// \defgroup List_of_sort_methods List of sort methods
/// \addtogroup List_of_sort_methods
///
/// \brief These ID's can be used with the \ref built_in_functions_6 "Container.SetSortMethod(id)" function
/// \note The on field named part with <b>String</b> shows the string used on
/// GUI to set this sort type.
///
///@{
typedef enum
{
  SortByStartMarker = -1,
  /// __0__  : Unsorted
  SortByNone,
  /// __1__  : Sort by Name                       <em>(String: <b><c>Label</c></b>)</em>
  SortByLabel,
  /// __2__  : Sort by Date                       <em>(String: <b><c>Date</c></b>)</em>
  SortByDate,
  /// __3__  : Sort by Size                       <em>(String: <b><c>Size</c></b>)</em>
  SortBySize,
  /// __4__  : Sort by filename                   <em>(String: <b><c>File</c></b>)</em>
  SortByFile,
  /// __5__  : Sort by path                       <em>(String: <b><c>Path</c></b>)</em>
  SortByPath,
  /// __6__  : Sort by drive type                 <em>(String: <b><c>DriveType</c></b>)</em>
  SortByDriveType,
  /// __7__  : Sort by title                      <em>(String: <b><c>Title</c></b>)</em>
  SortByTitle,
  /// __8__  : Sort by track number               <em>(String: <b><c>TrackNumber</c></b>)</em>
  SortByTrackNumber,
  /// __9__  : Sort by time                       <em>(String: <b><c>Time</c></b>)</em>
  SortByTime,
  /// __10__ : Sort by artist                     <em>(String: <b><c>Artist</c></b>)</em>
  SortByArtist,
  /// __11__ : Sort by first artist then year     <em>(String: <b><c>ArtistYear</c></b>)</em>
  SortByArtistThenYear,
  /// __12__ : Sort by album                      <em>(String: <b><c>Album</c></b>)</em>
  SortByAlbum,
  /// __13__ : Sort by album type                 <em>(String: <b><c>AlbumType</c></b>)</em>
  SortByAlbumType,
  /// __14__ : Sort by genre                      <em>(String: <b><c>Genre</c></b>)</em>
  SortByGenre,
  /// __15__ : Sort by country                     <em>(String: <b><c>Country</c></b>)</em>
  SortByCountry,
  /// __16__ : Sort by year                       <em>(String: <b><c>Year</c></b>)</em>
  SortByYear,
  /// __17__ : Sort by rating                     <em>(String: <b><c>Rating</c></b>)</em>
  SortByRating,
  /// __18__ : Sort by user rating                <em>(String: <b><c>UserRating</c></b>)</em>
  SortByUserRating,
  /// __19__ : Sort by votes                      <em>(String: <b><c>Votes</c></b>)</em>
  SortByVotes,
  /// __20__ : Sort by top 250                    <em>(String: <b><c>Top250</c></b>)</em>
  SortByTop250,
  /// __21__ : Sort by program count              <em>(String: <b><c>ProgramCount</c></b>)</em>
  SortByProgramCount,
  /// __22__ : Sort by playlist order             <em>(String: <b><c>Playlist</c></b>)</em>
  SortByPlaylistOrder,
  /// __23__ : Sort by episode number             <em>(String: <b><c>Episode</c></b>)</em>
  SortByEpisodeNumber,
  /// __24__ : Sort by season                     <em>(String: <b><c>Season</c></b>)</em>
  SortBySeason,
  /// __25__ : Sort by number of episodes         <em>(String: <b><c>TotalEpisodes</c></b>)</em>
  SortByNumberOfEpisodes,
  /// __26__ : Sort by number of watched episodes <em>(String: <b><c>WatchedEpisodes</c></b>)</em>
  SortByNumberOfWatchedEpisodes,
  /// __27__ : Sort by TV show status             <em>(String: <b><c>TvShowStatus</c></b>)</em>
  SortByTvShowStatus,
  /// __28__ : Sort by TV show title              <em>(String: <b><c>TvShowTitle</c></b>)</em>
  SortByTvShowTitle,
  /// __29__ : Sort by sort title                 <em>(String: <b><c>SortTitle</c></b>)</em>
  SortBySortTitle,
  /// __30__ : Sort by production code            <em>(String: <b><c>ProductionCode</c></b>)</em>
  SortByProductionCode,
  /// __31__ : Sort by MPAA                       <em>(String: <b><c>MPAA</c></b>)</em>
  SortByMPAA,
  /// __32__ : Sort by video resolution           <em>(String: <b><c>VideoResolution</c></b>)</em>
  SortByVideoResolution,
  /// __33__ : Sort by video codec                <em>(String: <b><c>VideoCodec</c></b>)</em>
  SortByVideoCodec,
  /// __34__ : Sort by video aspect ratio         <em>(String: <b><c>VideoAspectRatio</c></b>)</em>
  SortByVideoAspectRatio,
  /// __35__ : Sort by audio channels             <em>(String: <b><c>AudioChannels</c></b>)</em>
  SortByAudioChannels,
  /// __36__ : Sort by audio codec                <em>(String: <b><c>AudioCodec</c></b>)</em>
  SortByAudioCodec,
  /// __37__ : Sort by audio language             <em>(String: <b><c>AudioLanguage</c></b>)</em>
  SortByAudioLanguage,
  /// __38__ : Sort by subtitle language          <em>(String: <b><c>SubtitleLanguage</c></b>)</em>
  SortBySubtitleLanguage,
  /// __39__ : Sort by studio                     <em>(String: <b><c>Studio</c></b>)</em>
  SortByStudio,
  /// __40__ : Sort by date added                 <em>(String: <b><c>DateAdded</c></b>)</em>
  SortByDateAdded,
  /// __41__ : Sort by last played                <em>(String: <b><c>LastPlayed</c></b>)</em>
  SortByLastPlayed,
  /// __42__ : Sort by playcount                  <em>(String: <b><c>PlayCount</c></b>)</em>
  SortByPlaycount,
  /// __43__ : Sort by listener                   <em>(String: <b><c>Listeners</c></b>)</em>
  SortByListeners,
  /// __44__ : Sort by bitrate                    <em>(String: <b><c>Bitrate</c></b>)</em>
  SortByBitrate,
  /// __45__ : Sort by random                     <em>(String: <b><c>Random</c></b>)</em>
  SortByRandom,
  /// __46__ : Sort by channel                    <em>(String: <b><c>Channel</c></b>)</em>
  SortByChannel,
  /// __47__ : Sort by channel number             <em>(String: <b><c>ChannelNumber</c></b>)</em>
  SortByChannelNumber,
  /// __48__ : Sort by date taken                 <em>(String: <b><c>DateTaken</c></b>)</em>
  SortByDateTaken,
  /// __49__ : Sort by relevance
  SortByRelevance,
  /// __50__ : Sort by installation date          <en>(String: <b><c>installdate</c></b>)</em>
  SortByInstallDate,
  /// __51__ : Sort by last updated               <en>(String: <b><c>lastupdated</c></b>)</em>
  SortByLastUpdated,
  /// __52__ : Sort by last used                  <em>(String: <b><c>lastused</c></b>)</em>
  SortByLastUsed,
  /// __53__ : Sort by client channel order       <em>(String: <b><c>ClientChannelOrder</c></b>)</em>
  SortByClientChannelOrder,
  /// __54__ : Sort by total number of discs      <em>(String: <b><c>totaldiscs</c></b>)</em>
  SortByTotalDiscs,
  /// __55__ : Sort by original release date      <em>(String: <b><c>Originaldate</c></b>)</em>
  SortByOrigDate,
  /// __56__ : Sort by BPM                        <em>(String: <b><c>bpm</c></b>)</em>
  SortByBPM,
  /// __57__ : Sort by original title             <em>(String: <b><c>OriginalTitle</c></b>)</em>
  SortByOriginalTitle,
  /// __58__ : Sort by provider                   <em>(String: <b><c>Provider</c></b>)</em>
  /// @skinning_v20 <b>SortByProvider</b> New sort method added.
  SortByProvider,
  /// __59__ : Sort by user preference            <em>(String: <b><c>UserPreference</c></b>)</em>
  /// @skinning_v20 <b>SortByUserPreference</b> New sort method added.
  SortByUserPreference,
  // SortByEndMarker must remain as the last item in the enum.
  // All new sort methods to be added prior to this end marker.
  SortByEndMarker,
} SortBy;
///@}

typedef struct SortDescription {
  SortBy sortBy = SortByNone;
  SortOrder sortOrder = SortOrderAscending;
  SortAttribute sortAttributes = SortAttributeNone;
  int limitStart = 0;
  int limitEnd = -1;
} SortDescription;

typedef struct GUIViewSortDetails
{
  SortDescription m_sortDescription;
  int m_buttonLabel;
  LABEL_MASKS m_labelMasks;
} GUIViewSortDetails;

typedef DatabaseResult SortItem;
typedef std::shared_ptr<SortItem> SortItemPtr;
typedef std::vector<SortItemPtr> SortItems;

class SortUtils
{
public:
  static SortMethod TranslateOldSortMethod(SortBy sortBy, bool ignoreArticle);
  static SortDescription TranslateOldSortMethod(SortMethod sortBy);

  static SortBy SortMethodFromString(const std::string& sortMethod);
  static const std::string& SortMethodToString(SortBy sortMethod);
  static SortOrder SortOrderFromString(const std::string& sortOrder);
  static const std::string& SortOrderToString(SortOrder sortOrder);

  /*! \brief retrieve the label id associated with a sort method for displaying in the UI.
   \param sortBy the sort method in question.
   \return the label id of the sort method.
   */
  static int GetSortLabel(SortBy sortBy);

  static void Sort(SortBy sortBy, SortOrder sortOrder, SortAttribute attributes, DatabaseResults& items, int limitEnd = -1, int limitStart = 0);
  static void Sort(SortBy sortBy, SortOrder sortOrder, SortAttribute attributes, SortItems& items, int limitEnd = -1, int limitStart = 0);
  static void Sort(const SortDescription &sortDescription, DatabaseResults& items);
  static void Sort(const SortDescription &sortDescription, SortItems& items);
  static bool SortFromDataset(const SortDescription& sortDescription,
                              const MediaType& mediaType,
                              dbiplus::Dataset& dataset,
                              DatabaseResults& results);

  static void GetFieldsForSQLSort(const MediaType& mediaType, SortBy sortMethod, FieldList& fields);
  static const Fields& GetFieldsForSorting(SortBy sortBy);
  static std::string RemoveArticles(const std::string &label);

  typedef std::string (*SortPreparator) (SortAttribute, const SortItem&);
  typedef bool (*Sorter) (const DatabaseResult &, const DatabaseResult &);
  typedef bool (*SorterIndirect) (const SortItemPtr &, const SortItemPtr &);

private:
  static const SortPreparator& getPreparator(SortBy sortBy);
  static Sorter getSorter(SortOrder sortOrder, SortAttribute attributes);
  static SorterIndirect getSorterIndirect(SortOrder sortOrder, SortAttribute attributes);

  static std::map<SortBy, SortPreparator> m_preparators;
  static std::map<SortBy, Fields> m_sortingFields;
};
