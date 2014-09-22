#ifndef PLEXPLAYQUEUELOCAL_H
#define PLEXPLAYQUEUELOCAL_H

#include <map>
#include "PlexPlayQueueManager.h"
#include "Job.h"
#include "Client/PlexServer.h"

class CPlexPlayQueueLocal : public CPlexPlayQueue, public IJobCallback
{
public:
  CPlexPlayQueueLocal(const CPlexServerPtr& server, ePlexMediaType type = PLEX_MEDIA_TYPE_UNKNOWN, int version = 0);

  static bool isSupported(const CPlexServerPtr& server)
  {
    // always supported
    return true;
  }

  const std::string implementationName() { return "local"; }

  virtual bool create(const CFileItem& container, const CStdString& uri,
                      const CPlexPlayQueueOptions& options = CPlexPlayQueueOptions());
  virtual bool refresh();
  virtual bool get(CFileItemList& list);
  virtual const CFileItemList* get() { return m_list.get(); }
  virtual void removeItem(const CFileItemPtr &item);
  virtual bool addItem(const CFileItemPtr &item, bool next);
  virtual bool moveItem(const CFileItemPtr& item, const CFileItemPtr& afteritem);
  virtual int getID();
  virtual int getPlaylistID();
  virtual CStdString getPlaylistTitle();
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
