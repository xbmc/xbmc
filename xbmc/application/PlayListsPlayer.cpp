/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayListsPlayer.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "filesystem/PluginDirectory.h"
#include "filesystem/UPnPDirectory.h"
#include "filesystem/VideoDatabaseFile.h"
#include "utils/URIUtils.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

using namespace KODI;

namespace
{
std::shared_ptr<CApplicationPlayer> GetAppPlayer()
{
  return CServiceBroker::GetAppComponents().GetComponent<CApplicationPlayer>();
}
} // namespace

bool CPlayListsPlayer::Open(const CFileItem& item,
                            const CApplicationPlayLists::PlayOptions& options,
                            APPLICATION::StartsRun startsRun)
{
  return g_application.PlayFile(item, options.player, options.reopen, startsRun) !=
         CApplication::PlayResult::Failed;
}

bool CPlayListsPlayer::LoadLibraryTag(CFileItem& item) const
{
  if (!VIDEO::IsVideoDb(item) || item.HasVideoInfoTag())
    return false;
  *item.GetVideoInfoTag() = XFILE::CVideoDatabaseFile::GetVideoTag(CURL(item.GetDynPath()));
  return true;
}

CPlayListsPlayer::Queued CPlayListsPlayer::QueueNext(const CFileItem& item)
{
  CFileItem file(item);
  if (const CURL url(file.GetDynPath()); url.IsProtocol("plugin"))
    XFILE::CPluginDirectory::GetPluginResult(url.Get(), file, false);

#ifdef HAS_UPNP
  if (URIUtils::IsUPnP(file.GetDynPath()) &&
      !XFILE::CUPnPDirectory::GetResource(file.GetDynURL(), file))
    return Queued::Unresolved;
#endif

  return GetAppPlayer()->QueueNextFile(file) ? Queued::Yes : Queued::Refused;
}

void CPlayListsPlayer::NothingToQueue()
{
  GetAppPlayer()->OnNothingToQueueNotify();
}

void CPlayListsPlayer::Stop()
{
  g_application.StopPlaying();
}

void CPlayListsPlayer::Close()
{
  GetAppPlayer()->ClosePlayer();
}

bool CPlayListsPlayer::IsPlaying() const
{
  return GetAppPlayer()->IsPlaying();
}

bool CPlayListsPlayer::IsPlayingVideo() const
{
  return GetAppPlayer()->IsPlayingVideo();
}

bool CPlayListsPlayer::IsPlayingAudio() const
{
  return GetAppPlayer()->IsPlayingAudio();
}

std::string CPlayListsPlayer::GetName() const
{
  return GetAppPlayer()->GetName();
}

bool CPlayListsPlayer::RestartsOnPrevious() const
{
  return GetAppPlayer()->CanSeek() &&
         g_application.GetTime() > CApplication::ACTION_PREV_ITEM_THRESHOLD;
}
