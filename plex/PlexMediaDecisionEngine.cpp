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
#include "AdvancedSettings.h"
#include "Client/PlexExtraInfoLoader.h"
#include "dialogs/GUIDialogOK.h"
#include "LocalizeStrings.h"
#include "GUI/GUIDialogPlexMedia.h"
#include "PlayListPlayer.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexMediaDecisionEngine::checkItemPlayability(const CFileItem& item)
{
  /* something went wrong ... */
  if (item.HasProperty("unavailable") && item.GetProperty("unavailable").asBoolean())
  {
    CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(52000), g_localizeStrings.Get(52010), "", "");
    return false;
  }
  /* webkit can't be played by PHT */
  if (item.HasProperty("protocol") && item.GetProperty("protocol").asString() == "webkit")
  {
    CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(52000), g_localizeStrings.Get(52011), "", "");
    return false;
  }
  /* and we defintely not playing isos. */
  if (item.HasProperty("isdvd") && item.GetProperty("isdvd").asBoolean())
  {
    CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(52000), g_localizeStrings.Get(52012), "", "");
    return false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexMediaDecisionEngine::resolveItem(const CFileItem& _item, CFileItem &resolvedItem)
{
  // if we are trasnscoding (Matroska), then we want to rebuild the trasncoding url for seeking

  CFileItem item(_item);

  if (item.GetProperty("plexDidTranscode").asBoolean())
  {
    CPlexServerPtr server = g_plexApplication.serverManager->FindByUUID(item.GetProperty("plexserver").asString());

    if ((CPlexTranscoderClient::getServerTranscodeMode(server) == CPlexTranscoderClient::PLEX_TRANSCODE_MODE_MKV))
    {
      CStdString transcodeURL = CPlexTranscoderClient::GetTranscodeURL(server, item).Get();
      item.SetPath(transcodeURL);
    }
  }

  if (!checkItemPlayability(item))
    return false;

  int offset = item.m_lStartOffset;

  if (!item.GetProperty("isResolved").asBoolean())
  {
    if (!g_playlistPlayer.HasPlayedFirstFile() && item.IsVideo())
    {
      int selectedMedia = CGUIDialogPlexMedia::ProcessMediaChoice(item);
      if (selectedMedia == -1)
        return false;
      item.SetProperty("selectedMediaItem", selectedMedia);
      offset = CGUIDialogPlexMedia::ProcessResumeChoice(item);

      // we use -2 for "abort"
      if (offset == -2)
        return false;
    }

    g_plexApplication.busy.blockWaitingForJob(new CPlexMediaDecisionJob(item), this);
    CLog::Log(LOGDEBUG, "CPlexMediaDecisionEngine::BlockAndResolve resolve done, success: %s", m_success ? "Yes" : "No");
  }
  else
  {
    m_success = true;
    m_resolvedItem = item;
    CLog::Log(LOGDEBUG, "CPlexMediaDecisionEngine::resolveItem item already resolved");
  }
 
  if (m_success)
  {
    resolvedItem = m_resolvedItem;
    resolvedItem.SetProperty("isResolved", true);
    resolvedItem.m_lStartOffset = offset;
    resolvedItem.m_lEndOffset = item.m_lEndOffset;
    resolvedItem.SetProperty("viewOffset", item.GetProperty("viewOffset"));
    return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaDecisionEngine::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  m_success = success;

  CPlexMediaDecisionJob* mdeJob = static_cast<CPlexMediaDecisionJob*>(job);
  if (mdeJob)
    m_resolvedItem = mdeJob->m_choosenMedia;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/* Items from the library can be ordered in different ways, so they store
 * the id property in selectedMediaItem, but channels don't have id
 * properties, so we need to rely on the correct indexing. Let's trust that
 * shall we
 */
CFileItemPtr CPlexMediaDecisionEngine::getSelectedMediaItem(const CFileItem &item)
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

///////////////////////////////////////////////////////////////////////////////////////////////////
/* This method fetches a mediaPart from a "root" fileItem. You can specify the partId
 * or leave blank to use the current selected or first one in the lists */
CFileItemPtr CPlexMediaDecisionEngine::getMediaPart(const CFileItem &item, int partId)
{
  CFileItemPtr mediaItem = getSelectedMediaItem(item);
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

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaDecisionEngine::ProcessStack(const CFileItem& item, const CFileItemList &stack)
{
  CFileItemPtr mediaItem = getSelectedMediaItem(item);
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

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// MediaDecisionJob below
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaDecisionJob::Cancel()
{
  m_bStop = true;
  m_dir.CancelDirectory();
  m_http.Cancel();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CFileItemPtr CPlexMediaDecisionJob::ResolveIndirect(CFileItemPtr item)
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

  CLog::Log(LOGDEBUG, "CPlexMediaDecisionJob::ResolveIndirect Recursing %s", m_choosenMedia.GetPath().c_str());
  return ResolveIndirect(item);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaDecisionJob::AddHeaders()
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
      CLog::Log(LOGDEBUG, "CPlexMediaDecisionJob::AddHeaders Cookie header %s", m_choosenMedia.GetProperty("httpCookies").asString().c_str());
    }

    if (m_choosenMedia.HasProperty("userAgent"))
    {
      CStdString ua="User-Agent=" + CURL::Encode(m_choosenMedia.GetProperty("userAgent").asString());
      if (protocolOpts.empty())
        protocolOpts = ua;
      else
        protocolOpts += "&" + ua;

      CLog::Log(LOGDEBUG, "CPlexMediaDecisionJob::AddHeaders User-Agent header %s", m_choosenMedia.GetProperty("userAgent").asString().c_str());
    }
  }

  if (!protocolOpts.empty())
  {
    CURL url(m_choosenMedia.GetPath());
    url.SetProtocolOptions(protocolOpts);

    m_choosenMedia.SetPath(url.Get());
    CLog::Log(LOGDEBUG, "CPlexMediaDecisionJob::AddHeaders new URL %s", m_choosenMedia.GetPath().c_str());
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CStdString CPlexMediaDecisionJob::GetPartURL(CFileItemPtr mediaPart)
{
  CStdString unprocessed_key = mediaPart->GetProperty("unprocessed_key").asString();
  if (mediaPart->HasProperty("file"))
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

///////////////////////////////////////////////////////////////////////////////////////////////////
/* this method is responsible for resolving and chosing what media item
 * should be passed to the player core */
bool CPlexMediaDecisionJob::DoWork()
{
  /* resolve items that are not synthesized */
  if (m_item.IsPlexMediaServerLibrary() && m_item.IsVideo() &&
      !m_item.GetProperty("isSynthesized").asBoolean())
  {
    CFileItemListPtr list = CFileItemListPtr(new CFileItemList);

    CLog::Log(LOGDEBUG, "CPlexMediaDecisionJob::DoWork loading extra information for item");

    if (!m_dir.GetDirectory(m_item.GetPath(), *list))
      return false;

    m_choosenMedia = *list->Get(0);

    // since this item is loaded again we need to call the extra info loader
    g_plexApplication.extraInfo->LoadExtraInfoForItem(list);
  }
  else
  {
    m_choosenMedia = m_item;
    if (m_choosenMedia.m_mediaItems.size() == 0)
      return false;
  }
  
  if (m_item.HasProperty("selectedMediaItem"))
    m_choosenMedia.SetProperty("selectedMediaItem", m_item.GetProperty("selectedMediaItem"));

  CFileItemPtr mediaItem = CPlexMediaDecisionEngine::getSelectedMediaItem(m_choosenMedia);
  if (!mediaItem)
    return false;
  
  mediaItem = ResolveIndirect(mediaItem);
  if (!mediaItem)
    return false;

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
      m_choosenMedia.SetPath(stackUrl);
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
    if (server && CPlexTranscoderClient::GetInstance()->ShouldTranscode(server, m_choosenMedia))
    {
      CLog::Log(LOGDEBUG, "CPlexMediaDecisionJob::DoWork Item should be transcoded");
      m_choosenMedia.SetPath(CPlexTranscoderClient::GetTranscodeURL(server, m_choosenMedia).Get());
      m_choosenMedia.SetProperty("plexDidTranscode", true);
    }
  }

  AddHeaders();

  CLog::Log(LOGDEBUG, "CPlexMediaDecisionJob::DoWork final URL from MDE is %s", m_choosenMedia.GetPath().c_str());

  return true;
}

