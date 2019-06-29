/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRRecordings.h"

#include <utility>

#include "FileItem.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "threads/SingleLock.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecordingsPath.h"

using namespace PVR;

CPVRRecordings::~CPVRRecordings()
{
  if (m_database && m_database->IsOpen())
    m_database->Close();
}

void CPVRRecordings::UpdateFromClients(void)
{
  CSingleLock lock(m_critSection);
  Unload();
  CServiceBroker::GetPVRManager().Clients()->GetRecordings(this, false);
  CServiceBroker::GetPVRManager().Clients()->GetRecordings(this, true);
}

int CPVRRecordings::Load(void)
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

void CPVRRecordings::Update(void)
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

  CServiceBroker::GetPVRManager().SetChanged();
  CServiceBroker::GetPVRManager().NotifyObservers(ObservableMessageRecordings);
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

bool CPVRRecordings::Delete(const CFileItem& item)
{
  return item.m_bIsFolder ? DeleteDirectory(item) : DeleteRecording(item);
}

bool CPVRRecordings::DeleteDirectory(const CFileItem& directory)
{
  CFileItemList items;
  XFILE::CDirectory::GetDirectory(directory.GetPath(), items, "", XFILE::DIR_FLAG_DEFAULTS);

  bool allDeleted = true;
  for (const auto& item : items)
    allDeleted &= Delete(*item);

  return allDeleted;
}

bool CPVRRecordings::DeleteRecording(const CFileItem &item)
{
  if (!item.IsPVRRecording())
  {
    CLog::LogF(LOGERROR, "Cannot delete item: no valid recording tag");
    return false;
  }

  CPVRRecordingPtr tag = item.GetPVRRecordingInfoTag();
  return tag->Delete();
}

bool CPVRRecordings::Undelete(const CFileItem &item)
{
  if (!item.IsDeletedPVRRecording())
  {
    CLog::LogF(LOGERROR, "Cannot undelete item: no valid recording tag");
    return false;
  }

  CPVRRecordingPtr tag = item.GetPVRRecordingInfoTag();
  return tag->Undelete();
}

bool CPVRRecordings::RenameRecording(CFileItem &item, std::string &strNewName)
{
  if (!item.IsUsablePVRRecording())
  {
    CLog::LogF(LOGERROR, "Cannot rename item: no valid recording tag");
    return false;
  }

  CPVRRecordingPtr tag = item.GetPVRRecordingInfoTag();
  return tag->Rename(strNewName);
}

bool CPVRRecordings::DeleteAllRecordingsFromTrash()
{
  return CServiceBroker::GetPVRManager().Clients()->DeleteAllRecordingsFromTrash() == PVR_ERROR_NO_ERROR;
}

bool CPVRRecordings::SetRecordingsPlayCount(const CFileItemPtr &item, int count)
{
  return ChangeRecordingsPlayCount(item, count);
}

bool CPVRRecordings::IncrementRecordingsPlayCount(const CFileItemPtr &item)
{
  return ChangeRecordingsPlayCount(item, INCREMENT_PLAY_COUNT);
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

CFileItemPtr CPVRRecordings::GetById(unsigned int iId) const
{
  CFileItemPtr item;
  CSingleLock lock(m_critSection);
  for (const auto recording : m_recordings)
  {
    if (iId == recording.second->m_iRecordingId)
      item = CFileItemPtr(new CFileItem(recording.second));
  }

  return item;
}

CFileItemPtr CPVRRecordings::GetByPath(const std::string &path)
{
  CSingleLock lock(m_critSection);

  CPVRRecordingsPath recPath(path);
  if (recPath.IsValid())
  {
    bool bDeleted = recPath.IsDeleted();
    bool bRadio   = recPath.IsRadio();

    for (const auto recording : m_recordings)
    {
      CPVRRecordingPtr current = recording.second;
      // Omit recordings not matching criteria
      if (!URIUtils::PathEquals(path, current->m_strFileNameAndPath) ||
          bDeleted != current->IsDeleted() || bRadio != current->IsRadio())
        continue;

      CFileItemPtr fileItem(new CFileItem(current));
      return fileItem;
    }
  }

  CFileItemPtr fileItem(new CFileItem);
  return fileItem;
}

CPVRRecordingPtr CPVRRecordings::GetById(int iClientId, const std::string &strRecordingId) const
{
  CPVRRecordingPtr retVal;
  CSingleLock lock(m_critSection);
  const auto it = m_recordings.find(CPVRRecordingUid(iClientId, strRecordingId));
  if (it != m_recordings.end())
    retVal = it->second;

  return retVal;
}

void CPVRRecordings::UpdateFromClient(const CPVRRecordingPtr &tag)
{
  CSingleLock lock(m_critSection);

  if (tag->IsDeleted())
  {
    if (tag->IsRadio())
      m_bDeletedRadioRecordings = true;
    else
      m_bDeletedTVRecordings = true;
  }

  CPVRRecordingPtr newTag = GetById(tag->m_iClientId, tag->m_strRecordingId);
  if (newTag)
  {
    newTag->Update(*tag);
  }
  else
  {
    newTag = CPVRRecordingPtr(new CPVRRecording);
    newTag->Update(*tag);
    newTag->UpdateMetadata(GetVideoDatabase());
    newTag->m_iRecordingId = ++m_iLastId;
    m_recordings.insert(std::make_pair(CPVRRecordingUid(newTag->m_iClientId, newTag->m_strRecordingId), newTag));
    if (newTag->IsRadio())
      ++m_iRadioRecordings;
    else
      ++m_iTVRecordings;
  }
}

CPVRRecordingPtr CPVRRecordings::GetRecordingForEpgTag(const CPVREpgInfoTagPtr &epgTag) const
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

  return CPVRRecordingPtr();
}

bool CPVRRecordings::ChangeRecordingsPlayCount(const CFileItemPtr &item, int count)
{
  bool bResult = false;

  CSingleLock lock(m_critSection);

  CVideoDatabase& db = GetVideoDatabase();
  if (db.IsOpen())
  {
    bResult = true;

    CLog::LogFC(LOGDEBUG, LOGPVR, "Item path %s", item->GetPath().c_str());
    CFileItemList items;
    if (item->m_bIsFolder)
    {
      XFILE::CDirectory::GetDirectory(item->GetPath(), items, "", XFILE::DIR_FLAG_DEFAULTS);
    }
    else
      items.Add(item);

    CLog::LogFC(LOGDEBUG, LOGPVR, "Will set watched for %d items", items.Size());
    for (int i = 0; i < items.Size(); ++i)
    {
      CLog::LogFC(LOGDEBUG, LOGPVR, "Setting watched for item %d", i);

      CFileItemPtr pItem=items[i];
      if (pItem->m_bIsFolder)
      {
        CLog::LogFC(LOGDEBUG, LOGPVR, "Path %s is a folder, will call recursively", pItem->GetPath().c_str());
        if (pItem->GetLabel() != "..")
        {
          ChangeRecordingsPlayCount(pItem, count);
        }
        continue;
      }

      if (!pItem->HasPVRRecordingInfoTag())
        continue;

      const CPVRRecordingPtr recording = pItem->GetPVRRecordingInfoTag();
      if (recording)
      {
        if (count == INCREMENT_PLAY_COUNT)
          recording->IncrementPlayCount();
        else
          recording->SetPlayCount(count);

        // Clear resume bookmark
        if (recording->GetPlayCount() > 0)
        {
          db.ClearBookMarksOfFile(pItem->GetPath(), CBookmark::RESUME);
          recording->SetResumePoint(CBookmark());
        }

        if (count == INCREMENT_PLAY_COUNT)
          db.IncrementPlayCount(*pItem);
        else
          db.SetPlayCount(*pItem, count);
      }
    }

    CServiceBroker::GetPVRManager().PublishEvent(PVREvent::RecordingsInvalidated);
  }

  return bResult;
}

bool CPVRRecordings::MarkWatched(const CFileItemPtr &item, bool bWatched)
{
  if (bWatched)
    return IncrementRecordingsPlayCount(item);
  else
    return SetRecordingsPlayCount(item, 0);
}

bool CPVRRecordings::ResetResumePoint(const CFileItemPtr item)
{
  bool bResult = false;

  const CPVRRecordingPtr recording = item->GetPVRRecordingInfoTag();
  if (recording)
  {
    CSingleLock lock(m_critSection);

    CVideoDatabase& db = GetVideoDatabase();
    if (db.IsOpen())
    {
      bResult = true;

      db.ClearBookMarksOfFile(item->GetPath(), CBookmark::RESUME);
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
