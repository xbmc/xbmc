#ifndef PLAYQUEUEMANAGER_H
#define PLAYQUEUEMANAGER_H

#include "FileItem.h"
#include "Job.h"

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

  static CStdString getURIFromItem(const CFileItemPtr& item);

  CURL getCreatePlayQueueURL(const CPlexServerPtr& server, ePlexMediaType type,
                             const std::string& uri, const std::string& key = "",
                             bool shuffle = false, bool continuous = false, int limit = 0);

  int getPlaylistFromString(const CStdString &typeStr);

private:
  void OnJobComplete(unsigned int jobID, bool success, CJob* job);
};

#endif // PLAYQUEUEMANAGER_H
