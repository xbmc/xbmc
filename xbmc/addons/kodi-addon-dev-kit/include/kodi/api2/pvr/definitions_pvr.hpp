#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "../definitions-all.hpp"

extern "C"
{
namespace V2
{
namespace KodiAPI
{

  //============================================================================
  /// \ingroup V2_KodiAPI_PVR
  /// @name EPG entry content event types
  /// @{
  /// These IDs come from the DVB-SI EIT table "content descriptor"
  /// Also known under the name "E-book genre assignments"
  ///

  /// <b>EPG event content:</b> Not defined and is unknown
  #define EPG_EVENT_CONTENTMASK_UNDEFINED                0x00
  /// <b>EPG event content:</b> Movie / Drama
  #define EPG_EVENT_CONTENTMASK_MOVIEDRAMA               0x10
  /// <b>EPG event content:</b> News / Current affairs
  #define EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS       0x20
  /// <b>EPG event content:</b> Show / Game show
  #define EPG_EVENT_CONTENTMASK_SHOW                     0x30
  /// <b>EPG event content:</b> Sports
  #define EPG_EVENT_CONTENTMASK_SPORTS                   0x40
  /// <b>EPG event content:</b> Children's / Youth programmes
  #define EPG_EVENT_CONTENTMASK_CHILDRENYOUTH            0x50
  /// <b>EPG event content:</b> Music / Ballet / Dance
  #define EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE         0x60
  /// <b>EPG event content:</b> Arts / Culture
  #define EPG_EVENT_CONTENTMASK_ARTSCULTURE              0x70
  /// <b>EPG event content:</b> Social / Political / Economics
  #define EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS 0x80
  /// <b>EPG event content:</b> Education / Science / Factua
  #define EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE       0x90
  /// <b>EPG event content:</b> Leisure / Hobbies
  #define EPG_EVENT_CONTENTMASK_LEISUREHOBBIES           0xA0
  /// <b>EPG event content:</b> Specials
  #define EPG_EVENT_CONTENTMASK_SPECIAL                  0xB0
  /// <b>EPG event content:</b> User defined
  #define EPG_EVENT_CONTENTMASK_USERDEFINED              0xF0
  /// @}
  //----------------------------------------------------------------------------


  //============================================================================
  /// \ingroup V2_KodiAPI_PVR
  /// @name Set EPGTAG.iGenreType to EPG_GENRE_USE_STRING to transfer genre
  /// strings to Kodi
  /// @{

  /// Set <c>EPGTAG.iGenreType</c> to <c>EPG_GENRE_USE_STRING</c> to transfer
  /// genre strings to Kodi
  #define EPG_GENRE_USE_STRING                          0x100
  /// @}
  //----------------------------------------------------------------------------


  //============================================================================
  /// \ingroup V2_KodiAPI_PVR
  /// @name EPG_TAG.iFlags values
  /// @{

  /// nothing special to say about this entry
  const unsigned int EPG_TAG_FLAG_UNDEFINED =           0x00000000;

  /// this EPG entry is part of a series
  const unsigned int EPG_TAG_FLAG_IS_SERIES =           0x00000001;
  /// @}
  //----------------------------------------------------------------------------


  //============================================================================
  /// \ingroup V2_KodiAPI_PVR
  /// @brief Representation of an EPG event.
  ///
  typedef struct EPG_TAG {
    /// (required) identifier for this event
    unsigned int  iUniqueBroadcastId;

    /// (required) this event's title
    const char *  strTitle;

    /// (required) the number of the channel this event occurs on
    unsigned int  iChannelNumber;

    /// (required) start time in UTC
    time_t        startTime;

    /// (required) end time in UTC
    time_t        endTime;

    /// (optional) plot outline
    const char *  strPlotOutline;

    /// (optional) plot
    const char *  strPlot;

    /// (optional) originaltitle
    const char *  strOriginalTitle;

    /// (optional) cast
    const char *  strCast;

    /// (optional) director
    const char *  strDirector;

    /// (optional) writer
    const char *  strWriter;

    /// (optional) year
    int           iYear;

    /// (optional) IMDBNumber
    const char *  strIMDBNumber;

    /// (optional) icon path
    const char *  strIconPath;

    /// (optional) genre type
    int           iGenreType;

    /// (optional) genre sub type
    int           iGenreSubType;

    /// (optional) genre. Will be used only when <c>iGenreType = EPG_GENRE_USE_STRING</c>
    const char *  strGenreDescription;

    /// (optional) first aired in UTC
    time_t        firstAired;

    /// (optional) parental rating
    int           iParentalRating;

    /// (optional) star rating
    int           iStarRating;

    /// (optional) notify the user when this event starts
    bool          bNotify;

    /// (optional) series number
    int           iSeriesNumber;

    /// (optional) episode number
    int           iEpisodeNumber;

    /// (optional) episode part number
    int           iEpisodePartNumber;

    /// (optional) episode name
    const char *  strEpisodeName;

    /// (optional) bit field of independent flags associated with the EPG entry
    unsigned int  iFlags;
  } ATTRIBUTE_PACKED EPG_TAG;
  /// @}
  //----------------------------------------------------------------------------

}; /* namespace KodiAPI */
}; /* namespace V2 */
}; /* extern "C" */
