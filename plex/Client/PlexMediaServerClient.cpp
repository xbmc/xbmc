//
//  PlexMediaServerClient.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-06-11.
//
//

#include "PlexMediaServerClient.h"

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <string>

#include "PlexFile.h"
#include "video/VideoInfoTag.h"
#include "music/tags/MusicInfoTag.h"
#include "Client/PlexTranscoderClient.h"
#include "PlexJobs.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "guilib/Key.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"

#include "Application.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "PlexApplication.h"

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CPlexMediaServerClientJob *clientJob = static_cast<CPlexMediaServerClientJob*>(job);

  if (success && clientJob->m_msg.GetMessage() != 0)
    g_windowManager.SendThreadMessage(clientJob->m_msg);
  else if (!success && clientJob->m_errorMsg)
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(257), g_localizeStrings.Get(clientJob->m_errorMsg));
  
  CJobQueue::OnJobComplete(jobID, success, job);
}


////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::SelectStream(const CFileItemPtr &item,
                                          int partID,
                                          int subtitleStreamID,
                                          int audioStreamID)
{
  CURL u(item->GetPath());
  
  u.SetFileName("/library/parts/" + boost::lexical_cast<std::string>(partID));
  if (subtitleStreamID != -1)
    u.SetOption("subtitleStreamID", boost::lexical_cast<std::string>(subtitleStreamID));
  if (audioStreamID != -1)
    u.SetOption("audioStreamID", boost::lexical_cast<std::string>(audioStreamID));
  
  AddJob(new CPlexMediaServerClientJob(u, "PUT"));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::SetItemWatchStatus(const CFileItemPtr &item, bool watched)
{
  CURL u(item->GetPath());
  
  u.SetFileName(GetPrefix(item) + (watched ? "scrobble" : "unscrobble"));
  u.SetOption("key", item->GetProperty("ratingKey").asString());
  u.SetOption("identifier", item->GetProperty("identifier").asString());
  
  AddJob(new CPlexMediaServerClientJob(u));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::SetItemRating(const CFileItemPtr &item, float rating)
{
  CURL u(item->GetPath());
  
  u.SetFileName(GetPrefix(item) + "rate");
  u.SetOption("rating", boost::lexical_cast<std::string>(rating));
  
  AddJob(new CPlexMediaServerClientJob(u));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::SendServerTimeline(const CFileItemPtr &item, const CUrlOptions &options)
{
  CURL u(item->GetPath());
  u.SetFileName(":/timeline");
  u.AddOptions(options);

  AddJob(new CPlexMediaServerClientJob(u));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::SetViewMode(const CFileItem &item, int viewMode, int sortMode, int sortAsc)
{
  CURL u(item.GetPath());
  u.SetFileName("/:/viewChange");
  u.SetOption("identifier", item.GetProperty("identifier").asString());
  u.SetOption("viewGroup", item.GetProperty("viewGroup").asString());

  u.SetOption("viewMode", boost::lexical_cast<CStdString>(viewMode));
  u.SetOption("sortMode", boost::lexical_cast<CStdString>(sortMode));
  u.SetOption("sortAsc", boost::lexical_cast<CStdString>(sortAsc));

  AddJob(new CPlexMediaServerClientJob(u));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::StopTranscodeSession(CPlexServerPtr server)
{
  AddJob(new CPlexMediaServerClientJob(CPlexTranscoderClient::GetTranscodeStopURL(server)));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::deleteItem(const CFileItemPtr &item)
{
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE, g_windowManager.GetActiveWindow());
  AddJob(new CPlexMediaServerClientJob(item->GetPath(), "DELETE", msg, 16205));
}
