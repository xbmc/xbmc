#ifndef PLEXPLAYQUEUELOCAL_H
#define PLEXPLAYQUEUELOCAL_H

#include <map>
#include "PlexPlayQueueManager.h"
#include "Job.h"
#include "Client/PlexServer.h"

class CPlexPlayQueueLocal : public IPlexPlayQueueBase, public IJobCallback
{
public:
  CPlexPlayQueueLocal(const CPlexServerPtr& server);

  static bool isSupported(const CPlexServerPtr& server)
  {
    // always supported
    return true;
  }

  virtual void create(const CFileItem& container, const CStdString& uri,
                      const CStdString& startItemKey, bool shuffle);
  virtual bool refreshCurrent();
  virtual bool getCurrent(CFileItemList& list);
  virtual void removeItem(const CFileItemPtr &item);
  virtual void addItem(const CFileItemPtr &item);
  virtual int getCurrentID();
  virtual void get(const CStdString &playQueueID);
  virtual CPlexServerPtr server() const
  {
    return m_server;
  }

private:
  void OnJobComplete(unsigned int jobID, bool success, CJob *job);
  CFileItemListPtr m_list;
  CPlexServerPtr m_server;
};

#endif // PLEXPLAYQUEUELOCAL_H
