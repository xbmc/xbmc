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

bool CPlexMediaDecisionEngine::BlockAndResolve(const CFileItem &item, CFileItem &resolvedItem)
{
  m_item = item;

  Create();
  WaitForThreadExit(0xFFFFFFFF);

  if (m_success)
  {
    resolvedItem = m_choosenMedia;
    return true;
  }
  return false;
}

void CPlexMediaDecisionEngine::Process()
{
  ChooseMedia();
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
    m_choosenMedia = m_item;

  int64_t mediaItemIdx = 0;
  if (m_choosenMedia.HasProperty("selectedMediaItem"))
    mediaItemIdx = m_choosenMedia.GetProperty("selectedMediaItem").asInteger();

  CFileItemPtr mediaItem;
  if (m_choosenMedia.m_mediaItems.size() > 0 && m_choosenMedia.m_mediaItems.size() > mediaItemIdx)
  {
    mediaItem = m_choosenMedia.m_mediaItems[mediaItemIdx];
  }
  else
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
  CFileItemPtr mediaPart = mediaItem->m_mediaParts[0];
  CStdString unprocessed_key = mediaItem->GetProperty("unprocessed_key").asString();
  if (!mediaPart->IsRemotePlexMediaServerLibrary() && mediaPart->HasProperty("file"))
  {
    CStdString localPath = mediaPart->GetProperty("file").asString();
    if (XFILE::CFile::Exists(localPath))
      m_choosenMedia.SetPath(localPath);
    else if (boost::starts_with(unprocessed_key, "rtmp"))
      m_choosenMedia.SetPath(unprocessed_key);
    else
      m_choosenMedia.SetPath(mediaPart->GetPath());
  }
  else
  {
    m_choosenMedia.SetPath(mediaPart->GetPath());
  }

  m_choosenMedia.m_selectedMediaPart = mediaPart;

  // Get details on the item we're playing.
  if (m_choosenMedia.IsPlexMediaServerLibrary())
  {
    /* find the server for the item */
    CPlexServerPtr server = g_plexServerManager.FindByUUID(m_choosenMedia.GetProperty("plexserver").asString());
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
