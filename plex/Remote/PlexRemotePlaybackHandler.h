#ifndef PLEXREMOTEPLAYBACKHANDLER_H
#define PLEXREMOTEPLAYBACKHANDLER_H

#include "PlexHTTPRemoteHandler.h"

class CPlexRemotePlaybackHandler : public IPlexRemoteHandler
{
public:
  virtual CPlexRemoteResponse handle(const CStdString &url, const ArgMap &arguments);

  CPlexRemoteResponse stepFunction(const CStdString &url, const ArgMap &arguments);
  CPlexRemoteResponse skipNext(const ArgMap &arguments);
  CPlexRemoteResponse skipPrevious(const ArgMap &arguments);
  CPlexRemoteResponse pausePlay(const ArgMap &arguments);
  CPlexRemoteResponse stop(const ArgMap &arguments);
  CPlexRemoteResponse seekTo(const ArgMap &arguments);
  CPlexRemoteResponse showDetails(const ArgMap &arguments);
  CPlexRemoteResponse set(const ArgMap &arguments);
  CPlexRemoteResponse setVolume(const ArgMap &arguments);
  CPlexRemoteResponse sendString(const ArgMap &arguments);
  CPlexRemoteResponse sendVKey(const ArgMap &arguments);
  CPlexRemoteResponse setStreams(const ArgMap &arguments);
  CPlexRemoteResponse skipTo(const ArgMap &arguments);

};

#endif // PLEXREMOTEPLAYBACKHANDLER_H
