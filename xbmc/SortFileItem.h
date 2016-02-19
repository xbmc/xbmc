#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

///
/// \defgroup python_xbmcplugin_sort_method List Item sort methods.
/// \ingroup python_xbmcplugin
/// @brief List of sort methods
///
/// Here are the Python integer constant names listed and the related integer
/// value which are used on xbmcplugin.addSortMethod(...).
///
///@{
typedef enum {
  SORT_METHOD_NONE=0,                             //!< Integer: __0__ - Do not sort
  SORT_METHOD_LABEL,                              //!< Integer: __1__ - Sort by label
  SORT_METHOD_LABEL_IGNORE_THE,                   //!< Integer: __2__ - Sort without the label
  SORT_METHOD_DATE,                               //!< Integer: __3__ - Sort by date
  SORT_METHOD_SIZE,                               //!< Integer: __4__ - Sort by File Size
  SORT_METHOD_FILE,                               //!< Integer: __5__ - Sort by file name
  SORT_METHOD_DRIVE_TYPE,                         //!< Integer: __6__ - Sort by drive type
  SORT_METHOD_TRACKNUM,                           //!< Integer: __7__ - Sort by track number
  SORT_METHOD_DURATION,                           //!< Integer: __8__ - Sort by duration
  SORT_METHOD_TITLE,                              //!< Integer: __9__ - Sort by title
  SORT_METHOD_TITLE_IGNORE_THE,                   //!< Integer: __10__ - Sort without the title
  SORT_METHOD_ARTIST,                             //!< Integer: __11__ - Sort by Artist
  SORT_METHOD_ARTIST_AND_YEAR,                    //!< Integer: __12__ - Sort by Artist and the year of them
  SORT_METHOD_ARTIST_IGNORE_THE,                  //!< Integer: __13__ - Sort without Artist
  SORT_METHOD_ALBUM,                              //!< Integer: __14__ - Sort by Album
  SORT_METHOD_ALBUM_IGNORE_THE,                   //!< Integer: __15__ - Sort without album
  SORT_METHOD_GENRE,                              //!< Integer: __16__ - Sort by Genre
  SORT_METHOD_COUNTRY,                            //!< Integer: __17__ - Sort by the Country
  SORT_METHOD_YEAR,                               //!< Integer: __18__ - Sort by the year
  SORT_METHOD_VIDEO_RATING,                       //!< Integer: __19__ - Sort by video rating
  SORT_METHOD_VIDEO_USER_RATING,                  //!< Integer: __20__ - Sort by video user rating
  SORT_METHOD_DATEADDED,                          //!< Integer: __21__ - Sort by added date
  SORT_METHOD_PROGRAM_COUNT,                      //!< Integer: __22__ - Sort by program count
  SORT_METHOD_PLAYLIST_ORDER,                     //!< Integer: __23__ - Sort by playlist order
  SORT_METHOD_EPISODE,                            //!< Integer: __24__ - Sort by episode number
  SORT_METHOD_VIDEO_TITLE,                        //!< Integer: __25__ - Sort by video title
  SORT_METHOD_VIDEO_SORT_TITLE,                   //!< Integer: __26__ - Sort by video sort title
  SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE,        //!< Integer: __27__ - Sort without video sort title
  SORT_METHOD_PRODUCTIONCODE,                     //!< Integer: __28__ - Sort by song rating
  SORT_METHOD_SONG_RATING,                        //!< Integer: __29__ - Sort by song user rating
  SORT_METHOD_SONG_USER_RATING,                   //!< Integer: __30__ - Sort by user rating
  SORT_METHOD_MPAA_RATING,                        //!< Integer: __31__ - Sort by MPAA rating
  SORT_METHOD_VIDEO_RUNTIME,                      //!< Integer: __32__ - Sort by video runtime
  SORT_METHOD_STUDIO,                             //!< Integer: __33__ - Sort by studio
  SORT_METHOD_STUDIO_IGNORE_THE,                  //!< Integer: __34__ - Sort without the studio
  SORT_METHOD_FULLPATH,                           //!< Integer: __35__ - Sort by the full path
  SORT_METHOD_LABEL_IGNORE_FOLDERS,               //!< Integer: __36__ - Sort by labels and ingore folders
  SORT_METHOD_LASTPLAYED,                         //!< Integer: __37__ - Sort by last played time
  SORT_METHOD_PLAYCOUNT,                          //!< Integer: __38__ - Sort by play count
  SORT_METHOD_LISTENERS,                          //!< Integer: __39__ - Sort by listener
  SORT_METHOD_UNSORTED,                           //!< Integer: __40__ - Do not sort
  SORT_METHOD_CHANNEL,                            //!< Integer: __41__ - Sort by channel
  SORT_METHOD_CHANNEL_NUMBER,                     //!< Integer: __42__ - Sort by channel number
  SORT_METHOD_BITRATE,                            //!< Integer: __43__ - Sort by bitrate
  SORT_METHOD_DATE_TAKEN,                         //!< Integer: __44__ - Sort by taken date
  SORT_METHOD_MAX                                 //!< Integer: __45__ - Not used, only maximum value
} SORT_METHOD;
///@}
