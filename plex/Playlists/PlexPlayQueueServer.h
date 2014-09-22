#ifndef PLEXPLAYQUEUESERVER_H
#define PLEXPLAYQUEUESERVER_H

#include "PlexPlayQueueManager.h"
#include "Client/PlexServerVersion.h"
#include "Client/PlexServer.h"
#include "Job.h"
#include "threads/CriticalSection.h"

class PlayQueueServerTest;

class CPlexPlayQueueServer : public CPlexPlayQueue,
                             public IJobCallback
{
  friend class PlayQueueServerTest;

  FRIEND_TEST(PlayQueueServerTest, GetPlayQueueURL_validItem);
  FRIEND_TEST(PlayQueueServerTest, GetPlayQueueURL_playlistID);
  FRIEND_TEST(PlayQueueServerTest, GetPlayQueueURL_limit);
  FRIEND_TEST(PlayQueueServerTest, GetPlayQueueURL_haveKey);
  FRIEND_TEST(PlayQueueServerTest, GetPlayQueueURL_hasNext);

public:
  CPlexPlayQueueServer(const CPlexServerPtr& server, ePlexMediaType type = PLEX_MEDIA_TYPE_UNKNOWN, int version = 0) : CPlexPlayQueue(type, version)
  {
    m_server = server;
  }

  static bool isSupported(const CPlexServerPtr& server)
  {
    CPlexServerVersion serverVersion(server->GetVersion());
    return (!server->IsSecondary() && serverVersion > CPlexServerVersion("0.9.9.6.0-abc123"));
  }

  const std::string implementationName() { return "server"; }

  virtual bool create(const CFileItem &container, const CStdString& uri = "",
                      const CPlexPlayQueueOptions& options = CPlexPlayQueueOptions());
  virtual bool refresh();
  virtual bool get(CFileItemList& list);
  virtual const CFileItemList* get() { return m_list.get(); }
  virtual void removeItem(const CFileItemPtr& item);
  virtual bool addItem(const CFileItemPtr& item, bool next);
  virtual bool moveItem(const CFileItemPtr& item, const CFileItemPtr& afteritem);
  virtual int getID();
  virtual int getPlaylistID();
  virtual CStdString getPlaylistTitle();
  virtual void get(const CStdString& playQueueID,
                   const CPlexPlayQueueOptions& options = CPlexPlayQueueOptions());
  virtual CPlexServerPtr server() const
  {
    return m_server;
  }

protected:
  bool sendRequest(const CURL &url, const CStdString &verb, const CPlexPlayQueueOptions& options);
  CURL getPlayQueueURL(ePlexMediaType type, const std::string& uri, const std::string &playlistID,
                       const std::string& key = "", bool shuffle = false, bool continuous = false,
                       int limit = 0, bool next = false);
  void OnJobComplete(unsigned int jobID, bool success, CJob *job);
  void reconcilePlayQueueChanges(ePlexMediaType type, const CFileItemList &list);

  CCriticalSection m_mapLock;
  CFileItemListPtr m_list;

  CPlexServerPtr m_server;
};

#endif // PLEXPLAYQUEUESERVER_H
