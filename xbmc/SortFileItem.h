#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "utils/LabelFormatter.h"
#include <boost/shared_ptr.hpp>

class CFileItem; typedef boost::shared_ptr<CFileItem> CFileItemPtr;

struct SSortFileItem
{
  /*! \brief Remove any articles (eg "the", "a") from the start of a label
   \param label the label to process
   \return the label stripped of any articles
   */
  static CStdString RemoveArticles(const CStdString &label);

  // Sort by sort field
  static bool Ascending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool Descending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool IgnoreFoldersAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool IgnoreFoldersDescending(const CFileItemPtr &left, const CFileItemPtr &right);

  // Fill in sort field
  static void ByLabel(CFileItemPtr &item);
  static void ByLabelNoThe(CFileItemPtr &item);
  static void ByFile(CFileItemPtr &item);
  static void ByFullPath(CFileItemPtr &item);
  static void ByDate(CFileItemPtr &item);
  static void ByDateAdded(CFileItemPtr &item);
  static void BySize(CFileItemPtr &item);
  static void ByDriveType(CFileItemPtr &item);
  static void BySongTitle(CFileItemPtr &item);
  static void BySongTitleNoThe(CFileItemPtr &item);
  static void BySongAlbum(CFileItemPtr &item);
  static void BySongAlbumNoThe(CFileItemPtr &item);
  static void BySongArtist(CFileItemPtr &item);
  static void BySongArtistNoThe(CFileItemPtr &item);
  static void BySongTrackNum(CFileItemPtr &item);
  static void BySongDuration(CFileItemPtr &item);
  static void BySongRating(CFileItemPtr &item);

  static void ByProgramCount(CFileItemPtr &item);

  static void ByGenre(CFileItemPtr &item);
  static void ByCountry(CFileItemPtr &item);
  static void ByYear(CFileItemPtr &item);

  static void ByMovieTitle(CFileItemPtr &item);
  static void ByMovieSortTitle(CFileItemPtr &item);
  static void ByMovieSortTitleNoThe(CFileItemPtr &item);
  static void ByMovieRating(CFileItemPtr &item);
  static void ByMovieRuntime(CFileItemPtr &item);
  static void ByMPAARating(CFileItemPtr &item);
  static void ByStudio(CFileItemPtr &item);
  static void ByStudioNoThe(CFileItemPtr &item);

  static void ByEpisodeNum(CFileItemPtr &item);
  static void ByProductionCode(CFileItemPtr &item);
  static void ByLastPlayed(CFileItemPtr &item);
  static void ByPlayCount(CFileItemPtr &item);
  static void ByBitrate(CFileItemPtr &item);
  static void ByListeners(CFileItemPtr &item);
};

typedef enum {
  SORT_METHOD_NONE=0,
  SORT_METHOD_LABEL,
  SORT_METHOD_LABEL_IGNORE_THE,
  SORT_METHOD_DATE,
  SORT_METHOD_SIZE,
  SORT_METHOD_FILE,
  SORT_METHOD_DRIVE_TYPE,
  SORT_METHOD_TRACKNUM,
  SORT_METHOD_DURATION,
  SORT_METHOD_TITLE,
  SORT_METHOD_TITLE_IGNORE_THE,
  SORT_METHOD_ARTIST,
  SORT_METHOD_ARTIST_IGNORE_THE,
  SORT_METHOD_ALBUM,
  SORT_METHOD_ALBUM_IGNORE_THE,
  SORT_METHOD_GENRE,
  SORT_METHOD_COUNTRY,
  SORT_METHOD_YEAR,
  SORT_METHOD_VIDEO_RATING,
  SORT_METHOD_DATEADDED,
  SORT_METHOD_PROGRAM_COUNT,
  SORT_METHOD_PLAYLIST_ORDER,
  SORT_METHOD_EPISODE,
  SORT_METHOD_VIDEO_TITLE,
  SORT_METHOD_VIDEO_SORT_TITLE,
  SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE,
  SORT_METHOD_PRODUCTIONCODE,
  SORT_METHOD_SONG_RATING,
  SORT_METHOD_MPAA_RATING,
  SORT_METHOD_VIDEO_RUNTIME,
  SORT_METHOD_STUDIO,
  SORT_METHOD_STUDIO_IGNORE_THE,
  SORT_METHOD_FULLPATH,
  SORT_METHOD_LABEL_IGNORE_FOLDERS,
  SORT_METHOD_LASTPLAYED,
  SORT_METHOD_PLAYCOUNT,
  SORT_METHOD_LISTENERS,
  SORT_METHOD_UNSORTED,
  SORT_METHOD_BITRATE,
  SORT_METHOD_MAX
} SORT_METHOD;

typedef enum {
  SORT_ORDER_NONE=0,
  SORT_ORDER_ASC,
  SORT_ORDER_DESC
} SORT_ORDER;

typedef enum {
  SORT_NORMALLY=0,
  SORT_ON_TOP,
  SORT_ON_BOTTOM
} SPECIAL_SORT;

typedef struct
{
  SORT_METHOD m_sortMethod;
  int m_buttonLabel;
  LABEL_MASKS m_labelMasks;
} SORT_METHOD_DETAILS;

