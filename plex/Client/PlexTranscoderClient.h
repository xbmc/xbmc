//
//  PlexTranscoderClient.h
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-08-01.
//
//

#ifndef __Plex_Home_Theater__PlexTranscoderClient__
#define __Plex_Home_Theater__PlexTranscoderClient__

#include "Client/PlexServer.h"
#include "FileItem.h"
#include "URL.h"

#define PLEX_ONLINE_QUALITY_ALWAYS_ASK 0
#define PLEX_ONLINE_QUALITY_1080p 1
#define PLEX_ONLINE_QUALITY_720p 2
#define PLEX_ONLINE_QUALITY_480p 3
#define PLEX_ONLINE_QUALITY_SD 4

class CPlexTranscoderClient
{
public:
  CPlexTranscoderClient();
  static int SelectATranscoderQuality(CPlexServerPtr server, int currentQuality = 0);
  static std::string GetPrettyBitrate(int br);
  static bool ShouldTranscode(CPlexServerPtr server, const CFileItem& item);
  static CURL GetTranscodeURL(CPlexServerPtr server, const CFileItem& item);
  static std::string GetCurrentBitrate(bool local);
  static CURL GetTranscodeStopURL(CPlexServerPtr server);
  static PlexIntStringMap getOnlineQualties();
  static int SelectAOnlineQuality(int currentQuality);
  static int getBandwidthForQuality(int quality);
};

#endif /* defined(__Plex_Home_Theater__PlexTranscoderClient__) */
