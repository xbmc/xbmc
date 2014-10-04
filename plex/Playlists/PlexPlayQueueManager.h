#ifndef PLEXPLAYQUEUEMANAGER_H
#define PLEXPLAYQUEUEMANAGER_H

#include "PlexTypes.h"
#include "FileItem.h"
#include "Client/PlexServer.h"
#include "PlexJobs.h"
#include "gtest/gtest_prod.h"
#include <map>

///////////////////////////////////////////////////////////////////////////////////////////////////
class CPlexPlayQueueOptions
{
public:
  CPlexPlayQueueOptions(bool playing = true, bool prompts = true, bool doshuffle = false,
                        const std::string& startItem = "")
    : startPlaying(playing), showPrompts(prompts), shuffle(doshuffle), startItemKey(startItem), forceTrailers(false)
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
  
  // force trailer queuing
  bool forceTrailers;

  // creation Url Options
  CUrlOptions urlOptions;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class CPlexPlayQueue : public boost::enable_shared_from_this<CPlexPlayQueue>
{
protected:
  ePlexMediaType m_Type;
  int m_Version;

public:
  CPlexPlayQueue(ePlexMediaType type = PLEX_MEDIA_TYPE_UNKNOWN, int version = 0) : m_Type(type), m_Version(version)  {};
  virtual const std::string implementationName() = 0;

  static bool isSupported(const CPlexServerPtr& server)
  {
    return false;
  }

  virtual bool create(const CFileItem& container, const CStdString& uri = "",
                      const CPlexPlayQueueOptions& options = CPlexPlayQueueOptions()) = 0;
  virtual bool refresh() = 0;
  virtual bool get(CFileItemList& list) = 0;
  virtual const CFileItemList* get() = 0;
  virtual void removeItem(const CFileItemPtr& item) = 0;
  virtual bool addItem(const CFileItemPtr& item, bool next) = 0;
  virtual bool moveItem(const CFileItemPtr& item, const CFileItemPtr& afteritem) = 0;
  virtual int getID() = 0;
  virtual int getPlaylistID() = 0;
  virtual CStdString getPlaylistTitle() = 0;
  virtual void get(const CStdString& playQueueID,
                   const CPlexPlayQueueOptions& = CPlexPlayQueueOptions()) = 0;
  virtual CPlexServerPtr server() const = 0;
  
  ePlexMediaType getType() const
  {
    return m_Type;
  }
  
  int getVersion() const
  {
    return m_Version;
  }
  
  inline void setVersion(int version) { m_Version = version; };
  inline void setType(ePlexMediaType type) { m_Type = type; };
};

typedef boost::shared_ptr<CPlexPlayQueue> CPlexPlayQueuePtr;
typedef std::map<ePlexMediaType, CPlexPlayQueuePtr> PlayQueueMap;
typedef std::map<ePlexMediaType, CPlexPlayQueuePtr> PlayQueueMapIterator;

///////////////////////////////////////////////////////////////////////////////////////////////////
class CPlexPlayQueueFetchJob : public CPlexDirectoryFetchJob
{
public:
  CPlexPlayQueueFetchJob(const CURL& url, const CPlexPlayQueueOptions& options)
    : CPlexDirectoryFetchJob(url), m_options(options)
  { }

  CPlexPlayQueueOptions m_options;
  CPlexPlayQueuePtr m_caller;
};


///////////////////////////////////////////////////////////////////////////////////////////////////
class CPlexPlayQueueManager
{
  friend class PlayQueueManagerTest;
  FRIEND_TEST(PlayQueueManagerTest, ReconcilePlayQueueChanges_basic);
  FRIEND_TEST(PlayQueueManagerTest, ReconcilePlayQueueChanges_noMatching);
  FRIEND_TEST(PlayQueueManagerTest, ReconcilePlayQueueChanges_gapInMiddle);
  FRIEND_TEST(PlayQueueManagerTest, ReconcilePlayQueueChanges_largedataset);

public:
  CPlexPlayQueueManager()
  { 
  };
  
  virtual ~CPlexPlayQueueManager() {}

  bool create(const CFileItem& container, const CStdString& uri = "",
              const CPlexPlayQueueOptions& options = CPlexPlayQueueOptions());
  void clear();
  void clear(ePlexMediaType type);

  static CStdString getURIFromItem(const CFileItem& item, const CStdString& uri = "");
  static int getPlaylistFromType(ePlexMediaType type);

  void playQueueUpdated(const ePlexMediaType& type, bool startPlaying, int id = -1);

  virtual EPlexDirectoryType getPlayQueueDirType(ePlexMediaType type) const;

  virtual CPlexPlayQueuePtr getPlayQueueOfType(ePlexMediaType type) const;
  virtual CPlexPlayQueuePtr getPlayingPlayQueue() const;
  CPlexPlayQueuePtr getPlayQueueFromID(int id) const;
  
  inline int getPlayQueuesCount() { return m_playQueues.size(); }
  
  bool getPlayQueue(ePlexMediaType type, CFileItemList& list);
  bool loadPlayQueue(const CPlexServerPtr& server, const std::string& playQueueID,
                     const CPlexPlayQueueOptions& = CPlexPlayQueueOptions());
  void playId(ePlexMediaType type, int id);
  void QueueItem(const CFileItemPtr &item, bool next);

  /* proxy current implementation */
  bool addItem(const CFileItemPtr& item, bool next);
  bool moveItem(const CFileItemPtr &item, const CFileItemPtr& afteritem);
  bool moveItem(const CFileItemPtr& item, int offset);
  void removeItem(const CFileItemPtr& item);
  int getID(ePlexMediaType type);
  bool refresh(ePlexMediaType type);
  CPlexPlayQueuePtr getImpl(const CFileItem &container);

private:
  bool reconcilePlayQueueChanges(int playlistType, const CFileItemList& list);

protected:
  PlayQueueMap m_playQueues;
};

typedef boost::shared_ptr<CPlexPlayQueueManager> CPlexPlayQueueManagerPtr;

#endif // PLEXPLAYQUEUEMANAGER_H
