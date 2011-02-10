/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "utils/URIUtils.h"
#include "pvr/PVRManager.h"
#include "PVRRecordings.h"

CPVRRecordings g_PVRRecordings;

CPVRRecordings::CPVRRecordings(void)
{

}

void CPVRRecordings::Process()
{
  CSingleLock lock(m_critSection);
  CLIENTMAP *clients = g_PVRManager.Clients();

  Clear();

  /* Go thru all clients and receive there Recordings */
  CLIENTMAPITR itr = clients->begin();
  int cnt = 0;
  while (itr != clients->end())
  {
    /* Load only if the client have Recordings */
    if ((*itr).second->GetNumRecordings() > 0)
    {
      (*itr).second->GetAllRecordings(this);
    }
    itr++;
    cnt++;
  }

  for (unsigned int i = 0; i < size(); ++i)
  {
    CFileItemPtr pFileItem(new CFileItem(at(i)));

    CStdString Path;
    CStdString strTitle = at(i).Title();
    strTitle.Replace('/','-');

    Path.Format("pvr://recordings/client_%04i/", at(i).ClientID());
    if (at(i).Directory() != "")
      Path += at(i).Directory();

    URIUtils::AddSlashAtEnd(Path);
    Path += strTitle + ".pvr";
    at(i).SetPath(Path);
  }
  return;
}

void CPVRRecordings::Unload()
{
  CSingleLock lock(m_critSection);
  Clear();
}

bool CPVRRecordings::Update(bool Wait)
{
  if (Wait)
  {
    Process();
    return true;
  }
  else
  {
    Create();
    SetName("PVR Recordings Update");
    SetPriority(-5);
  }
  return false;
}

int CPVRRecordings::GetNumRecordings()
{
  CSingleLock lock(m_critSection);
  return size();
}

int CPVRRecordings::GetRecordings(CFileItemList* results)
{
  CSingleLock lock(m_critSection);
  for (unsigned int i = 0; i < size(); ++i)
  {
    CFileItemPtr pFileItem(new CFileItem(at(i)));
    results->Add(pFileItem);
  }
  return size();
}

bool CPVRRecordings::DeleteRecording(const CFileItem &item)
{
  /* Check if a CPVRRecordingInfoTag is inside file item */
  if (!item.IsPVRRecording())
  {
    CLog::Log(LOGERROR, "cPVRRecordings: DeleteRecording no RecordingInfoTag given!");
    return false;
  }

  const CPVRRecordingInfoTag* tag = item.GetPVRRecordingInfoTag();
  return tag->Delete();
}

bool CPVRRecordings::RenameRecording(CFileItem &item, CStdString &newname)
{
  /* Check if a CPVRRecordingInfoTag is inside file item */
  if (!item.IsPVRRecording())
  {
    CLog::Log(LOGERROR, "cPVRRecordings: RenameRecording no RecordingInfoTag given!");
    return false;
  }

  CPVRRecordingInfoTag* tag = item.GetPVRRecordingInfoTag();
  return tag->Rename(newname);
}

bool CPVRRecordings::RemoveRecording(const CFileItem &item)
{
  CSingleLock lock(m_critSection);

  if (!item.IsPVRRecording())
  {
    CLog::Log(LOGERROR, "cPVRRecordings: RemoveRecording no RecordingInfoTag given!");
    return false;
  }

  const CPVRRecordingInfoTag* tag = item.GetPVRRecordingInfoTag();

  for (unsigned int i = 0; i < size(); ++i)
  {
    if (tag == &at(i))
    {
      erase(begin()+i);
      return true;
    }
  }
  return false;
}

bool CPVRRecordings::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CSingleLock lock(m_critSection);

  CStdString base(strPath);
  URIUtils::RemoveSlashAtEnd(base);

  CURL url(strPath);
  CStdString fileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(fileName);

  if (fileName == "recordings")
  {
    for (unsigned int iRecordingPtr = 0; iRecordingPtr < size(); iRecordingPtr++)
    {
      CPVRRecordingInfoTag *recording = &at(iRecordingPtr);
      CFileItemPtr pFileItem(new CFileItem(*recording));
      pFileItem->SetLabel2(recording->RecordingTime().GetAsLocalizedDateTime(true, false));
      pFileItem->m_dateTime = recording->RecordingTime();
      pFileItem->m_strPath.Format("pvr://recordings/%05i-%05i.pvr", recording->ClientID(), recording->ClientIndex());
      items.Add(pFileItem);
    }
    return true;
  }

  return false;
}

CPVRRecordingInfoTag *CPVRRecordings::GetByPath(CStdString &path)
{
  CPVRRecordingInfoTag *tag = NULL;
  CSingleLock lock(m_critSection);

  CURL url(path);
  CStdString fileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(fileName);

  if (fileName.Left(11) == "recordings/")
  {
    fileName.erase(0,11);
    int iClientID = atoi(fileName.c_str());
    fileName.erase(0,5);

    if (fileName.IsEmpty())
      return tag;

    int iClientIndex = atoi(fileName.c_str());

    for (unsigned int iRecordingPtr = 0; iRecordingPtr < size(); iRecordingPtr++)
    {
      CPVRRecordingInfoTag *recording = &at(iRecordingPtr);

      if (recording->ClientID() == iClientID && recording->ClientIndex() == iClientIndex)
      {
        tag = &at(iRecordingPtr);
        break;
      }
    }
  }

  return tag;
}

void CPVRRecordings::Clear()
{
  /* Clear all current present Recordings inside list */
  erase(begin(), end());
  return;
}
