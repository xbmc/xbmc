#ifndef PLEXREMOTENAVIGATIONHANDLER_H
#define PLEXREMOTENAVIGATIONHANDLER_H

#include "PlexHTTPRemoteHandler.h"

class CPlexRemoteNavigationHandler : public IPlexRemoteHandler
{
public:
  virtual CPlexRemoteResponse handle(const CStdString &url, const ArgMap &arguments);
};

#endif // PLEXREMOTENAVIGATIONHANDLER_H
