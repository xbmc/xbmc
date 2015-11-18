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

#include <string.h>
#include <time.h>

#undef ATTRIBUTE_PACKED
#undef PRAGMA_PACK_BEGIN
#undef PRAGMA_PACK_END

#if defined(__GNUC__)
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define ATTRIBUTE_PACKED __attribute__ ((packed))
#define PRAGMA_PACK 0
#endif
#endif

#if !defined(ATTRIBUTE_PACKED)
#define ATTRIBUTE_PACKED
#define PRAGMA_PACK 1
#endif

/*! @name EPG entry content event types */
//@{
/* These IDs come from the DVB-SI EIT table "content descriptor"
 * Also known under the name "E-book genre assignments"
 */
#define EPG_EVENT_CONTENTMASK_UNDEFINED                0x00
#define EPG_EVENT_CONTENTMASK_MOVIEDRAMA               0x10
#define EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS       0x20
#define EPG_EVENT_CONTENTMASK_SHOW                     0x30
#define EPG_EVENT_CONTENTMASK_SPORTS                   0x40
#define EPG_EVENT_CONTENTMASK_CHILDRENYOUTH            0x50
#define EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE         0x60
#define EPG_EVENT_CONTENTMASK_ARTSCULTURE              0x70
#define EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS 0x80
#define EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE       0x90
#define EPG_EVENT_CONTENTMASK_LEISUREHOBBIES           0xA0
#define EPG_EVENT_CONTENTMASK_SPECIAL                  0xB0
#define EPG_EVENT_CONTENTMASK_USERDEFINED              0xF0
//@}

/* Set EPGTAG.iGenreType to EPG_GENRE_USE_STRING to transfer genre strings to XBMC */
#define EPG_GENRE_USE_STRING                          0x100

/* EPG_TAG.iFlags values */
const unsigned int EPG_TAG_FLAG_UNDEFINED =           0x00000000; /*!< @brief nothing special to say about this entry */
const unsigned int EPG_TAG_FLAG_IS_SERIES =           0x00000001; /*!< @brief this EPG entry is part of a series */
const unsigned int EPG_TAG_FLAG_IS_LIVE   =           0x00000002; /*!< @brief this EPG entry is broadcast live */
const unsigned int EPG_TAG_FLAG_IS_FINAL  =           0x00000004; /*!< @brief this EPG entry is a competition final */
const unsigned int EPG_TAG_FLAG_IS_MOVIE  =           0x00000008; /*!< @brief this EPG entry is a movie (aka made for cinema / film) */
const unsigned int EPG_TAG_FLAG_IS_NEW    =           0x00000010; /*!< @brief this EPG entry is the first time this movie/episode/event has been shown on TV in this region (aka 'new', not a repeat, TV movie premiere) */
const unsigned int EPG_TAG_FLAG_HAS_SUBTITLES  =      0x00000020; /*!< @brief this EPG entry has subtitles */
const unsigned int EPG_TAG_FLAG_HAS_SIGNING    =      0x00000040; /*!< @brief this EPG entry has a sign language inset (for the deaf) */
const unsigned int EPG_TAG_FLAG_HAS_AUDIO_DESC =      0x00000080; /*!< @brief this EPG entry has an audio description track (for the partially sighted / blind) */
const unsigned int EPG_TAG_FLAG_IS_WIDESCREEN  =      0x00000100; /*!< @brief this EPG entry is shown in Widescreen (e.g. 16:9) format */
const unsigned int EPG_TAG_FLAG_IS_HD          =      0x00000200; /*!< @brief this EPG entry is shown in HD */
const unsigned int EPG_TAG_FLAG_IS_3D          =      0x00000400; /*!< @brief this EPG entry is shown in 3D */
const unsigned int EPG_TAG_FLAG_IS_4K          =      0x00000800; /*!< @brief this EPG entry is shown in 4k high resolution format */
const unsigned int EPG_TAG_FLAG_HAS_MONO_SOUND     =  0x00001000; /*!< @brief this EPG entry has a mono sound audio track */
const unsigned int EPG_TAG_FLAG_HAS_STEREO_SOUND   =  0x00002000; /*!< @brief this EPG entry has a stereo sound audio track */
const unsigned int EPG_TAG_FLAG_HAS_SURROUND_SOUND =  0x00004000; /*!< @brief this EPG entry has a surround sound audio track */
const unsigned int EPG_TAG_FLAG_IS_SPLIT_EVENT     =  0x00008000; /*!< @brief this EPG entry is a split event (e.g. continues after the news */

#ifdef __cplusplus
extern "C" {
#endif

  /*!
   * @brief Representation of an EPG event.
   */
  typedef struct EPG_TAG {
    unsigned int  iUniqueBroadcastId;  /*!< @brief (required) identifier for this event */
    const char *  strTitle;            /*!< @brief (required) this event's title */
    unsigned int  iChannelNumber;      /*!< @brief (required) the number of the channel this event occurs on */
    time_t        startTime;           /*!< @brief (required) start time in UTC */
    time_t        endTime;             /*!< @brief (required) end time in UTC */
    const char *  strPlotOutline;      /*!< @brief (optional) plot outline */
    const char *  strPlot;             /*!< @brief (optional) plot */
    const char *  strOriginalTitle;    /*!< @brief (optional) originaltitle */
    const char *  strCast;             /*!< @brief (optional) cast */
    const char *  strDirector;         /*!< @brief (optional) director */
    const char *  strWriter;           /*!< @brief (optional) writer */
    int           iYear;               /*!< @brief (optional) year */
    const char *  strIMDBNumber;       /*!< @brief (optional) IMDBNumber */
    const char *  strIconPath;         /*!< @brief (optional) icon path */
    int           iGenreType;          /*!< @brief (optional) genre type */
    int           iGenreSubType;       /*!< @brief (optional) genre sub type */
    const char *  strGenreDescription; /*!< @brief (optional) genre. Will be used only when iGenreType = EPG_GENRE_USE_STRING */
    time_t        firstAired;          /*!< @brief (optional) first aired in UTC */
    int           iParentalRating;     /*!< @brief (optional) parental rating */
    int           iStarRating;         /*!< @brief (optional) star rating */
    bool          bNotify;             /*!< @brief (optional) notify the user when this event starts */
    int           iSeriesNumber;       /*!< @brief (optional) series number */
    int           iEpisodeNumber;      /*!< @brief (optional) episode number */
    int           iEpisodePartNumber;  /*!< @brief (optional) episode part number */
    const char *  strEpisodeName;      /*!< @brief (optional) episode name */
    unsigned int  iFlags;              /*!< @brief (optional) bit field of independent flags associated with the EPG entry */
  } ATTRIBUTE_PACKED EPG_TAG;

#ifdef __cplusplus
}
#endif
