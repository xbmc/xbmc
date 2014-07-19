#ifndef PLEXPLAYQUEUEMANAGER_H
#define PLEXPLAYQUEUEMANAGER_H

#include "PlexTypes.h"
#include "FileItem.h"
#include "Client/PlexServer.h"
#include "PlexJobs.h"
#include "gtest/gtest_prod.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
class CPlexPlayQueueOptions
{
public:
  CPlexPlayQueueOptions(bool playing = true, bool prompts = true, bool doshuffle = false,
                        const std::string& startItem = "")
    : startPlaying(playing), showPrompts(prompts), shuffle(doshuffle), startItemKey(startItem)
  {}

  // if the PQ should start playing when it's loaded or created
  bool startPlaying;

  // if we should show resume / media choice dialogs
  bool showPrompts;

  // if the PQ should be shuffled
  bool shuffle;

  // The key of the item that should be played first.
  std::string startItemKey;

  // a resume offset of the FIRST item in the PQ
  int64_t resumeOffset;

  // creation Url Options
  CUrlOptions urlOptions;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class CPlexPlayQueueFetchJob : public CPlexDirectoryFetchJob
{
public:
  CPlexPlayQueueFetchJob(const CURL& url, const CPlexPlayQueueOptions& options)
    : CPlexDirectoryFetchJob(url), m_options(options)
  { }

  CPlexPlayQueueOptions m_options;
  IPlexPlayQueueBasePtr m_caller;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class IPlexPlayQueueBase
{
public:
  static bool isSupported(const CPlexServerPtr& server)
  {
    return false;
  }
  virtual bool create(const CFileItem& container, const CStdString& uri = "",
                      const CPlexPlayQueueOptions& options = CPlexPlayQueueOptions()) = 0;
  virtual bool refreshCurrent() = 0;
  virtual bool getCurrent(CFileItemList& list) = 0;
  virtual const CFileItemList* getCurrent() = 0;
  virtual void removeItem(const CFileItemPtr& item) = 0;
  virtual bool addItem(const CFileItemPtr& item, bool next) = 0;
  virtual int getCurrentID() = 0;
  virtual void get(const CStdString& playQueueID,
                   const CPlexPlayQueueOptions& = CPlexPlayQueueOptions()) = 0;
  virtual CPlexServerPtr server() const = 0;
};

typedef boost::shared_ptr<IPlexPlayQueueBase> IPlexPlayQueueBasePtr;

///////////////////////////////////////////////////////////////////////////////////////////////////
class CPlexPlayQueueManager
{
  friend class PlayQueueManagerTest;
  FRIEND_TEST(PlayQueueManagerTest, ReconcilePlayQueueChanges_basic);
  FRIEND_TEST(PlayQueueManagerTest, ReconcilePlayQueueChanges_noMatching);
  FRIEND_TEST(PlayQueueManagerTest, ReconcilePlayQueueChanges_gapInMiddle);
  FRIEND_TEST(PlayQueueManagerTest, ReconcilePlayQueueChanges_largedataset);

public:
  CPlexPlayQueueManager() : m_playQueueVersion(0), m_playQueueType(PLEX_MEDIA_TYPE_UNKNOWN)
  {

  }

  bool create(const CFileItem& container, const CStdString& uri = "",
              const CPlexPlayQueueOptions& options = CPlexPlayQueueOptions());
  void clear();

  static CStdString getURIFromItem(const CFileItem& item, const CStdString& uri = "");
  static int getPlaylistFromType(ePlexMediaType type);

  void playQueueUpdated(const ePlexMediaType& type, bool startPlaying, int id = -1);

  virtual ePlexMediaType getCurrentPlayQueueType() const
  {
    return m_playQueueType;
  }
  virtual EPlexDirectoryType getCurrentPlayQueueDirType() const;

  int getCurrentPlayQueuePlaylist() const
  {
    return getPlaylistFromType(m_playQueueType);
  }

  bool getCurrentPlayQueue(CFileItemList& list);
  bool loadPlayQueue(const CPlexServerPtr& server, const std::string& playQueueID,
                     const CPlexPlayQueueOptions& = CPlexPlayQueueOptions());
  void loadSavedPlayQueue();
  void playCurrentId(int id);
  void QueueItem(const CFileItemPtr &item, bool next);

  int getCurrentPlayQueueVersion() const
  {
    return m_playQueueVersion;
  }

  /* proxy current implementation */
  bool addItem(const CFileItemPtr& item, bool next);
  void removeItem(const CFileItemPtr& item);
  int getCurrentID();
  bool refreshCurrent();


private:
  IPlexPlayQueueBasePtr getImpl(const CFileItem &container);
  bool reconcilePlayQueueChanges(int playlistType, const CFileItemList& list);
  void saveCurrentPlayQueue(const CPlexServerPtr& server, const CFileItemList& list);

  IPlexPlayQueueBasePtr m_currentImpl;
  ePlexMediaType m_playQueueType;
  int m_playQueueVersion;
};

typedef boost::shared_ptr<CPlexPlayQueueManager> CPlexPlayQueueManagerPtr;

#endif // PLEXPLAYQUEUEMANAGER_H
