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
#include "GlobalsHandling.h"
#include "guilib/GUIMessage.h"

class CPlexMediaServerClient : public CJobQueue
{
public:
  enum MediaState {
    MEDIA_STATE_STOPPED,
    MEDIA_STATE_PLAYING,
    MEDIA_STATE_BUFFERING,
    MEDIA_STATE_PAUSED
  };
  
  CPlexMediaServerClient() : CJobQueue() {}
  void SelectStream(const CFileItemPtr& item, int partID, int subtitleStreamID, int audioStreamID);
  
  /* scrobble events */
  void SetItemWatched(const CFileItemPtr& item) { SetItemWatchStatus(item, true); }
  void SetItemUnWatched(const CFileItemPtr& item) { SetItemWatchStatus(item, false); }
  void SetItemWatchStatus(const CFileItemPtr& item, bool watched);
  
  /* Rating */
  void SetItemRating(const CFileItemPtr& item, float rating);
  
  /* timeline api */
  void ReportItemProgress(const CFileItemPtr& item, MediaState state, int64_t currentPosition = 0);
  void ReportItemProgress(const CFileItemPtr& item, const CStdString& state, int64_t currentPosition = 0);
  
  /* Set viewMode */
  void SetViewMode(const CFileItem& item, int viewMode, int sortMode = -1, int sortAsc = 1);
  
  /* stop a transcode session */
  void StopTranscodeSession(CPlexServerPtr server);
  
  void deleteItem(const CFileItemPtr &item);
  
  void OnJobComplete(unsigned int jobID, bool success, CJob *job);

private:
  CStdString GetPrefix(const CFileItemPtr& item) const
  {
    CStdString prefix = "/:/";
    if (item->GetProperty("plexserver") == "myplex")
      prefix = "/pms/:/";
    return prefix;
  }
};

XBMC_GLOBAL_REF(CPlexMediaServerClient, g_plexMediaServerClient);
#define g_plexMediaServerClient XBMC_GLOBAL_USE(CPlexMediaServerClient)

#endif /* defined(__Plex_Home_Theater__PlexMediaServerClient__) */
