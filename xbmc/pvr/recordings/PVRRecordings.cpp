/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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

#include "utils/URIUtils.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "PVRRecordings.h"

using namespace PVR;

CPVRRecordings::CPVRRecordings(void) :
    m_bIsUpdating(false),
    m_strDirectoryHistory("pvr://recordings/")
{
    m_thumbLoader.SetNumOfWorkers(1); 
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
  while (strReturn.Left(1) == "/")
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
  if (!strUseBase.IsEmpty())
  {
    if (strUsePath.GetLength() <= strUseBase.GetLength() || strUsePath.Left(strUseBase.GetLength()) != strUseBase)
      return strReturn;
    strUsePath.erase(0, strUseBase.GetLength());
  }

  /* check for more occurences */
  int iDelimiter = strUsePath.Find('/');
  if (iDelimiter > 0)
    strReturn = strUsePath.Left(iDelimiter);
  else
    strReturn = strUsePath;

  return TrimSlashes(strReturn);
}

bool CPVRRecordings::IsDirectoryMember(const CStdString &strDirectory, const CStdString &strEntryDirectory, bool bDirectMember /* = true */) const
{
  CStdString strUseDirectory = TrimSlashes(strDirectory);
  CStdString strUseEntryDirectory = TrimSlashes(strEntryDirectory);

  return strUseEntryDirectory.Left(strUseDirectory.GetLength()).Equals(strUseDirectory) &&
      (!bDirectMember || strUseEntryDirectory.Equals(strUseDirectory));
}

void CPVRRecordings::GetContents(const CStdString &strDirectory, CFileItemList *results)
{
  for (unsigned int iRecordingPtr = 0; iRecordingPtr < size(); iRecordingPtr++)
  {
    CPVRRecording *current = at(iRecordingPtr);
    if (!IsDirectoryMember(strDirectory, current->m_strDirectory, true))
      continue;

    CFileItemPtr pFileItem(new CFileItem(*current));
    pFileItem->SetLabel2(current->RecordingTimeAsLocalTime().GetAsLocalizedDateTime(true, false));
    pFileItem->m_dateTime = current->RecordingTimeAsLocalTime();
    pFileItem->SetPath(current->m_strFileNameAndPath);

    // Set the play count either directly from client (if supported) or from video db
    if (g_PVRClients->GetAddonCapabilities(pFileItem->GetPVRRecordingInfoTag()->m_iClientId).bSupportsRecordingPlayCount)
    {
      pFileItem->GetPVRRecordingInfoTag()->m_playCount=pFileItem->GetPVRRecordingInfoTag()->m_iRecPlayCount;
    }
    else
    {
      CVideoDatabase db;
      if (db.Open())
      pFileItem->GetPVRRecordingInfoTag()->m_playCount=db.GetPlayCount(*pFileItem);
    }
    pFileItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, pFileItem->GetPVRRecordingInfoTag()->m_playCount > 0);

    results->Add(pFileItem);
  }
}

void CPVRRecordings::GetSubDirectories(const CStdString &strBase, CFileItemList *results, bool bAutoSkip /* = true */)
{
  CStdString strUseBase = TrimSlashes(strBase);

  std::set<CStdString> unwatchedFolders;

  for (unsigned int iRecordingPtr = 0; iRecordingPtr < size(); iRecordingPtr++)
  {
    CPVRRecording *current = at(iRecordingPtr);
    const CStdString strCurrent = GetDirectoryFromPath(current->m_strDirectory, strUseBase);
    if (strCurrent.IsEmpty())
      continue;

    CStdString strFilePath;
    if(strUseBase.empty())
      strFilePath.Format("pvr://recordings/%s/", strCurrent.c_str());
    else
      strFilePath.Format("pvr://recordings/%s/%s/", strUseBase.c_str(), strCurrent.c_str());

    if (!results->Contains(strFilePath))
    {
      CFileItemPtr pFileItem;
      pFileItem.reset(new CFileItem(strCurrent, true));
      pFileItem->SetPath(strFilePath);
      pFileItem->SetLabel(strCurrent);
      pFileItem->SetLabelPreformated(true);
      pFileItem->m_dateTime = current->RecordingTimeAsLocalTime();

      // Initialize folder overlay from play count (either directly from client or from video database)
      CVideoDatabase db;
      bool supportsPlayCount = g_PVRClients->GetAddonCapabilities(current->m_iClientId).bSupportsRecordingPlayCount;
      if ((supportsPlayCount && current->m_iRecPlayCount > 0) ||
          (!supportsPlayCount && db.Open() && db.GetPlayCount(*pFileItem) > 0))
      {
        pFileItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_WATCHED, false);
      }
      else
      {
        unwatchedFolders.insert(strFilePath);
      }

      results->Add(pFileItem);
    }
    else
    {
      CFileItemPtr pFileItem;
      pFileItem=results->Get(strFilePath);
      if (pFileItem->m_dateTime<current->RecordingTimeAsLocalTime())
        pFileItem->m_dateTime  = current->RecordingTimeAsLocalTime();

      // Unset folder overlay if recording is unwatched
      if (unwatchedFolders.find(strFilePath) == unwatchedFolders.end()) {
        CVideoDatabase db;
        bool supportsPlayCount = g_PVRClients->GetAddonCapabilities(current->m_iClientId).bSupportsRecordingPlayCount;
        if ((supportsPlayCount && current->m_iRecPlayCount == 0) || (!supportsPlayCount && db.Open() && db.GetPlayCount(*pFileItem) == 0))
        {
          pFileItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, false);
          unwatchedFolders.insert(strFilePath);
        }
      }
    }
  }

  CFileItemList files;
  GetContents(strBase, &files);

  if (bAutoSkip && results->Size() == 1 && files.Size() == 0)
  {
    CStdString strGetPath;
    strGetPath.Format("%s/%s/", strUseBase.c_str(), results->Get(0)->GetLabel());

    results->Clear();

    CLog::Log(LOGDEBUG, "CPVRRecordings - %s - '%s' only has 1 subdirectory, selecting that directory ('%s')",
        __FUNCTION__, strUseBase.c_str(), strGetPath.c_str());
    GetSubDirectories(strGetPath, results, true);
    return;
  }

  results->Append(files);

  if (!strUseBase.IsEmpty())
  {
    CStdString strLabel("..");
    CFileItemPtr pItem(new CFileItem(strLabel));
    pItem->SetPath(m_strDirectoryHistory);
    pItem->m_bIsFolder = true;
    pItem->m_bIsShareOrDrive = false;
    results->AddFront(pItem, 0);
  }
  m_strDirectoryHistory.Format("pvr://recordings/%s", strUseBase.c_str());
}

int CPVRRecordings::Load(void)
{
  Update();

  return size();
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

  NotifyObservers("recordings-reset");
}

int CPVRRecordings::GetNumRecordings()
{
  CSingleLock lock(m_critSection);
  return size();
}

int CPVRRecordings::GetRecordings(CFileItemList* results)
{
  CSingleLock lock(m_critSection);

  for (unsigned int iRecordingPtr = 0; iRecordingPtr < size(); iRecordingPtr++)
  {
    CFileItemPtr pFileItem(new CFileItem(*at(iRecordingPtr)));
    results->Add(pFileItem);
  }

  return size();
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
        database.ClearBookMarksOfFile(pItem->GetPath(), CBookmark::RESUME);

      database.SetPlayCount(*pItem, count);
    }

    database.Close();
  }

  return bResult;
}

bool CPVRRecordings::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  bool bSuccess(false);
  CFileItemList files;

  {
    CSingleLock lock(m_critSection);

    CURL url(strPath);
    CStdString strFileName = url.GetFileName();
    URIUtils::RemoveSlashAtEnd(strFileName);

    if (strFileName.Left(10) == "recordings")
    {
      strFileName.erase(0, 10);
      GetSubDirectories(strFileName, &items, true);
      GetContents(strFileName, &files);
      bSuccess = true;
    }
  }

  if(bSuccess)
  {
    for (int i = 0; i < files.Size(); i++)
    {
      CFileItemPtr pFileItem = files.Get(i);
      CFileItemPtr pThumbItem = items.Get(pFileItem->GetPath());
      if (!pThumbItem->HasThumbnail())
        m_thumbLoader.LoadItem(pThumbItem.get());
    }
  }

  return bSuccess;
}

CPVRRecording *CPVRRecordings::GetByPath(const CStdString &path)
{
  CPVRRecording *tag = NULL;
  CSingleLock lock(m_critSection);

  CURL url(path);
  CStdString fileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(fileName);

  if (fileName.Left(11) == "recordings/")
  {
    if (fileName.IsEmpty())
      return tag;

    for (unsigned int iRecordingPtr = 0; iRecordingPtr < size(); iRecordingPtr++)
    {
      CPVRRecording *recording = at(iRecordingPtr);

      if(path.Equals(recording->m_strFileNameAndPath))
      {
        tag = recording;
        break;
      }
    }
  }

  return tag;
}

void CPVRRecordings::Clear()
{
  CSingleLock lock(m_critSection);

  for (unsigned int iRecordingPtr = 0; iRecordingPtr < size(); iRecordingPtr++)
    delete at(iRecordingPtr);
  erase(begin(), end());
}

void CPVRRecordings::UpdateEntry(const CPVRRecording &tag)
{
  bool bFound = false;
  CSingleLock lock(m_critSection);

  for (unsigned int iRecordingPtr = 0; iRecordingPtr < size(); iRecordingPtr++)
  {
    CPVRRecording *currentTag = at(iRecordingPtr);
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
    push_back(newTag);
  }
}
