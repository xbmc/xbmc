//
//  GUIDialogPlayListSelection.cpp
//  Plex Home Theater
//
//  Created by Lionel CHAZALLON on 29/09/2014.
//
//

#include "GUIDialogPlayListSelection.h"
#include "PlexJobs.h"
#include "PlexApplication.h"
#include "PlexPlayQueueManager.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIDialogPlaylistSelection::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    if (m_server)
    {
      CURL url = m_server->BuildPlexURL("/playlists/all");
      
      // we only want dumb playlists
      url.SetOption("smart", "0");
      
      switch (m_plType)
      {
        case PLEX_MEDIA_TYPE_MUSIC:
          url.SetOption("playlistType", "audio");
          break;
          
        case PLEX_MEDIA_TYPE_VIDEO:
          url.SetOption("playlistType", "video");
          break;
          
        default:
          break;
      }
      
      // grab the playlist items list
      CPlexDirectoryFetchJob *plJob = new CPlexDirectoryFetchJob(url);
      if (g_plexApplication.busy.blockWaitingForJob(plJob, this))
      {
        if (m_vecList->Size())
          SetHeading("Select a Playlist on " + m_server->GetName());
        else
          SetHeading("No Available Playlist on " + m_server->GetName());
        
        SetMultiSelection(false);
        SetSelected(0);
      }
    }
  }
  
  return CGUIDialogSelect::OnMessage(message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIDialogPlaylistSelection::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  if (success)
  {
    CPlexDirectoryFetchJob *plJob = dynamic_cast<CPlexDirectoryFetchJob*>(job);
    if (plJob)
    {
      Reset();
      SetItems(&plJob->m_items);
    }
  }
}

