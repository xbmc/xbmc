#ifndef PLAYQUEUEMANAGER_H
#define PLAYQUEUEMANAGER_H

#include "FileItem.h"
#include "Job.h"
#include "playlists/PlayList.h"
#include "gtest/gtest.h"

class PlayQueueManagerTest;

class CPlayQueueManager : public IJobCallback
{
  friend class PlayQueueManagerTest;
  FRIEND_TEST(PlayQueueManagerTest, VerifyServerInItem_basic);

public:
  CPlayQueueManager();
  void getPlayQueue(CPlexServerPtr server, int id, bool startPlaying);
  bool createPlayQueue(const CPlexServerPtr& server, ePlexMediaType type, const std::string& uri,
                       const std::string& key, bool shuffle = false, bool continuous = false,
                       int limit = 0);

  bool addItemToCurrentPlayQueue(const CFileItemPtr& item, bool playNext);
  bool removeItemFromCurrentPlayQueue(const CFileItemPtr& item);
  bool createPlayQueueFromItem(const CPlexServerPtr& server, const CFileItemPtr& item,
                               bool shuffle = false, bool continuous = false, int limit = 0);

  static CStdString getURIFromItem(const CFileItem& item, const CStdString& uri = "");

  CURL getPlayQueueURL(const CPlexServerPtr& server, ePlexMediaType type, const std::string& uri,
                       const std::string& key = "", bool shuffle = false, bool continuous = false,
                       int limit = 0, bool next = false);

  int getPlaylistFromString(const CStdString& typeStr);

  void refreshPlayQueue(const CFileItemPtr& item);
  void reconcilePlayQueueChanges(int playlistType, const CFileItemList& list);

  int getPlaylistFromType(ePlexMediaType type);
  bool verifyServerInItem(const CFileItemPtr& item);
  void loadMostRecentPlayQueue();

  int getCurrentPlayQueueType() const
  {
    return m_currentPlayQueuePlaylist;
  }

protected:
  void OnJobComplete(unsigned int jobID, bool success, CJob* job);
  int m_currentPlayQueueId;
  int m_currentPlayQueuePlaylist;
  CPlexServerPtr m_currentPlayQueueServer;
};

#endif // PLAYQUEUEMANAGER_H
