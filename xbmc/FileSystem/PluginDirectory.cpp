/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "stdafx.h"
#include "PluginDirectory.h"
#include "util.h"
#include "lib/libPython/XBPython.h"
 
using namespace DIRECTORY;

vector<CPluginDirectory *> CPluginDirectory::globalHandles;

CPluginDirectory::CPluginDirectory(void)
{
  m_directoryFetched = CreateEvent(NULL, false, false, NULL);
}

CPluginDirectory::~CPluginDirectory(void)
{
  CloseHandle(m_directoryFetched);
}

int CPluginDirectory::getNewHandle(CPluginDirectory *cp)
{
  CLog::Log(LOGDEBUG, __FUNCTION__" - getHandle called.");
  int handle = (int)globalHandles.size();
  globalHandles.push_back(cp);
  CLog::Log(LOGDEBUG, __FUNCTION__" - Handle #:%i",handle);
  CLog::Log(LOGDEBUG, __FUNCTION__" - Address:%p",&cp);
  return handle;
}

void CPluginDirectory::removeHandle(int handle)
{
  CLog::Log(LOGDEBUG, __FUNCTION__" - RemoveHandle called with handle %i.",handle);
  if (handle > 0 && handle < (int)globalHandles.size())
    globalHandles.erase(globalHandles.begin() + handle);
}

void CPluginDirectory::AddItem(int handle, const CFileItem *item)
{
  if (handle < 0 || handle >= (int)globalHandles.size())
  {
    CLog::Log(LOGERROR, __FUNCTION__" called with an invalid handle.");
    return;
  }
  CPluginDirectory *dir = globalHandles[handle];
  CFileItem *pItem = new CFileItem(*item);
  dir->m_listItems.Add(pItem);
}

void CPluginDirectory::EndOfDirectory(int handle)
{
  if (handle < 0 || handle >= (int)globalHandles.size())
  {
    CLog::Log(LOGERROR, __FUNCTION__" called with an invalid handle.");
    return;
  }
  CPluginDirectory *dir = globalHandles[handle];
  CLog::Log(LOGDEBUG, __FUNCTION__" setting event of dir at address: %p.", dir);

  // TODO: Add to this method (or another suitable method) so it's settable from the plugin
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    dir->m_listItems.AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%T", "%R"));  // Filename, Duration | Foldername, empty
  else
    dir->m_listItems.AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%T", "%R"));  // Filename, Duration | Foldername, empty
  dir->m_listItems.AddSortMethod(SORT_METHOD_VIDEO_RATING, 563, LABEL_MASKS("%T", "%R"));  // Filename, Duration | Foldername, empty
  dir->m_listItems.AddSortMethod(SORT_METHOD_VIDEO_YEAR,345, LABEL_MASKS("%T", "%Y"));

  // set the event to mark that we're done
  SetEvent(dir->m_directoryFetched);
}

bool CPluginDirectory::GetDirectory(const CStdString& strPath, CFileItemList& items)
{
  CLog::Log(LOGDEBUG, __FUNCTION__" - opened with %s", strPath.c_str());
  CLog::Log(LOGDEBUG, __FUNCTION__" - doing arguments stuff...");
  CURL url(strPath);
  if (url.GetFileName().IsEmpty())
  { // called with no script - should never happen
    return GetPluginsDirectory(url.GetHostName(), items);
  }

  CStdString fileName;
  CUtil::AddFileToFolder(url.GetFileName(), "default.py", fileName);

  // path is Q:\plugins\<path from here>
  CStdString pathToScript = "Q:\\plugins\\";
  CUtil::AddFileToFolder(pathToScript, url.GetHostName(), pathToScript);
  CUtil::AddFileToFolder(pathToScript, fileName, pathToScript);
  pathToScript.Replace("/", "\\");

  // base path
  CStdString basePath = "plugin://";
  CUtil::AddFileToFolder(basePath, url.GetHostName(), basePath);
  CUtil::AddFileToFolder(basePath, url.GetFileName(), basePath);

  // options
  CStdString options = url.GetOptions();
  CUtil::RemoveSlashAtEnd(options); // This MAY kill some scripts (eg though with a URL ending with a slash), but
                                    // is needed for all others, as XBMC adds slashes to "folders"

  // reset our wait event, and grab a new handle
  ResetEvent(m_directoryFetched);
  int handle = getNewHandle(this);
  m_listItems.Clear();
  m_listItems.m_strPath = strPath;

  // setup our parameters to send the script
  CStdString strHandle;
  strHandle.Format("%i", handle);
  const char *argv[3];
  argv[0] = basePath.c_str();
  argv[1] = strHandle.c_str();
  argv[2] = options.c_str();

  // run the script
  CLog::Log(LOGDEBUG, __FUNCTION__" - calling script %s('%s','%s','%s')", pathToScript.c_str(), argv[0], argv[1], argv[2]);
  bool success = false;
  if (g_pythonParser.evalFile(pathToScript.c_str(), 3, (const char**)argv) >= 0)
  { // wait for our script to finish
    CLog::Log(LOGDEBUG, __FUNCTION__" - waiting...");
    success = WaitForSingleObject(m_directoryFetched, 10000) == WAIT_OBJECT_0;
    CLog::Log(LOGDEBUG, __FUNCTION__" - wait %s", success ? "ended by script" : "timed out");
  }
  else
    CLog::Log(LOGERROR, "Unable to run plugin %s", pathToScript.c_str());

  // free our handle
  removeHandle(handle);

  // append the items to the list, and return true
  items.AssignPointer(m_listItems);
  m_listItems.ClearKeepPointer();
  return success;
}

bool CPluginDirectory::HasPlugins(const CStdString &type)
{
  CStdString path = "Q:\\plugins\\";
  CUtil::AddFileToFolder(path, type, path);
  CFileItemList items;
  if (CDirectory::GetDirectory(path, items, "/", false))
  {
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItem *item = items[i];
      if (item->m_bIsFolder && !item->IsParentFolder() && !item->m_bIsShareOrDrive)
      {
        CStdString defaultPY;
        CUtil::AddFileToFolder(item->m_strPath, "default.py", defaultPY);
        if (XFILE::CFile::Exists(defaultPY))
          return true;
      }
    }
  }
  return false;
}

bool CPluginDirectory::GetPluginsDirectory(const CStdString &type, CFileItemList &items)
{
  // retrieve our folder
  CStdString pluginsFolder = "Q:\\plugins";
  CUtil::AddFileToFolder(pluginsFolder, type, pluginsFolder);

  if (!CDirectory::GetDirectory(pluginsFolder, items, "*.py", false))
    return false;

  // flatten any folders
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItem *item = items[i];
    /*
    if (item->m_bIsFolder && !item->IsParentFolder() && !item->m_bIsShareOrDrive)
    { // folder item - let's check for a default.py file, and flatten if we have one
      CStdString defaultPY;
      CUtil::AddFileToFolder(item->m_strPath, "default.py", defaultPY);
      if (XFILE::CFile::Exists(defaultPY))
      { // yes, format the item up
        item->m_strPath = defaultPY;
        item->m_bIsFolder = true;
        item->FillInDefaultIcon();
        item->SetLabelPreformated(true);
      }
    } */
    item->m_strPath.Replace("Q:\\plugins\\", "plugin://");
    item->m_strPath.Replace("\\", "/");
  }
  items.SetProgramThumbs();
  return true;
}
