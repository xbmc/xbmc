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

#include "Directory.h"
#include "DirectoryFactory.h"
#include "FileDirectoryFactory.h"
#include "commons/Exception.h"
#include "FileItem.h"
#include "DirectoryCache.h"
#include "settings/GUISettings.h"
#include "utils/log.h"
#include "utils/Job.h"
#include "utils/JobManager.h"
#include "Application.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogBusy.h"
#include "threads/SingleLock.h"
#include "utils/URIUtils.h"

using namespace std;
using namespace XFILE;

#define TIME_TO_BUSY_DIALOG 500

class CGetDirectory
{
private:

  struct CResult
  {
    CResult(const CStdString& dir) : m_event(true), m_dir(dir), m_result(false) {}
    CEvent        m_event;
    CFileItemList m_list;
    CStdString    m_dir;
    bool          m_result;
  };

  struct CGetJob
    : CJob
  {
    CGetJob(boost::shared_ptr<IDirectory>& imp
          , boost::shared_ptr<CResult>& result)
      : m_result(result)
      , m_imp(imp)
    {}
  public:
    virtual bool DoWork()
    {
      m_result->m_list.SetPath(m_result->m_dir);
      m_result->m_result         = m_imp->GetDirectory(m_result->m_dir, m_result->m_list);
      m_result->m_event.Set();
      return m_result->m_result;
    }

    boost::shared_ptr<CResult>    m_result;
    boost::shared_ptr<IDirectory> m_imp;
  };

public:

  CGetDirectory(boost::shared_ptr<IDirectory>& imp, const CStdString& dir) 
    : m_result(new CResult(dir))
  {
    m_id = CJobManager::GetInstance().AddJob(new CGetJob(imp, m_result)
                                           , NULL
                                           , CJob::PRIORITY_HIGH);
  }
 ~CGetDirectory()
  {
    CJobManager::GetInstance().CancelJob(m_id);
  }

  bool Wait(unsigned int timeout)
  {
    return m_result->m_event.WaitMSec(timeout);
  }

  bool GetDirectory(CFileItemList& list)
  {
    /* if it was not finished or failed, return failure */
    if(!m_result->m_event.WaitMSec(0) || !m_result->m_result)
    {
      list.Clear();
      return false;
    }

    list.Copy(m_result->m_list);
    return true;
  }
  boost::shared_ptr<CResult> m_result;
  unsigned int               m_id;
};


CDirectory::CDirectory()
{}

CDirectory::~CDirectory()
{}

bool CDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items, const CStdString &strMask /*=""*/, int flags /*=DIR_FLAG_DEFAULTS*/, bool allowThreads /* = false */)
{
  CHints hints;
  hints.flags = flags;
  hints.mask = strMask;
  return GetDirectory(strPath, items, hints, allowThreads);
}

bool CDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items, const CHints &hints, bool allowThreads)
{
  try
  {
    CStdString realPath = URIUtils::SubstitutePath(strPath);
    boost::shared_ptr<IDirectory> pDirectory(CDirectoryFactory::Create(realPath));
    if (!pDirectory.get())
      return false;

    // check our cache for this path
    if (g_directoryCache.GetDirectory(strPath, items, (hints.flags & DIR_FLAG_READ_CACHE) == DIR_FLAG_READ_CACHE))
      items.SetPath(strPath);
    else
    {
      // need to clear the cache (in case the directory fetch fails)
      // and (re)fetch the folder
      if (!(hints.flags & DIR_FLAG_BYPASS_CACHE))
        g_directoryCache.ClearDirectory(strPath);

      pDirectory->SetFlags(hints.flags);

      bool result = false, cancel = false;
      while (!result && !cancel)
      {
        if (g_application.IsCurrentThread() && allowThreads && !URIUtils::IsSpecial(strPath))
        {
          CSingleExit ex(g_graphicsContext);

          CGetDirectory get(pDirectory, realPath);
          if(!get.Wait(TIME_TO_BUSY_DIALOG))
          {
            CGUIDialogBusy* dialog = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);
            dialog->Show();

            while(!get.Wait(10))
            {
              CSingleLock lock(g_graphicsContext);

              // update progress
              float progress = pDirectory->GetProgress();
              if (progress > 0)
                dialog->SetProgress(progress);

              if(dialog->IsCanceled())
              {
                cancel = true;
                pDirectory->CancelDirectory();
                break;
              }

              lock.Leave(); // prevent an occasional deadlock on exit
              g_windowManager.ProcessRenderLoop(false);
            }
            if(dialog)
              dialog->Close();
          }
          result = get.GetDirectory(items);
        }
        else
        {
          items.SetPath(strPath);
          result = pDirectory->GetDirectory(realPath, items);
        }

        if (!result)
        {
          if (!cancel && g_application.IsCurrentThread() && pDirectory->ProcessRequirements())
            continue;
          CLog::Log(LOGERROR, "%s - Error getting %s", __FUNCTION__, strPath.c_str());
          return false;
        }
      }

      // cache the directory, if necessary
      if (!(hints.flags & DIR_FLAG_BYPASS_CACHE))
        g_directoryCache.SetDirectory(strPath, items, pDirectory->GetCacheType(strPath));
    }

    // now filter for allowed files
    pDirectory->SetMask(hints.mask);
    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItemPtr item = items[i];
      // TODO: we shouldn't be checking the gui setting here;
      // callers should use getHidden instead
      if ((!item->m_bIsFolder && !pDirectory->IsAllowed(item->GetPath())) ||
          (item->GetProperty("file:hidden").asBoolean() && !(hints.flags & DIR_FLAG_GET_HIDDEN) && !g_guiSettings.GetBool("filelists.showhidden")))
      {
        items.Remove(i);
        i--; // don't confuse loop
      }
    }

    //  Should any of the files we read be treated as a directory?
    //  Disable for database folders, as they already contain the extracted items
    if (!(hints.flags & DIR_FLAG_NO_FILE_DIRS) && !items.IsMusicDb() && !items.IsVideoDb() && !items.IsSmartPlayList())
      FilterFileDirectories(items, hints.mask);

    return true;
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error getting %s", __FUNCTION__, strPath.c_str());
  return false;
}

bool CDirectory::Create(const CStdString& strPath)
{
  try
  {
    CStdString realPath = URIUtils::SubstitutePath(strPath);
    auto_ptr<IDirectory> pDirectory(CDirectoryFactory::Create(realPath));
    if (pDirectory.get())
      if(pDirectory->Create(realPath.c_str()))
        return true;
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error creating %s", __FUNCTION__, strPath.c_str());
  return false;
}

bool CDirectory::Exists(const CStdString& strPath)
{
  try
  {
    CStdString realPath = URIUtils::SubstitutePath(strPath);
    auto_ptr<IDirectory> pDirectory(CDirectoryFactory::Create(realPath));
    if (pDirectory.get())
      return pDirectory->Exists(realPath.c_str());
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error checking for %s", __FUNCTION__, strPath.c_str());
  return false;
}

bool CDirectory::Remove(const CStdString& strPath)
{
  try
  {
    CStdString realPath = URIUtils::SubstitutePath(strPath);
    auto_ptr<IDirectory> pDirectory(CDirectoryFactory::Create(realPath));
    if (pDirectory.get())
      if(pDirectory->Remove(realPath.c_str()))
        return true;
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error removing %s", __FUNCTION__, strPath.c_str());
  return false;
}

void CDirectory::FilterFileDirectories(CFileItemList &items, const CStdString &mask)
{
  for (int i=0; i< items.Size(); ++i)
  {
    CFileItemPtr pItem=items[i];
    if ((!pItem->m_bIsFolder) && (!pItem->IsInternetStream()))
    {
      auto_ptr<IFileDirectory> pDirectory(CFileDirectoryFactory::Create(pItem->GetPath(),pItem.get(),mask));
      if (pDirectory.get())
        pItem->m_bIsFolder = true;
      else
        if (pItem->m_bIsFolder)
        {
          items.Remove(i);
          i--; // don't confuse loop
        }
    }
  }
}
