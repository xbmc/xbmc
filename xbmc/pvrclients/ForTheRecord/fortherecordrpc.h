#pragma once
/*
 *      Copyright (C) 2010 Marcel Groothuis
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

#include <string>
#include <json/json.h>
#include <cstdlib>

#define FTR_1_5_0_3 (40)
#define FTR_1_6_0_1 (45)
#define FTR_1_6_1_0 (48)
#define FTR_REST_MINIMUM_API_VERSION FTR_1_6_0_1
#define FTR_REST_MAXIMUM_API_VERSION FTR_1_6_1_0
#define E_FAILED -1
#define E_EMPTYRESPONSE -2

namespace ForTheRecord
{
  enum ChannelType {
    Television = 0,
    Radio = 1
  };

  enum RecordingGroupMode {
    GroupByProgramTitle = 0,
    GroupBySchedule = 1,
    GroupByCategory = 2,
    GroupByChannel = 3,
    GroupByRecordingDay = 4
  };

  enum SchedulePriority {
    VeryLow = 0,
    Low = 1,
    Normal = 2,
    High = 3,
    VeryHigh = 4
  };

  enum ScheduleType {
    Recording = 82,   // 'R'
    Suggestion = 83,  // 'S'
    Alert = 45        // 'A'
  };



  enum KeepUntilMode {
    UntilSpaceIsNeeded = 0,
    Forever = 1,
    NumberOfDays = 2,
    NumberOfEpisodes =3
  };

  enum VideoAspectRatio {
    Unknown = 0,
    Standard = 1,
    Widescreen = 2
  };

  enum LiveStreamResult {
    Succeed = 0,
    NoFreeCardFound = 1,
    ChannelTuneFailed = 2,
    NoReTunePossible = 3,
    UnknownError = 4,
    NotSupported = 5
  };

  /**
   * \brief Send a REST command to 4TR and return the JSON response string
   * \param command       The command string url (starting from "ForTheRecord/")
   * \param json_response Reference to a std::string used to store the json response string
   * \return 0 on ok, -1 on a failure
   */
  int ForTheRecordRPC(const std::string& command, const std::string& arguments, std::string& json_response);

  /**
   * \brief Send a REST command to 4TR and return the JSON response 
   * \param command       The command string url (starting from "ForTheRecord/")
   * \param json_response Reference to a Json::Value used to store the parsed Json value
   * \return 0 on ok, -1 on a failure
   */
  int ForTheRecordJSONRPC(const std::string& command, const std::string& arguments, Json::Value& json_response);

  /**
   * \brief Send a REST command to 4TR, write the response to a file and return the filename
   * \param command       The command string url (starting from "ForTheRecord/")
   * \param newfilename   Reference to a std::string used to store the output file name
   * \param htt_presponse Reference to a long used to store the HTTP response code
   * \return 0 on ok, -1 on a failure
   */
  int ForTheRecordRPCToFile(const std::string& command, const std::string& arguments, std::string& newfilename, long& http_response);

  /**
   * \brief Ping core service.
   * \param requestedApiVersion  The API version the client needs, pass in Constants.ForTheRecordRestApiVersion.
   * \return  0 if client and server are compatible, -1 if the client is too old, +1 if the client is newer than the server and -2 if the connection failed (server down?)
   */
  int Ping(int requestedApiVersion);

  /**
   * \brief TuneLiveStream
   * \param channel_id  The ForTheRecord ChannelID of the channel
   * \param stream      Reference to a string that will point to the tsbuffer file/RTSP stream
   */
  int TuneLiveStream(const std::string& channel_id, ChannelType channeltype, std::string& stream);

  /**
   * \brief Stops the last tuned live stream
   */
  int StopLiveStream();

  /**
   * \brief Returns the URL of the current live stream
   */
  std::string GetLiveStreamURL(void);

  /**
   * \brief Returns the Signal information of the current live stream
   */
  int SignalQuality(Json::Value& response);

  /**
   * \brief Tell the recorder/tuner we are still showing this stream and to keep it alive. Call this every 30 seconds or so.
   */
  bool KeepLiveStreamAlive();

  /**
   * \brief Fetch the list of availalable channels for tv or radio
   * \param channeltype  The type of channel to fetch the list for
   */
  int GetChannelList(enum ChannelType channelType, Json::Value& response);

  /**
   * \brief Fetch the EPG data for the given guidechannel id
   * \param guidechannel_id  String containing the 4TR guidechannel_id (not the channel_id)
   * \param epg_start        Start from this date
   * \param epg_stop         Until this date
   */
  int GetEPGData(const int backendversion, const std::string& guidechannel_id, struct tm epg_start, struct tm epg_end, Json::Value& response);

  /**
   * \brief Fetch the recording groups sorted by title
   * \param response Reference to a std::string used to store the json response string
   */
  int GetRecordingGroupByTitle(Json::Value& response);

  /**
   * \brief Fetch the data for all recordings for a given title
   * \param title Program title of recording
   * \param response Reference to a std::string used to store the json response string
   */
  int GetRecordingsForTitle(const std::string& title, Json::Value& response);

  /**
   * \brief Fetch the detailed information of a recorded show
   * \param id unique id (guid) of the recording
   * \param response Reference to a std::string used to store the json response string
   */
  int GetRecordingById(const std::string& id, Json::Value& response);

  /**
   * \brief Delete the recording on the pvr backend
   * \param recordingfilename UNC filename to delete
   */
  int DeleteRecording(const std::string recordingfilename);

  /**
   * \brief Fetch the detailed information of a schedule
   * \param id unique id (guid) of the schedule
   * \param response Reference to a std::string used to store the json response string
   */
  int GetScheduleById(const std::string& id, Json::Value& response);

  /**
   * \brief Fetch the detailed information of a guide program
   * \param id unique id (guid) of the program
   * \param response Reference to a std::string used to store the json response string
   */
  int GetProgramById(const std::string& id, Json::Value& response);

  /**
   * \brief Fetch the list of schedules for tv or radio
   * \param channeltype  The type of channel to fetch the list for
   */
  int GetScheduleList(enum ChannelType channelType, Json::Value& response);

  /**
   * \brief Fetch the list of upcoming programs
   */
  int GetUpcomingPrograms(Json::Value& response);

  /**
   * \brief Fetch the list of currently active recordings
   */
  int GetActiveRecordings(Json::Value& response);

  /**
   * \brief Cancel a currently active recording
   */
  int AbortActiveRecording(Json::Value& activeRecording);

  /**
   * \brief Cancel an upcoming program
   */
  int CancelUpcomingProgram(const std::string& scheduleid, const std::string& channelid, const time_t starttime, const std::string& upcomingprogramid);

  /**
   * \brief Add a xbmc timer as a one time schedule
   */
  int AddOneTimeSchedule(const std::string& channelid, const time_t starttime, const std::string& title, int prerecordseconds, int postrecordseconds, Json::Value& response);

  /**
   * \brief Add a xbmc timer as a manual schedule
   */
  int AddManualSchedule(const std::string& channelid, const time_t starttime, const time_t duration, const std::string& title, int prerecordseconds, int postrecordseconds, Json::Value& response);

  /**
   * \brief Delete a ForTheRecord schedule
   */
  int DeleteSchedule(const std::string& scheduleid);

  /**
   * \brief Get the upcoming programs for a given schedule
   */
  int GetUpcomingProgramsForSchedule(const Json::Value& schedule, Json::Value& response);

  /*
   * \brief Get the list with TV channel groups from 4TR
   */
  int RequestTVChannelGroups(Json::Value& response);

    /*
   * \brief Get the list with Radio channel groups from 4TR
   */
  int RequestRadioChannelGroups(Json::Value& response);

  /*
   * \brief Get the list with channels for the given channel group from 4TR
   * \param channelGroupId GUID of the channel group
   */
  int RequestChannelGroupMembers(const std::string& channelGroupId, Json::Value& response);

  /*
   * \brief Get the logo for a channel
   * \param channelGUID GUID of the channel
   */
  std::string GetChannelLogo(const std::string& channelGUID);

  time_t WCFDateToTimeT(const std::string& wcfdate, int& offset);
  std::string TimeTToWCFDate(const time_t thetime);

} //namespace ForTheRecord
