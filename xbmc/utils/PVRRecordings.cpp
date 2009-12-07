/*
 *      Copyright (C) 2005-2009 Team XBMC
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

/*
 * DESCRIPTION:
 *
 * cPVRRecordingInfoTag is part of the XBMC PVR system to support recording entrys,
 * stored on a other Backend like VDR or MythTV.
 *
 * The recording information tag holds data about name, length, recording time
 * and so on of recorded stream stored on the backend.
 *
 * The filename string is used to by the PVRManager and passed to DVDPlayer
 * to stream data from the backend to XBMC.
 *
 * It is a also CVideoInfoTag and some of his variables must be set!
 *
 */

#include "FileItem.h"
#include "PVRRecordings.h"
#include "PVRManager.h"
#include "GUIDialogOK.h"
#include "LocalizeStrings.h"
#include "Util.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/SingleLock.h"

/**
 * Create a blank unmodified recording tag
 */
cPVRRecordingInfoTag::cPVRRecordingInfoTag()
{
  Reset();
}

bool cPVRRecordingInfoTag::operator ==(const cPVRRecordingInfoTag& right) const
{

  if (this == &right) return true;

  return (m_clientIndex         == right.m_clientIndex &&
          m_clientID            == right.m_clientID &&
          m_strChannel          == right.m_strChannel &&
          m_recordingTime       == right.m_recordingTime &&
          m_duration            == right.m_duration &&
          m_strPlotOutline      == right.m_strPlotOutline &&
          m_strPlot             == right.m_strPlot &&
          m_strStreamURL        == right.m_strStreamURL &&
          m_Priority            == right.m_Priority &&
          m_Lifetime            == right.m_Lifetime &&
          m_strDirectory        == right.m_strDirectory &&
          m_strFileNameAndPath  == right.m_strFileNameAndPath &&
          m_strTitle            == right.m_strTitle);
}

bool cPVRRecordingInfoTag::operator !=(const cPVRRecordingInfoTag& right) const
{

  if (this == &right) return false;

  if (m_clientIndex             != right.m_clientIndex) return true;
  if (m_clientID                != right.m_clientID) return true;
  if (m_strChannel              != right.m_strChannel) return true;
  if (m_recordingTime           != right.m_recordingTime) return true;
  if (m_duration                != right.m_duration) return true;
  if (m_strPlotOutline          != right.m_strPlotOutline) return true;
  if (m_strPlot                 != right.m_strPlot) return true;
  if (m_strStreamURL            != right.m_strStreamURL) return true;
  if (m_Priority                != right.m_Priority) return true;
  if (m_Lifetime                != right.m_Lifetime) return true;
  if (m_strTitle                != right.m_strTitle) return true;
  if (m_strDirectory            != right.m_strDirectory) return true;
  if (m_strFileNameAndPath      != right.m_strFileNameAndPath) return true;

  return false;
}

/**
 * Initialize blank cPVRRecordingInfoTag
 */
void cPVRRecordingInfoTag::Reset(void)
{
  m_clientIndex           = -1;
  m_clientID              = g_PVRManager.GetFirstClientID(); // Temporary until we support multiple backends
  m_strChannel            = "";
  m_strDirectory          = "";
  m_recordingTime         = NULL;
  m_strStreamURL          = "";
  m_Priority              = -1;
  m_Lifetime              = -1;
  m_strFileNameAndPath    = "";

  CVideoInfoTag::Reset();
}

int cPVRRecordingInfoTag::GetDuration() const
{
  int duration;
  duration =  m_duration.GetDays()*60*60*24;
  duration += m_duration.GetHours()*60*60;
  duration += m_duration.GetMinutes()*60;
  duration += m_duration.GetSeconds();
  duration /= 60;
  return duration;
}

bool cPVRRecordingInfoTag::Delete(void) const
{
  try
  {
    CLIENTMAP *clients = g_PVRManager.Clients();

    /* and write it to the backend */
    PVR_ERROR err = clients->find(m_clientID)->second->DeleteRecording(*this);

    if (err != PVR_ERROR_NO_ERROR)
      throw err;

    PVRRecordings.Update();
    return true;
  }
  catch (PVR_ERROR err)
  {
    DisplayError(err);
  }
  return false;
}

bool cPVRRecordingInfoTag::Rename(CStdString &newName) const
{
  try
  {
    CLIENTMAP *clients = g_PVRManager.Clients();

    /* and write it to the backend */
    PVR_ERROR err = clients->find(m_clientID)->second->RenameRecording(*this, newName);

    if (err != PVR_ERROR_NO_ERROR)
      throw err;

    PVRRecordings.Update();
    return true;
  }
  catch (PVR_ERROR err)
  {
    DisplayError(err);
  }
  return false;
}

void cPVRRecordingInfoTag::DisplayError(PVR_ERROR err) const
{
  if (err == PVR_ERROR_SERVER_ERROR)
    CGUIDialogOK::ShowAndGetInput(18100,18801,18803,0); /* print info dialog "Server error!" */
  else if (err == PVR_ERROR_NOT_SYNC)
    CGUIDialogOK::ShowAndGetInput(18100,18810,18803,0); /* print info dialog "Recordings not in sync!" */
  else if (err == PVR_ERROR_NOT_DELETED)
    CGUIDialogOK::ShowAndGetInput(18100,18811,18803,0); /* print info dialog "Couldn't delete recording!" */
  else
    CGUIDialogOK::ShowAndGetInput(18100,18106,18803,0); /* print info dialog "Unknown error!" */

  return;
}


// --- cPVRRecordings ---------------------------------------------------------------

cPVRRecordings PVRRecordings;

cPVRRecordings::cPVRRecordings(void)
{

}

void cPVRRecordings::Process()
{
  CSingleLock lock(m_critSection);

  CLIENTMAP *clients  = g_PVRManager.Clients();

  Clear();

  /* Go thru all clients and receive there Recordings */
  CLIENTMAPITR itr = clients->begin();
  while (itr != clients->end())
  {
    /* Load only if the client have Recordings */
    if ((*itr).second->GetNumRecordings() > 0)
    {
      (*itr).second->GetAllRecordings(this);
    }
    itr++;
  }

  for (unsigned int i = 0; i < size(); ++i)
  {
    CFileItemPtr pFileItem(new CFileItem(at(i)));

    CStdString Path;
    Path.Format("pvr://recordings/client_%04i/", at(i).ClientID());
    if (at(i).Directory() != "")
      Path += at(i).Directory();

    CUtil::AddSlashAtEnd(Path);
    Path += at(i).Title() + ".pvr";
    at(i).SetPath(Path);
  }
  return;
}

bool cPVRRecordings::Update(bool Wait)
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

int cPVRRecordings::GetNumRecordings()
{
  return size();
}

int cPVRRecordings::GetRecordings(CFileItemList* results)
{
  for (unsigned int i = 0; i < size(); ++i)
  {
    CFileItemPtr pFileItem(new CFileItem(at(i)));
    results->Add(pFileItem);
  }
  return size();
}

bool cPVRRecordings::DeleteRecording(const CFileItem &item)
{
  /* Check if a cPVRRecordingInfoTag is inside file item */
  if (!item.IsPVRRecording())
  {
    CLog::Log(LOGERROR, "cPVRRecordings: DeleteRecording no RecordingInfoTag given!");
    return false;
  }

  const cPVRRecordingInfoTag* tag = item.GetPVRRecordingInfoTag();
  return tag->Delete();
}

bool cPVRRecordings::RenameRecording(CFileItem &item, CStdString &newname)
{
  /* Check if a cPVRRecordingInfoTag is inside file item */
  if (!item.IsPVRRecording())
  {
    CLog::Log(LOGERROR, "cPVRRecordings: RenameRecording no RecordingInfoTag given!");
    return false;
  }

  cPVRRecordingInfoTag* tag = item.GetPVRRecordingInfoTag();
  return tag->Rename(newname);
}

bool cPVRRecordings::RemoveRecording(const CFileItem &item)
{
  if (!item.IsPVRRecording())
  {
    CLog::Log(LOGERROR, "cPVRRecordings: RemoveRecording no RecordingInfoTag given!");
    return false;
  }

  const cPVRRecordingInfoTag* tag = item.GetPVRRecordingInfoTag();

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

bool cPVRRecordings::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString base(strPath);
  CUtil::RemoveSlashAtEnd(base);

  CURL url(strPath);
  CStdString fileName = url.GetFileName();
  CUtil::RemoveSlashAtEnd(fileName);

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
    CUtil::AddSlashAtEnd(curDir);

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
      strEntryName.Format("recordings/client_%04i/%s", clientID, at(i).Directory());
      CUtil::AddSlashAtEnd(strEntryName);
      strEntryName += at(i).Title();
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

cPVRRecordingInfoTag *cPVRRecordings::GetByPath(CStdString &path)
{
  CURL url(path);
  CStdString fileName = url.GetFileName();
  CUtil::RemoveSlashAtEnd(fileName);

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
    CUtil::RemoveExtension(title);
    unsigned int index = atoi(title.substr(title.size()-5).c_str());
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

void cPVRRecordings::Clear()
{
  /* Clear all current present Recordings inside list */
  erase(begin(), end());
  return;
}
