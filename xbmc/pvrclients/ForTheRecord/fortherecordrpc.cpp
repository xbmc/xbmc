/**
 * \brief  Proof of concept code to access ForTheRecord's REST api using C++ code
 * \author Marcel Groothuis, Fred Hoogduin
 ***************************************************************************
 *      Copyright (C) 2010-2011 Marcel Groothuis, Fred Hoogduin
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***************************************************************************
 * Depends on:
 * - libcurl: http://curl.haxx.se/
 * - jsoncpp: http://jsoncpp.sourceforge.net/
 *
 * Tested under Windows and Linux
 */

#include <stdio.h>
#include <sys/stat.h>
#include "libPlatform/os-dependent.h"
#include "curl/curl.h"
#include "client.h"
#include "pvrclient-fortherecord.h"
#include "utils.h"
#include "fortherecordrpc.h"
#include "CriticalSection.h"
#include "AutoLock.h"

using namespace ADDON;

// Some version dependent API strings
#define FTR_GETEPG_45 "ForTheRecord/Guide/FullPrograms/%s/%i-%02i-%02iT%02i:%02i:%02i/%i-%02i-%02iT%02i:%02i:%02i/false"


// set l_logCurl to true to enable detailed protocol info logging by CURL
static bool l_logCurl = false;

/**
 * \brief CURL callback function that receives the return data from the HTTP get/post calls
 */
static size_t curl_write_data(void *buffer, size_t size, size_t nmemb, void *stream)
{
  //XBMC->Log(LOG_DEBUG, "\nwrite_data size=%i, nmemb=%i\n", size, nmemb, (char*) buffer);

  // Calculate the real size of the incoming buffer
  size_t realsize = size * nmemb;

  std::string* response = (std::string*) stream;
  // Dirty... needs some checking
  *response += (char*) buffer;

  return realsize;
}

/**
 * \brief CURL callback function that receives the return data from the HTTP get/post calls and writes it to a file
 */
static size_t curl_write_data_to_file(void *buffer, size_t size, size_t nmemb, void *stream)
{
  //XBMC->Log(LOG_DEBUG, "\nwrite_data size=%i, nmemb=%i\n", size, nmemb, (char*) buffer);

  int written = fwrite(buffer, size, nmemb, (FILE *) stream);
  return written;
}

/**
 * \brief CURL debug callback function
 */
static int my_curl_debug_callback(CURL* curl, curl_infotype infotype, char* data, size_t size, void* p)
{
  char *pch = new char[size +1];
    strncpy(pch, data, size);
    pch[size] = '\0';
  switch(infotype)
  {
  case CURLINFO_TEXT:
    XBMC->Log(LOG_DEBUG, "CURL info    : %s", pch);
    break;
  case CURLINFO_DATA_OUT:
    XBMC->Log(LOG_DEBUG, "CURL data-out: %s", pch);
    break;
  case CURLINFO_DATA_IN:
    XBMC->Log(LOG_DEBUG, "CURL data-in : %s", pch);
    break;
  default:
    // do nothing
    break;
  }
  delete [] pch;
  return 0;
}

/**
 * \brief Namespace with ForTheRecord related code
 */
namespace ForTheRecord
{
  CCriticalSection communication_mutex;

  /**
   * \brief Do some internal housekeeping at the start
   */
  void Initialize(void)
  {
    // due to lack of static constructors...
    communication_mutex.Initialize();
  }


  // The usable urls:
  //http://localhost:49943/ForTheRecord/Control/help
  //http://localhost:49943/ForTheRecord/Scheduler/help
  //http://localhost:49943/ForTheRecord/Guide/help
  //http://localhost:49943/ForTheRecord/Core/help
  //http://localhost:49943/ForTheRecord/Configuration/help
  //http://localhost:49943/ForTheRecord/Log/help

  int ForTheRecordRPC(const std::string& command, const std::string& arguments, std::string& json_response)
  {
    CURL *curl;
    CURLcode res;
    std::string url = g_szBaseURL + command;
    CAutoLock critsec(&communication_mutex);

    XBMC->Log(LOG_DEBUG, "URL: %s\n", url.c_str());

    curl = curl_easy_init();

    if(curl)
    {
      struct curl_slist *chunk = NULL;

      chunk = curl_slist_append(chunk, "Content-type: application/json; charset=UTF-8");
      chunk = curl_slist_append(chunk, "Accept: application/json; charset=UTF-8");

      /* Specify the URL */
      if ((res = curl_easy_setopt(curl, CURLOPT_URL, url.c_str())) != CURLE_OK)
        XBMC->Log(LOG_NOTICE, "curl_easy_setop CURLOPT_URL returned %d (%s).\n", res, curl_easy_strerror(res));
      if ((res = curl_easy_setopt(curl, CURLOPT_TIMEOUT, g_iConnectTimeout)) != CURLE_OK)
        XBMC->Log(LOG_NOTICE, "curl_easy_setop CURLOPT_TIMEOUT returned %d (%s).\n", res, curl_easy_strerror(res));
      if ((res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk)) != CURLE_OK)
        XBMC->Log(LOG_NOTICE, "curl_easy_setop CURLOPT_HTTPHEADER returned %d (%s).\n", res, curl_easy_strerror(res));
      /* Now specify the POST data */
      if ((res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, arguments.c_str())) != CURLE_OK)
        XBMC->Log(LOG_NOTICE, "curl_easy_setop CURLOPT_POSTFIELDS returned %d (%s).\n", res, curl_easy_strerror(res));
      /* Define our callback to get called when there's data to be written */ 
      if ((res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data)) != CURLE_OK)
        XBMC->Log(LOG_NOTICE, "curl_easy_setop CURLOPT_WRITEFUNCTION returned %d (%s).\n", res, curl_easy_strerror(res));
      /* Set a pointer to our struct to pass to the callback */ 
      json_response = "";
      if ((res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json_response)) != CURLE_OK)
        XBMC->Log(LOG_NOTICE, "curl_easy_setop CURLOPT_WRITEDATA returned %d (%s).\n", res, curl_easy_strerror(res));

      /* debugging only */
      if (l_logCurl)
      {
        if ((res = curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_curl_debug_callback)) != CURLE_OK)
          XBMC->Log(LOG_NOTICE, "curl_easy_setop CURLOPT_DEBUGFUNCTION returned %d (%s).\n", res, curl_easy_strerror(res));
        if ((res = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1)) != CURLE_OK)
          XBMC->Log(LOG_NOTICE, "curl_easy_setop CURLOPT_VERBOSE returned %d (%s).\n", res, curl_easy_strerror(res));
      }

      /* Perform the request */
      if ((res = curl_easy_perform(curl)) != CURLE_OK)
        XBMC->Log(LOG_NOTICE, "curl_easy_perform returned %d (%s).\n", res, curl_easy_strerror(res));

      /* always cleanup */
      curl_easy_cleanup(curl);
      return 0;
    }
    else
    {
      return E_FAILED;
    }
  }

  int ForTheRecordRPCToFile(const std::string& command, const std::string& arguments, std::string& filename, long& http_response)
  {
    CURL *curl;
    CURLcode res;
    std::string url = g_szBaseURL + command;
    CAutoLock critsec(&communication_mutex);

    XBMC->Log(LOG_DEBUG, "URL: %s writing to file %s\n", url.c_str(), filename.c_str());

    curl = curl_easy_init();

    if(curl)
    {
      struct curl_slist *chunk = NULL;

      chunk = curl_slist_append(chunk, "Content-type: application/json; charset=UTF-8");
      chunk = curl_slist_append(chunk, "Accept: application/json; charset=UTF-8");

      /* Specify the URL */
      if ((res = curl_easy_setopt(curl, CURLOPT_URL, url.c_str())) != CURLE_OK)
        XBMC->Log(LOG_NOTICE, "curl_easy_setop CURLOPT_URL returned %d (%s).\n", res, curl_easy_strerror(res));
      if ((res = curl_easy_setopt(curl, CURLOPT_TIMEOUT, g_iConnectTimeout)) != CURLE_OK)
        XBMC->Log(LOG_NOTICE, "curl_easy_setop CURLOPT_TIMEOUT returned %d (%s).\n", res, curl_easy_strerror(res));
      if ((res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk)) != CURLE_OK)
        XBMC->Log(LOG_NOTICE, "curl_easy_setop CURLOPT_HTTPHEADER returned %d (%s).\n", res, curl_easy_strerror(res));
      /* Now specify the POST data */
      if ((res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, arguments.c_str())) != CURLE_OK)
        XBMC->Log(LOG_NOTICE, "curl_easy_setop CURLOPT_POSTFIELDS returned %d (%s).\n", res, curl_easy_strerror(res));
      /* ask curl to write to file */ 
      if ((res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data_to_file)) != CURLE_OK)
        XBMC->Log(LOG_NOTICE, "curl_easy_setop CURLOPT_WRITEFUNCTION returned %d (%s).\n", res, curl_easy_strerror(res));
      /* Open the output file */
      FILE *ofile = fopen(filename.c_str(), "w+b");
      if (ofile == NULL)
      {
        XBMC->Log(LOG_ERROR, "can not open %s", filename.c_str());
        curl_easy_cleanup(curl);
        return E_FAILED;
      }
      if ((res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, ofile)) != CURLE_OK)
        XBMC->Log(LOG_NOTICE, "curl_easy_setop CURLOPT_WRITEDATA returned %d (%s).\n", res, curl_easy_strerror(res));

      /* debugging only */
      if (l_logCurl)
      {
        if ((res = curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_curl_debug_callback)) != CURLE_OK)
          XBMC->Log(LOG_NOTICE, "curl_easy_setop CURLOPT_DEBUGFUNCTION returned %d (%s).\n", res, curl_easy_strerror(res));
        if ((res = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1)) != CURLE_OK)
          XBMC->Log(LOG_NOTICE, "curl_easy_setop CURLOPT_VERBOSE returned %d (%s).\n", res, curl_easy_strerror(res));
      }

      /* Perform the request */
      if ((res = curl_easy_perform(curl)) != CURLE_OK)
        XBMC->Log(LOG_NOTICE, "curl_easy_perform returned %d (%s).\n", res, curl_easy_strerror(res));

      /* Retrieve HTTP response code */
      if ((res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_response)) != CURLE_OK)
        XBMC->Log(LOG_NOTICE, "curl_easy_getinfo CURLINFO_RESPONSE_CODE returned %d (%s).\n", res, curl_easy_strerror(res));

      /* close output file */
      fclose(ofile);

      /* always cleanup */
      curl_easy_cleanup(curl);
      return 0;
    }
    else
    {
      return E_FAILED;
    }
  }

  int ForTheRecordJSONRPC(const std::string& command, const std::string& arguments, Json::Value& json_response)
  {
    std::string response;
    int retval = E_FAILED;
    retval = ForTheRecordRPC(command, arguments, response);

    if (retval == CURLE_OK)
    {
#ifdef DEBUG
      // Print only the first 512 bytes, otherwise XBMC will crash...
      XBMC->Log(LOG_DEBUG, "Response: %s\n", response.substr(0,512).c_str());
#endif
      if (response.length() != 0)
      {
        Json::Reader reader;

        bool parsingSuccessful = reader.parse(response, json_response);

        if ( !parsingSuccessful )
        {
            XBMC->Log(LOG_DEBUG, "Failed to parse %s: \n%s\n", 
            response.c_str(),
            reader.getFormatedErrorMessages().c_str() );
            return E_FAILED;
        }
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Empty response");
        return E_EMPTYRESPONSE;
      }
#ifdef DEBUG
      printValueTree(stdout, json_response);
#endif
    }

    return retval;
  }

  /*
   * \brief Retrieve the TV channels that are in the guide
   */
  int RequestGuideChannelList()
  {
    Json::Value root;
    int retval = E_FAILED;

    retval = ForTheRecordJSONRPC("ForTheRecord/Guide/Channels/Television", "", root);

    if(retval >= 0)
    {
      if( root.type() == Json::arrayValue)
      {
        int size = root.size();

        // parse channel list
        for ( int index =0; index < size; ++index )
        {
          std::string name = root[index]["Name"].asString();
          XBMC->Log(LOG_DEBUG, "Found channel %i: %s\n", index, name.c_str());
        }
        return size;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "RequestChannelList failed. Return value: %i\n", retval);
    }

    return retval;
  }

  /*
   * \brief Get the logo for a channel
   * \param channelGUID GUID of the channel
   */
  std::string GetChannelLogo(const std::string& channelGUID)
  {
#if defined(TARGET_WINDOWS)
    char tmppath[MAX_PATH];
    GetTempPath(MAX_PATH, tmppath);
#elif defined(TARGET_LINUX) || defined(TARGET_OSX)
    std::string tmppath = "/tmp/";
#else
#error implement for your OS!
#endif
    std::string finalpath = tmppath;
    finalpath += channelGUID;
    std::string path = finalpath;
    finalpath += ".png";
    path += ".$$$";

    struct tm* modificationtime;
    struct stat buf;
    if (stat(finalpath.c_str(), &buf) != -1)
    {
      modificationtime = localtime(&buf.st_mtime);
    }
    else
    {
      time_t prehistoric = 0;
      modificationtime = localtime(&prehistoric);
    }

    char command[512];

    snprintf(command, 512, "ForTheRecord/Scheduler/ChannelLogo/%s/100/100/false/%d-%02d-%02d", channelGUID.c_str(), 
    //snprintf(command, 512, "ForTheRecord/Scheduler/ChannelLogo/%s/100/100/false/2011-01-01", channelGUID.c_str(), 
      modificationtime->tm_year + 1900, modificationtime->tm_mon + 1, modificationtime->tm_mday);

    long http_response;
    int retval = ForTheRecordRPCToFile(command, "", path, http_response);
    if (retval != 0)
    {
      XBMC->Log(LOG_ERROR, "couldn't retrieve the temporary channel logo file %s.\n", path.c_str());
      return "";
    }

    if (http_response == 200)
    {
      (void) remove(finalpath.c_str());
      if (rename(path.c_str(), finalpath.c_str()) == -1)
      {
        XBMC->Log(LOG_ERROR, "couldn't rename temporary channel logo file %s to %s.\n", path.c_str(), finalpath.c_str());
        finalpath = "";
      }
    }
    else
    {
      // cleanup temporary file
      if (remove(path.c_str()) == -1)
        XBMC->Log(LOG_ERROR, "couldn't delete temporary channel logo file %s.\n", path.c_str());
      // so was the logo not there (204) or was our local version still valid (304)?
      if (http_response == 204)
      {
        finalpath = "";
      }
    }

    return finalpath;
  }

  /*
   * \brief Get the list with channel groups from 4TR
   * \param channelType The channel type (Television or Radio)
   */
  int RequestChannelGroups(enum ChannelType channelType, Json::Value& response)
  {
    int retval = -1;
        
    if (channelType == Television)
    {
      retval = ForTheRecordJSONRPC("ForTheRecord/Scheduler/ChannelGroups/Television", "?visibleOnly=false", response);
    }
    else if (channelType == Radio)
    {
      retval = ForTheRecordJSONRPC("ForTheRecord/Scheduler/ChannelGroups/Radio", "?visibleOnly=false", response);
    }
        
    if(retval >= 0)
    {
      if( response.type() == Json::arrayValue)
      {
        int size = response.size();
        return size;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "RequestChannelGroups failed. Return value: %i\n", retval);
    }

    return retval;
  }

  /*
   * \brief Get the list with channels for the given channel group from 4TR
   * \param channelGroupId GUID of the channel group
   */
  int RequestChannelGroupMembers(const std::string& channelGroupId, Json::Value& response)
  {
    int retval = -1;

    std::string command = "ForTheRecord/Scheduler/ChannelsInGroup/" + channelGroupId;
    retval = ForTheRecordJSONRPC(command, "", response);

    if(retval >= 0)
    {
      if( response.type() == Json::arrayValue)
      {
        int size = response.size();
        return size;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_ERROR, "RequestChannelGroupMembers failed. Return value: %i\n", retval);
    }
        
    return retval;
  }
    
  /*
   * \brief Get the list with TV channel groups from 4TR
   */
  int RequestTVChannelGroups(Json::Value& response)
  {
    return RequestChannelGroups(Television, response);
  }
    
  /*
   * \brief Get the list with Radio channel groups from 4TR
   */
  int RequestRadioChannelGroups(Json::Value& response)
  {
    return RequestChannelGroups(Radio, response);
  }

  /*
   * \brief Get the list with channels from 4TR
   * \param channelType The channel type (Television or Radio)
   */
  int GetChannelList(enum ChannelType channelType, Json::Value& response)
  {
    int retval = -1;

    if (channelType == Television)
    {
      retval = ForTheRecordJSONRPC("ForTheRecord/Scheduler/Channels/Television", "?visibleOnly=false", response);
    }
    else if (channelType == Radio)
    {
      retval = ForTheRecordJSONRPC("ForTheRecord/Scheduler/Channels/Radio", "?visibleOnly=false", response);        
    }

    if(retval >= 0)
    {           
      if( response.type() == Json::arrayValue)
      {
        int size = response.size();
        return size;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "RequestChannelList failed. Return value: %i\n", retval);
    }

    return retval;
  }

#if FALSE
  /**
   * \brief Fetch the Logo for the given channel id
   * \param channel_id       String containing the 4TR channel_id
   * \param filename         filename returned here
   */
  int GetChannelLogo(const std::string& channel_id, std::string& filename)
  {
    char command[512];

    snprintf(command, 512, "ForTheRecord/Scheduler/ChannelLogo/%s/100/100/false/2011-01-01", channel_id.c_str());
    
    filename = _tempnam(NULL, "ftrico");
    int retval = ForTheRecordRPCToFile(command, "", filename);

    return retval;
  }
#endif

  /*
   * \brief Ping core service.
   * \param requestedApiVersion The API version the client needs, pass in Constants.ForTheRecordRestApiVersion.
   * \return 0 if client and server are compatible, -1 if the client is too old, +1 if the client is newer than the server and -2 if the connection failed (server down?)
   */
  int Ping(int requestedApiVersion)
  {
    Json::Value response;
    char command[128];
    int version = -2;

    snprintf(command, 128, "ForTheRecord/Core/Ping/%i", requestedApiVersion);

    int retval = ForTheRecordJSONRPC(command, "", response);

    if (retval != E_FAILED)
    {
      if (response.type() == Json::intValue)
      {
        version = response.asInt();
      }
    }

    return version;
  }

  /**
   * \brief Returns information (free disk space) from all recording disks.
   */
  int GetRecordingDisksInfo(Json::Value& response)
  {
    XBMC->Log(LOG_DEBUG, "GetRecordingDisksInfo");
    int retval = ForTheRecordJSONRPC("ForTheRecord/Control/GetRecordingDisksInfo", "", response);

    if (response < 0)
    {
      XBMC->Log(LOG_ERROR, "GetRecordingDisksInfo failed");
    }
    return retval;
  }

  /**
   * \brief GetPluginServices Get all configured plugin services. {activeOnly} = Set to true to only receive active plugins. 
   * \brief Returns an array containing zero or more plugin services.
   * \param activeonly  set to true to only receive active plugins
   * \param response Reference to a std::string used to store the json response string
   * \return  0 when successful
   */
  int GetPluginServices(bool activeonly, Json::Value& response)
  {
    XBMC->Log(LOG_DEBUG, "GetPluginServices");
    int retval = E_FAILED;

    std::string args = activeonly ? "?activeOnly=true" : "?activeOnly=false";
    retval = ForTheRecord::ForTheRecordJSONRPC("ForTheRecord/Control/PluginServices", args, response);
    if(retval >= 0)
    {           
      if (response.type() != Json::arrayValue)
      {
        retval = E_FAILED;
        XBMC->Log(LOG_NOTICE, "GetPluginServices did not return a Json::arrayValue [%d].", response.type());
      }
    }
    else
    {
      XBMC->Log(LOG_NOTICE, "GetPluginServices remote call failed.");
    }
    return retval;
  }

  /**
   * \brief AreRecordingSharesAccessible
   * \param thisplugin the plugin to check
   * \param response Reference to a std::string used to store the json response string
   * \return  0 when successful
   */
  int AreRecordingSharesAccessible(Json::Value& thisplugin, Json::Value& response)
  {
    XBMC->Log(LOG_DEBUG, "AreRecordingSharesAccessible");
    Json::StyledWriter writer;
    std::string arguments = writer.write(thisplugin);

    int retval = ForTheRecordJSONRPC("ForTheRecord/Control/AreRecordingSharesAccessible", arguments, response);

    if (response.type() != Json::arrayValue)
    {
      // response on error is a objectValue
      // TODO: parse it to display the error
      return -1;
    }

    return retval;
  }

  int GetLiveStreams()
  {
    Json::Value response;
    int retval = ForTheRecordJSONRPC("ForTheRecord/Control/GetLiveStreams", "", response);

    if (retval != E_FAILED)
    {
      if (response.type() == Json::arrayValue)
      {
        // int size = response.size();

        // parse live stream list
        // for ( int index =0; index < size; ++index )
        // {
        // printf("Found live stream %i: %s\n", index, response["LiveStream"]["RtspUrl"].asString.c_str());
        // }
      }
    }
    return retval;
  }

  //Remember the last LiveStream object to be able to stop the stream again
  Json::Value g_current_livestream;

  int TuneLiveStream(const std::string& channel_id, ChannelType channeltype, std::string& stream)
  {
    // Send the channel object in json format, *and* a LiveStream object when there is a current 
    // LiveStream present.
    // FTR will answer with a LiveStream object.
    stream = "";

    char command[512];
      
    snprintf(command, 512, "{\"Channel\":{\"BroadcastStart\":\"\",\"BroadcastStop\":\"\",\"ChannelId\":\"%s\",\"ChannelType\":%i,\"DefaultPostRecordSeconds\":0,\"DefaultPreRecordSeconds\":0,\"DisplayName\":\"\",\"GuideChannelId\":\"00000000-0000-0000-0000-000000000000\",\"LogicalChannelNumber\":0,\"Sequence\":0,\"Version\":0,\"VisibleInGuide\":true},\"LiveStream\":",
      channel_id.c_str(), channeltype);
    std::string arguments = command;
    if (!g_current_livestream.empty())
    {
      Json::FastWriter writer;
      arguments.append(writer.write(g_current_livestream)).append("}");
    }
    else
    {
      arguments.append("null}");
    }

    XBMC->Log(LOG_DEBUG, "ForTheRecord/Control/TuneLiveStream, body [%s]", arguments.c_str());

    Json::Value response;
    int retval = ForTheRecordJSONRPC("ForTheRecord/Control/TuneLiveStream", arguments, response);

    if (retval != E_FAILED)
    {
      if (response.type() == Json::objectValue)
      {
        // First analyse the return code from the server
        ForTheRecord::LiveStreamResult livestreamresult = (ForTheRecord::LiveStreamResult) response["LiveStreamResult"].asInt();
        XBMC->Log(LOG_DEBUG, "TuneLiveStream result %d.", livestreamresult);
        if (livestreamresult != ForTheRecord::Succeed)
        {
          XBMC->Log(LOG_ERROR, "TuneLiveStream result %d.", livestreamresult);
          return E_FAILED;
        }

        // Ok, pick up the returned LiveStream object
        Json::Value livestream = response["LiveStream"];
        if (livestream != Json::nullValue)
        {
          g_current_livestream = livestream;
        }
        else
        {
          XBMC->Log(LOG_DEBUG, "No LiveStream received from server.");
          return E_FAILED;
        }
#ifdef TSREADER
        stream = g_current_livestream["TimeshiftFile"].asString();
#else
        stream = g_current_livestream["RtspUrl"].asString();
#endif
        XBMC->Log(LOG_DEBUG, "Tuned live stream: %s\n", stream.c_str());
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::objectValue");
        return E_FAILED;
      }
    }
    else
    {
      XBMC->Log(LOG_ERROR, "TuneLiveStream failed");
    }
    return retval;
  }


  int StopLiveStream()
  {
    if(!g_current_livestream.empty())
    {
      Json::StyledWriter writer;
      std::string arguments = writer.write(g_current_livestream);

      std::string response;
      int retval = ForTheRecordRPC("ForTheRecord/Control/StopLiveStream", arguments, response);

      g_current_livestream.clear();

      return retval;
    }
    else
    {
      return E_FAILED;
    }
  }

  std::string GetLiveStreamURL(void)
  {
    std::string stream = "";

    if(!g_current_livestream.empty())
    {
      stream = g_current_livestream["RtspUrl"].asString();
    }
    return stream;
  }

  int SignalQuality(Json::Value& response)
  {
    if(!g_current_livestream.empty())
    {
      Json::StyledWriter writer;
      std::string arguments = writer.write(g_current_livestream);

      int retval = ForTheRecordJSONRPC("ForTheRecord/Control/GetLiveStreamTuningDetails", arguments, response);

      //if (retval != E_FAILED)
      //{
      //  printValueTree(response);
      //}

      return retval;
    }
    else
    {
      return E_FAILED;
    }
  }

  bool KeepLiveStreamAlive()
  {
    //Example request:
    //{"CardId":"String content","Channel":{"BroadcastStart":"String content","BroadcastStop":"String content","ChannelId":"1627aea5-8e0a-4371-9022-9b504344e724","ChannelType":0,"DefaultPostRecordSeconds":2147483647,"DefaultPreRecordSeconds":2147483647,"DisplayName":"String content","GuideChannelId":"1627aea5-8e0a-4371-9022-9b504344e724","LogicalChannelNumber":2147483647,"Sequence":2147483647,"Version":2147483647,"VisibleInGuide":true},"RecorderTunerId":"1627aea5-8e0a-4371-9022-9b504344e724","RtspUrl":"String content","StreamLastAliveTime":"\/Date(928142400000+0200)\/","StreamStartedTime":"\/Date(928142400000+0200)\/","TimeshiftFile":"String content"}
    //Example response:
    //true
    if(!g_current_livestream.empty())
    {
      Json::StyledWriter writer;
      std::string arguments = writer.write(g_current_livestream);

      Json::Value response;
      int retval = ForTheRecordJSONRPC("ForTheRecord/Control/KeepLiveStreamAlive", arguments, response);

      if (retval != E_FAILED)
      {
        //if (response == "true")
        //{
        return true;
        //}
      }
    }

    return false;
  }

  int GetEPGData(const int backendversion, const std::string& guidechannel_id, struct tm epg_start, struct tm epg_end, Json::Value& response)
  {
    if ( guidechannel_id.length() > 0 )
    {
      char command[256];
      
      //Format: ForTheRecord/Guide/Programs/{guideChannelId}/{lowerTime}/{upperTime}
      snprintf(command, 256, FTR_GETEPG_45, 
               guidechannel_id.c_str(),
               epg_start.tm_year + 1900, epg_start.tm_mon + 1, epg_start.tm_mday,
               epg_start.tm_hour, epg_start.tm_min, epg_start.tm_sec,
               epg_end.tm_year + 1900, epg_end.tm_mon + 1, epg_end.tm_mday,
               epg_end.tm_hour, epg_end.tm_min, epg_end.tm_sec);

      int retval = ForTheRecordJSONRPC(command, "", response);

      return retval;
    }

    return E_FAILED;
  }

  int GetRecordingGroupByTitle(Json::Value& response)
  {
    XBMC->Log(LOG_DEBUG, "GetRecordingGroupByTitle");
    int retval = E_FAILED;
 
    retval = ForTheRecord::ForTheRecordJSONRPC("ForTheRecord/Control/RecordingGroups/Television/GroupByProgramTitle", "", response);
    if(retval >= 0)
    {           
      if (response.type() != Json::arrayValue)
      {
        retval = E_FAILED;
        XBMC->Log(LOG_NOTICE, "GetRecordingGroupByTitle did not return a Json::arrayValue [%d].", response.type());
      }
    }
    else
    {
      XBMC->Log(LOG_NOTICE, "GetRecordingGroupByTitle remote call failed.");
    }
    return retval;
  }

  int GetRecordingsForTitle(const std::string& title, Json::Value& response)
  {
    int retval = E_FAILED;
    CURL *curl;
    
    XBMC->Log(LOG_DEBUG, "GetRecordingsForTitle");

    curl = curl_easy_init();
    
    if(curl)
    {
      std::string command = "ForTheRecord/Control/RecordingsForProgramTitle/Television/";
      char* pch = curl_easy_escape(curl, title.c_str(), 0);
      command += pch;
      curl_free(pch);

      retval = ForTheRecord::ForTheRecordJSONRPC(command, "?includeNonExisting=false", response);
      if(retval >= 0)
      {           
        if (response.type() != Json::arrayValue)
        {
          retval = E_FAILED;
          XBMC->Log(LOG_NOTICE, "GetRecordingsForTitle did not return a Json::arrayValue [%d].", response.type());
        }
      }
      else
      {
        XBMC->Log(LOG_NOTICE, "GetRecordingsForTitle remote call failed.");
      }

      curl_easy_cleanup(curl);
    }
    return retval;
  }

  int GetRecordingById(const std::string& id, Json::Value& response)
  {
    int retval = E_FAILED;
    CURL *curl;

    XBMC->Log(LOG_DEBUG, "GetRecordingById");

    curl = curl_easy_init();

    if(curl)
    {
      std::string command = "ForTheRecord/Control/RecordingById/" + id;

      retval = ForTheRecord::ForTheRecordJSONRPC(command, "", response);

      curl_easy_cleanup(curl);
    }
    return retval;
  }

  int DeleteRecording(const std::string recordingfilename)
  {
    int retval = E_FAILED;
    CURL *curl;
    std::string response;

    XBMC->Log(LOG_DEBUG, "DeleteRecording");

    curl = curl_easy_init();

    if(curl)
    {
      std::string command = "ForTheRecord/Control/DeleteRecording?deleteRecordingFile=true";
      std::string arguments = recordingfilename;
      
      retval = ForTheRecord::ForTheRecordRPC(command, arguments, response);

      curl_easy_cleanup(curl);
    }

    return retval;
  }

  int GetScheduleById(const std::string& id, Json::Value& response)
  {
    int retval = E_FAILED;
    CURL *curl;

    XBMC->Log(LOG_DEBUG, "GetScheduleById");

    curl = curl_easy_init();

    if(curl)
    {
      std::string command = "ForTheRecord/Scheduler/ScheduleById/" + id;

      retval = ForTheRecord::ForTheRecordJSONRPC(command, "", response);
      if(retval >= 0)
      {           
        if (response.type() != Json::objectValue)
        {
          retval = E_FAILED;
          XBMC->Log(LOG_NOTICE, "GetScheduleById did not return a Json::objectValue [%d].", response.type());
        }
      }
      else
      {
        XBMC->Log(LOG_NOTICE, "GetScheduleById remote call failed.");
      }

      curl_easy_cleanup(curl);
    }
    return retval;
  }

  /**
   * \brief Fetch the detailed information of a guide program
   * \param id unique id (guid) of the program
   * \param response Reference to a std::string used to store the json response string
   */
  int GetProgramById(const std::string& id, Json::Value& response)
  {
    int retval = E_FAILED;
    CURL *curl;

    XBMC->Log(LOG_DEBUG, "GetProgramById");

    curl = curl_easy_init();

    if(curl)
    {
      std::string command = "ForTheRecord/Guide/Program/" + id;

      retval = ForTheRecord::ForTheRecordJSONRPC(command, "", response);
      if(retval >= 0)
      {           
        if (response.type() != Json::objectValue)
        {
          retval = E_FAILED;
          XBMC->Log(LOG_NOTICE, "GetProgramById did not return a Json::objectValue [%d].", response.type());
        }
      }
      else
      {
        XBMC->Log(LOG_NOTICE, "GetProgramById remote call failed.");
      }

      curl_easy_cleanup(curl);
    }
    return retval;
  }

  /**
   * \brief Fetch the list of schedules for tv or radio
   * \param channeltype  The type of channel to fetch the list for
   */
  int GetScheduleList(enum ChannelType channelType, Json::Value& response)
  {
    int retval = -1;

    XBMC->Log(LOG_DEBUG, "GetScheduleList");

    // http://madcat:49943/ForTheRecord/Scheduler/Schedules/0/82
    char command[256];
    
    //Format: ForTheRecord/Guide/Programs/{guideChannelId}/{lowerTime}/{upperTime}
    snprintf(command, 256, "ForTheRecord/Scheduler/Schedules/%i/%i" ,
      channelType, Recording );
    retval = ForTheRecordJSONRPC(command, "", response);

    if(retval >= 0)
    {
      if( response.type() == Json::arrayValue)
      {
        int size = response.size();
        return size;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "GetScheduleList failed. Return value: %i\n", retval);
    }

    return retval;
  }

  /**
   * \brief Fetch the list of upcoming programs
   */
  int GetUpcomingPrograms(Json::Value& response)
  {
    int retval = -1;

    XBMC->Log(LOG_DEBUG, "GetUpcomingPrograms");

    // http://madcat:49943/ForTheRecord/Scheduler/UpcomingPrograms/82?includeCancelled=true
    retval = ForTheRecordJSONRPC("ForTheRecord/Scheduler/UpcomingPrograms/82?includeCancelled=true", "", response);

    if(retval >= 0)
    {
      if( response.type() == Json::arrayValue)
      {
        int size = response.size();
        return size;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "GetUpcomingPrograms failed. Return value: %i\n", retval);
    }

    return retval;
  }

    /**
   * \brief Fetch the list of currently active recordings
   */
  int GetActiveRecordings(Json::Value& response)
  {
    int retval = -1;

    XBMC->Log(LOG_DEBUG, "GetActiveRecordings");

    retval = ForTheRecordJSONRPC("ForTheRecord/Control/ActiveRecordings", "", response);

    if(retval >= 0)
    {           
      if( response.type() == Json::arrayValue)
      {
        int size = response.size();
        return size;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "GetActiveRecordings failed. Return value: %i\n", retval);
    }

    return retval;
  }

  /**
   * \brief Cancel a currently active recording
   */
  int AbortActiveRecording(Json::Value& activeRecording)
  {
    int retval = -1;

    XBMC->Log(LOG_DEBUG, "AbortActiveRecording");

    Json::StyledWriter writer;
    std::string arguments = writer.write(activeRecording);

    std::string response;
    retval = ForTheRecordRPC("ForTheRecord/Control/AbortActiveRecording", arguments, response);

    if(retval != 0)
    {
      XBMC->Log(LOG_DEBUG, "AbortActiveRecording failed. Return value: %i\n", retval);
    }

    return retval;
  }


  /**
   * \brief Cancel an upcoming program
   */
  int CancelUpcomingProgram(const std::string& scheduleid, const std::string& channelid, const time_t starttime, const std::string& upcomingprogramid)
  {
    int retval = -1;
    std::string response;

    XBMC->Log(LOG_DEBUG, "CancelUpcomingProgram");
    struct tm* convert = gmtime(&starttime);
    struct tm tm_start = *convert;

    //Format: ForTheRecord/Scheduler/CancelUpcomingProgram/{scheduleId}/{channelId}/{startTime}?guideProgramId={guideProgramId}
    char command[256];
    snprintf(command, 256, "ForTheRecord/Scheduler/CancelUpcomingProgram/%s/%s/%i-%02i-%02iT%02i:%02i:%02i?guideProgramId=%s" ,
      scheduleid.c_str(), channelid.c_str(),
      tm_start.tm_year + 1900, tm_start.tm_mon + 1, tm_start.tm_mday,
      tm_start.tm_hour, tm_start.tm_min, tm_start.tm_sec,
      upcomingprogramid.c_str() );
    retval = ForTheRecordRPC(command, "", response);

    if (retval < 0)
    {
      XBMC->Log(LOG_DEBUG, "CancelUpcomingProgram failed. Return value: %i\n", retval);
    }

    return retval;
  }

  /**
   * \brief Add a xbmc timer as a one time schedule
   */
  int AddOneTimeSchedule(const std::string& channelid, const time_t starttime, const std::string& title, int prerecordseconds, int postrecordseconds, Json::Value& response)
  {
    int retval = -1;

    XBMC->Log(LOG_DEBUG, "AddOneTimeSchedule");
    struct tm* convert = localtime(&starttime);
    struct tm tm_start = *convert;

    // Format: ForTheRecord/Scheduler/SaveSchedule
    // argument: {"ChannelType":0,"IsActive":true,"IsOneTime":true,"KeepUntilMode":0,"KeepUntilValue":null,
    //    "LastModifiedTime":"\/Date(1297889326000+0100)\/","Name":"Astro TV","PostRecordSeconds":null,
    //    "PreRecordSeconds":null,"ProcessingCommands":[],"RecordingFileFormatId":null,
    //    "Rules":[{"Arguments":["Astro TV"],"Type":"TitleEquals"},{"Arguments":["2011-02-17T00:00:00+01:00"],"Type":"OnDate"},{"Arguments":["00:45:00"],"Type":"AroundTime"},{"Arguments":["ed49a4ef-5777-40c4-80b8-715e4c87f1a6"],"Type":"Channels"}],
    //    "ScheduleId":"00000000-0000-0000-0000-000000000000","SchedulePriority":0,"ScheduleType":82,"Version":0}

    time_t now = time(NULL);
    std::string modifiedtime = TimeTToWCFDate(mktime(localtime(&now)));
    char arguments[1024];
    snprintf( arguments, sizeof(arguments),
      "{\"ChannelType\":0,\"IsActive\":true,\"IsOneTime\":true,\"KeepUntilMode\":0,\"KeepUntilValue\":null,\"LastModifiedTime\":\"%s\",\"Name\":\"XBMC - %s\",\"PostRecordSeconds\":%i,\"PreRecordSeconds\":%i,\"ProcessingCommands\":[],\"RecordingFileFormatId\":null,\"Rules\":[{\"Arguments\":[\"%s\"],\"Type\":\"TitleEquals\"},{\"Arguments\":[\"%i-%02i-%02iT00:00:00\"],\"Type\":\"OnDate\"},{\"Arguments\":[\"%02i:%02i:%02i\"],\"Type\":\"AroundTime\"},{\"Arguments\":[\"%s\"],\"Type\":\"Channels\"}],\"ScheduleId\":\"00000000-0000-0000-0000-000000000000\",\"SchedulePriority\":0,\"ScheduleType\":82,\"Version\":0}",
      modifiedtime.c_str(), title.c_str(), postrecordseconds, prerecordseconds, title.c_str(),
      tm_start.tm_year + 1900, tm_start.tm_mon + 1, tm_start.tm_mday,
      tm_start.tm_hour, tm_start.tm_min, tm_start.tm_sec,
      channelid.c_str());

    retval = ForTheRecordJSONRPC("ForTheRecord/Scheduler/SaveSchedule", arguments, response);

    if(retval >= 0)
    {
      if( response.type() != Json::objectValue)
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::objectValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "AddOneTimeSchedule failed. Return value: %i\n", retval);
    }

    return retval;
  }

  /**
   * \brief Add a xbmc timer as a manual schedule
   */
  int AddManualSchedule(const std::string& channelid, const time_t starttime, const time_t duration, const std::string& title, int prerecordseconds, int postrecordseconds, Json::Value& response)
  {
    int retval = -1;

    XBMC->Log(LOG_DEBUG, "AddManualSchedule");
    struct tm* convert = localtime(&starttime);
    struct tm tm_start = *convert;
    time_t recordingduration = duration;
    int duration_sec = recordingduration % 60;
    recordingduration /= 60;
    int duration_min = recordingduration % 60;
    recordingduration /= 60;
    int duration_hrs = recordingduration;

    // Format: ForTheRecord/Scheduler/SaveSchedule
    // argument: {"ChannelType":0,"IsActive":true,"IsOneTime":true,"KeepUntilMode":0,"KeepUntilValue":null,
    //    "LastModifiedTime":"\/Date(1307645182000+0100)\/","Name":"XBMC (manual) - blup","PostRecordSeconds":600,
    //    "PreRecordSeconds":120,"ProcessingCommands":[],"RecordingFileFormatId":null,
    //    "Rules":[{"Arguments":["2011-06-11T22:10:00", "01:13:00"],"Type":"ManualSchedule"},{"Arguments":["6a14caaf-5e39-4750-b7b7-eae8c741c094"],"Type":"Channels"}],
    //    "ScheduleId":"00000000-0000-0000-0000-000000000000","SchedulePriority":0,"ScheduleType":82,"Version":0}

    time_t now = time(NULL);
    std::string modifiedtime = TimeTToWCFDate(mktime(localtime(&now)));
    char arguments[1024];
    snprintf( arguments, sizeof(arguments),
      "{\"ChannelType\":0,\"IsActive\":true,\"IsOneTime\":true,\"KeepUntilMode\":0,\"KeepUntilValue\":null,\"LastModifiedTime\":\"%s\",\"Name\":\"XBMC (manual) - %s\",\"PostRecordSeconds\":%i,\"PreRecordSeconds\":%i,\"ProcessingCommands\":[],\"RecordingFileFormatId\":null,"
      "\"Rules\":[{\"Arguments\":[\"%i-%02i-%02iT%02i:%02i:%02i\", \"%02i:%02i:%02i\"],\"Type\":\"ManualSchedule\"},{\"Arguments\":[\"%s\"],\"Type\":\"Channels\"}],\"ScheduleId\":\"00000000-0000-0000-0000-000000000000\",\"SchedulePriority\":0,\"ScheduleType\":82,\"Version\":0}",
      modifiedtime.c_str(), title.c_str(), postrecordseconds, prerecordseconds,
      tm_start.tm_year + 1900, tm_start.tm_mon + 1, tm_start.tm_mday,
      tm_start.tm_hour, tm_start.tm_min, tm_start.tm_sec,
      duration_hrs, duration_min, duration_sec,
      channelid.c_str());

    retval = ForTheRecordJSONRPC("ForTheRecord/Scheduler/SaveSchedule", arguments, response);

    if(retval >= 0)
    {
      if( response.type() != Json::objectValue)
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::objectValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "AddManualSchedule failed. Return value: %i\n", retval);
    }

    return retval;
  }

  /**
   * \brief Delete a ForTheRecord schedule
   */
  int DeleteSchedule(const std::string& scheduleid)
  {
    int retval = -1;
    std::string response;

    XBMC->Log(LOG_DEBUG, "DeleteSchedule");

    //Format: ForTheRecord/Scheduler/DeleteSchedule/d21ec04f-22e0-4bf8-accf-317ecc0fb0f9
    char command[256];
    snprintf(command, 256, "ForTheRecord/Scheduler/DeleteSchedule/%s" , scheduleid.c_str());
    retval = ForTheRecordRPC(command, "", response);

    if (retval < 0)
    {
      XBMC->Log(LOG_DEBUG, "DeleteSchedule failed. Return value: %i\n", retval);
    }

    return retval;
  }

  /**
   * \brief Get the upcoming programs for a given schedule
   */
  int GetUpcomingProgramsForSchedule(const Json::Value& schedule, Json::Value& response)
  {
    int retval = -1;

    XBMC->Log(LOG_DEBUG, "GetUpcomingProgramsForSchedule");

    char arguments[1024];
    Json::FastWriter writer;
    snprintf( arguments, sizeof(arguments), "{\"IncludeCancelled\":true,\"Schedule\":%s}", writer.write(schedule).c_str());

    retval = ForTheRecordJSONRPC("ForTheRecord/Scheduler/UpcomingProgramsForSchedule", arguments, response);

    if(retval >= 0)
    {
      if( response.type() == Json::arrayValue)
      {
        int size = response.size();
        return size;
      }
      else
      {
        XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
        return -1;
      }
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "GetUpcomingProgramsForSchedule failed. Return value: %i\n", retval);
    }

    return retval;
  }


  time_t WCFDateToTimeT(const std::string& wcfdate, int& offset)
  {
    time_t ticks;
    char offsetc;
    int offsetv;

    if (wcfdate.empty())
    {
      return 0;
    }

    //WCF compatible format "/Date(1290896700000+0100)/" => 2010-11-27 23:25:00
    ticks = atoi(wcfdate.substr(6, 10).c_str()); //only take the first 10 chars (fits in a 32-bit time_t value)
    offsetc = wcfdate[19]; // + or -
    offsetv = atoi(wcfdate.substr(20, 4).c_str());

    offset = (offsetc == '+' ? offsetv : -offsetv);

    return ticks;
  }

  std::string TimeTToWCFDate(const time_t thetime)
  {
    std::string wcfdate;

    wcfdate.clear();
    if (thetime != 0)
    {
      struct tm *gmTime;
      time_t localEpoch, gmEpoch;

      /*First get local epoch time*/
      localEpoch = time(NULL);

      /* Using local time epoch get the GM Time */
      gmTime = gmtime(&localEpoch);

      /* Convert gm time in to epoch format */
      gmEpoch = mktime(gmTime);

      /* get the absolute different between them */
      double utcoffset = difftime(localEpoch, gmEpoch);
      int iOffset = (int) utcoffset;

      time_t utctime = thetime - iOffset;

      iOffset = (iOffset / 36);

      char ticks[15], offset[8];
      snprintf(ticks, sizeof(ticks), "%010i", (int) utctime);
      snprintf(offset, sizeof(offset), "%s%04i", iOffset < 0 ? "-" : "+", abs(iOffset));
      char result[29];
      snprintf(result, sizeof(result), "\\/Date(%s000%s)\\/", ticks, offset );
      wcfdate = result;
    }
    return wcfdate;
  }
}

   
//TODO: implement all functionality for a XBMC PVR client
// Misc:
//------
// -GetDriveSpace
//   Return the Total and Free Drive space on the PVR Backend
// -GetBackendTime
//   The time at the PVR Backend side
//
// EPG
//-----
// -RequestEPGForChannel
//
// Channels
// -GetNumChannels
// -RequestChannelList
// -DeleteChannel (optional)
// -RenameChannel (optional)
// -MoveChannel (optional)
//
// Recordings
//------------
// -GetNumRecordings
// -RequestRecordingsList
// -DeleteRecording
// -RenameRecording
// -Cutmark functions  (optional)
// Playback:
// -OpenRecordedStream
// -CloseRecordedStream
// -ReadRecordedStream
// -PauseRecordedStream
//
// Timers (schedules)
//--------------------
// -GetNumTimers 
// -RequestTimerList
// -AddTimer
// -DeleteTimer
// -RenameTimer
// -UpdateTimer
//
// Live TV/Radio
// -OpenLiveStream
// -CloseLiveStream
// -ReadLiveStream (from TS buffer file)
// -PauseLiveStream
// -GetCurrentClientChannel
// -SwitchChannel
// -SignalQuality (optional)
// -GetLiveStreamURL (for RTSP based streaming)
