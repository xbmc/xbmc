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
#include "dialogs/GUIDialogOK.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "Util.h"
#include "URL.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "video/VideoDatabase.h"
#include "settings/Settings.h"

#include "utils/URIUtils.h"
#include "utils/StringUtils.h"

#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "PVRRecordings.h"

using namespace PVR;

CPVRRecordings::CPVRRecordings(void) :
    m_bIsUpdating(false),
    m_iLastId(0),
    m_bGroupItems(true)
{

}

void CPVRRecordings::UpdateFromClients(void)
{
  CSingleLock lock(m_critSection);
  Clear();
  g_PVRClients->GetRecordings(this);
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
  std::string strUseBase = TrimSlashes(strBase);

  std::set<std::string> unwatchedFolders;

  for (PVR_RECORDINGMAP_CITR it = m_recordings.begin(); it != m_recordings.end(); it++)
  {
    CPVRRecordingPtr current = it->second;
    const std::string strCurrent = GetDirectoryFromPath(current->m_strDirectory, strUseBase);
    if (strCurrent.empty())
      continue;

    std::string strFilePath;
    if(strUseBase.empty())
      strFilePath = StringUtils::Format("pvr://recordings/%s/", strCurrent.c_str());
    else
      strFilePath = StringUtils::Format("pvr://recordings/%s/%s/", strUseBase.c_str(), strCurrent.c_str());

    if (!results->Contains(strFilePath))
    {
      current->UpdateMetadata();
      CFileItemPtr pFileItem;
      pFileItem.reset(new CFileItem(strCurrent, true));
      pFileItem->SetPath(strFilePath);
      pFileItem->SetLabel(strCurrent);
      pFileItem->SetLabelPreformated(true);
      pFileItem->m_dateTime = current->RecordingTimeAsLocalTime();

      // Initialize folder overlay from play count (either directly from client or from video database)
      if (current->m_playCount > 0)
        pFileItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_WATCHED, false);
      else
        unwatchedFolders.insert(strFilePath);

      results->Add(pFileItem);
    }
    else
    {
      CFileItemPtr pFileItem;
      pFileItem=results->Get(strFilePath);
      if (pFileItem->m_dateTime<current->RecordingTimeAsLocalTime())
        pFileItem->m_dateTime  = current->RecordingTimeAsLocalTime();

      // Unset folder overlay if recording is unwatched
      if (unwatchedFolders.find(strFilePath) == unwatchedFolders.end())
      {
        if (current->m_playCount == 0)
        {
          pFileItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, false);
          unwatchedFolders.insert(strFilePath);
        }
      }
    }
  }
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

int CPVRRecordings::GetRecordings(CFileItemList* results)
{
  CSingleLock lock(m_critSection);

  for (PVR_RECORDINGMAP_CITR it = m_recordings.begin(); it != m_recordings.end(); it++)
  {
    CFileItemPtr pFileItem(new CFileItem(*it->second));
    results->Add(pFileItem);
  }

  return m_recordings.size();
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

  CPVRRecording *tag = (CPVRRecording *)item.GetPVRRecordingInfoTag();
  return tag->Delete();
}

bool CPVRRecordings::RenameRecording(CFileItem &item, std::string &strNewName)
{
  bool bReturn = false;

  if (!item.IsPVRRecording())
  {
    CLog::Log(LOGERROR, "CPVRRecordings - %s - cannot rename file: no valid recording tag", __FUNCTION__);
    return bReturn;
  }

  CPVRRecording* tag = item.GetPVRRecordingInfoTag();
  return tag->Rename(strNewName);
}

bool CPVRRecordings::SetRecordingsPlayCount(const CFileItemPtr &item, int count)
{
  bool bResult = false;

  CVideoDatabase database;
  if (database.Open())
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

      pItem->GetPVRRecordingInfoTag()->SetPlayCount(count);

      // Clear resume bookmark
      if (count > 0)
      {
        database.ClearBookMarksOfFile(pItem->GetPath(), CBookmark::RESUME);
        pItem->GetPVRRecordingInfoTag()->SetLastPlayedPosition(0);
      }

      database.SetPlayCount(*pItem, count);
    }

    database.Close();
  }

  return bResult;
}

bool CPVRRecordings::GetDirectory(const std::string& strPath, CFileItemList &items)
{
  CSingleLock lock(m_critSection);

  CURL url(strPath);
  std::string strDirectoryPath = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(strDirectoryPath);

  if (StringUtils::StartsWith(strDirectoryPath, "recordings"))
  {
    strDirectoryPath.erase(0, 10);

    // get the directory structure if in non-flatten mode
    if (m_bGroupItems)
      GetSubDirectories(strDirectoryPath, &items);

    // get all files of the currrent directory or recursively all files starting at the current directory if in flatten mode
    for (PVR_RECORDINGMAP_CITR it = m_recordings.begin(); it != m_recordings.end(); it++)
    {
      CPVRRecordingPtr current = it->second;

      // skip items that are not members of the target directory
      if (!IsDirectoryMember(strDirectoryPath, current->m_strDirectory))
        continue;

      current->UpdateMetadata();
      CFileItemPtr pFileItem(new CFileItem(*current));
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

void CPVRRecordings::SetPlayCount(const CFileItem &item, int iPlayCount)
{
  if (!item.HasPVRRecordingInfoTag())
    return;

  const CPVRRecording *recording = item.GetPVRRecordingInfoTag();
  CPVRRecordingPtr foundRecording = GetById(recording->m_iClientId, recording->m_strRecordingId);
  if (foundRecording)
  {
    CSingleLock lock(m_critSection);
    foundRecording->SetPlayCount(iPlayCount);
  }
}

void CPVRRecordings::GetAll(CFileItemList &items)
{
  CSingleLock lock(m_critSection);
  for (PVR_RECORDINGMAP_CITR it = m_recordings.begin(); it != m_recordings.end(); it++)
  {
    CPVRRecordingPtr current = it->second;
    current->UpdateMetadata();

    CFileItemPtr pFileItem(new CFileItem(*current));
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
      item = CFileItemPtr(new CFileItem(*(it->second)));
  }

  return item;
}

CFileItemPtr CPVRRecordings::GetByPath(const std::string &path)
{
  CURL url(path);
  std::string fileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(fileName);

  CSingleLock lock(m_critSection);

  if (StringUtils::StartsWith(fileName, "recordings/"))
  {
    for (PVR_RECORDINGMAP_CITR it = m_recordings.begin(); it != m_recordings.end(); it++)
    {
      CPVRRecordingPtr current = it->second;
      if (URIUtils::PathEquals(path, current->m_strFileNameAndPath))
      {
        CFileItemPtr fileItem(new CFileItem(*current));
        return fileItem;
      }
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
  m_recordings.clear();
}

void CPVRRecordings::UpdateEntry(const CPVRRecording &tag)
{
  CSingleLock lock(m_critSection);

  CPVRRecordingPtr newTag = GetById(tag.m_iClientId, tag.m_strRecordingId);
  if (newTag)
  {
    newTag->Update(tag);
  }
  else
  {
    newTag = CPVRRecordingPtr(new CPVRRecording);
    newTag->Update(tag);
    newTag->m_iRecordingId = ++m_iLastId;
    m_recordings.insert(std::make_pair(CPVRRecordingUid(newTag->m_iClientId, newTag->m_strRecordingId), newTag));
  }
}
