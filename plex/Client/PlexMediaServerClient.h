//
//  PlexMediaServerClient.h
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-06-11.
//
//

#ifndef __Plex_Home_Theater__PlexMediaServerClient__
#define __Plex_Home_Theater__PlexMediaServerClient__

#include "JobManager.h"
#include "Client/PlexServer.h"
#include "FileItem.h"
#include "guilib/GUIMessage.h"
#include "Remote/PlexRemoteSubscriberManager.h"

class CPlexMediaServerClient : public CJobQueue, public boost::enable_shared_from_this<CPlexMediaServerClient>
{
public:
  CPlexMediaServerClient() : CJobQueue(false, 2, CJob::PRIORITY_HIGH) {}
  void SelectStream(const CFileItemPtr& item, int partID, int subtitleStreamID, int audioStreamID);
  
  /* scrobble events */
  void SetItemWatched(const CFileItemPtr& item, bool sendMessage = false) { SetItemWatchStatus(item, true, sendMessage); }
  void SetItemUnWatched(const CFileItemPtr& item, bool sendMessage = false) { SetItemWatchStatus(item, false, sendMessage); }
  void SetItemWatchStatus(const CFileItemPtr& item, bool watched, bool sendMessage = false);
  
  /* Rating */
  void SetItemRating(const CFileItemPtr& item, float rating);
  
  /* timeline api */
  void SendServerTimeline(const CFileItemPtr& item, const CUrlOptions &options);
  void SendSubscriberTimeline(const CURL& url, const CStdString &postData);

  /* Set viewMode */
  void SetViewMode(CFileItemPtr item, int viewMode, int sortMode = -1, int sortAsc = 1);
  
  /* stop a transcode session */
  void StopTranscodeSession(CPlexServerPtr server);
  
  void deleteItem(const CFileItemPtr& item);
  void deleteItemFromPath(const CStdString path);
  void OnJobComplete(unsigned int jobID, bool success, CJob *job);

  void share(const CFileItemPtr &item, const CStdString &network, const CStdString &message);
  
  void movePlayListItem(CFileItemPtr item, CFileItemPtr after);
  bool addItemToPlayList(CPlexServerPtr server, CFileItemPtr item, CStdString playlistID, bool block = false);
  bool createPlayList(CPlexServerPtr server, CStdString name, CFileItemPtr item, bool smart, bool block);
  CFileItemListPtr getPlayLists();

  CURL GetItemURL(CFileItemPtr item);
  void SendTranscoderPing(CPlexServerPtr server);
  void publishDevice();

  private:
  CStdString GetPrefix(const CFileItemPtr& item) const
  {
    CStdString prefix = "/:/";
    if (item->GetProperty("plexserver") == "myplex")
      prefix = "/pms/:/";
    return prefix;
  }  
};

typedef boost::shared_ptr<CPlexMediaServerClient> CPlexMediaServerClientPtr;

#endif /* defined(__Plex_Home_Theater__PlexMediaServerClient__) */
