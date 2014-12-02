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
#include "PlexMediaDecisionEngine.h"
#include "Client/PlexServerVersion.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "AdvancedSettings.h"
#include "Client/PlexTranscoderClientRPi.h"
#include "log.h"

#include <map>

typedef std::map<std::string, std::string> str2str;

static str2str _resolutions = boost::assign::list_of<std::pair<std::string, std::string> >
  ("64", "220x180") ("96", "220x128") ("208", "284x160") ("320", "420x240") ("720", "576x320") ("1500", "720x480") ("2000", "1024x768")
  ("3000", "1280x720") ("4000", "1280x720") ("8000", "1920x1080") ("10000", "1920x1080") ("12000", "1920x1080") ("20000", "1920x1080");

static str2str _qualities = boost::assign::list_of<std::pair<std::string, std::string> >
  ("64", "10") ("96", "20") ("208", "30") ("320", "30") ("720", "40") ("1500", "60") ("2000", "60")
  ("3000", "75") ("4000", "100") ("8000", "60") ("10000", "75") ("12000", "90") ("20000", "100");

CPlexTranscoderClient *CPlexTranscoderClient::_Instance = NULL;

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
int CPlexTranscoderClient::getIntegerRepresentation(int qualitySetting)
{
  switch (qualitySetting)
  {
    case PLEX_ONLINE_QUALITY_1080p:
      return 1080;
    case PLEX_ONLINE_QUALITY_720p:
      return 720;
    case PLEX_ONLINE_QUALITY_480p:
      return 480;
    case PLEX_ONLINE_QUALITY_SD:
      return 320;
    default:
      return -1;
  }
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
int CPlexTranscoderClient::autoSelectQuality(const CFileItem& file, int target)
{
  int selectedMediaItem = 0;
  int onlineQuality = CPlexTranscoderClient::getIntegerRepresentation(target);

  // Try to pick something that's equal or less than the preferred resolution.
  std::map<int, int> qualityMap;
  std::vector<int> qualities;
  int sd = getIntegerRepresentation(PLEX_ONLINE_QUALITY_SD);

  for (size_t i = 0; i < file.m_mediaItems.size(); i++)
  {
    CFileItemPtr item = file.m_mediaItems[i];
    CStdString videoRes =
        CStdString(item->GetProperty("mediaTag-videoResolution").asString()).ToUpper();

    // Compute the quality, subsequent SDs get lesser values, assuming they're ordered
    // descending.
    int q = sd;
    if (videoRes != "SD" && videoRes.empty() == false)
    {
      try { q = boost::lexical_cast<int>(videoRes); }
      catch (...) { }
    }
    else
    {
      sd -= 10;
    }

    qualityMap[q] = i;
    qualities.push_back(q);
  }

  // Sort on quality descending.
  std::sort(qualities.begin(), qualities.end());
  std::reverse(qualities.begin(), qualities.end());

  int pickedIndex = qualities[qualities.size() - 1];
  BOOST_FOREACH(int q, qualities)
  {
    if (q <= onlineQuality)
    {
      pickedIndex = qualityMap[q];
      selectedMediaItem = file.m_mediaItems[pickedIndex]->GetProperty("id").asInteger();
      break;
    }
  }
  return selectedMediaItem;
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
        if (s && s->IsComplete() && s->SupportsVideoTranscoding())
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
  
  if (server && server->SupportsVideoTranscoding())
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
  return boost::lexical_cast<std::string>(local ? localBitrate() : remoteBitrate());
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

  int bitrate = item.GetProperty("bitrate").asInteger();
  int transcodeSetting;
  
  if (server->GetActiveConnection()->IsLocal())
    transcodeSetting = localBitrate();
  else
    transcodeSetting = remoteBitrate();
  
  // temporary force HEVC to transcode
  if (item.GetProperty("mediatag-videocodec").asString() == "hevc")
    return true;

  if (transcodeForced())
    return transcodeSetting != 0;
  else
    return transcodeSetting ? transcodeSetting < bitrate : false;
  
  return false;
}

///////////////////////////////////////////////////////////////////////////////
typedef std::pair<std::string, std::string> stringPair;
CURL CPlexTranscoderClient::GetTranscodeURL(CPlexServerPtr server, const CFileItem& item)
{
  bool isLocal = server->GetActiveConnection()->IsLocal();

  CURL tURL;

  switch (getServerTranscodeMode(server))
  {
    case PLEX_TRANSCODE_MODE_HLS:
    {
      tURL = server->BuildURL("/video/:/transcode/universal/start.m3u8");
      tURL.SetOption("protocol", "hls");
      tURL.SetOption("fastSeek", "1");

      /* since we are passing the URL to FFMPEG we need to pass our
       * headers as well */
      std::vector<stringPair> hdrs = XFILE::CPlexFile::GetHeaderList();
      BOOST_FOREACH(stringPair p, hdrs)
      {
        // Let's ignore X-Plex-Client-Capabilities, ffmpeg is cranky about {}
        if (p.first == "X-Plex-Client-Capabilities")
          continue;

        tURL.SetOption(p.first, p.second);
      }
      break;
    }

    case PLEX_TRANSCODE_MODE_MKV:
    {
      tURL = server->BuildPlexURL("/video/:/transcode/universal/start.mkv");
      tURL.SetOption("protocol", "http");
      tURL.SetOption("copyts", "1");

      if (item.HasProperty("viewOffset") &&
          item.m_lStartOffset == STARTOFFSET_RESUME)
      {
        int offset = item.GetProperty("viewOffset").asInteger() / 1000;
        tURL.SetOption("offset", boost::lexical_cast<std::string>(offset));
      }
      else if (item.HasProperty("viewOffsetSeek"))
      {
        // Here we handle seek offset for Matroska seeking
        // Define transcoder start point
        int offset = item.GetProperty("viewOffsetSeek").asInteger() / 1000;
        tURL.SetOption("offset", boost::lexical_cast<std::string>(offset));
      }
      break;
    }

    default:
    {
      CLog::Log(LOGERROR,"CPlexTranscoderClient::GetTranscodeURL : item Transcode mode is unknown");
      break;
    }
  }

  tURL.SetOption("path", item.GetProperty("unprocessed_key").asString());
  tURL.SetOption("session", g_guiSettings.GetString("system.uuid"));
  tURL.SetOption("directPlay", "0");
  tURL.SetOption("directStream", "1");


  CFileItemPtr mediaItem = CPlexMediaDecisionEngine::getSelectedMediaItem(item);
  if (mediaItem)
    tURL.SetOption("mediaIndex", mediaItem->GetProperty("mediaIndex").asString());

  if (mediaItem->m_selectedMediaPart)
    tURL.SetOption("partIndex", mediaItem->m_selectedMediaPart->GetProperty("partIndex").asString());
  
  std::string bitrate = GetInstance()->GetCurrentBitrate(isLocal);

  // if we have no bitrate setting and still want to transcode
  // default to 20 mbps
  if (bitrate == "0")
    bitrate = "20000";

  tURL.SetOption("maxVideoBitrate", bitrate);
  tURL.SetOption("videoQuality", _qualities[bitrate]);
  tURL.SetOption("videoResolution", _resolutions[bitrate]);

  /* PHT can render subtitles itself no need to include them in the transcoded video
   * UNLESS it's a embedded subtitle, we can't extract it from the file or UNLESS the
   * user have checked the always transcode subtitles option in settings */
  if (!transcodeSubtitles())
  {
    CFileItemPtr subStream = PlexUtils::GetItemSelectedStreamOfType(item, PLEX_STREAM_SUBTITLE);
    if (subStream && subStream->HasProperty("key"))
    {
      CLog::Log(LOGDEBUG, "CPlexTranscoderClient::GetTranscodeURL file has a selected subtitle that is external.");
      tURL.SetOption("skipSubtitles", "1");
    }
  }
  
  return tURL;
}

///////////////////////////////////////////////////////////////////////////////
std::string CPlexTranscoderClient::GetCurrentSession()
{
  return g_guiSettings.GetString("system.uuid");
}

///////////////////////////////////////////////////////////////////////////////
CPlexTranscoderClient::PlexTranscodeMode CPlexTranscoderClient::getServerTranscodeMode(const CPlexServerPtr& server)
{
  if (!server)
    return PLEX_TRANSCODE_MODE_NONE;
  
  // This is a ugly work-around, since OSX doesn't support transcoding in PHT
  // we need to force the HLS mode, otherwise there can be certain codec
  // combinations that will not work correctly. This should be removed
  // when we merge with the next version of XBMC
#ifdef TARGET_DARWIN_OSX
  if (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_IEC958)
    return PLEX_TRANSCODE_MODE_HLS;
#endif

  if (g_advancedSettings.m_bUseMatroskaTranscodes)
  {
    CPlexServerVersion serverVersion(server->GetVersion());
    CPlexServerVersion needVersion("0.9.10.0.0-abc123");
    if (serverVersion > needVersion)
      return PLEX_TRANSCODE_MODE_MKV;
  }

  // default to HLS
  return PLEX_TRANSCODE_MODE_HLS;
}

///////////////////////////////////////////////////////////////////////////////
CPlexTranscoderClient::PlexTranscodeMode CPlexTranscoderClient::getItemTranscodeMode(const CFileItem& item)
{
  if (item.GetProperty("plexDidTranscode").asBoolean(false) == false)
    return PLEX_TRANSCODE_MODE_NONE;

  CFileItemPtr pItem(new CFileItem(item));
  CPlexServerPtr pServer = g_plexApplication.serverManager->FindFromItem(pItem);
  return getServerTranscodeMode(pServer);
}

///////////////////////////////////////////////////////////////////////////////
CPlexTranscoderClient *CPlexTranscoderClient::GetInstance()
{
   if (!_Instance)
  {
#if defined(TARGET_RASPBERRY_PI)
    _Instance = new CPlexTranscoderClientRPi();
#else
    _Instance = new CPlexTranscoderClient();
#endif
  }
  return _Instance;
}

///////////////////////////////////////////////////////////////////////////////
void CPlexTranscoderClient::DeleteInstance()
{
  if (_Instance)
    delete _Instance;
}
