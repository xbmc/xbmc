//
//  PlexTranscoderClient.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-08-01.
//
//

#include <string>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/assign.hpp>

#include "PlexTranscoderClient.h"
#include "settings/GUISettings.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "plex/PlexUtils.h"
#include "Client/PlexConnection.h"
#include "FileSystem/PlexFile.h"
#include "Client/PlexServerManager.h"
#include "Client/PlexServer.h"

#include "log.h"

#include <map>

typedef std::map<std::string, std::string> str2str;

static str2str _resolutions = boost::assign::list_of<std::pair<std::string, std::string> >
  ("64", "220x180") ("96", "220x128") ("208", "284x160") ("320", "420x240") ("720", "576x320") ("1500", "720x480") ("2000", "1024x768")
  ("3000", "1280x720") ("4000", "1280x720") ("8000", "1920x1080") ("10000", "1920x1080") ("12000", "1920x1080") ("20000", "1920x1080");

static str2str _qualities = boost::assign::list_of<std::pair<std::string, std::string> >
  ("64", "10") ("96", "20") ("208", "30") ("320", "30") ("720", "40") ("1500", "60") ("2000", "60")
  ("3000", "75") ("4000", "100") ("8000", "60") ("10000", "75") ("12000", "90") ("20000", "100");


///////////////////////////////////////////////////////////////////////////////////////////////////
PlexIntStringMap CPlexTranscoderClient::getOnlineQualties()
{
  PlexIntStringMap qual;
  qual[PLEX_ONLINE_QUALITY_ALWAYS_ASK] = g_localizeStrings.Get(13181);
  qual[PLEX_ONLINE_QUALITY_1080p] = g_localizeStrings.Get(13182);
  qual[PLEX_ONLINE_QUALITY_720p] = g_localizeStrings.Get(13183);
  qual[PLEX_ONLINE_QUALITY_480p] = g_localizeStrings.Get(13184);
  qual[PLEX_ONLINE_QUALITY_SD] = g_localizeStrings.Get(13185);

  return qual;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int CPlexTranscoderClient::getBandwidthForQuality(int quality)
{
  switch (quality)
  {
    case PLEX_ONLINE_QUALITY_SD:
      return 50 * 8;
    case PLEX_ONLINE_QUALITY_480p:
      return 150 * 8;
    case PLEX_ONLINE_QUALITY_720p:
      return 400 * 8;
    case PLEX_ONLINE_QUALITY_1080p:
      return INT_MAX;
    default:
      return 400 * 8;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int CPlexTranscoderClient::SelectAOnlineQuality(int currentQuality)
{
  PlexIntStringMap qualities = getOnlineQualties();

  CGUIDialogSelect* select = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (!select)
    return currentQuality;

  int idx = 0;
  BOOST_FOREACH(PlexIntStringPair p, qualities)
  {
    select->Add(p.second);
    if (p.first == currentQuality)
      select->SetSelected(idx);

    idx++;
  }

  select->DoModal();
  return select->GetSelectedLabel();
}

///////////////////////////////////////////////////////////////////////////////
int CPlexTranscoderClient::SelectATranscoderQuality(CPlexServerPtr server, int currentQuality)
{
  if (!server)
  {
    server = g_plexApplication.serverManager->GetBestServer();
    if (!server || !server->SupportsVideoTranscoding())
    {
      server.reset();

      PlexServerList allServers = g_plexApplication.serverManager->GetAllServers();
      BOOST_FOREACH(CPlexServerPtr s, allServers)
      {
        if (s->IsComplete() && s->SupportsVideoTranscoding())
        {
          server = s;
          break;
        }
      }
    }
  }

  std::vector<std::string> qualities;

  if (server)
    qualities = server->GetTranscoderBitrates();
  
  CGUIDialogSelect *select = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  select->Add(g_localizeStrings.Get(42999));
  
  if (currentQuality == 0)
    select->SetSelected(0);
  
  if (server->SupportsVideoTranscoding())
  {
    int idx = 1;
    BOOST_FOREACH(std::string qual, qualities)
    {
      int qualint = 0;
      try { qualint = boost::lexical_cast<int>(qual); }
      catch (...) {}

      select->Add(GetPrettyBitrate(qualint));
      if (currentQuality == qualint)
        select->SetSelected(idx);
      
      idx++;
    }
  }
  
  select->DoModal();
  int newqual = select->GetSelectedLabel();
  if (newqual > 0)
  {
    try {
      return boost::lexical_cast<int>(qualities[newqual-1]);
    } catch (...) {
      return currentQuality;
    }
  }
  
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
std::string CPlexTranscoderClient::GetCurrentBitrate(bool local)
{
  return boost::lexical_cast<std::string>(g_guiSettings.GetInt(local ? "plexmediaserver.localquality" : "plexmediaserver.remotequality"));
}

///////////////////////////////////////////////////////////////////////////////
std::string CPlexTranscoderClient::GetPrettyBitrate(int rawbitrate)
{
  if (rawbitrate < 1000)
    return boost::lexical_cast<std::string>(rawbitrate) + " kbps";
  else
    return boost::lexical_cast<std::string>(rawbitrate / 1000) + " Mbps";
}

///////////////////////////////////////////////////////////////////////////////
bool CPlexTranscoderClient::ShouldTranscode(CPlexServerPtr server, const CFileItem& item)
{
  if (!item.IsVideo())
    return false;

  if (!server || !server->GetActiveConnection())
    return false;
  
  if (server->GetActiveConnection()->IsLocal())
    return g_guiSettings.GetInt("plexmediaserver.localquality") != 0;
  else
    return g_guiSettings.GetInt("plexmediaserver.remotequality") != 0;
}

///////////////////////////////////////////////////////////////////////////////
typedef std::pair<std::string, std::string> stringPair;
CURL CPlexTranscoderClient::GetTranscodeURL(CPlexServerPtr server, const CFileItem& item)
{
  bool isLocal = server->GetActiveConnection()->IsLocal();
  
  /* Note that we are building a HTTP URL here, because XBMC will pass the
   * URL directly to FFMPEG, and as we all know, ffmpeg doesn't handle
   * plexserver:// protocol */
  CURL tURL = server->BuildURL("/video/:/transcode/universal/start.m3u8");
  
  tURL.SetOption("path", item.GetProperty("unprocessed_key").asString());
  tURL.SetOption("session", g_guiSettings.GetString("system.uuid"));
  tURL.SetOption("protocol", "hls");
  tURL.SetOption("directPlay", "0");
  tURL.SetOption("directStream", "1");
  
  /*
  if (item.HasProperty("viewOffset"))
  {
    int offset = item.GetProperty("viewOffset").asInteger() / 1000;
    tURL.SetOption("offset", boost::lexical_cast<std::string>(offset));
  }*/
  tURL.SetOption("fastSeek", "1");
  
  std::string bitrate = GetCurrentBitrate(isLocal);
  tURL.SetOption("maxVideoBitrate", bitrate);
  tURL.SetOption("videoQuality", _qualities[bitrate]);
  tURL.SetOption("videoResolution", _resolutions[bitrate]);
  
  /* PHT can render subtitles itself no need to include them in the transcoded video
   * UNLESS it's a embedded subtitle, we can't extract it from the file */
  CFileItemPtr subStream = PlexUtils::GetItemSelectedStreamOfType(item, PLEX_STREAM_SUBTITLE);
  if (subStream && subStream->HasProperty("key"))
  {
    CLog::Log(LOGDEBUG, "CPlexTranscoderClient::GetTranscodeURL file has a selected subtitle that is external.");
    tURL.SetOption("skipSubtitles", "1");
  }
  
  CStdString extraAudioFormats;
  int audioMode = g_guiSettings.GetInt("audiooutput.mode");
  
  if (AUDIO_IS_BITSTREAM(audioMode))
  {
    if (g_guiSettings.GetBool("audiooutput.ac3passthrough"))
    {
      extraAudioFormats += "add-transcode-target-audio-codec(type=videoProfile&context=streaming&protocol=hls&audioCodec=ac3)";
      extraAudioFormats += "+add-transcode-target-audio-codec(type=videoProfile&context=streaming&protocol=hls&audioCodec=eac3)";
    }
    
    if (g_guiSettings.GetBool("audiooutput.dtspassthrough"))
    {
      if (!extraAudioFormats.empty())
        extraAudioFormats+="+";
      extraAudioFormats += "add-transcode-target-audio-codec(type=videoProfile&context=streaming&protocol=hls&audioCodec=dca)";
    }
    
    if (!extraAudioFormats.empty())
      tURL.SetProtocolOption("X-Plex-Client-Profile-Extra", extraAudioFormats);
  }
  
  /* since we are passing the URL to FFMPEG we need to pass our 
   * headers as well */
  std::vector<stringPair> hdrs = XFILE::CPlexFile::GetHeaderList();
  BOOST_FOREACH(stringPair p, hdrs)
    tURL.SetOption(p.first, p.second);
  
  return tURL;
}

///////////////////////////////////////////////////////////////////////////////
CURL CPlexTranscoderClient::GetTranscodeStopURL(CPlexServerPtr server)
{
  CURL url = server->BuildPlexURL("/video/:/transcode/universal/stop");
  url.SetOption("session", g_guiSettings.GetString("system.uuid"));
  return url;
}
