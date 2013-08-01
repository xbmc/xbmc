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

class CPlexTranscoderClient
{
public:
  CPlexTranscoderClient();
  static int SelectATranscoderQuality(CPlexServerPtr server, int currentQuality = 0);
  static std::string GetPrettyBitrate(int br);
  static bool ShouldTranscode(CPlexServerPtr server, const CFileItem& item);
  static CURL GetTranscodeURL(CPlexServerPtr server, const CFileItem& item);
  static std::string GetCurrentBitrate(bool local);
};

#endif /* defined(__Plex_Home_Theater__PlexTranscoderClient__) */
