#ifndef PLEXPLAYQUEUEMANAGER_H
#define PLEXPLAYQUEUEMANAGER_H

#include "PlexTypes.h"
#include "FileItem.h"
#include "Client/PlexServer.h"
#include "gtest/gtest.h"

class IPlexPlayQueueBase
{
public:
  static bool isSupported(const CPlexServerPtr& server)
  {
    return false;
  }
  virtual void create(const CFileItem& container, const CStdString& uri = "",
                      const CStdString& startItemKey = "", bool shuffle = false) = 0;
  virtual bool refreshCurrent() = 0;
  virtual bool getCurrent(CFileItemList& list) = 0;
  virtual void removeItem(const CFileItemPtr& item) = 0;
  virtual void addItem(const CFileItemPtr& item) = 0;
  virtual int getCurrentID() = 0;
  virtual void get(const CStdString& playQueueID, bool startPlay) = 0;
  virtual CPlexServerPtr server() const = 0;
};

typedef boost::shared_ptr<IPlexPlayQueueBase> IPlexPlayQueueBasePtr;

class CPlexPlayQueueManager
{
  friend class PlayQueueManagerTest;
  FRIEND_TEST(PlayQueueManagerTest, ReconcilePlayQueueChanges_basic);
  FRIEND_TEST(PlayQueueManagerTest, ReconcilePlayQueueChanges_noMatching);
  FRIEND_TEST(PlayQueueManagerTest, ReconcilePlayQueueChanges_gapInMiddle);
  FRIEND_TEST(PlayQueueManagerTest, ReconcilePlayQueueChanges_largedataset);

public:
  void create(const CFileItem& container, const CStdString& uri = "",
              const CStdString& startItemKey = "", bool shuffle = false);

  IPlexPlayQueueBasePtr current()
  {
    return m_currentImpl;
  }

  static CStdString getURIFromItem(const CFileItem& item, const CStdString& uri = "");
  static int getPlaylistFromType(ePlexMediaType type);

  void playQueueUpdated(const ePlexMediaType& type, bool startPlaying, int id = -1);

  ePlexMediaType getCurrentPlayQueueType() const
  {
    return m_playQueueType;
  }

  int getCurrentPlayQueuePlaylist() const
  {
    return getPlaylistFromType(m_playQueueType);
  }

  bool getCurrentPlayQueue(CFileItemList& list);
  bool loadPlayQueue(const CPlexServerPtr& server, const std::string& playQueueID);
  void loadSavedPlayQueue();
  void playCurrentId(int id);

private:
  IPlexPlayQueueBasePtr getImpl(const CFileItem &container);
  bool reconcilePlayQueueChanges(int playlistType, const CFileItemList& list);
  void saveCurrentPlayQueue(const CPlexServerPtr& server, const CFileItemList& list);

  IPlexPlayQueueBasePtr m_currentImpl;
  CStdString m_pqKey;
  ePlexMediaType m_playQueueType;
};

typedef boost::shared_ptr<CPlexPlayQueueManager> CPlexPlayQueueManagerPtr;

#endif // PLEXPLAYQUEUEMANAGER_H
