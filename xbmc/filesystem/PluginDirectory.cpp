/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */


#include "threads/SystemClock.h"
#include "system.h"
#include "PluginDirectory.h"
#include "utils/URIUtils.h"
#include "addons/AddonManager.h"
#include "addons/AddonInstaller.h"
#include "addons/IAddon.h"
#ifdef HAS_PYTHON
#include "interfaces/python/XBPython.h"
#endif
#include "threads/SingleLock.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogProgress.h"
#include "settings/GUISettings.h"
#include "FileItem.h"
#include "video/VideoInfoTag.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "ApplicationMessenger.h"
#include "Application.h"
#include "URL.h"

using namespace XFILE;
using namespace std;
using namespace ADDON;

vector<CPluginDirectory *> CPluginDirectory::globalHandles;
CCriticalSection CPluginDirectory::m_handleLock;

CPluginDirectory::CPluginDirectory()
{
  m_listItems = new CFileItemList;
  m_fileResult = new CFileItem;
}

CPluginDirectory::~CPluginDirectory(void)
{
  delete m_listItems;
  delete m_fileResult;
}

int CPluginDirectory::getNewHandle(CPluginDirectory *cp)
{
  CSingleLock lock(m_handleLock);
  int handle = (int)globalHandles.size();
  globalHandles.push_back(cp);
  return handle;
}

void CPluginDirectory::removeHandle(int handle)
{
  CSingleLock lock(m_handleLock);
  if (handle >= 0 && handle < (int)globalHandles.size())
    globalHandles.erase(globalHandles.begin() + handle);
}

bool CPluginDirectory::StartScript(const CStdString& strPath, bool retrievingDir)
{
  CURL url(strPath);

  if (!CAddonMgr::Get().GetAddon(url.GetHostName(), m_addon, ADDON_PLUGIN) && !CAddonInstaller::Get().PromptForInstall(url.GetHostName(), m_addon))
  {
    CLog::Log(LOGERROR, "Unable to find plugin %s", url.GetHostName().c_str());
    return false;
  }

  // get options
  CStdString options = url.GetOptions();
  URIUtils::RemoveSlashAtEnd(options); // This MAY kill some scripts (eg though with a URL ending with a slash), but
                                    // is needed for all others, as XBMC adds slashes to "folders"
  url.SetOptions(""); // do this because we can then use the url to generate the basepath
                      // which is passed to the plugin (and represents the share)

  CStdString basePath(url.Get());
  // reset our wait event, and grab a new handle
  m_fetchComplete.Reset();
  int handle = getNewHandle(this);

  // clear out our status variables
  m_fileResult->Reset();
  m_listItems->Clear();
  m_listItems->SetPath(strPath);
  m_listItems->SetLabel(m_addon->Name());
  m_cancelled = false;
  m_success = false;
  m_totalItems = 0;

  // setup our parameters to send the script
  CStdString strHandle;
  strHandle.Format("%i", handle);
  vector<CStdString> argv;
  argv.push_back(basePath);
  argv.push_back(strHandle);
  argv.push_back(options);

  // run the script
  CLog::Log(LOGDEBUG, "%s - calling plugin %s('%s','%s','%s')", __FUNCTION__, m_addon->Name().c_str(), argv[0].c_str(), argv[1].c_str(), argv[2].c_str());
  bool success = false;
#ifdef HAS_PYTHON
  CStdString file = m_addon->LibPath();
  if (g_pythonParser.evalFile(file, argv,m_addon) >= 0)
  { // wait for our script to finish
    CStdString scriptName = m_addon->Name();
    success = WaitOnScriptResult(file, scriptName, retrievingDir);
  }
  else
#endif
    CLog::Log(LOGERROR, "Unable to run plugin %s", m_addon->Name().c_str());

  // free our handle
  removeHandle(handle);

  return success;
}

bool CPluginDirectory::GetPluginResult(const CStdString& strPath, CFileItem &resultItem)
{
  CURL url(strPath);
  CPluginDirectory* newDir = new CPluginDirectory();

  bool success = newDir->StartScript(strPath, false);

  if (success)
  { // update the play path and metadata, saving the old one as needed
    if (!resultItem.HasProperty("original_listitem_url"))
      resultItem.SetProperty("original_listitem_url", resultItem.GetPath());
    resultItem.SetPath(newDir->m_fileResult->GetPath());
    resultItem.SetMimeType(newDir->m_fileResult->GetMimeType(false));
    resultItem.UpdateInfo(*newDir->m_fileResult);
    if (newDir->m_fileResult->HasVideoInfoTag() && newDir->m_fileResult->GetVideoInfoTag()->m_resumePoint.IsSet())
      resultItem.m_lStartOffset = STARTOFFSET_RESUME; // resume point set in the resume item, so force resume
  }
  delete newDir;

  return success;
}

bool CPluginDirectory::AddItem(int handle, const CFileItem *item, int totalItems)
{
  CSingleLock lock(m_handleLock);
  if (handle < 0 || handle >= (int)globalHandles.size())
  {
    CLog::Log(LOGERROR, " %s - called with an invalid handle.", __FUNCTION__);
    return false;
  }

  CPluginDirectory *dir = globalHandles[handle];
  CFileItemPtr pItem(new CFileItem(*item));
  dir->m_listItems->Add(pItem);
  dir->m_totalItems = totalItems;

  return !dir->m_cancelled;
}

bool CPluginDirectory::AddItems(int handle, const CFileItemList *items, int totalItems)
{
  CSingleLock lock(m_handleLock);
  if (handle < 0 || handle >= (int)globalHandles.size())
  {
    CLog::Log(LOGERROR, " %s - called with an invalid handle.", __FUNCTION__);
    return false;
  }

  CPluginDirectory *dir = globalHandles[handle];
  CFileItemList pItemList;
  pItemList.Copy(*items);
  dir->m_listItems->Append(pItemList);
  dir->m_totalItems = totalItems;

  return !dir->m_cancelled;
}

void CPluginDirectory::EndOfDirectory(int handle, bool success, bool replaceListing, bool cacheToDisc)
{
  CSingleLock lock(m_handleLock);
  if (handle < 0 || handle >= (int)globalHandles.size())
  {
    CLog::Log(LOGERROR, " %s - called with an invalid handle.", __FUNCTION__);
    return;
  }
  CPluginDirectory *dir = globalHandles[handle];

  // set cache to disc
  dir->m_listItems->SetCacheToDisc(cacheToDisc ? CFileItemList::CACHE_IF_SLOW : CFileItemList::CACHE_NEVER);

  dir->m_success = success;
  dir->m_listItems->SetReplaceListing(replaceListing);

  if (!dir->m_listItems->HasSortDetails())
    dir->m_listItems->AddSortMethod(SORT_METHOD_NONE, 552, LABEL_MASKS("%L", "%D"));

  // set the event to mark that we're done
  dir->m_fetchComplete.Set();
}

void CPluginDirectory::AddSortMethod(int handle, SORT_METHOD sortMethod, const CStdString &label2Mask)
{
  CSingleLock lock(m_handleLock);
  if (handle < 0 || handle >= (int)globalHandles.size())
  {
    CLog::Log(LOGERROR, "%s - called with an invalid handle.", __FUNCTION__);
    return;
  }

  CPluginDirectory *dir = globalHandles[handle];

  // TODO: Add all sort methods and fix which labels go on the right or left
  switch(sortMethod)
  {
    case SORT_METHOD_LABEL:
    case SORT_METHOD_LABEL_IGNORE_THE:
      {
        if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
          dir->m_listItems->AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%T", label2Mask));
        else
          dir->m_listItems->AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_TITLE:
    case SORT_METHOD_TITLE_IGNORE_THE:
      {
        if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
          dir->m_listItems->AddSortMethod(SORT_METHOD_TITLE_IGNORE_THE, 556, LABEL_MASKS("%T", label2Mask));
        else
          dir->m_listItems->AddSortMethod(SORT_METHOD_TITLE, 556, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_ARTIST:
    case SORT_METHOD_ARTIST_IGNORE_THE:
      {
        if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
          dir->m_listItems->AddSortMethod(SORT_METHOD_ARTIST_IGNORE_THE, 557, LABEL_MASKS("%T", "%A"));
        else
          dir->m_listItems->AddSortMethod(SORT_METHOD_ARTIST, 557, LABEL_MASKS("%T", "%A"));
        break;
      }
    case SORT_METHOD_ALBUM:
    case SORT_METHOD_ALBUM_IGNORE_THE:
      {
        if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
          dir->m_listItems->AddSortMethod(SORT_METHOD_ALBUM_IGNORE_THE, 558, LABEL_MASKS("%T", "%B"));
        else
          dir->m_listItems->AddSortMethod(SORT_METHOD_ALBUM, 558, LABEL_MASKS("%T", "%B"));
        break;
      }
    case SORT_METHOD_DATE:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS("%T", "%J"));
        break;
      }
    case SORT_METHOD_BITRATE:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_BITRATE, 623, LABEL_MASKS("%T", "%X"));
        break;
      }             
    case SORT_METHOD_SIZE:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_SIZE, 553, LABEL_MASKS("%T", "%I"));
        break;
      }
    case SORT_METHOD_FILE:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_FILE, 561, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_TRACKNUM:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_TRACKNUM, 554, LABEL_MASKS("[%N. ]%T", label2Mask));
        break;
      }
    case SORT_METHOD_DURATION:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_DURATION, 555, LABEL_MASKS("%T", "%D"));
        break;
      }
    case SORT_METHOD_VIDEO_RATING:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_VIDEO_RATING, 563, LABEL_MASKS("%T", "%R"));
        break;
      }
    case SORT_METHOD_YEAR:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_YEAR, 562, LABEL_MASKS("%T", "%Y"));
        break;
      }
    case SORT_METHOD_SONG_RATING:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_SONG_RATING, 563, LABEL_MASKS("%T", "%R"));
        break;
      }
    case SORT_METHOD_GENRE:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_GENRE, 515, LABEL_MASKS("%T", "%G"));
        break;
      }
    case SORT_METHOD_COUNTRY:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_COUNTRY, 574, LABEL_MASKS("%T", "%G"));
        break;
      }
    case SORT_METHOD_VIDEO_TITLE:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_VIDEO_TITLE, 369, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_VIDEO_SORT_TITLE:
    case SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE:
      {
        if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
          dir->m_listItems->AddSortMethod(SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE, 369, LABEL_MASKS("%T", label2Mask));
        else
          dir->m_listItems->AddSortMethod(SORT_METHOD_VIDEO_SORT_TITLE, 369, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_MPAA_RATING:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_MPAA_RATING, 563, LABEL_MASKS("%T", "%O"));
        break;
      }
    case SORT_METHOD_VIDEO_RUNTIME:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_VIDEO_RUNTIME, 555, LABEL_MASKS("%T", "%D"));
        break;
      }
    case SORT_METHOD_STUDIO:
    case SORT_METHOD_STUDIO_IGNORE_THE:
      {
        if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
          dir->m_listItems->AddSortMethod(SORT_METHOD_STUDIO_IGNORE_THE, 572, LABEL_MASKS("%T", "%U"));
        else
          dir->m_listItems->AddSortMethod(SORT_METHOD_STUDIO, 572, LABEL_MASKS("%T", "%U"));
        break;
      }
    case SORT_METHOD_PROGRAM_COUNT:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_PROGRAM_COUNT, 567, LABEL_MASKS("%T", "%C"));
        break;
      }
    case SORT_METHOD_UNSORTED:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_UNSORTED, 571, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_NONE:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_NONE, 552, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_DRIVE_TYPE:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_DRIVE_TYPE, 564, LABEL_MASKS()); // Preformatted
        break;
      }
    case SORT_METHOD_PLAYLIST_ORDER:
      {
        CStdString strTrackLeft=g_guiSettings.GetString("musicfiles.trackformat");
        CStdString strTrackRight=g_guiSettings.GetString("musicfiles.trackformatright");

        dir->m_listItems->AddSortMethod(SORT_METHOD_PLAYLIST_ORDER, 559, LABEL_MASKS(strTrackLeft, strTrackRight));
        break;
      }
    case SORT_METHOD_EPISODE:
      {
        dir->m_listItems->AddSortMethod(SORT_METHOD_EPISODE,20359,LABEL_MASKS("%E. %T","%R"));
        break;
      }
    case SORT_METHOD_PRODUCTIONCODE:
      {
        //dir->m_listItems.AddSortMethod(SORT_METHOD_PRODUCTIONCODE,20368,LABEL_MASKS("%E. %T","%P", "%E. %T","%P"));
        dir->m_listItems->AddSortMethod(SORT_METHOD_PRODUCTIONCODE,20368,LABEL_MASKS("%H. %T","%P", "%H. %T","%P"));
        break;
      }
    case SORT_METHOD_LISTENERS:
      {
       dir->m_listItems->AddSortMethod(SORT_METHOD_LISTENERS,20455,LABEL_MASKS("%T","%W"));
       break;
      }
   
    default:
      break;
  }
}

bool CPluginDirectory::GetDirectory(const CStdString& strPath, CFileItemList& items)
{
  CURL url(strPath);

  bool success = StartScript(strPath, true);

  // append the items to the list
  items.Assign(*m_listItems, true); // true to keep the current items
  m_listItems->Clear();
  return success;
}

bool CPluginDirectory::RunScriptWithParams(const CStdString& strPath)
{
  CURL url(strPath);
  if (url.GetHostName().IsEmpty()) // called with no script - should never happen
    return false;

  AddonPtr addon;
  if (!CAddonMgr::Get().GetAddon(url.GetHostName(), addon, ADDON_PLUGIN) && !CAddonInstaller::Get().PromptForInstall(url.GetHostName(), addon))
  {
    CLog::Log(LOGERROR, "Unable to find plugin %s", url.GetHostName().c_str());
    return false;
  }

  // options
  CStdString options = url.GetOptions();
  URIUtils::RemoveSlashAtEnd(options); // This MAY kill some scripts (eg though with a URL ending with a slash), but
                                    // is needed for all others, as XBMC adds slashes to "folders"
  url.SetOptions(""); // do this because we can then use the url to generate the basepath
                      // which is passed to the plugin (and represents the share)

  CStdString basePath(url.Get());

  // setup our parameters to send the script
  CStdString strHandle;
  strHandle.Format("%i", -1);
  vector<CStdString> argv;
  argv.push_back(basePath);
  argv.push_back(strHandle);
  argv.push_back(options);

  // run the script
#ifdef HAS_PYTHON
  CLog::Log(LOGDEBUG, "%s - calling plugin %s('%s','%s','%s')", __FUNCTION__, addon->Name().c_str(), argv[0].c_str(), argv[1].c_str(), argv[2].c_str());
  if (g_pythonParser.evalFile(addon->LibPath(), argv,addon) >= 0)
    return true;
  else
#endif
    CLog::Log(LOGERROR, "Unable to run plugin %s", addon->Name().c_str());

  return false;
}

bool CPluginDirectory::WaitOnScriptResult(const CStdString &scriptPath, const CStdString &scriptName, bool retrievingDir)
{
  const unsigned int timeBeforeProgressBar = 1500;
  const unsigned int timeToKillScript = 1000;

  unsigned int startTime = XbmcThreads::SystemClockMillis();
  CGUIDialogProgress *progressBar = NULL;
  bool cancelled = false;
  bool inMainAppThread = g_application.IsCurrentThread();

  CLog::Log(LOGDEBUG, "%s - waiting on the %s plugin...", __FUNCTION__, scriptName.c_str());
  while (true)
  {
    {
      CSingleExit ex(g_graphicsContext);
      // check if the python script is finished
      if (m_fetchComplete.WaitMSec(20))
      { // python has returned
        CLog::Log(LOGDEBUG, "%s- plugin returned %s", __FUNCTION__, m_success ? "successfully" : "failure");
        break;
      }
    }
    // check our script is still running
#ifdef HAS_PYTHON
    if (!g_pythonParser.isRunning(g_pythonParser.getScriptId(scriptPath.c_str())))
#endif
    { // check whether we exited normally
      if (!m_fetchComplete.WaitMSec(0))
      { // python didn't return correctly
        CLog::Log(LOGDEBUG, " %s - plugin exited prematurely - terminating", __FUNCTION__);
        m_success = false;
      }
      break;
    }

    // check whether we should pop up the progress dialog
    if (!retrievingDir && !progressBar && XbmcThreads::SystemClockMillis() - startTime > timeBeforeProgressBar)
    { // loading takes more then 1.5 secs, show a progress dialog
      progressBar = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      // if script has shown progressbar don't override it
      if (progressBar && progressBar->IsActive())
      {
        startTime = XbmcThreads::SystemClockMillis();
        progressBar = NULL;
      }

      if (progressBar)
      {
        progressBar->SetHeading(scriptName);
        progressBar->SetLine(0, retrievingDir ? 1040 : 10214);
        progressBar->SetLine(1, "");
        progressBar->SetLine(2, "");
        progressBar->ShowProgressBar(retrievingDir);
        progressBar->StartModal();
      }
    }

    if (progressBar)
    { // update the progress bar and check for user cancel
      progressBar->Progress();
      if (progressBar->IsCanceled())
      { // user has cancelled our process - cancel our process
        m_cancelled = true;
      }
    }
    else // if the progressBar exists and we call StartModal or Progress we get the
         //  ProcessRenderLoop call anyway.
      if (inMainAppThread) 
        g_windowManager.ProcessRenderLoop();

    if (!cancelled && m_cancelled)
    {
      cancelled = true;
      startTime = XbmcThreads::SystemClockMillis();
    }
    if (cancelled && XbmcThreads::SystemClockMillis() - startTime > timeToKillScript)
    { // cancel our script
#ifdef HAS_PYTHON
      int id = g_pythonParser.getScriptId(scriptPath.c_str());
      if (id != -1 && g_pythonParser.isRunning(id))
      {
        CLog::Log(LOGDEBUG, "%s- cancelling plugin %s", __FUNCTION__, scriptName.c_str());
        g_pythonParser.stopScript(id);
        break;
      }
#endif
    }
  }

  if (progressBar)
    CApplicationMessenger::Get().Close(progressBar, false, false);

  return !cancelled && m_success;
}

void CPluginDirectory::SetResolvedUrl(int handle, bool success, const CFileItem *resultItem)
{
  CSingleLock lock(m_handleLock);
  if (handle < 0 || handle >= (int)globalHandles.size())
  {
    CLog::Log(LOGERROR, " %s - called with an invalid handle.", __FUNCTION__);
    return;
  }
  CPluginDirectory* dir  = globalHandles[handle];

  dir->m_success = success;
  *dir->m_fileResult = *resultItem;

  // set the event to mark that we're done
  dir->m_fetchComplete.Set();
}

CStdString CPluginDirectory::GetSetting(int handle, const CStdString &strID)
{
  if (handle < 0 || handle >= (int)globalHandles.size())
  {
    CLog::Log(LOGERROR, "%s called with an invalid handle.", __FUNCTION__);
    return "";
  }

  CPluginDirectory *dir = globalHandles[handle];
  if(dir->m_addon)
    return dir->m_addon->GetSetting(strID);
  else
    return "";
}

void CPluginDirectory::SetSetting(int handle, const CStdString &strID, const CStdString &value)
{
  if (handle < 0 || handle >= (int)globalHandles.size())
  {
    CLog::Log(LOGERROR, "%s called with an invalid handle.", __FUNCTION__);
    return;
  }

  CPluginDirectory *dir = globalHandles[handle];
  if(dir->m_addon)
    dir->m_addon->UpdateSetting(strID, value);
}

void CPluginDirectory::SetContent(int handle, const CStdString &strContent)
{
  if (handle < 0 || handle >= (int)globalHandles.size())
  {
    CLog::Log(LOGERROR, "%s called with an invalid handle.", __FUNCTION__);
    return;
  }

  CPluginDirectory *dir = globalHandles[handle];
  dir->m_listItems->SetContent(strContent);
}

void CPluginDirectory::SetProperty(int handle, const CStdString &strProperty, const CStdString &strValue)
{
  CSingleLock lock(m_handleLock);
  if (handle < 0 || handle >= (int)globalHandles.size())
  {
    CLog::Log(LOGERROR, "%s called with an invalid handle.", __FUNCTION__);
    return;
  }

  CPluginDirectory *dir = globalHandles[handle];
  if (strProperty == "fanart_image")
    dir->m_listItems->SetArt("fanart", strValue);
  else
    dir->m_listItems->SetProperty(strProperty, strValue);
}

void CPluginDirectory::CancelDirectory()
{
  m_cancelled = true;
}

float CPluginDirectory::GetProgress() const
{
  if (m_totalItems > 0)
    return (m_listItems->Size() * 100.0f) / m_totalItems;
  return 0.0f;
}
