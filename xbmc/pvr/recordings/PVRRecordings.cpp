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
//    if (g_PVRManager.Clients()->size() > 1)
    {
      CLIENTMAPITR itr = g_PVRManager.Clients()->begin();
      while (itr != g_PVRManager.Clients()->end())
      {
        CFileItemPtr item;
        CStdString dirName;
        CStdString clientName;

        int clientID = g_PVRManager.Clients()->find((*itr).first)->second->GetID();
        clientName.Format(g_localizeStrings.Get(19016), clientID, g_PVRManager.Clients()->find((*itr).first)->second->GetBackendName());
        dirName.Format("%s/client_%04i/", base, clientID);
        item.reset(new CFileItem(dirName, true));
        item->SetLabel(clientName);
        item->SetLabelPreformated(true);
        items.Add(item);

        itr++;
      }
      return true;
    }
  }
  else if (fileName.Left(18) == "recordings/client_")
  {
    fileName.erase(0,18);
    int clientID = atoi(fileName.c_str());
    CStdString curDir = url.GetFileName();
    URIUtils::AddSlashAtEnd(curDir);

    CStdString strBuffer;
    CStdString strSkip;
    std::vector<CStdString> baseTokens;
    if (!curDir.IsEmpty())
      CUtil::Tokenize(curDir, baseTokens, "/");

    for (unsigned int i = 0; i < size(); ++i)
    {
      if (clientID != at(i).ClientID())
        continue;

      CStdString strEntryName;
      CStdString strTitle = at(i).Title();
      strEntryName.Format("recordings/client_%04i/%s", clientID, at(i).Directory());
      URIUtils::AddSlashAtEnd(strEntryName);

      strTitle.Replace('/','-');
      strEntryName += strTitle;
      strEntryName.Replace('\\','/');
      CStdString strOrginalName = strEntryName;

      if (strEntryName == curDir) // skip the listed dir
        continue;

      std::vector<CStdString> pathTokens;
      CUtil::Tokenize(strEntryName, pathTokens, "/");
      if (pathTokens.size() < baseTokens.size()+1)
        continue;

      bool bAdd = true;
      strEntryName = "";
      for (unsigned int j = 0; j < baseTokens.size(); ++j)
      {
        if (pathTokens[j] != baseTokens[j])
        {
          bAdd = false;
          break;
        }
        strEntryName += pathTokens[j] + "/";
      }
      if (!bAdd)
        continue;

      strEntryName += pathTokens[baseTokens.size()];
      char c=strOrginalName[strEntryName.size()];
      if (c == '/' || c == '\\')
        strEntryName += '/';

      CFileItemPtr pFileItem;
      bool bIsFolder = false;
      if (strEntryName[strEntryName.size()-1] != '/') // this is a file
      {
        pFileItem.reset(new CFileItem(at(i)));
        pFileItem->SetLabel(pathTokens[baseTokens.size()]);
        pFileItem->SetLabel2(at(i).RecordingTime().GetAsLocalizedDateTime(true, false));
        pFileItem->m_dateTime = at(i).RecordingTime();
        pFileItem->m_strPath.Format("pvr://%s-%05i.pvr", strEntryName, at(i).ClientIndex());
      }
      else
      { // this is new folder. add if not already added
        bIsFolder = true;
        strBuffer = "pvr://" + strEntryName;
        if (items.Contains(strBuffer)) // already added
          continue;

        pFileItem.reset(new CFileItem(strBuffer, true));
        pFileItem->SetLabel(pathTokens[baseTokens.size()]);
      }
      pFileItem->SetLabelPreformated(true);
      items.Add(pFileItem);
    }
    return true;
  }
  return false;
}

CPVRRecordingInfoTag *CPVRRecordings::GetByPath(CStdString &path)
{
  CSingleLock lock(m_critSection);

  CURL url(path);
  CStdString fileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(fileName);

  if (fileName.Left(18) == "recordings/client_")
  {
    fileName.erase(0,18);
    int clientID = atoi(fileName.c_str());
    fileName.erase(0,5);

    if (fileName.IsEmpty())
      return NULL;

    CStdString title;
    CStdString dir;
    size_t found = fileName.find_last_of("/");
    if (found != CStdString::npos)
    {
      title = fileName.substr(found+1);
      dir = fileName.substr(0, found);
    }
    else
    {
      title = fileName;
      dir = "";
    }
    URIUtils::RemoveExtension(title);
    int index = atoi(title.substr(title.size()-5).c_str());
    title.erase(title.size()-6);

    for (unsigned int i = 0; i < size(); ++i)
    {
      if (index > 0)
      {
        if (index == at(i).ClientIndex())
          return &at(i);
      }
      else
      {
        if ((title == at(i).Title()) && (dir == at(i).Directory()) && (clientID == at(i).ClientID()))
          return &at(i);
      }
    }
  }

  return NULL;
}

void CPVRRecordings::Clear()
{
  /* Clear all current present Recordings inside list */
  erase(begin(), end());
  return;
}
