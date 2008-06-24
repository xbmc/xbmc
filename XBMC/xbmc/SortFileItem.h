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
  // Sort by file - this sorts by entire filepath, followed by the startoffset (for .cue items)
  static bool FileAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool FileDescending(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by date
  static bool DateAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool DateDescending(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by filesize
  static bool SizeAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool SizeDescending(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by Drive type, and then by label
  static bool DriveTypeAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool DriveTypeDescending(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by Label - the NoThe methods remove the "The " in front of items
  static bool LabelAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool LabelDescending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool LabelAscendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool LabelDescendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by Title
  static bool SongTitleAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool SongTitleDescending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool SongTitleAscendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool SongTitleDescendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by Movie Title
  static bool MovieTitleAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool MovieTitleDescending(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by album (then artist, then tracknumber)
  static bool SongAlbumAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool SongAlbumDescending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool SongAlbumAscendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool SongAlbumDescendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by artist (then album, then tracknumber)
  static bool SongArtistAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool SongArtistDescending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool SongArtistAscendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool SongArtistDescendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by genre
  static bool GenreAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool GenreDescending(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by track number
  static bool SongTrackNumAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool SongTrackNumDescending(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by episode number
  static bool EpisodeNumAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool EpisodeNumDescending(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by song duration
  static bool SongDurationAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool SongDurationDescending(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by program count
  static bool ProgramCountAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool ProgramCountDescending(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by Movie Year, and if equal, sort by label
  static bool MovieYearAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool MovieYearDescending(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by Movie Rating, and if equal, sort by label
  static bool MovieRatingAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool MovieRatingDescending(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by MPAA Rating, and if equal, sort by label
  static bool MPAARatingAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool MPAARatingDescending(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by Production Code
  static bool ProductionCodeAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool ProductionCodeDescending(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by Song Rating (0 -> 5)
  static bool SongRatingAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool SongRatingDescending(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by Movie Runtime, and if equal, sort by label
  static bool MovieRuntimeAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool MovieRuntimeDescending(const CFileItemPtr &left, const CFileItemPtr &right);

  // Sort by Studio, and if equal, sort by label
  static bool StudioAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool StudioDescending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool StudioAscendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool StudioDescendingNoThe(const CFileItemPtr &left, const CFileItemPtr &right);

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
  SORT_METHOD_VIDEO_YEAR,
  SORT_METHOD_VIDEO_RATING,
  SORT_METHOD_PROGRAM_COUNT,
  SORT_METHOD_PLAYLIST_ORDER,
  SORT_METHOD_EPISODE,
  SORT_METHOD_VIDEO_TITLE,
  SORT_METHOD_PRODUCTIONCODE,
  SORT_METHOD_SONG_RATING,
  SORT_METHOD_MPAA_RATING,
  SORT_METHOD_VIDEO_RUNTIME,
  SORT_METHOD_STUDIO,
  SORT_METHOD_STUDIO_IGNORE_THE,
  SORT_METHOD_UNSORTED,
  SORT_METHOD_MAX
} SORT_METHOD;

typedef enum {
  SORT_ORDER_NONE=0,
  SORT_ORDER_ASC,
  SORT_ORDER_DESC
} SORT_ORDER;

typedef struct
{
  SORT_METHOD m_sortMethod;
  int m_buttonLabel;
  LABEL_MASKS m_labelMasks;
} SORT_METHOD_DETAILS;

