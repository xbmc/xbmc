#ifndef PLEXREMOTEAPPLICATIONHANDLER_H
#define PLEXREMOTEAPPLICATIONHANDLER_H

#include "PlexHTTPRemoteHandler.h"

class CPlexRemoteApplicationHandler : public IPlexRemoteHandler
{
public:
  CPlexRemoteResponse handle(const CStdString &url, const ArgMap &arguments);
private:
  CPlexRemoteResponse sendVKey(const ArgMap &arguments);
  CPlexRemoteResponse sendString(const ArgMap &arguments);
};

#endif // PLEXREMOTEAPPLICATIONHANDLER_H
