#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "../definitions.hpp"

namespace V2
{
namespace KodiAPI
{

namespace PVR
{

  //============================================================================
  ///
  /// \defgroup CPP_V2_KodiAPI_PVR_Transfer
  /// \ingroup CPP_V2_KodiAPI_PVR
  /// @{
  /// @brief <b>Callback functions for PVR add-on</b>
  ///
  /// Functions inside this class are used after an Kodi to PVR client request
  /// for transfer related data to Kodi.
  ///
  /// <em>This is a list of calls related to this class:</em>
  ///  - <tt>PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);</tt>
  ///  - <tt>PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);</tt>
  ///  - <tt>PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP& group);</tt>
  ///  - <tt>PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel, time_t iStart, time_t iEnd);</tt>
  ///  - <tt>PVR_ERROR GetRecordings(ADDON_HANDLE handle, bool deleted);</tt>
  ///  - <tt>PVR_ERROR GetTimers(ADDON_HANDLE handle);</tt>
  ///
  /// These are pure static functions them no other initialization need.
  ///
  /// It has the header \ref Transfer.h "#include <kodi/api2/pvr/Transfer.h>" be included
  /// to enjoy it
  ///
  namespace Transfer
  {
    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_PVR_Transfer
    /// @brief Transfer an EPG tag from the add-on to Kodi
    ///
    /// Callback for <b><tt>PVR_ERROR GetChannels(GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel, time_t iStart, time_t iEnd));</tt></b>
    /// from PVR system.
    ///
    /// @param[in] handle The handle parameter that Kodi used when requesting the
    ///                   EPG data
    /// @param[in] entry  The entry to transfer to Kodi
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// <em>Here structure used for them about requested source:</em>
    /// ~~~~~~~~~~~~~
    /// /*
    ///  * Representation of a EPG event.
    ///  *
    ///  * Only as example shown here! See always the original structure on related header.
    ///  */
    /// typedef struct EPG_TAG
    /// {
    ///   unsigned int  iUniqueBroadcastId;  /* (required) identifier for this event */
    ///   const char *  strTitle;            /* (required) this event's title */
    ///   unsigned int  iChannelNumber;      /* (required) the number of the channel this event occurs on */
    ///   time_t        startTime;           /* (required) start time in UTC */
    ///   time_t        endTime;             /* (required) end time in UTC */
    ///   const char *  strPlotOutline;      /* (optional) plot outline */
    ///   const char *  strPlot;             /* (optional) plot */
    ///   const char *  strOriginalTitle;    /* (optional) originaltitle */
    ///   const char *  strCast;             /* (optional) cast */
    ///   const char *  strDirector;         /* (optional) director */
    ///   const char *  strWriter;           /* (optional) writer */
    ///   int           iYear;               /* (optional) year */
    ///   const char *  strIMDBNumber;       /* (optional) IMDBNumber */
    ///   const char *  strIconPath;         /* (optional) icon path */
    ///   int           iGenreType;          /* (optional) genre type */
    ///   int           iGenreSubType;       /* (optional) genre sub type */
    ///   const char *  strGenreDescription; /* (optional) genre. Will be used only when iGenreType = EPG_GENRE_USE_STRING */
    ///   time_t        firstAired;          /* (optional) first aired in UTC */
    ///   int           iParentalRating;     /* (optional) parental rating */
    ///   int           iStarRating;         /* (optional) star rating */
    ///   bool          bNotify;             /* (optional) notify the user when this event starts */
    ///   int           iSeriesNumber;       /* (optional) series number */
    ///   int           iEpisodeNumber;      /* (optional) episode number */
    ///   int           iEpisodePartNumber;  /* (optional) episode part number */
    ///   const char *  strEpisodeName;      /* (optional) episode name */
    ///   unsigned int  iFlags;              /* (optional) bit field of independent flags associated with the EPG entry */
    /// } ATTRIBUTE_PACKED EPG_TAG;
    /// ~~~~~~~~~~~~~
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Code Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/pvr/Transfer.h>
    ///
    /// ...
    /// PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end)
    /// {
    ///   ...
    ///
    ///   for (int i = 0; i < <YOUR_AMOUNT_OF_EPG_ENTRIES>; ++i)
    ///   {
    ///     EPG_TAG tag;
    ///     memset(&tag, 0 , sizeof(tag));
    ///
    ///     tag.iChannelNumber      = <YOUR_EPG_CHANNEL_NUMBER>;
    ///     tag.iUniqueBroadcastId  = <YOUR_EPG_ID>;
    ///     tag.startTime           = <YOUR_EPG_START_TIME>;
    ///     tag.endTime             = <YOUR_EPG_END_TIME>;
    ///     tag.strTitle            = <YOUR_EPG_TITLE>;
    ///     ...
    ///
    ///     KodiAPI::PVR::Transfer::EpgEntry(handle, &tag);
    ///   }
    ///
    ///   return PVR_ERROR_NO_ERROR;
    /// }
    /// ~~~~~~~~~~~~~
    ///
    void EpgEntry(
                         const ADDON_HANDLE handle,
                         const EPG_TAG*     entry);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_PVR_Transfer
    /// @brief Transfer a channel entry from the add-on to Kodi
    ///
    /// Callback for <b><tt>PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);</tt></b>
    /// from PVR system.
    ///
    /// @param[in] handle The handle parameter that Kodi used when requesting the
    ///                   channel list
    /// @param[in] entry  The entry to transfer to Kodi
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// <em>Here structure used for them about requested source:</em>
    /// ~~~~~~~~~~~~~
    /// /*
    ///  * Representation of a TV or radio channel.
    ///  *
    ///  * Only as example shown here! See always the original structure on related header.
    ///  */
    /// typedef struct PVR_CHANNEL
    /// {
    ///   unsigned int iUniqueId;                                            /* (required) unique identifier for this channel */
    ///   bool         bIsRadio;                                             /* (required) true if this is a radio channel, false if it's a TV channel */
    ///   unsigned int iChannelNumber;                                       /* (optional) channel number of this channel on the backend */
    ///   unsigned int iSubChannelNumber;                                    /* (optional) sub channel number of this channel on the backend (ATSC) */
    ///   char         strChannelName[PVR_ADDON_NAME_STRING_LENGTH];         /* (optional) channel name given to this channel */
    ///   char         strInputFormat[PVR_ADDON_INPUT_FORMAT_STRING_LENGTH]; /* (optional) input format type. types can be found in ffmpeg/libavformat/allformats.c
    ///                                                                       * leave empty if unknown */
    ///   char         strStreamURL[PVR_ADDON_URL_STRING_LENGTH];            /* (optional) the URL to use to access this channel leave empty to use this add-on to
    ///                                                                       * access the stream set to a path that's supported by Kodi otherwise. */
    ///   unsigned int iEncryptionSystem;                                    /* (optional) the encryption ID or CaID of this channel */
    ///   char         strIconPath[PVR_ADDON_URL_STRING_LENGTH];             /* (optional) path to the channel icon (if present) */
    ///   bool         bIsHidden;                                            /* (optional) true if this channel is marked as hidden */
    /// } ATTRIBUTE_PACKED PVR_CHANNEL;
    /// ~~~~~~~~~~~~~
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Code Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/pvr/Transfer.h>
    ///
    /// ...
    /// PVR_ERROR cVNSIData::GetChannelsList(ADDON_HANDLE handle, bool radio)
    /// {
    ///   ...
    ///
    ///   for (int i = 0; i < <YOUR_AMOUNT_OF_CHANNELS>; ++i)
    ///   {
    ///     PVR_CHANNEL tag;
    ///     memset(&tag, 0 , sizeof(tag));
    ///
    ///     tag.iChannelNumber    = <YOUR_CHANNEL_NUMBER>;
    ///     strncpy(tag.strChannelName, <YOUR_CHANNEL_NAME>, sizeof(tag.strChannelName) - 1);
    ///     tag.iUniqueId         = <YOUR_CHANNEL_UNIQUE_ID>;
    ///     tag.bIsRadio          = <YOUR_CHANNEL_RADIO>;
    ///     ...
    ///
    ///     KodiAPI::PVR::Transfer::ChannelEntry(handle, &tag);
    ///   }
    ///
    ///   return PVR_ERROR_NO_ERROR;
    /// }
    /// ~~~~~~~~~~~~~
    ///
    void ChannelEntry(
                         const ADDON_HANDLE handle,
                         const PVR_CHANNEL* entry);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_PVR_Transfer
    /// @brief Transfer a timer entry from the add-on to Kodi
    ///
    /// Callback for <b><tt>PVR_ERROR GetTimers(ADDON_HANDLE handle);</tt></b>
    /// from PVR system.
    ///
    /// @param[in] handle The handle parameter that Kodi used when requesting the
    ///               timers list
    /// @param[in] entry The entry to transfer to Kodi
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// <em>Here structure used for them about requested source:</em>
    /// ~~~~~~~~~~~~~
    /// /*
    ///  * Representation of a timer event.
    ///  *
    ///  * Only as example shown here! See always the original structure on related header.
    ///  */
    /// typedef struct PVR_TIMER
    /// {
    ///   unsigned int    iClientIndex;                              /* (required) the index of this timer given by the client. PVR_TIMER_NO_CLIENT_INDEX indicates that the
    ///                                                               * index was not yet set by the client, for example for new timers created by
    ///                                                               * Kodi and passed the first time to the client. A valid index must be greater than PVR_TIMER_NO_CLIENT_INDEX. */
    ///   unsigned int    iParentClientIndex;                        /* (optional) for timers scheduled by a repeating timer, the index of the repeating timer that scheduled this
    ///                                                               * timer (it's PVR_TIMER.iClientIndex value). Use PVR_TIMER_NO_PARENT to indicate that this timer was no
    ///                                                               * scheduled by a repeating timer. */
    ///   int             iClientChannelUid;                         /* (optional) unique identifier of the channel to record on. PVR_TIMER_ANY_CHANNEL will denote "any channel",
    ///                                                               * not a specifoc one. */
    ///   time_t          startTime;                                 /* (optional) start time of the recording in UTC. Instant timers that are sent to the add-on by Kodi will have
    ///                                                               * this value set to 0.*/
    ///   time_t          endTime;                                   /* (optional) end time of the recording in UTC. */
    ///   bool            bStartAnyTime;                             /* (optional) for EPG based (not Manual) timers indicates startTime does not apply. Default = false */
    ///   bool            bEndAnyTime;                               /* (optional) for EPG based (not Manual) timers indicates endTime does not apply. Default = false */
    ///
    ///   PVR_TIMER_STATE state;                                     /* (required) the state of this timer */
    ///   unsigned int    iTimerType;                                /* (required) the type of this timer. It is private to the addon and can be freely defined by the addon.
    ///                                                               * The value must be greater than PVR_TIMER_TYPE_NONE.
    ///                                                               * Kodi does not interpret this value (except for checking for PVR_TIMER_TYPE_NONE), but will pass the right
    ///                                                               * id to the addon with every PVR_TIMER instance, thus the addon easily can determine the timer type. */
    ///   char            strTitle[PVR_ADDON_NAME_STRING_LENGTH];    /* (required) a title for this timer */
    ///   char            strEpgSearchString[PVR_ADDON_NAME_STRING_LENGTH]; /* (optional) a string used to search epg data for repeating epg-based timers. Format is backend-dependent,
    ///                                                               * for example regexp */
    ///   bool            bFullTextEpgSearch;                        /* (optional) indicates, whether strEpgSearchString is to match against the epg episode title only or also
    ///                                                               * against "other" epg data (backend-dependent) */
    ///   char            strDirectory[PVR_ADDON_URL_STRING_LENGTH]; /* (optional) the (relative) directory where the recording will be stored in */
    ///   char            strSummary[PVR_ADDON_DESC_STRING_LENGTH];  /* (optional) the summary for this timer */
    ///   int             iPriority;                                 /* (optional) the priority of this timer */
    ///   int             iLifetime;                                 /* (optional) lifetime of recordings created by this timer. > 0 days after which recordings will be deleted by
    ///                                                               * the backend, < 0 addon defined integer list reference, == 0 disabled */
    ///   int             iMaxRecordings;                            /* (optional) maximum number of recordings this timer shall create. > 0 number of recordings, < 0 addon
    ///                                                               * defined integer list reference, == 0 disabled */
    ///   unsigned int    iRecordingGroup;                           /* (optional) integer ref to addon/backend defined list of recording groups*/
    ///   time_t          firstDay;                                  /* (optional) the first day this timer is active, for repeating timers */
    ///   unsigned int    iWeekdays;                                 /* (optional) week days, for repeating timers */
    ///   unsigned int    iPreventDuplicateEpisodes;                 /* (optional) 1 if backend should only record new episodes in case of a repeating epg-based timer, 0 if all
    ///                                                               * episodes shall be recorded (no duplicate detection). Actual algorithm for duplicate detection is defined by the
    ///                                                               * backend. Addons may define own values for different duplicate detection algorithms, thus this is not just a bool. */
    ///   unsigned int    iEpgUid;                                   /* (optional) epg event id. Use PVR_TIMER_NO_EPG_UID to state that there is no EPG event id available for this
    ///                                                               * timer. Values greater than PVR_TIMER_NO_EPG_UID represent a valid epg event id. */
    ///   unsigned int    iMarginStart;                              /* (optional) if set, the backend starts the recording iMarginStart minutes before startTime. */
    ///   unsigned int    iMarginEnd;                                /* (optional) if set, the backend ends the recording iMarginEnd minutes after endTime. */
    ///   int             iGenreType;                                /* (optional) genre type */
    ///   int             iGenreSubType;                             /* (optional) genre sub type */
    /// } ATTRIBUTE_PACKED PVR_TIMER;
    /// ~~~~~~~~~~~~~
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Code Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/pvr/Transfer.h>
    ///
    /// ...
    /// PVR_ERROR GetTimersList(ADDON_HANDLE handle)
    /// {
    ///   ...
    ///   for (int i = 0; i < <YOUR_AMOUNT_OF_TIMERS>; ++i)
    ///   {
    ///     {
    ///       PVR_TIMER tag;
    ///       memset(&tag, 0, sizeof(tag));
    ///
    ///       tag.iTimerType        = PVR_TIMER_TYPE_NONE; //< or <YOUR_TIMER_TYPE>
    ///       tag.iClientIndex      = <YOUR_TIMER_CLIENT_INDEX>;
    ///       int iActive           = <YOUR_TIMER_ACTIVE>;
    ///       int iRecording        = <YOUR_TIMER_RECORDING>;
    ///       int iPending          = <YOUR_TIMER_PENDING>;
    ///       if (iRecording)
    ///         tag.state = PVR_TIMER_STATE_RECORDING;
    ///       else if (iPending || iActive)
    ///         tag.state = PVR_TIMER_STATE_SCHEDULED;
    ///       else
    ///         tag.state = PVR_TIMER_STATE_CANCELLED;
    ///       tag.startTime         = <YOUR_TIMER_START_TIME>;
    ///       tag.endTime           = <YOUR_TIMER_END_TIME>;
    ///       strncpy(tag.strTitle, <YOUR_TIMER_TITLE>, sizeof(tag.strTitle) - 1);
    ///       ...
    ///
    ///       KodiAPI::PVR::Transfer::TimerEntry(handle, &tag);
    ///     }
    ///   }
    ///   return PVR_ERROR_NO_ERROR;
    /// }
    /// ~~~~~~~~~~~~~
    ///
    void TimerEntry(
                         const ADDON_HANDLE handle,
                         const PVR_TIMER*   entry);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_PVR_Transfer
    /// @brief Transfer a recording entry from the add-on to Kodi
    ///
    /// Callback for <b><tt>GetRecordings(ADDON_HANDLE handle, bool deleted);</tt></b>
    /// from PVR system.
    ///
    /// @param[in] handle The handle parameter that Kodi used when requesting the
    ///               recordings list
    /// @param[in] entry The entry to transfer to Kodi
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// <em>Here structure used for them about requested source:</em>
    /// ~~~~~~~~~~~~~
    /// /*
    ///  * Representation of a recording.
    ///  *
    ///  * Only as example shown here! See always the original structure on related header.
    ///  */
    /// typedef struct PVR_RECORDING
    /// {
    ///   char   strRecordingId[PVR_ADDON_NAME_STRING_LENGTH];  /* (required) unique id of the recording on the client. */
    ///   char   strTitle[PVR_ADDON_NAME_STRING_LENGTH];        /* (required) the title of this recording */
    ///   char   strEpisodeName[PVR_ADDON_NAME_STRING_LENGTH];  /* (optional) episode name (also known as subtitle) */
    ///   int    iSeriesNumber;                                 /* (optional) series number (usually called season). Set to "0" for specials/pilot. For 'invalid'
    ///                                                          * see iEpisodeNumber or set to -1 */
    ///   int    iEpisodeNumber;                                /* (optional) episode number within the "iSeriesNumber" season. For 'invalid' set to -1 or
    ///                                                          * iSeriesNumber=0 to show both are invalid */
    ///   int    iYear;                                         /* (optional) year of first release (use to identify a specific movie re-make) / first airing for
    ///                                                          * TV shows. Set to '0' for invalid. */
    ///
    ///   char   strStreamURL[PVR_ADDON_URL_STRING_LENGTH];     /* (required) stream URL to access this recording */
    ///   char   strDirectory[PVR_ADDON_URL_STRING_LENGTH];     /* (optional) directory of this recording on the client */
    ///   char   strPlotOutline[PVR_ADDON_DESC_STRING_LENGTH];  /* (optional) plot outline */
    ///   char   strPlot[PVR_ADDON_DESC_STRING_LENGTH];         /* (optional) plot */
    ///   char   strChannelName[PVR_ADDON_NAME_STRING_LENGTH];  /* (optional) channel name */
    ///   char   strIconPath[PVR_ADDON_URL_STRING_LENGTH];      /* (optional) icon path */
    ///   char   strThumbnailPath[PVR_ADDON_URL_STRING_LENGTH]; /* (optional) thumbnail path */
    ///   char   strFanartPath[PVR_ADDON_URL_STRING_LENGTH];    /* (optional) fanart path */
    ///   time_t recordingTime;                                 /* (optional) start time of the recording */
    ///   int    iDuration;                                     /* (optional) duration of the recording in seconds */
    ///   int    iPriority;                                     /* (optional) priority of this recording (from 0 - 100) */
    ///   int    iLifetime;                                     /* (optional) life time in days of this recording */
    ///   int    iGenreType;                                    /* (optional) genre type */
    ///   int    iGenreSubType;                                 /* (optional) genre sub type */
    ///   int    iPlayCount;                                    /* (optional) play count of this recording on the client */
    ///   int    iLastPlayedPosition;                           /* (optional) last played position of this recording on the client */
    ///   bool   bIsDeleted;                                    /* (optional) shows this recording is deleted and can be undelete */
    ///   unsigned int iEpgEventId;                             /* (optional) EPG event id associated with this recording */
    /// } ATTRIBUTE_PACKED PVR_RECORDING;
    /// ~~~~~~~~~~~~~
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Code Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/pvr/Transfer.h>
    ///
    /// ...
    /// PVR_ERROR GetRecordingsList(ADDON_HANDLE handle)
    /// {
    ///   ...
    ///   for (int i = 0; i < <YOUR_AMOUNT_OF_RECORDINGS>; ++i)
    ///   {
    ///     PVR_RECORDING tag;
    ///     memset(&tag, 0, sizeof(tag));
    ///     tag.recordingTime   = <YOUR_RECORDING_TIME>;
    ///     tag.iDuration       = <YOUR_RECORDING_DURATION>;
    ///     tag.bIsDeleted      = <YOUR_RECORDING_DELETED>;
    ///     ...
    ///
    ///     strncpy(tag.strChannelName, <YOUR_RECORDING_TV_CHANNEL>, sizeof(tag.strChannelName) - 1);
    ///     strncpy(tag.strTitle, <YOUR_RECORDING_TITLE>, sizeof(tag.strTitle) - 1);
    ///     strncpy(tag.strPlotOutline, <YOUR_RECORDING_OUTLINE_PLOT>, sizeof(tag.strPlotOutline) - 1);
    ///     strncpy(tag.strPlot, <YOUR_RECORDING_PLOT>, sizeof(tag.strPlot) - 1);
    ///     strncpy(tag.strDirectory, YOUR_RECORDING_DIRECTORY>, sizeof(tag.strDirectory) - 1);
    ///     strncpy(tag.strRecordingId, YOUR_RECORDING_IDENT_STRING>, sizeof(tag.strRecordingId) - 1);
    ///     ...
    ///
    ///     KodiAPI::PVR::Transfer::RecordingEntry(handle, &tag);
    ///   }
    ///
    ///   return PVR_ERROR_NO_ERROR;
    /// }
    /// ~~~~~~~~~~~~~
    ///
    void RecordingEntry(
                         const ADDON_HANDLE handle,
                         const PVR_RECORDING* entry);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_PVR_Transfer
    /// @brief Transfer a channel group from the add-on to Kodi. The group will
    /// be created if it doesn't exist.
    ///
    /// Callback for <b><tt>GetChannelGroups(ADDON_HANDLE handle, bool bRadio);</tt></b>
    /// from PVR system.
    ///
    /// @param[in] handle The handle parameter that Kodi used when requesting the
    ///                   channel groups list
    /// @param[in] entry  The entry to transfer to Kodi
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// <em>Here structure used for them about requested source:</em>
    /// ~~~~~~~~~~~~~
    /// /*
    ///  * Structure about channel group
    ///  *
    ///  * Only as example shown here! See always the original structure on related header.
    ///  */
    /// typedef struct PVR_CHANNEL_GROUP
    /// {
    ///   char         strGroupName[PVR_ADDON_NAME_STRING_LENGTH]; /* Name of this channel group */
    ///   bool         bIsRadio;                                   /* True if this is a radio channel group, false otherwise. */
    ///   unsigned int iPosition;                                  /* Sort position of the group, is optional (0 indicates that
    ///                                                             * the backend doesn't support sorting of groups) */
    /// } ATTRIBUTE_PACKED PVR_CHANNEL_GROUP;
    /// ~~~~~~~~~~~~~
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Code Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/pvr/Transfer.h>
    ///
    /// ...
    /// PVR_ERROR GetChannelGroupList(ADDON_HANDLE handle, bool bRadio)
    /// {
    ///   ...
    ///
    ///   for (int i = 0; i < <YOUR_AMOUNT_OF_CHANNEL_GROUPS>; ++i)
    ///   {
    ///     PVR_CHANNEL_GROUP tag;
    ///     memset(&tag, 0, sizeof(tag));
    ///
    ///     strncpy(tag.strGroupName, <YOUR_GROUP_NAME>, sizeof(tag.strGroupName) - 1);
    ///     tag.bIsRadio = <YOUR_GROUP_FOR_RADIO> ? true : false;
    ///     tag.iPosition = <YOUR_OPTIONAL_GROUP_POSITION>;  // Optional, make 0 if unused
    ///
    ///     KodiAPI::PVR::Transfer::ChannelGroup(handle, &tag);
    ///   }
    ///
    ///   return PVR_ERROR_NO_ERROR;
    /// }
    /// ~~~~~~~~~~~~~
    ///
    void ChannelGroup(
                         const ADDON_HANDLE handle,
                         const PVR_CHANNEL_GROUP* entry);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// @ingroup CPP_V2_KodiAPI_PVR_Transfer
    /// @brief Transfer a channel group member entry from the add-on to Kodi. The
    /// channel will be added to the group if the group can be found.
    ///
    /// Callback for <b><tt>GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP& group);</tt></b>
    /// from PVR system.
    ///
    /// @param[in] handle The handle parameter that Kodi used when requesting the
    ///               channel group members list
    /// @param[in] entry The entry to transfer to Kodi
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// <em>Here structure used for them about requested source:</em>
    /// ~~~~~~~~~~~~~
    /// /*
    ///  * Structure about channel group
    ///  *
    ///  * Only as example shown here! See always the original structure on related header.
    ///  */
    /// typedef struct PVR_CHANNEL_GROUP
    /// {
    ///   char         strGroupName[PVR_ADDON_NAME_STRING_LENGTH]; /* Name of this channel group */
    ///   bool         bIsRadio;                                   /* True if this is a radio channel group, false otherwise. */
    ///   unsigned int iPosition;                                  /* Sort position of the group, is optional (0 indicates that
    ///                                                             * the backend doesn't support sorting of groups) */
    /// } ATTRIBUTE_PACKED PVR_CHANNEL_GROUP;
    /// ~~~~~~~~~~~~~
    ///
    /// <em>And here the structure used for transfer the data to Kodi on this call:</em>
    /// ~~~~~~~~~~~~~
    /// /*
    ///  * Structure about channel group member channel
    ///  *
    ///  * Only as example shown here! See always the original structure on related header.
    ///  */
    /// typedef struct PVR_CHANNEL_GROUP_MEMBER
    /// {
    ///   char         strGroupName[PVR_ADDON_NAME_STRING_LENGTH]; /* Name of the channel group to add the channel to */
    ///   unsigned int iChannelUniqueId;                           /* Unique id of the member */
    ///   unsigned int iChannelNumber;                             /* Channel number within the group, is optional  */
    /// } ATTRIBUTE_PACKED PVR_CHANNEL_GROUP_MEMBER;
    /// ~~~~~~~~~~~~~
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Code Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/pvr/Transfer.h>
    ///
    /// ...
    /// PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
    /// {
    ///   ...
    ///
    ///   for (int i = 0; i < <YOUR_AMOUNT_OF_CHANNEL_GROUP_MEMBERS>; ++i)
    ///   {
    ///     PVR_CHANNEL_GROUP_MEMBER tag;
    ///     memset(&tag, 0, sizeof(tag));
    ///
    ///     strncpy(tag.strGroupName, <YOUR_GROUP_NAME>, sizeof(tag.strGroupName) - 1);
    ///     tag.iChannelUniqueId = <YOUR_CHANNEL_UNIQUE_ID>;
    ///     tag.iChannelNumber = <YOUR_CHANNEL_NUMBER>;
    ///
    ///     KodiAPI::PVR::Transfer::ChannelGroupMember(handle, &tag);
    ///   }
    ///
    ///   return PVR_ERROR_NO_ERROR;
    /// }
    /// ~~~~~~~~~~~~~
    ///
    void ChannelGroupMember(
                         const ADDON_HANDLE handle,
                         const PVR_CHANNEL_GROUP_MEMBER* entry);
    //--------------------------------------------------------------------------

  };
  // @}

}; /* namespace PVR */

}; /* namespace KodiAPI */
}; /* namespace V2 */
