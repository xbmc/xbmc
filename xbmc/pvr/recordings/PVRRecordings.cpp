/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRRecordings.h"

#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "threads/SingleLock.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include <memory>
#include <utility>
#include <vector>

using namespace PVR;

CPVRRecordings::CPVRRecordings() = default;

CPVRRecordings::~CPVRRecordings()
{
  if (m_database && m_database->IsOpen())
    m_database->Close();
}

void CPVRRecordings::UpdateFromClients()
{
  CSingleLock lock(m_critSection);
  Unload();
  CServiceBroker::GetPVRManager().Clients()->GetRecordings(this, false);
  CServiceBroker::GetPVRManager().Clients()->GetRecordings(this, true);
}

int CPVRRecordings::Load()
{
  Unload();
  Update();
  return m_recordings.size();
}

void CPVRRecordings::Unload()
{
  CSingleLock lock(m_critSection);
  m_bDeletedTVRecordings = false;
  m_bDeletedRadioRecordings = false;
  m_iTVRecordings = 0;
  m_iRadioRecordings = 0;
  m_recordings.clear();
}

void CPVRRecordings::Update()
{
  CSingleLock lock(m_critSection);
  if (m_bIsUpdating)
    return;
  m_bIsUpdating = true;
  lock.Leave();

  CLog::LogFC(LOGDEBUG, LOGPVR, "Updating recordings");
  UpdateFromClients();

  lock.Enter();
  m_bIsUpdating = false;
  lock.Leave();

  CServiceBroker::GetPVRManager().PublishEvent(PVREvent::RecordingsInvalidated);
}

int CPVRRecordings::GetNumTVRecordings() const
{
  CSingleLock lock(m_critSection);
  return m_iTVRecordings;
}

bool CPVRRecordings::HasDeletedTVRecordings() const
{
  CSingleLock lock(m_critSection);
  return m_bDeletedTVRecordings;
}

int CPVRRecordings::GetNumRadioRecordings() const
{
  CSingleLock lock(m_critSection);
  return m_iRadioRecordings;
}

bool CPVRRecordings::HasDeletedRadioRecordings() const
{
  CSingleLock lock(m_critSection);
  return m_bDeletedRadioRecordings;
}

std::vector<std::shared_ptr<CPVRRecording>> CPVRRecordings::GetAll() const
{
  std::vector<std::shared_ptr<CPVRRecording>> recordings;

  CSingleLock lock(m_critSection);
  for (const auto& recordingEntry : m_recordings)
  {
    recordings.emplace_back(recordingEntry.second);
  }

  return recordings;
}

std::shared_ptr<CPVRRecording> CPVRRecordings::GetById(unsigned int iId) const
{
  CSingleLock lock(m_critSection);
  for (const auto recording : m_recordings)
  {
    if (iId == recording.second->m_iRecordingId)
      return recording.second;
  }

  return {};
}

std::shared_ptr<CPVRRecording> CPVRRecordings::GetByPath(const std::string& path) const
{
  CSingleLock lock(m_critSection);

  CPVRRecordingsPath recPath(path);
  if (recPath.IsValid())
  {
    bool bDeleted = recPath.IsDeleted();
    bool bRadio = recPath.IsRadio();

    for (const auto recording : m_recordings)
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

std::shared_ptr<CPVRRecording> CPVRRecordings::GetById(int iClientId, const std::string& strRecordingId) const
{
  std::shared_ptr<CPVRRecording> retVal;
  CSingleLock lock(m_critSection);
  const auto it = m_recordings.find(CPVRRecordingUid(iClientId, strRecordingId));
  if (it != m_recordings.end())
    retVal = it->second;

  return retVal;
}

void CPVRRecordings::UpdateFromClient(const std::shared_ptr<CPVRRecording>& tag)
{
  CSingleLock lock(m_critSection);

  if (tag->IsDeleted())
  {
    if (tag->IsRadio())
      m_bDeletedRadioRecordings = true;
    else
      m_bDeletedTVRecordings = true;
  }

  std::shared_ptr<CPVRRecording> existingTag = GetById(tag->m_iClientId, tag->m_strRecordingId);
  if (existingTag)
  {
    existingTag->Update(*tag);
  }
  else
  {
    tag->UpdateMetadata(GetVideoDatabase());
    tag->m_iRecordingId = ++m_iLastId;
    m_recordings.insert({CPVRRecordingUid(tag->m_iClientId, tag->m_strRecordingId), tag});
    if (tag->IsRadio())
      ++m_iRadioRecordings;
    else
      ++m_iTVRecordings;
  }
}

std::shared_ptr<CPVRRecording> CPVRRecordings::GetRecordingForEpgTag(const std::shared_ptr<CPVREpgInfoTag>& epgTag) const
{
  if (!epgTag)
    return {};

  CSingleLock lock(m_critSection);

  for (const auto recording : m_recordings)
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

bool CPVRRecordings::SetRecordingsPlayCount(const std::shared_ptr<CPVRRecording>& recording, int count)
{
  return ChangeRecordingsPlayCount(recording, count);
}

bool CPVRRecordings::IncrementRecordingsPlayCount(const std::shared_ptr<CPVRRecording>& recording)
{
  return ChangeRecordingsPlayCount(recording, INCREMENT_PLAY_COUNT);
}

bool CPVRRecordings::ChangeRecordingsPlayCount(const std::shared_ptr<CPVRRecording>& recording, int count)
{
  if (recording)
  {
    CSingleLock lock(m_critSection);

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
    CSingleLock lock(m_critSection);

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
    m_database.reset(new CVideoDatabase());
    m_database->Open();

    if (!m_database->IsOpen())
      CLog::LogF(LOGERROR, "Failed to open the video database");
  }

  return *m_database;
}
