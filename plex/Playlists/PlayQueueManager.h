#ifndef PLAYQUEUEMANAGER_H
#define PLAYQUEUEMANAGER_H

#include "FileItem.h"
#include "Job.h"
#include "playlists/PlayList.h"

class CPlayQueueManager : public IJobCallback
{
public:
  CPlayQueueManager();
  void getPlayQueue(CPlexServerPtr server, int id);
  bool createPlayQueue(const CPlexServerPtr& server, ePlexMediaType type, const std::string& uri,
                       const std::string& key, bool shuffle = false, bool continuous = false,
                       int limit = 0);
  bool createPlayQueueFromItem(const CPlexServerPtr& server, const CFileItemPtr& item,
                               bool shuffle = false, bool continuous = false, int limit = 0);

  static CStdString getURIFromItem(const CFileItem &item);

  CURL getCreatePlayQueueURL(const CPlexServerPtr& server, ePlexMediaType type,
                             const std::string& uri, const std::string& key = "",
                             bool shuffle = false, bool continuous = false, int limit = 0);

  int getPlaylistFromString(const CStdString& typeStr);

  void refreshPlayQueue(const CFileItemPtr& item);
  void reconcilePlayQueueChanges(int playlistType, const CFileItemList& list);

  int getPlaylistFromType(ePlexMediaType type);

private:
  void OnJobComplete(unsigned int jobID, bool success, CJob* job);
  int m_currentPlayQueueId;
  int m_currentPlayQueuePlaylist;
};

#endif // PLAYQUEUEMANAGER_H
