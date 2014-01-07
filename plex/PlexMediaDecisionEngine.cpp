//
//  PlexMediaDecisionEngine.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-08-02.
//
//

#include "PlexMediaDecisionEngine.h"

#include "FileItem.h"
#include "Variant.h"
#include "FileSystem/PlexDirectory.h"
#include "File.h"
#include "Client/PlexTranscoderClient.h"
#include "Client/PlexServerManager.h"
#include "utils/StringUtils.h"
#include "filesystem/StackDirectory.h"
#include "dialogs/GUIDialogBusy.h"
#include "guilib/GUIWindowManager.h"
#include "PlexApplication.h"

bool CPlexMediaDecisionEngine::BlockAndResolve(const CFileItem &item, CFileItem &resolvedItem)
{
  if (item.GetProperty("isResolved").asBoolean())
  {
    resolvedItem = item;
    return true;
  }

  m_item = item;

  m_done.Reset();
  Create();
  
  CLog::Log(LOGDEBUG, "CPlexMediaDecisionEngine::BlockAndResolve waiting for resolve to return");
  if (!m_done.WaitMSec(100))
  {
    CLog::Log(LOGDEBUG, "CPlexMediaDecisionEngine::BlockAndResolve show busy dialog");
    CGUIDialogBusy* dialog = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);
    if(dialog)
    {
      dialog->Show();
      while(!m_done.WaitMSec(1))
      {
        if (dialog->IsCanceled())
        {
          Cancel();
          return false;
        }
        
        g_windowManager.ProcessRenderLoop(false);
      }
      dialog->Close();
    }
  }
  
  CLog::Log(LOGDEBUG, "CPlexMediaDecisionEngine::BlockAndResolve resolve done, success: %s", m_success ? "Yes" : "No");
 
  if (m_success)
  {
    resolvedItem = m_choosenMedia;
    resolvedItem.SetProperty("isResolved", true);
    return true;
  }
  return false;
}

void CPlexMediaDecisionEngine::Process()
{
  ChooseMedia();
  m_done.Set();
}

void CPlexMediaDecisionEngine::Cancel()
{
  m_dir.CancelDirectory();
  m_http.Cancel();

  StopThread();
}

CFileItemPtr CPlexMediaDecisionEngine::ResolveIndirect(CFileItemPtr item)
{
  if (!item) return CFileItemPtr();

  if (!item->GetProperty("indirect").asBoolean())
    return item;

  if (item->m_mediaParts.size() != 1)
    return CFileItemPtr();

  CFileItemPtr part = item->m_mediaParts[0];
  CURL partUrl(part->GetPath());

  if (m_bStop)
    return CFileItemPtr();

  if (part->HasProperty("postURL"))
  {
    m_http.SetUserAgent("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_2) AppleWebKit/537.17 (KHTML, like Gecko) Chrome/24.0.1312.52 Safari/537.17");
    m_http.ClearCookies();

    CLog::Log(LOGDEBUG, "CPlexMediaDecisionEngine::ResolveIndirect fetching PostURL");
    CStdString postDataStr;
    if (!m_http.Get(part->GetProperty("postURL").asString(), postDataStr))
    {
      return CFileItemPtr();
    }
    CHttpHeader headers = m_http.GetHttpHeader();
    CStdString data = headers.GetHeaders() + postDataStr;
    partUrl.SetOption("postURL", part->GetProperty("postURL").asString());

    if (data.length() > 0)
      m_dir.SetBody(data);

    m_http.Reset();
  }

  if (!m_bStop)
  {
    CFileItemList list;
    if (!m_dir.GetDirectory(partUrl, list))
      return CFileItemPtr();

    CFileItemPtr i = list.Get(0);

    if (!i || i->m_mediaItems.size() == 0)
      return CFileItemPtr();

    item = i->m_mediaItems[0];

    /* check if we got some httpHeaders from this mediaContainer */
    if (list.HasProperty("httpHeaders"))
      item->SetProperty("httpHeaders", list.GetProperty("httpHeaders"));
  }

  CLog::Log(LOGDEBUG, "CPlexMediaDecisionEngine::ResolveIndirect Recursing %s", m_choosenMedia.GetPath().c_str());
  return ResolveIndirect(item);
}

void CPlexMediaDecisionEngine::AddHeaders()
{
  CStdString protocolOpts;
  if (m_choosenMedia.HasProperty("httpHeaders"))
  {
    protocolOpts = m_choosenMedia.GetProperty("httpHeaders").asString();
  }
  else
  {
    if (m_choosenMedia.HasProperty("httpCookies"))
    {
      protocolOpts = "Cookie=" + CURL::Encode(m_choosenMedia.GetProperty("httpCookies").asString());
      CLog::Log(LOGDEBUG, "CPlexMediaDecisionEngine::AddHeaders Cookie header %s", m_choosenMedia.GetProperty("httpCookies").asString().c_str());
    }

    if (m_choosenMedia.HasProperty("userAgent"))
    {
      CStdString ua="User-Agent=" + CURL::Encode(m_choosenMedia.GetProperty("userAgent").asString());
      if (protocolOpts.empty())
        protocolOpts = ua;
      else
        protocolOpts += "&" + ua;

      CLog::Log(LOGDEBUG, "CPlexMediaDecisionEngine::AddHeaders User-Agent header %s", m_choosenMedia.GetProperty("userAgent").asString().c_str());
    }
  }

  if (!protocolOpts.empty())
  {
    CURL url(m_choosenMedia.GetPath());
    url.SetProtocolOptions(protocolOpts);

    m_choosenMedia.SetPath(url.Get());
    CLog::Log(LOGDEBUG, "CPlexMediaDecisionEngine::AddHeaders new URL %s", m_choosenMedia.GetPath().c_str());
  }
}

CStdString CPlexMediaDecisionEngine::GetPartURL(CFileItemPtr mediaPart)
{
  CStdString unprocessed_key = mediaPart->GetProperty("unprocessed_key").asString();
  if (!mediaPart->IsRemotePlexMediaServerLibrary() && mediaPart->HasProperty("file"))
  {
    CStdString localPath = mediaPart->GetProperty("file").asString();
    if (XFILE::CFile::Exists(localPath))
      return localPath;
    else if (boost::starts_with(unprocessed_key, "rtmp"))
      return unprocessed_key;
    else
      return mediaPart->GetPath();
  }
  return mediaPart->GetPath();
}

/* this method is responsible for resolving and chosing what media item
 * should be passed to the player core */
void CPlexMediaDecisionEngine::ChooseMedia()
{
  /* resolve items that are not synthesized */
  if (m_item.IsPlexMediaServerLibrary() && m_item.IsVideo() &&
      !m_item.GetProperty("isSynthesized").asBoolean())
  {
    CFileItemList list;

    CLog::Log(LOGDEBUG, "CPlexMediaDecisionEngine::ChooseMedia loading extra information for item");

    if (!m_dir.GetDirectory(m_item.GetPath(), list))
    {
      m_success = false;
      return;
    }

    m_choosenMedia = *list.Get(0).get();
  }
  else
  {
    m_choosenMedia = m_item;
    if (m_choosenMedia.m_mediaItems.size() == 0)
    {
      m_success = true;
      return;
    }
  }
  
  if (m_item.HasProperty("selectedMediaItem"))
    m_choosenMedia.SetProperty("selectedMediaItem", m_item.GetProperty("selectedMediaItem"));

  CFileItemPtr mediaItem = getSelecteMediaItem(m_choosenMedia);
  if (!mediaItem)
  {
    m_success = false;
    return;
  }
  
  mediaItem = ResolveIndirect(mediaItem);
  if (!mediaItem)
  {
    m_success = false;
    return;
  }

  /* check if we got some httpHeaders from indirected item */
  if (mediaItem->HasProperty("httpHeaders"))
    m_choosenMedia.SetProperty("httpHeaders", mediaItem->GetProperty("httpHeaders"));

  /* FIXME: we really need to handle multiple parts */
  if (mediaItem->m_mediaParts.size() > 1)
  {
    /* Multi-part video, now we build a stack URL */
    CStdStringArray urls;
    BOOST_FOREACH(CFileItemPtr mediaPart, mediaItem->m_mediaParts)
      urls.push_back(GetPartURL(mediaPart));

    CStdString stackUrl;
    if (XFILE::CStackDirectory::ConstructStackPath(urls, stackUrl))
    {
      CLog::Log(LOGDEBUG, "%s created stack with URL %s", __FUNCTION__, stackUrl.c_str());
      m_choosenMedia.SetPath(stackUrl);
    }
  }
  else if (mediaItem->m_mediaParts.size() == 1)
  {
    m_choosenMedia.SetPath(GetPartURL(mediaItem->m_mediaParts[0]));
    m_choosenMedia.m_selectedMediaPart = mediaItem->m_mediaParts[0];
  }

  // Get details on the item we're playing.
  if (m_choosenMedia.IsPlexMediaServerLibrary())
  {
    /* find the server for the item */
    CPlexServerPtr server = g_plexApplication.serverManager->FindByUUID(m_choosenMedia.GetProperty("plexserver").asString());
    if (server && CPlexTranscoderClient::ShouldTranscode(server, m_choosenMedia))
    {
      CLog::Log(LOGDEBUG, "CPlexMediaDecisionEngine::ChooseMedia Item should be transcoded");
      m_choosenMedia.SetPath(CPlexTranscoderClient::GetTranscodeURL(server, m_choosenMedia).Get());
      m_choosenMedia.SetProperty("plexDidTranscode", true);
    }
  }

  AddHeaders();

  CLog::Log(LOGDEBUG, "CPlexMediaDecisionEngine::ChooseMedia final URL from MDE is %s", m_choosenMedia.GetPath().c_str());

  m_success = true;
}

/* Items from the library can be ordered in different ways, so they store
 * the id property in selectedMediaItem, but channels don't have id
 * properties, so we need to rely on the correct indexing. Let's trust that
 * shall we
 */
CFileItemPtr CPlexMediaDecisionEngine::getSelecteMediaItem(const CFileItem &item)
{
  int mediaItemIdx = 0;
  CFileItemPtr mediaItem;
  
  if (item.HasProperty("selectedMediaItem"))
    mediaItemIdx = item.GetProperty("selectedMediaItem").asInteger();
  
  for (int i = 0; i < item.m_mediaItems.size(); i ++)
  {
    if (item.m_mediaItems[i]->HasProperty("id") &&
        item.m_mediaItems[i]->GetProperty("id").asInteger() == mediaItemIdx)
      mediaItem = item.m_mediaItems[i];
  }
  
  if (!mediaItem && item.m_mediaItems.size() > 0)
  {
    if (mediaItemIdx > 0 && item.m_mediaItems.size() > mediaItemIdx)
      mediaItem = item.m_mediaItems[mediaItemIdx];
    else
      mediaItem = item.m_mediaItems[0];
  }
  
  return mediaItem;
}

/* This method fetches a mediaPart from a "root" fileItem. You can specify the partId
 * or leave blank to use the current selected or first one in the lists */
CFileItemPtr CPlexMediaDecisionEngine::getMediaPart(const CFileItem &item, int partId)
{
  CFileItemPtr mediaItem = getSelecteMediaItem(item);
  if (mediaItem && mediaItem->m_mediaParts.size() > 0)
  {
    if (partId == -1)
      return mediaItem->m_mediaParts.at(0);

    BOOST_FOREACH(CFileItemPtr mP, mediaItem->m_mediaParts)
    {
      if (mP->GetProperty("id").asInteger() == partId)
        return mP;
    }
  }

  return CFileItemPtr();
}

void CPlexMediaDecisionEngine::ProcessStack(const CFileItem& item, const CFileItemList &stack)
{
  CFileItemPtr mediaItem = getSelecteMediaItem(item);
  int64_t totalDuration = 0;
  
  for (int i = 0; i < stack.Size(); i++)
  {
    CFileItemPtr stackItem = stack.Get(i);
    CFileItemPtr mediaPart = mediaItem->m_mediaParts[i];
    CFileItemPtr currMediaItem = CFileItemPtr(new CFileItem);
    
    stackItem->SetProperty("isSynthesized", true);
    stackItem->SetProperty("partIndex", i);
    stackItem->SetProperty("file", mediaPart->GetProperty("file"));
    stackItem->SetProperty("selectedMediaItem", 0);
    
    int64_t dur = mediaPart->GetProperty("duration").asInteger();
    stackItem->SetProperty("duration", dur);
    totalDuration += dur;
    
    currMediaItem->m_mediaParts.clear();
    currMediaItem->m_mediaParts.push_back(mediaPart);
    stackItem->m_mediaItems.push_back(currMediaItem);
    
    stackItem->m_selectedMediaPart = mediaPart;
  }
}
