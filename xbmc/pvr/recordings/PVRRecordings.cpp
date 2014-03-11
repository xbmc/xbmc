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
    m_iLastId(0)
{

}

void CPVRRecordings::UpdateFromClients(void)
{
  CSingleLock lock(m_critSection);
  Clear();
  g_PVRClients->GetRecordings(this);
}

CStdString CPVRRecordings::TrimSlashes(const CStdString &strOrig) const
{
  CStdString strReturn(strOrig);
  while (strReturn[0] == '/')
    strReturn.erase(0, 1);

  URIUtils::RemoveSlashAtEnd(strReturn);

  return strReturn;
}

const CStdString CPVRRecordings::GetDirectoryFromPath(const CStdString &strPath, const CStdString &strBase) const
{
  CStdString strReturn;
  CStdString strUsePath = TrimSlashes(strPath);
  CStdString strUseBase = TrimSlashes(strBase);

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

bool CPVRRecordings::IsDirectoryMember(const CStdString &strDirectory, const CStdString &strEntryDirectory, bool bDirectMember /* = true */) const
{
  CStdString strUseDirectory = TrimSlashes(strDirectory);
  CStdString strUseEntryDirectory = TrimSlashes(strEntryDirectory);

  /* Case-insensitive comparison since sub folders are created with case-insensitive matching (GetSubDirectories) */
  return StringUtils::StartsWithNoCase(strUseEntryDirectory, strUseDirectory) &&
      (!bDirectMember || strUseEntryDirectory.Equals(strUseDirectory));
}

void CPVRRecordings::GetContents(const CStdString &strDirectory, CFileItemList *results)
{
  for (unsigned int iRecordingPtr = 0; iRecordingPtr < m_recordings.size(); iRecordingPtr++)
  {
    CPVRRecording *current = m_recordings.at(iRecordingPtr);
    bool directMember = !HasAllRecordingsPathExtension(strDirectory);
    if (!IsDirectoryMember(RemoveAllRecordingsPathExtension(strDirectory), current->m_strDirectory, directMember))
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

    results->Add(pFileItem);
  }
}

void CPVRRecordings::GetSubDirectories(const CStdString &strBase, CFileItemList *results)
{
  CStdString strUseBase = TrimSlashes(strBase);

  std::set<CStdString> unwatchedFolders;

  for (unsigned int iRecordingPtr = 0; iRecordingPtr < m_recordings.size(); iRecordingPtr++)
  {
    CPVRRecording *current = m_recordings.at(iRecordingPtr);
    const CStdString strCurrent = GetDirectoryFromPath(current->m_strDirectory, strUseBase);
    if (strCurrent.empty())
      continue;

    CStdString strFilePath;
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

  int subDirectories = results->Size();
  CFileItemList files;
  GetContents(strBase, &files);

  if (results->Size() == 1 && files.Size() == 0)
  {
    CStdString strGetPath = StringUtils::Format("%s/%s/", strUseBase.c_str(), results->Get(0)->GetLabel().c_str());

    results->Clear();

    CLog::Log(LOGDEBUG, "CPVRRecordings - %s - '%s' only has 1 subdirectory, selecting that directory ('%s')",
        __FUNCTION__, strUseBase.c_str(), strGetPath.c_str());
    GetSubDirectories(strGetPath, results);
    return;
  }

  results->Append(files);

  // Add 'All Recordings' item (if we have at least one subdirectory in the list)
  if (subDirectories > 0)
  {
    CStdString strLabel(g_localizeStrings.Get(19270)); // "* All recordings"
    CFileItemPtr pItem(new CFileItem(strLabel));
    CStdString strAllPath;
    if(strUseBase.empty())
      strAllPath = "pvr://recordings";
    else
      strAllPath = StringUtils::Format("pvr://recordings/%s", strUseBase.c_str());
    pItem->SetPath(AddAllRecordingsPathExtension(strAllPath));
    pItem->SetSpecialSort(SortSpecialOnTop);
    pItem->SetLabelPreformated(true);
    pItem->m_bIsFolder = true;
    pItem->m_bIsShareOrDrive = false;
    for(int i=0; i<results->Size(); ++i)
    {
      if(pItem->m_dateTime < results->Get(i)->m_dateTime)
        pItem->m_dateTime = results->Get(i)->m_dateTime;
    }
    results->AddFront(pItem, 0);
  }
}

bool CPVRRecordings::HasAllRecordingsPathExtension(const CStdString &strDirectory)
{
  CStdString strUseDir = TrimSlashes(strDirectory);
  CStdString strAllRecordingsPathExtension(PVR_ALL_RECORDINGS_PATH_EXTENSION);

  if (strUseDir.size() < strAllRecordingsPathExtension.size())
    return false;

  if (strUseDir.size() == strAllRecordingsPathExtension.size())
    return strUseDir.Equals(strAllRecordingsPathExtension);

  return StringUtils::EndsWith(strUseDir, "/" + strAllRecordingsPathExtension);
}

CStdString CPVRRecordings::AddAllRecordingsPathExtension(const CStdString &strDirectory)
{
  if (HasAllRecordingsPathExtension(strDirectory))
    return strDirectory;

  CStdString strResult = strDirectory;
  if (!StringUtils::EndsWith(strDirectory, "/"))
    strResult = strResult + "/";

  return strResult + PVR_ALL_RECORDINGS_PATH_EXTENSION + "/";
}

CStdString CPVRRecordings::RemoveAllRecordingsPathExtension(const CStdString &strDirectory)
{
  if (!HasAllRecordingsPathExtension(strDirectory))
    return strDirectory;

  return strDirectory.substr(0, strDirectory.size() - strlen(PVR_ALL_RECORDINGS_PATH_EXTENSION) - 1);
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

  for (unsigned int iRecordingPtr = 0; iRecordingPtr < m_recordings.size(); iRecordingPtr++)
  {
    CFileItemPtr pFileItem(new CFileItem(*m_recordings.at(iRecordingPtr)));
    results->Add(pFileItem);
  }

  return m_recordings.size();
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

bool CPVRRecordings::RenameRecording(CFileItem &item, CStdString &strNewName)
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
      CStdString strPath = item->GetPath();
      CDirectory::GetDirectory(strPath, items);
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

bool CPVRRecordings::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  bool bSuccess(false);

  {
    CSingleLock lock(m_critSection);

    CURL url(strPath);
    CStdString strFileName = url.GetFileName();
    URIUtils::RemoveSlashAtEnd(strFileName);

    if (StringUtils::StartsWith(strFileName, "recordings"))
    {
      strFileName.erase(0, 10);
      GetSubDirectories(strFileName, &items);
      bSuccess = true;
    }
  }

  return bSuccess;
}

void CPVRRecordings::SetPlayCount(const CFileItem &item, int iPlayCount)
{
  if (!item.HasPVRRecordingInfoTag())
    return;

  const CPVRRecording *recording = item.GetPVRRecordingInfoTag();
  CSingleLock lock(m_critSection);
  for (unsigned int iRecordingPtr = 0; iRecordingPtr < m_recordings.size(); iRecordingPtr++)
  {
    CPVRRecording *current = m_recordings.at(iRecordingPtr);
    if (current->m_iClientId == recording->m_iClientId && current->m_strRecordingId.Equals(recording->m_strRecordingId))
    {
      current->SetPlayCount(iPlayCount);
      break;
    }
  }
}

void CPVRRecordings::GetAll(CFileItemList &items)
{
  CSingleLock lock(m_critSection);
  for (unsigned int iRecordingPtr = 0; iRecordingPtr < m_recordings.size(); iRecordingPtr++)
  {
    CPVRRecording *current = m_recordings.at(iRecordingPtr);
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
  for (std::vector<CPVRRecording *>::const_iterator it = m_recordings.begin(); !item && it != m_recordings.end(); ++it)
  {
    if (iId == (*it)->m_iRecordingId)
      item = CFileItemPtr(new CFileItem(**it));
  }

  return item;
}

CFileItemPtr CPVRRecordings::GetByPath(const CStdString &path)
{
  CURL url(path);
  CStdString fileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(fileName);

  CSingleLock lock(m_critSection);

  if (StringUtils::StartsWith(fileName, "recordings/"))
  {
    for (unsigned int iRecordingPtr = 0; iRecordingPtr < m_recordings.size(); iRecordingPtr++)
    {
      if(path.Equals(m_recordings.at(iRecordingPtr)->m_strFileNameAndPath))
      {
        CFileItemPtr fileItem(new CFileItem(*m_recordings.at(iRecordingPtr)));
        return fileItem;
      }
    }
  }

  CFileItemPtr fileItem(new CFileItem);
  return fileItem;
}

void CPVRRecordings::Clear()
{
  CSingleLock lock(m_critSection);

  for (unsigned int iRecordingPtr = 0; iRecordingPtr < m_recordings.size(); iRecordingPtr++)
    delete m_recordings.at(iRecordingPtr);
  m_recordings.erase(m_recordings.begin(), m_recordings.end());
}

void CPVRRecordings::UpdateEntry(const CPVRRecording &tag)
{
  bool bFound = false;
  CSingleLock lock(m_critSection);

  for (unsigned int iRecordingPtr = 0; iRecordingPtr < m_recordings.size(); iRecordingPtr++)
  {
    CPVRRecording *currentTag = m_recordings.at(iRecordingPtr);
    if (currentTag->m_iClientId == tag.m_iClientId &&
        currentTag->m_strRecordingId.Equals(tag.m_strRecordingId))
    {
      currentTag->Update(tag);
      bFound = true;
      break;
    }
  }

  if (!bFound)
  {
    CPVRRecording *newTag = new CPVRRecording();
    newTag->Update(tag);
    newTag->m_iRecordingId = ++m_iLastId;
    m_recordings.push_back(newTag);
  }
}
