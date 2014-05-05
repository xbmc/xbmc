#ifndef PLEXREMOTEPLAYHANDLER_H
#define PLEXREMOTEPLAYHANDLER_H

#include "PlexHTTPRemoteHandler.h"

class CPlexRemotePlayHandler : public IPlexRemoteHandler
{
public:
  CPlexRemoteResponse handle(const CStdString &url, const ArgMap &arguments);
};

#endif // PLEXREMOTEPLAYHANDLER_H
