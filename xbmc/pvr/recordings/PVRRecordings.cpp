/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRRecordings.h"

#include "ServiceBroker.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_epg.h" // EPG_TAG_INVALID_UID
#include "pvr/PVRCachedImages.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordingsPath.h"
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

CPVRRecordings::CPVRRecordings() = default;

CPVRRecordings::~CPVRRecordings()
{
  if (m_database && m_database->IsOpen())
    m_database->Close();
}

bool CPVRRecordings::UpdateFromClients(const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (m_bIsUpdating)
    return false;

  m_bIsUpdating = true;

  for (const auto& recording : m_recordings)
    recording.second->SetDirty(true);

  std::vector<int> failedClients;
  CServiceBroker::GetPVRManager().Clients()->GetRecordings(clients, this, false, failedClients);
  CServiceBroker::GetPVRManager().Clients()->GetRecordings(clients, this, true, failedClients);

  // remove recordings that were deleted at the backend
  for (auto it = m_recordings.begin(); it != m_recordings.end();)
  {
    if ((*it).second->IsDirty() && std::find(failedClients.begin(), failedClients.end(),
                                             (*it).second->ClientID()) == failedClients.end())
      it = m_recordings.erase(it);
    else
      ++it;
  }

  m_bIsUpdating = false;
  CServiceBroker::GetPVRManager().PublishEvent(PVREvent::RecordingsInvalidated);
  return true;
}

bool CPVRRecordings::Update(const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  return UpdateFromClients(clients);
}

void CPVRRecordings::Unload()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_bDeletedTVRecordings = false;
  m_bDeletedRadioRecordings = false;
  m_iTVRecordings = 0;
  m_iRadioRecordings = 0;
  m_recordings.clear();
}

void CPVRRecordings::UpdateInProgressSize()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_bIsUpdating)
    return;
  m_bIsUpdating = true;

  bool bHaveUpdatedInProgessRecording = false;
  for (auto& recording : m_recordings)
  {
    if (recording.second->IsInProgress())
    {
      if (recording.second->UpdateRecordingSize())
        bHaveUpdatedInProgessRecording = true;
    }
  }

  m_bIsUpdating = false;

  if (bHaveUpdatedInProgessRecording)
    CServiceBroker::GetPVRManager().PublishEvent(PVREvent::RecordingsInvalidated);
}

int CPVRRecordings::GetNumTVRecordings() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_iTVRecordings;
}

bool CPVRRecordings::HasDeletedTVRecordings() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_bDeletedTVRecordings;
}

int CPVRRecordings::GetNumRadioRecordings() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_iRadioRecordings;
}

bool CPVRRecordings::HasDeletedRadioRecordings() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_bDeletedRadioRecordings;
}

std::vector<std::shared_ptr<CPVRRecording>> CPVRRecordings::GetAll() const
{
  std::vector<std::shared_ptr<CPVRRecording>> recordings;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  std::transform(m_recordings.cbegin(), m_recordings.cend(), std::back_inserter(recordings),
                 [](const auto& recordingEntry) { return recordingEntry.second; });

  return recordings;
}

std::shared_ptr<CPVRRecording> CPVRRecordings::GetById(unsigned int iId) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto it =
      std::find_if(m_recordings.cbegin(), m_recordings.cend(),
                   [iId](const auto& recording) { return recording.second->RecordingID() == iId; });
  return it != m_recordings.cend() ? (*it).second : std::shared_ptr<CPVRRecording>();
}

std::shared_ptr<CPVRRecording> CPVRRecordings::GetByPath(const std::string& path) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  CPVRRecordingsPath recPath(path);
  if (recPath.IsValid())
  {
    bool bDeleted = recPath.IsDeleted();
    bool bRadio = recPath.IsRadio();

    for (const auto& recording : m_recordings)
    {
      std::shared_ptr<CPVRRecording> current = recording.second;
      // Omit recordings not matching criteria
      if (!URIUtils::PathEquals(path, current->m_strFileNameAndPath) ||
          bDeleted != current->IsDeleted() || bRadio != current->IsRadio())
        continue;

      return current;
    }
  }

  return {};
}

std::shared_ptr<CPVRRecording> CPVRRecordings::GetById(int iClientId,
                                                       const std::string& strRecordingId) const
{
  std::shared_ptr<CPVRRecording> retVal;
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const auto it = m_recordings.find(CPVRRecordingUid(iClientId, strRecordingId));
  if (it != m_recordings.end())
    retVal = it->second;

  return retVal;
}

void CPVRRecordings::UpdateFromClient(const std::shared_ptr<CPVRRecording>& tag,
                                      const CPVRClient& client)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (tag->IsDeleted())
  {
    if (tag->IsRadio())
      m_bDeletedRadioRecordings = true;
    else
      m_bDeletedTVRecordings = true;
  }

  std::shared_ptr<CPVRRecording> existingTag = GetById(tag->ClientID(), tag->ClientRecordingID());
  if (existingTag)
  {
    existingTag->Update(*tag, client);
    existingTag->SetDirty(false);
  }
  else
  {
    tag->UpdateMetadata(GetVideoDatabase(), client);
    tag->SetRecordingID(++m_iLastId);
    m_recordings.insert({CPVRRecordingUid(tag->ClientID(), tag->ClientRecordingID()), tag});
    if (tag->IsRadio())
      ++m_iRadioRecordings;
    else
      ++m_iTVRecordings;
  }
}

std::shared_ptr<CPVRRecording> CPVRRecordings::GetRecordingForEpgTag(
    const std::shared_ptr<CPVREpgInfoTag>& epgTag) const
{
  if (!epgTag)
    return {};

  std::unique_lock<CCriticalSection> lock(m_critSection);

  for (const auto& recording : m_recordings)
  {
    if (recording.second->IsDeleted())
      continue;

    if (recording.second->ClientID() != epgTag->ClientID())
      continue;

    if (recording.second->ChannelUid() != epgTag->UniqueChannelID())
      continue;

    unsigned int iEpgEvent = recording.second->BroadcastUid();
    if (iEpgEvent != EPG_TAG_INVALID_UID)
    {
      if (iEpgEvent == epgTag->UniqueBroadcastID())
        return recording.second;
    }
    else
    {
      if (recording.second->RecordingTimeAsUTC() <= epgTag->StartAsUTC() &&
          recording.second->EndTimeAsUTC() >= epgTag->EndAsUTC())
        return recording.second;
    }
  }

  return std::shared_ptr<CPVRRecording>();
}

bool CPVRRecordings::SetRecordingsPlayCount(const std::shared_ptr<CPVRRecording>& recording,
                                            int count)
{
  return ChangeRecordingsPlayCount(recording, count);
}

bool CPVRRecordings::IncrementRecordingsPlayCount(const std::shared_ptr<CPVRRecording>& recording)
{
  return ChangeRecordingsPlayCount(recording, INCREMENT_PLAY_COUNT);
}

bool CPVRRecordings::ChangeRecordingsPlayCount(const std::shared_ptr<CPVRRecording>& recording,
                                               int count)
{
  if (recording)
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);

    CVideoDatabase& db = GetVideoDatabase();
    if (db.IsOpen())
    {
      if (count == INCREMENT_PLAY_COUNT)
        recording->IncrementPlayCount();
      else
        recording->SetPlayCount(count);

      // Clear resume bookmark
      if (recording->GetPlayCount() > 0)
      {
        db.ClearBookMarksOfFile(recording->m_strFileNameAndPath, CBookmark::RESUME);
        recording->SetResumePoint(CBookmark());
      }

      CServiceBroker::GetPVRManager().PublishEvent(PVREvent::RecordingsInvalidated);
      return true;
    }
  }

  return false;
}

bool CPVRRecordings::MarkWatched(const std::shared_ptr<CPVRRecording>& recording, bool bWatched)
{
  if (bWatched)
    return IncrementRecordingsPlayCount(recording);
  else
    return SetRecordingsPlayCount(recording, 0);
}

bool CPVRRecordings::ResetResumePoint(const std::shared_ptr<CPVRRecording>& recording)
{
  bool bResult = false;

  if (recording)
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);

    CVideoDatabase& db = GetVideoDatabase();
    if (db.IsOpen())
    {
      bResult = true;

      db.ClearBookMarksOfFile(recording->m_strFileNameAndPath, CBookmark::RESUME);
      recording->SetResumePoint(CBookmark());

      CServiceBroker::GetPVRManager().PublishEvent(PVREvent::RecordingsInvalidated);
    }
  }
  return bResult;
}

CVideoDatabase& CPVRRecordings::GetVideoDatabase()
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

int CPVRRecordings::CleanupCachedImages()
{
  std::vector<std::string> urlsToCheck;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    for (const auto& recording : m_recordings)
    {
      urlsToCheck.emplace_back(recording.second->ClientIconPath());
      urlsToCheck.emplace_back(recording.second->ClientThumbnailPath());
      urlsToCheck.emplace_back(recording.second->ClientFanartPath());
      urlsToCheck.emplace_back(recording.second->m_strFileNameAndPath);
    }
  }

  static const std::vector<PVRImagePattern> urlPatterns = {
      {CPVRRecording::IMAGE_OWNER_PATTERN, ""}, // client-supplied icon, thumbnail, fanart
      {"video", "pvr://recordings/"}, // kodi-generated video thumbnail
  };
  return CPVRCachedImages::Cleanup(urlPatterns, urlsToCheck);
}
