#ifndef PLEXPLAYQUEUESERVER_H
#define PLEXPLAYQUEUESERVER_H

#include "PlexPlayQueueManager.h"
#include "Client/PlexServerVersion.h"
#include "Client/PlexServer.h"
#include "Job.h"
#include "threads/CriticalSection.h"

class PlayQueueServerTest;

class CPlexPlayQueueServer : public IPlexPlayQueueBase, public IJobCallback,
    public boost::enable_shared_from_this<CPlexPlayQueueServer>
{
  friend class PlayQueueServerTest;

  FRIEND_TEST(PlayQueueServerTest, GetPlayQueueURL_validItem);
  FRIEND_TEST(PlayQueueServerTest, GetPlayQueueURL_limit);
  FRIEND_TEST(PlayQueueServerTest, GetPlayQueueURL_haveKey);
  FRIEND_TEST(PlayQueueServerTest, GetPlayQueueURL_hasNext);

public:
  CPlexPlayQueueServer(const CPlexServerPtr& server)
  {
    m_server = server;
  }

  static bool isSupported(const CPlexServerPtr& server)
  {
    CPlexServerVersion serverVersion(server->GetVersion());
    return (!server->IsSecondary() && serverVersion > CPlexServerVersion("0.9.9.6.0-abc123"));
  }

  virtual void create(const CFileItem &container, const CStdString& uri,
                      const CStdString& startItemKey, bool shuffle);
  virtual bool refreshCurrent();
  virtual bool getCurrent(CFileItemList& list);
  virtual void removeItem(const CFileItemPtr& item);
  virtual void addItem(const CFileItemPtr& item);
  virtual int getCurrentID();
  virtual void get(const CStdString &playQueueID, bool startPlaying);
  virtual CPlexServerPtr server() const
  {
    return m_server;
  }

protected:
  void sendRequest(const CURL &url, const CStdString &verb, bool startPlaying);
  CURL getPlayQueueURL(ePlexMediaType type, const std::string& uri, const std::string& key="",
                       bool shuffle = false, bool continuous = false, int limit = 0,
                       bool next = false);
  void OnJobComplete(unsigned int jobID, bool success, CJob *job);
  void reconcilePlayQueueChanges(ePlexMediaType type, const CFileItemList &list);

  CCriticalSection m_mapLock;
  CFileItemListPtr m_list;

  CPlexServerPtr m_server;
};

#endif // PLEXPLAYQUEUESERVER_H
