#ifndef PLEXSERVERCACHEDATABASE_H
#define PLEXSERVERCACHEDATABASE_H

#include "dbwrappers/Database.h"
#include "PlexServer.h"
#include "PlexConnection.h"
#include "PlexServerManager.h"

class CPlexServerCacheDatabase : public CDatabase
{
  public:
    bool CreateTables();
    bool cacheServers();
    bool getCachedServers(std::vector<CPlexServerPtr>& servers);
    bool Open() { return CDatabase::Open(); }

  private:
    bool storeServer(const CPlexServerPtr& server);
    bool storeConnection(const CStdString& uuid, const CPlexConnectionPtr& connection);
    bool clearTables();

    virtual int GetMinVersion() const { return 1; }
    virtual const char* GetBaseDBName() const { return "PlexServerCache"; }
};

#endif // PLEXSERVERCACHEDATABASE_H
