/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string.h>
#include <time.h>

#undef ATTRIBUTE_PACKED
#undef PRAGMA_PACK_BEGIN
#undef PRAGMA_PACK_END

#if defined(__GNUC__)
#define ATTRIBUTE_PACKED __attribute__ ((packed))
#define PRAGMA_PACK 0
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

/* Set EPGTAG.iGenreType to EPG_GENRE_USE_STRING to transfer genre strings to Kodi */
#define EPG_GENRE_USE_STRING                           0x100

/* Separator to use in strings containing different tokens, for example writers, directors, actors of an event. */
#define EPG_STRING_TOKEN_SEPARATOR ","

#ifdef __cplusplus
extern "C" {
#endif

  /* EPG_TAG.iFlags values */
  const unsigned int EPG_TAG_FLAG_UNDEFINED = 0x00000000; /*!< @brief nothing special to say about this entry */
  const unsigned int EPG_TAG_FLAG_IS_SERIES = 0x00000001; /*!< @brief this EPG entry is part of a series */

  /* Special EPG_TAG.iUniqueBroadcastId value */

  /*!
   * @brief special EPG_TAG.iUniqueBroadcastId value to indicate that a tag has not a valid EPG event uid.
   */
  const unsigned int EPG_TAG_INVALID_UID = 0;

  /*!
   * @brief EPG event states. Used with EpgEventStateChange callback.
   */
  typedef enum
  {
    EPG_EVENT_CREATED = 0,  /*!< @brief event created */
    EPG_EVENT_UPDATED = 1,  /*!< @brief event updated */
    EPG_EVENT_DELETED = 2,  /*!< @brief event deleted */
  } EPG_EVENT_STATE;

  /*!
   * @brief Representation of an EPG event.
   */
  typedef struct EPG_TAG {
    unsigned int  iUniqueBroadcastId;  /*!< @brief (required) identifier for this event. Event uids must be unique for a channel. Valid uids must be greater than EPG_TAG_INVALID_UID. */
    unsigned int  iUniqueChannelId;    /*!< @brief (required) unique identifier of the channel this event belongs to. */
    const char *  strTitle;            /*!< @brief (required) this event's title */
    time_t        startTime;           /*!< @brief (required) start time in UTC */
    time_t        endTime;             /*!< @brief (required) end time in UTC */
    const char *  strPlotOutline;      /*!< @brief (optional) plot outline */
    const char *  strPlot;             /*!< @brief (optional) plot */
    const char *  strOriginalTitle;    /*!< @brief (optional) originaltitle */
    const char *  strCast;             /*!< @brief (optional) cast. Use EPG_STRING_TOKEN_SEPARATOR to separate different persons. */
    const char *  strDirector;         /*!< @brief (optional) director(s). Use EPG_STRING_TOKEN_SEPARATOR to separate different persons. */
    const char *  strWriter;           /*!< @brief (optional) writer(s). Use EPG_STRING_TOKEN_SEPARATOR to separate different persons. */
    int           iYear;               /*!< @brief (optional) year */
    const char *  strIMDBNumber;       /*!< @brief (optional) IMDBNumber */
    const char *  strIconPath;         /*!< @brief (optional) icon path */
    int           iGenreType;          /*!< @brief (optional) genre type */
    int           iGenreSubType;       /*!< @brief (optional) genre sub type */
    const char *  strGenreDescription; /*!< @brief (optional) genre. Will be used only when iGenreType == EPG_GENRE_USE_STRING. Use EPG_STRING_TOKEN_SEPARATOR to separate different genres. */
    time_t        firstAired;          /*!< @brief (optional) first aired in UTC */
    int           iParentalRating;     /*!< @brief (optional) parental rating */
    int           iStarRating;         /*!< @brief (optional) star rating */
    bool          bNotify;             /*!< @brief (optional) notify the user when this event starts */
    int           iSeriesNumber;       /*!< @brief (optional) series number */
    int           iEpisodeNumber;      /*!< @brief (optional) episode number */
    int           iEpisodePartNumber;  /*!< @brief (optional) episode part number */
    const char *  strEpisodeName;      /*!< @brief (optional) episode name */
    unsigned int  iFlags;              /*!< @brief (optional) bit field of independent flags associated with the EPG entry */
    const char *  strSeriesLink;       /*!< @brief (optional) series link for this event */
  } ATTRIBUTE_PACKED EPG_TAG;

#ifdef __cplusplus
}
#endif
