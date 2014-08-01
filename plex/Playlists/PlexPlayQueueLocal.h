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

  virtual bool create(const CFileItem& container, const CStdString& uri,
                      const CPlexPlayQueueOptions& options = CPlexPlayQueueOptions());
  virtual bool refreshCurrent();
  virtual bool getCurrent(CFileItemList& list);
  virtual const CFileItemList* getCurrent() { return m_list.get(); }
  virtual void removeItem(const CFileItemPtr &item);
  virtual bool addItem(const CFileItemPtr &item, bool next);
  virtual int getCurrentID();
  virtual void get(const CStdString &playQueueID,
                   const CPlexPlayQueueOptions& options = CPlexPlayQueueOptions());
  virtual CPlexServerPtr server() const
  {
    return m_server;
  }

private:
  void OnJobComplete(unsigned int jobID, bool success, CJob *job);
  void OnPlayQueueUpdated(ePlexMediaType type, bool startPlaying);
  CFileItemListPtr m_list;
  CPlexServerPtr m_server;
};

#endif // PLEXPLAYQUEUELOCAL_H
