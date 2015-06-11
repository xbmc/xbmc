/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "FileItem.h"
#include "epg/EpgContainer.h"
#include "URL.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "video/VideoDatabase.h"

#include "utils/URIUtils.h"
#include "utils/StringUtils.h"

#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "PVRRecordings.h"

using namespace PVR;

CPVRRecordings::CPVRRecordings(void) :
    m_bIsUpdating(false),
    m_iLastId(0),
    m_bGroupItems(true),
    m_bHasDeleted(false)
{
  m_database.Open();
}

CPVRRecordings::~CPVRRecordings()
{
  Clear();
  if (m_database.IsOpen())
    m_database.Close();
}

void CPVRRecordings::UpdateFromClients(void)
{
  CSingleLock lock(m_critSection);
  Clear();
  g_PVRClients->GetRecordings(this, false);
  g_PVRClients->GetRecordings(this, true);
}

std::string CPVRRecordings::TrimSlashes(const std::string &strOrig) const
{
  std::string strReturn(strOrig);
  while (strReturn[0] == '/')
    strReturn.erase(0, 1);

  URIUtils::RemoveSlashAtEnd(strReturn);

  return strReturn;
}

const std::string CPVRRecordings::GetDirectoryFromPath(const std::string &strPath, const std::string &strBase) const
{
  std::string strReturn;
  std::string strUsePath = TrimSlashes(strPath);
  std::string strUseBase = TrimSlashes(strBase);

  /* strip the base or return an empty value if it doesn't fit or match */
  if (!strUseBase.empty())
  {
    /* adding "/" to make sure that base matches the complete folder name and not only parts of it */
    if (strUsePath.size() <= strUseBase.size() || !StringUtils::StartsWith(strUsePath, strUseBase + "/"))
      return strReturn;
    strUsePath.erase(0, strUseBase.size());
  }

  /* check for more occurences */
  size_t iDelimiter = strUsePath.find('/');
  if (iDelimiter != std::string::npos && iDelimiter > 0)
    strReturn = strUsePath.substr(0, iDelimiter);
  else
    strReturn = strUsePath;

  return TrimSlashes(strReturn);
}

bool CPVRRecordings::IsDirectoryMember(const std::string &strDirectory, const std::string &strEntryDirectory) const
{
  std::string strUseDirectory = TrimSlashes(strDirectory);
  std::string strUseEntryDirectory = TrimSlashes(strEntryDirectory);

  /* Case-insensitive comparison since sub folders are created with case-insensitive matching (GetSubDirectories) */
  return m_bGroupItems ?
      StringUtils::EqualsNoCase(strUseDirectory, strUseEntryDirectory) :
        StringUtils::StartsWithNoCase(strUseEntryDirectory, strUseDirectory);
}

void CPVRRecordings::GetSubDirectories(const std::string &strBase, CFileItemList *results)
{
  // Only active recordings are fetched to provide sub directories.
  // Not applicable for deleted view which is supposed to be flattened.
  std::string strUseBase = TrimSlashes(strBase);
  std::set<CFileItemPtr> unwatchedFolders;

  for (PVR_RECORDINGMAP_CITR it = m_recordings.begin(); it != m_recordings.end(); it++)
  {
    CPVRRecordingPtr current = it->second;
    if (current->IsDeleted())
      continue;
    const std::string strCurrent = GetDirectoryFromPath(current->m_strDirectory, strUseBase);
    if (strCurrent.empty())
      continue;

    std::string strFilePath;
    if(strUseBase.empty())
      strFilePath = StringUtils::Format("pvr://" PVR_RECORDING_BASE_PATH "/" PVR_RECORDING_ACTIVE_PATH "/%s/", strCurrent.c_str());
    else
      strFilePath = StringUtils::Format("pvr://" PVR_RECORDING_BASE_PATH "/" PVR_RECORDING_ACTIVE_PATH "/%s/%s/", strUseBase.c_str(), strCurrent.c_str());

    CFileItemPtr pFileItem;
    if (m_database.IsOpen())
      current->UpdateMetadata(m_database);

    if (!results->Contains(strFilePath))
    {
      pFileItem.reset(new CFileItem(strCurrent, true));
      pFileItem->SetPath(strFilePath);
      pFileItem->SetLabel(strCurrent);
      pFileItem->SetLabelPreformated(true);
      pFileItem->m_dateTime = current->RecordingTimeAsLocalTime();

      // Assume all folders are watched, we'll change the overlay later
      pFileItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_WATCHED, false);
      results->Add(pFileItem);
    }
    else
    {
      pFileItem=results->Get(strFilePath);
      if (pFileItem->m_dateTime<current->RecordingTimeAsLocalTime())
        pFileItem->m_dateTime  = current->RecordingTimeAsLocalTime();
    }

    if (current->m_playCount == 0)
      unwatchedFolders.insert(pFileItem);
  }

  // Remove the watched overlay from folders containing unwatched entries
  for (std::set<CFileItemPtr>::iterator it = unwatchedFolders.begin(); it != unwatchedFolders.end(); ++it)
    (*it)->SetOverlayImage(CGUIListItem::ICON_OVERLAY_WATCHED, true);
}

int CPVRRecordings::Load(void)
{
  Update();

  return m_recordings.size();
}

void CPVRRecordings::Unload()
{
  Clear();
}

void CPVRRecordings::Update(void)
{
  CSingleLock lock(m_critSection);
  if (m_bIsUpdating)
    return;
  m_bIsUpdating = true;
  lock.Leave();

  CLog::Log(LOGDEBUG, "CPVRRecordings - %s - updating recordings", __FUNCTION__);
  UpdateFromClients();

  lock.Enter();
  m_bIsUpdating = false;
  SetChanged();
  lock.Leave();

  NotifyObservers(ObservableMessageRecordings);
}

int CPVRRecordings::GetNumRecordings()
{
  CSingleLock lock(m_critSection);
  return m_recordings.size();
}

bool CPVRRecordings::HasDeletedRecordings()
{
  CSingleLock lock(m_critSection);
  return m_bHasDeleted;
}

int CPVRRecordings::GetRecordings(CFileItemList* results, bool bDeleted)
{
  CSingleLock lock(m_critSection);

  int iRecCount = 0;
  for (PVR_RECORDINGMAP_CITR it = m_recordings.begin(); it != m_recordings.end(); it++)
  {
    if (it->second->IsDeleted() != bDeleted)
      continue;

    CFileItemPtr pFileItem(new CFileItem(it->second));
    results->Add(pFileItem);
    iRecCount++;
  }

  return iRecCount;
}

bool CPVRRecordings::Delete(const CFileItem& item)
{
  return item.m_bIsFolder ? DeleteDirectory(item) : DeleteRecording(item);
}

bool CPVRRecordings::DeleteDirectory(const CFileItem& directory)
{
  CFileItemList items;
  XFILE::CDirectory::GetDirectory(directory.GetPath(), items);

  bool allDeleted = true;

  VECFILEITEMS itemList = items.GetList();
  CFileItem item;

  for (VECFILEITEMS::const_iterator it = itemList.begin(); it != itemList.end(); ++it)
    allDeleted &= Delete(*(it->get()));

  return allDeleted;
}

bool CPVRRecordings::DeleteRecording(const CFileItem &item)
{
  if (!item.IsPVRRecording())
  {
    CLog::Log(LOGERROR, "CPVRRecordings - %s - cannot delete file: no valid recording tag", __FUNCTION__);
    return false;
  }

  CPVRRecordingPtr tag = item.GetPVRRecordingInfoTag();
  return tag->Delete();
}

bool CPVRRecordings::Undelete(const CFileItem &item)
{
  if (!item.IsDeletedPVRRecording())
  {
    CLog::Log(LOGERROR, "CPVRRecordings - %s - cannot undelete file: no valid recording tag", __FUNCTION__);
    return false;
  }

  CPVRRecordingPtr tag = item.GetPVRRecordingInfoTag();
  return tag->Undelete();
}

bool CPVRRecordings::RenameRecording(CFileItem &item, std::string &strNewName)
{
  if (!item.IsUsablePVRRecording())
  {
    CLog::Log(LOGERROR, "CPVRRecordings - %s - cannot rename file: no valid recording tag", __FUNCTION__);
    return false;
  }

  CPVRRecordingPtr tag = item.GetPVRRecordingInfoTag();
  return tag->Rename(strNewName);
}

bool CPVRRecordings::DeleteAllRecordingsFromTrash()
{
  return g_PVRClients->DeleteAllRecordingsFromTrash() == PVR_ERROR_NO_ERROR;
}

bool CPVRRecordings::SetRecordingsPlayCount(const CFileItemPtr &item, int count)
{
  bool bResult = false;

  if (m_database.IsOpen())
  {
    bResult = true;

    CLog::Log(LOGDEBUG, "CPVRRecordings - %s - item path %s", __FUNCTION__, item->GetPath().c_str());
    CFileItemList items;
    if (item->m_bIsFolder)
    {
      XFILE::CDirectory::GetDirectory(item->GetPath(), items);
    }
    else
      items.Add(item);

    CLog::Log(LOGDEBUG, "CPVRRecordings - %s - will set watched for %d items", __FUNCTION__, items.Size());
    for (int i=0;i<items.Size();++i)
    {
      CLog::Log(LOGDEBUG, "CPVRRecordings - %s - setting watched for item %d", __FUNCTION__, i);

      CFileItemPtr pItem=items[i];
      if (pItem->m_bIsFolder)
      {
        CLog::Log(LOGDEBUG, "CPVRRecordings - %s - path %s is a folder, will call recursively", __FUNCTION__, pItem->GetPath().c_str());
        if (pItem->GetLabel() != "..")
        {
          SetRecordingsPlayCount(pItem, count);
        }
        continue;
      }

      if (!pItem->HasPVRRecordingInfoTag())
        continue;

      const CPVRRecordingPtr recording = pItem->GetPVRRecordingInfoTag();
      if (recording)
      {
        recording->SetPlayCount(count);

        // Clear resume bookmark
        if (count > 0)
        {
          m_database.ClearBookMarksOfFile(pItem->GetPath(), CBookmark::RESUME);
          recording->SetLastPlayedPosition(0);
        }

        m_database.SetPlayCount(*pItem, count);
      }
    }
  }

  return bResult;
}

bool CPVRRecordings::GetDirectory(const std::string& strPath, CFileItemList &items)
{
  CSingleLock lock(m_critSection);

  CURL url(strPath);
  std::string strDirectoryPath = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(strDirectoryPath);

  if (StringUtils::StartsWith(strDirectoryPath, PVR_RECORDING_BASE_PATH))
  {
    strDirectoryPath.erase(0, sizeof(PVR_RECORDING_BASE_PATH) - 1);

    // Check directory name is for deleted recordings
    bool bDeleted = StringUtils::StartsWith(strDirectoryPath, "/" PVR_RECORDING_DELETED_PATH);
    strDirectoryPath.erase(0, bDeleted ? sizeof(PVR_RECORDING_DELETED_PATH) : sizeof(PVR_RECORDING_ACTIVE_PATH));

    // Get the directory structure if in non-flatten mode
    // Deleted view is always flatten. So only for an active view
    if (!bDeleted && m_bGroupItems)
      GetSubDirectories(strDirectoryPath, &items);

    // get all files of the currrent directory or recursively all files starting at the current directory if in flatten mode
    for (PVR_RECORDINGMAP_CITR it = m_recordings.begin(); it != m_recordings.end(); it++)
    {
      CPVRRecordingPtr current = it->second;

      // skip items that are not members of the target directory
      if (!IsDirectoryMember(strDirectoryPath, current->m_strDirectory) || current->IsDeleted() != bDeleted)
        continue;

      if (m_database.IsOpen())
        current->UpdateMetadata(m_database);
      CFileItemPtr pFileItem(new CFileItem(current));
      pFileItem->SetLabel2(current->RecordingTimeAsLocalTime().GetAsLocalizedDateTime(true, false));
      pFileItem->m_dateTime = current->RecordingTimeAsLocalTime();
      pFileItem->SetPath(current->m_strFileNameAndPath);

      // Set art
      if (!current->m_strIconPath.empty())
      {
        pFileItem->SetIconImage(current->m_strIconPath);
        pFileItem->SetArt("icon", current->m_strIconPath);
      }

      if (!current->m_strThumbnailPath.empty())
        pFileItem->SetArt("thumb", current->m_strThumbnailPath);

      if (!current->m_strFanartPath.empty())
        pFileItem->SetArt("fanart", current->m_strFanartPath);

      // Use the channel icon as a fallback when a thumbnail is not available
      pFileItem->SetArtFallback("thumb", "icon");

      pFileItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, pFileItem->GetPVRRecordingInfoTag()->m_playCount > 0);

      items.Add(pFileItem);
    }

    return true;
  }

  return false;
}

void CPVRRecordings::GetAll(CFileItemList &items, bool bDeleted)
{
  CSingleLock lock(m_critSection);
  for (PVR_RECORDINGMAP_CITR it = m_recordings.begin(); it != m_recordings.end(); it++)
  {
    CPVRRecordingPtr current = it->second;
    if (current->IsDeleted() != bDeleted)
      continue;

    if (m_database.IsOpen())
      current->UpdateMetadata(m_database);

    CFileItemPtr pFileItem(new CFileItem(current));
    pFileItem->SetLabel2(current->RecordingTimeAsLocalTime().GetAsLocalizedDateTime(true, false));
    pFileItem->m_dateTime = current->RecordingTimeAsLocalTime();
    pFileItem->SetPath(current->m_strFileNameAndPath);
    pFileItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, pFileItem->GetPVRRecordingInfoTag()->m_playCount > 0);

    items.Add(pFileItem);
  }
}

CFileItemPtr CPVRRecordings::GetById(unsigned int iId) const
{
  CFileItemPtr item;
  CSingleLock lock(m_critSection);
  for (PVR_RECORDINGMAP_CITR it = m_recordings.begin(); it != m_recordings.end(); it++)
  {
    if (iId == it->second->m_iRecordingId)
      item = CFileItemPtr(new CFileItem(it->second));
  }

  return item;
}

CFileItemPtr CPVRRecordings::GetByPath(const std::string &path)
{
  CURL url(path);
  std::string fileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(fileName);

  CSingleLock lock(m_critSection);

  if (StringUtils::StartsWith(fileName, PVR_RECORDING_BASE_PATH "/"))
  {
    // Check directory name is for deleted recordings
    fileName.erase(0, sizeof(PVR_RECORDING_BASE_PATH));
    bool bDeleted = StringUtils::StartsWith(fileName, PVR_RECORDING_DELETED_PATH "/");

    for (PVR_RECORDINGMAP_CITR it = m_recordings.begin(); it != m_recordings.end(); it++)
    {
      CPVRRecordingPtr current = it->second;
      if (!URIUtils::PathEquals(path, current->m_strFileNameAndPath) || bDeleted != current->IsDeleted())
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
  PVR_RECORDINGMAP_CITR it = m_recordings.find(CPVRRecordingUid(iClientId, strRecordingId));
  if (it != m_recordings.end())
    retVal = it->second;

  return retVal;
}

void CPVRRecordings::Clear()
{
  CSingleLock lock(m_critSection);
  m_bHasDeleted = false;
  m_recordings.clear();
}

void CPVRRecordings::UpdateFromClient(const CPVRRecordingPtr &tag)
{
  CSingleLock lock(m_critSection);

  if (tag->IsDeleted())
    m_bHasDeleted = true;

  CPVRRecordingPtr newTag = GetById(tag->m_iClientId, tag->m_strRecordingId);
  if (newTag)
  {
    newTag->Update(*tag);
  }
  else
  {
    newTag = CPVRRecordingPtr(new CPVRRecording);
    newTag->Update(*tag);
    if (newTag->EpgEvent() > 0)
    {
      EPG::CEpgInfoTagPtr epgTag = EPG::CEpgContainer::Get().GetTagById(newTag->EpgEvent());
      if (epgTag)
        epgTag->SetRecording(newTag);
    }
    newTag->m_iRecordingId = ++m_iLastId;
    m_recordings.insert(std::make_pair(CPVRRecordingUid(newTag->m_iClientId, newTag->m_strRecordingId), newTag));
  }
}

void CPVRRecordings::UpdateEpgTags(void)
{
  CSingleLock lock(m_critSection);
  int iEpgEvent;
  for (PVR_RECORDINGMAP_ITR it = m_recordings.begin(); it != m_recordings.end(); ++it)
  {
    iEpgEvent = it->second->EpgEvent();
    if (iEpgEvent > 0 && !it->second->IsDeleted())
    {
      EPG::CEpgInfoTagPtr epgTag = EPG::CEpgContainer::Get().GetTagById(iEpgEvent);
      if (epgTag)
        epgTag->SetRecording(it->second);
    }
  }
}
