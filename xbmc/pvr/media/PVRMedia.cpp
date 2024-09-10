/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRMedia.h"

#include "ServiceBroker.h"
#include "pvr/PVRCachedImages.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/media/PVRMediaPath.h"
#include "pvr/media/PVRMediaTag.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

using namespace PVR;

CPVRMedia::CPVRMedia() = default;

CPVRMedia::~CPVRMedia()
{
  if (m_database && m_database->IsOpen())
    m_database->Close();
}

bool CPVRMedia::UpdateFromClients(const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (m_bIsUpdating)
    return false;

  m_bIsUpdating = true;

  for (const auto& mediaTag : m_media)
    mediaTag.second->SetDirty(true);

  std::vector<int> failedClients;
  CServiceBroker::GetPVRManager().Clients()->GetMedia(clients, this, failedClients);

  // remove media that no longer exist at the backend
  for (auto it = m_media.begin(); it != m_media.end();)
  {
    if ((*it).second->IsDirty() && std::find(failedClients.begin(), failedClients.end(),
                                             (*it).second->ClientID()) == failedClients.end())
      it = m_media.erase(it);
    else
      ++it;
  }

  m_bIsUpdating = false;
  CServiceBroker::GetPVRManager().PublishEvent(PVREvent::MediaInvalidated);
  return true;
}

bool CPVRMedia::Update(const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  return UpdateFromClients(clients);
}

void CPVRMedia::Unload()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_iTVMedia = 0;
  m_iRadioMedia = 0;
  m_media.clear();
}

int CPVRMedia::GetNumTVMedia() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_iTVMedia;
}

int CPVRMedia::GetNumRadioMedia() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_iRadioMedia;
}

std::vector<std::shared_ptr<CPVRMediaTag>> CPVRMedia::GetAll() const
{
  std::vector<std::shared_ptr<CPVRMediaTag>> media;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  std::transform(m_media.cbegin(), m_media.cend(), std::back_inserter(media),
                 [](const auto& mediaTagEntry) { return mediaTagEntry.second; });

  return media;
}

std::shared_ptr<CPVRMediaTag> CPVRMedia::GetById(unsigned int iId) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto it =
      std::find_if(m_media.cbegin(), m_media.cend(),
                   [iId](const auto& mediaTag) { return mediaTag.second->MediaTagID() == iId; });
  return it != m_media.cend() ? (*it).second : std::shared_ptr<CPVRMediaTag>();
}

std::shared_ptr<CPVRMediaTag> CPVRMedia::GetByPath(const std::string& path) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  CPVRMediaPath mediaPath(path);
  if (mediaPath.IsValid())
  {
    bool bRadio = mediaPath.IsRadio();

    for (const auto& mediaTag : m_media)
    {
      std::shared_ptr<CPVRMediaTag> current = mediaTag.second;
      // Omit media not matching criteria
      if (!URIUtils::PathEquals(path, current->m_strFileNameAndPath) ||
          bRadio != current->IsRadio())
        continue;

      return current;
    }
  }

  return {};
}

std::shared_ptr<CPVRMediaTag> CPVRMedia::GetById(int iClientId,
                                                 const std::string& strMediaTagId) const
{
  std::shared_ptr<CPVRMediaTag> retVal;
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto it = m_media.find(CPVRMediaTagUid(iClientId, strMediaTagId));
  if (it != m_media.end())
    retVal = it->second;

  return retVal;
}

void CPVRMedia::UpdateFromClient(const std::shared_ptr<CPVRMediaTag>& tag, const CPVRClient& client)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  std::shared_ptr<CPVRMediaTag> existingTag = GetById(tag->ClientID(), tag->ClientMediaTagID());
  if (existingTag)
  {
    existingTag->Update(*tag, client);
    existingTag->SetDirty(false);
  }
  else
  {
    tag->UpdateMetadata(GetVideoDatabase(), client);
    tag->SetMediaTagID(++m_iLastId);
    m_media.insert({CPVRMediaTagUid(tag->ClientID(), tag->ClientMediaTagID()), tag});
    if (tag->IsRadio())
      ++m_iRadioMedia;
    else
      ++m_iTVMedia;
  }
}

bool CPVRMedia::SetMediaPlayCount(const std::shared_ptr<CPVRMediaTag>& mediaTag, int count)
{
  return ChangeMediaPlayCount(mediaTag, count);
}

bool CPVRMedia::IncrementMediaPlayCount(const std::shared_ptr<CPVRMediaTag>& mediaTag)
{
  return ChangeMediaPlayCount(mediaTag, INCREMENT_PLAY_COUNT);
}

bool CPVRMedia::ChangeMediaPlayCount(const std::shared_ptr<CPVRMediaTag>& mediaTag, int count)
{
  if (mediaTag)
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);

    CVideoDatabase& db = GetVideoDatabase();
    if (db.IsOpen())
    {
      if (count == INCREMENT_PLAY_COUNT)
        mediaTag->IncrementPlayCount();
      else
        mediaTag->SetPlayCount(count);

      // Clear resume bookmark
      if (mediaTag->GetPlayCount() > 0)
      {
        db.ClearBookMarksOfFile(mediaTag->m_strFileNameAndPath, CBookmark::RESUME);
        mediaTag->SetResumePoint(CBookmark());
      }

      CServiceBroker::GetPVRManager().PublishEvent(PVREvent::MediaInvalidated);
      return true;
    }
  }

  return false;
}

bool CPVRMedia::MarkWatched(const std::shared_ptr<CPVRMediaTag>& mediaTag, bool bWatched)
{
  if (bWatched)
    return IncrementMediaPlayCount(mediaTag);
  else
    return SetMediaPlayCount(mediaTag, 0);
}

bool CPVRMedia::ResetResumePoint(const std::shared_ptr<CPVRMediaTag>& mediaTag)
{
  bool bResult = false;

  if (mediaTag)
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);

    CVideoDatabase& db = GetVideoDatabase();
    if (db.IsOpen())
    {
      bResult = true;

      db.ClearBookMarksOfFile(mediaTag->m_strFileNameAndPath, CBookmark::RESUME);
      mediaTag->SetResumePoint(CBookmark());

      CServiceBroker::GetPVRManager().PublishEvent(PVREvent::MediaInvalidated);
    }
  }
  return bResult;
}

CVideoDatabase& CPVRMedia::GetVideoDatabase()
{
  if (!m_database)
  {
    m_database = std::make_unique<CVideoDatabase>();
    m_database->Open();

    if (!m_database->IsOpen())
      CLog::LogF(LOGERROR, "Failed to open the video database");
  }

  return *m_database;
}

int CPVRMedia::CleanupCachedImages()
{
  std::vector<std::string> urlsToCheck;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    for (const auto& mediaTag : m_media)
    {
      urlsToCheck.emplace_back(mediaTag.second->ClientIconPath());
      urlsToCheck.emplace_back(mediaTag.second->ClientThumbnailPath());
      urlsToCheck.emplace_back(mediaTag.second->ClientFanartPath());
      urlsToCheck.emplace_back(mediaTag.second->m_strFileNameAndPath);
    }
  }

  static const std::vector<PVRImagePattern> urlPatterns = {
      {CPVRMediaTag::IMAGE_OWNER_PATTERN, ""}, // client-supplied icon, thumbnail, fanart
      {"video", "pvr://media/"}, // kodi-generated video thumbnail
  };
  return CPVRCachedImages::Cleanup(urlPatterns, urlsToCheck);
}
