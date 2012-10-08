/*
 *      Copyright (C) 2012 Team XBMC
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
#include "PictureInfoScanner.h"
#include "PictureInfoTag.h"
#include "PictureInfoLoader.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "filesystem/Directory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "settings/Settings.h"
#include "settings/GUISettings.h"
#include "utils/log.h"
#include "utils/md5.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "FileItem.h"
#include "GUIUserMessages.h"
#include "GUIInfoManager.h"
#include "URL.h"

#include <algorithm>

using namespace std;
using namespace PICTURE_INFO;
using namespace XFILE;

CPictureInfoScanner::CPictureInfoScanner() : CThread("CPictureInfoScanner")
{
  m_showDialog = false;
  m_handle = NULL;
  m_currentItem = 0;
  m_itemCount = 0;
  m_bRunning = false;
  m_bCanInterrupt = false;
}

void CPictureInfoScanner::Start(const CStdString& strDirectory)
{
  m_pathsToScan.clear();

  if (strDirectory.IsEmpty())
  {
    // Scan all paths in the database. We do this by scanning all paths in the
    // db, and crossing them off the list as we go.
    m_pictureDatabase.Open();
    m_pictureDatabase.GetPaths(m_pathsToScan);
    m_pictureDatabase.Close();
  }
  else
  {
    m_pathsToScan.insert(strDirectory.c_str());
  }

  m_pathsToCount = m_pathsToScan;
  StopThread();
  Create();
  m_bRunning = true;
}

void CPictureInfoScanner::Stop()
{
  if (m_bCanInterrupt)
    m_pictureDatabase.Interupt();

  StopThread();
}

void CPictureInfoScanner::Process()
{
  try
  {
    unsigned int tick = XbmcThreads::SystemClockMillis();

    m_pictureDatabase.Open();

    m_bCanInterrupt = true;

    // Load info from files
    CLog::Log(LOGDEBUG, "%s - Starting scan", __FUNCTION__);

    if (m_showDialog && !g_guiSettings.GetBool("picturelibrary.backgroundupdate"))
    {
      CGUIDialogExtendedProgressBar* dialog =
        static_cast<CGUIDialogExtendedProgressBar*>(g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS));
      m_handle = dialog->GetHandle(g_localizeStrings.Get(505)); // Loading media info from files...
    }

    // Reset progress vars
    m_currentItem = 0;
    m_itemCount = -1;

    // Create the thread to count all files to be scanned
    SetPriority(GetMinPriority());
    CThread fileCountReader(this, "CPictureInfoScannerCounter");

    if (m_handle)
      fileCountReader.Create();

    // Database operations should not be canceled
    // using Interupt() [sic] while scanning as it could
    // result in unexpected behaviour.
    m_bCanInterrupt = false;

    // If no paths, cancel immediately
    bool cancelled = (m_pathsToScan.size() == 0);
    while (!cancelled && m_pathsToScan.size())
    {
      /*
       * A copy of the directory path is used because the path supplied is
       * immediately removed from the m_pathsToScan set in DoScan(). If the
       * reference points to the entry in the set a null reference error
       * occurs.
       */
      string directory = *m_pathsToScan.begin();
      if (!DoScan(directory))
        cancelled = true;
    }

    if (!cancelled)
    {
      g_infoManager.ResetLibraryBools();

      if (m_handle)
        m_handle->SetTitle(g_localizeStrings.Get(331)); // Compressing database...

      m_pictureDatabase.Compress(false);
    }

    fileCountReader.StopThread();

    m_pictureDatabase.Close();
    CLog::Log(LOGDEBUG, "%s - Finished scan", __FUNCTION__);

    tick = XbmcThreads::SystemClockMillis() - tick;
    CLog::Log(LOGNOTICE, "My Pictures: Scanning for picture info using worker thread, operation took %s",
        StringUtils::SecondsToTimeString(tick / 1000).c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "PictureInfoScanner: Exception while scanning.");
  }

  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::PictureLibrary, "xbmc", "OnScanFinished");
  m_bRunning = false;

  if (m_showDialog)
  {
    // send message
    CGUIMessage msg(GUI_MSG_SCAN_FINISHED, 0, 0, 0);
    g_windowManager.SendThreadMessage(msg);
  }

  if (m_handle)
    m_handle->MarkFinished();

  m_handle = NULL;
}

// This function is run by another thread
void CPictureInfoScanner::Run()
{
  int count = 0;
  while (!m_bStop && m_pathsToCount.size())
    count += CountFilesRecursively(*m_pathsToCount.begin());
  m_itemCount = count;
}

// Recurse through all folders we scan and count files
int CPictureInfoScanner::CountFilesRecursively(const string &strPath)
{
  // load subfolder
  CFileItemList items;
//  CLog::Log(LOGDEBUG, __FUNCTION__" - processing dir: %s", strPath.c_str());
  CDirectory::GetDirectory(strPath, items, g_settings.m_pictureExtensions, DIR_FLAG_NO_FILE_DIRS);

  if (m_bStop)
    return 0;

  // true for recursive counting
  int count = CountFiles(items, true);

  // remove this path from the list we're processing
  set<string>::iterator it = m_pathsToCount.find(strPath);
  if (it != m_pathsToCount.end())
    m_pathsToCount.erase(it);

//  CLog::Log(LOGDEBUG, __FUNCTION__" - finished processing dir: %s", strPath.c_str());
  return count;
}

int CPictureInfoScanner::CountFiles(const CFileItemList &items, bool recursive)
{
  int count = 0;
  for (int i = 0; i < items.Size(); ++i)
  {
    const CFileItemPtr pItem = items[i];

    if (recursive && pItem->m_bIsFolder)
      count += CountFilesRecursively(pItem->GetPath());
    else if (pItem->IsPicture())
      count++;
  }
  return count;
}

// To be used with the extended progress dialog box
static void OnDirectoryScanned(const CStdString& strDirectory)
{
  CGUIMessage msg(GUI_MSG_DIRECTORY_SCANNED, 0, 0, 0);
  msg.SetStringParam(strDirectory);
  g_windowManager.SendThreadMessage(msg);
}

static CStdString Prettify(const CStdString& strDirectory)
{
  CURL url(strDirectory);
  CStdString strStrippedPath = url.GetWithoutUserDetails();
  CURL::Decode(strStrippedPath);
  return strStrippedPath;
}

bool CPictureInfoScanner::DoScan(const string &strDirectory)
{
  if (m_handle)
    m_handle->SetText(Prettify(strDirectory));

  /*
   * remove this path from the list we're processing. This must be done prior to
   * the check for file or folder exclusion to prevent an infinite while loop
   * in Process().
   */
  set<string>::iterator it = m_pathsToScan.find(strDirectory);
  if (it != m_pathsToScan.end())
    m_pathsToScan.erase(it);

  // load subfolder
  CFileItemList items;
  CDirectory::GetDirectory(strDirectory, items, g_settings.m_pictureExtensions);

  // sort and get the path hash
  items.Sort(SORT_METHOD_LABEL, SortOrderAscending);
  string hash;
  GetPathHash(items, hash);

  // check whether we need to rescan or not
  string dbHash;
  if (!m_pictureDatabase.GetPathHash(strDirectory, dbHash) || dbHash != hash)
  {
    // path has changed - rescan
    if (dbHash.empty())
      CLog::Log(LOGDEBUG, "%s Scanning dir '%s' as not in the database", __FUNCTION__, strDirectory.c_str());
    else
      CLog::Log(LOGDEBUG, "%s Rescanning dir '%s' due to change", __FUNCTION__, strDirectory.c_str());

    items.Sort(SORT_METHOD_LABEL, SortOrderAscending);

    // scan in the new information
    int picturesToAdd = RetrievePictureInfo(items, strDirectory);

    if (picturesToAdd > 0 && m_handle)
      OnDirectoryScanned(strDirectory);

    // save information about this folder
    m_pictureDatabase.SetPathHash(strDirectory, hash);
  }
  else
  { // path is the same - no need to rescan
    CLog::Log(LOGDEBUG, "%s Skipping dir '%s' due to no change", __FUNCTION__, strDirectory.c_str());
    m_currentItem += CountFiles(items, false);  // false for non-recursive

    // updated the dialog with our progress
    if (m_handle)
    {
      if (m_itemCount > 0)
        m_handle->SetPercentage(m_currentItem / (float)m_itemCount * 100);
      OnDirectoryScanned(strDirectory);
    }
  }

  // now scan the subfolders
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];

    if (m_bStop)
      break;

    // if we have a directory item then recurse into that folder
    if (pItem->m_bIsFolder && !pItem->IsParentFolder())
    {
      CStdString strPath = pItem->GetPath();
      if (!DoScan(strPath))
        m_bStop = true;
    }
  }

  // If we were signaled to stop, mark the directory as dirty so the next scan
  // attempt picks it up and re-scans
  if (m_bStop)
  {
    m_pictureDatabase.SetPathHash(strDirectory, "");
    return false;
  }
  else
  {
    return true;
  }
}

int CPictureInfoScanner::GetPathHash(const CFileItemList &items, string &hash)
{
  // Create a hash based on the filenames, filesize and filedate.  Also count the number of files
  if (0 == items.Size())
    return 0;

  XBMC::XBMC_MD5 md5state;
  int count = 0;
  for (int i = 0; i < items.Size(); ++i)
  {
    const CFileItemPtr pItem = items[i];

    md5state.append(pItem->GetPath());
    md5state.append((unsigned char *)&pItem->m_dwSize, sizeof(pItem->m_dwSize));
    FILETIME time = pItem->m_dateTime;
    md5state.append((unsigned char *)&time, sizeof(FILETIME));

    if (pItem->IsPicture())
      count++;
  }
  CStdString strHash;
  md5state.getDigest(strHash);
  hash = strHash;
  return count;
}

int CPictureInfoScanner::RetrievePictureInfo(CFileItemList &items, const string &strDirectory)
{
  vector<CPictureInfoTag> pictures;
  vector<CPictureInfoTag> picturesToAdd;

  // get all information for all files in current directory from database, and remove them
  m_pictureDatabase.GetPicturesByPath(strDirectory, pictures);
  m_pictureDatabase.DeletePicturesByPath(strDirectory, false);

  // Read the tag from a file
  CPictureInfoLoader loader;
  loader.Load(items);
  while (loader.IsLoading())
    Sleep(10); // 100 fps

  // For every file found, but skip folder
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    
    if (m_bStop)
      return 0;

    // Don't try reading tags for folders
    if (!pItem->m_bIsFolder)
    {
      m_currentItem++;
      CLog::Log(LOGDEBUG, "%s - Reading tag for: %s", __FUNCTION__, pItem->GetPath().c_str());

      // if we have the itemcount, update our
      // dialog with the progress we made
      if (m_handle && m_itemCount > 0)
        m_handle->SetPercentage(m_currentItem / (float)m_itemCount * 100);

      if (pItem->GetPictureInfoTag() && pItem->GetPictureInfoTag()->Loaded())
      {
        // Look in pictures for a tag with the same file and path as pItem
        // copy persistent fields over (like playcount)
        picturesToAdd.push_back(*pItem->GetPictureInfoTag());
      }
    }
  }

  // Add to database
  m_pictureDatabase.BeginTransaction();
  for (vector<CPictureInfoTag>::const_iterator it = picturesToAdd.begin(); it != picturesToAdd.end(); it++)
  {
    CPictureInfoTag p = *it;
    m_pictureDatabase.AddObject(&p);
  }
  m_pictureDatabase.CommitTransaction();

  if (m_handle)
    m_handle->SetTitle(g_localizeStrings.Get(505)); // Loading media info from files...

  return picturesToAdd.size();
}
