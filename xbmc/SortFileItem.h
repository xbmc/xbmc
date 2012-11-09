#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "utils/LabelFormatter.h"

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
  SORT_METHOD_CHANNEL,
  SORT_METHOD_BITRATE,
  SORT_METHOD_MAX
} SORT_METHOD;

typedef struct
{
  SORT_METHOD m_sortMethod;
  int m_buttonLabel;
  LABEL_MASKS m_labelMasks;
} SORT_METHOD_DETAILS;

