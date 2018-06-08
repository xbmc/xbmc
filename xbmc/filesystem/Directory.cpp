/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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
#include "ServiceBroker.h"
#include "commons/Exception.h"
#include "FileItem.h"
#include "DirectoryCache.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/Job.h"
#include "utils/JobManager.h"
#include "Application.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogBusy.h"
#include "threads/SingleLock.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "PasswordManager.h"

using namespace XFILE;

#define TIME_TO_BUSY_DIALOG 500

class CGetDirectory
{
private:

  struct CResult
  {
    CResult(const CURL& dir, const CURL& listDir) : m_event(true), m_dir(dir), m_listDir(listDir), m_result(false) {}
    CEvent        m_event;
    CFileItemList m_list;
    CURL          m_dir;
    CURL          m_listDir;
    bool          m_result;
  };

  struct CGetJob
    : CJob
  {
    CGetJob(std::shared_ptr<IDirectory>& imp
          , std::shared_ptr<CResult>& result)
      : m_result(result)
      , m_imp(imp)
    {}
  public:
    bool DoWork() override
    {
      m_result->m_list.SetURL(m_result->m_listDir);
      m_result->m_result         = m_imp->GetDirectory(m_result->m_dir, m_result->m_list);
      m_result->m_event.Set();
      return m_result->m_result;
    }

    std::shared_ptr<CResult>    m_result;
    std::shared_ptr<IDirectory> m_imp;
  };

public:

  CGetDirectory(std::shared_ptr<IDirectory>& imp, const CURL& dir, const CURL& listDir)
    : m_result(new CResult(dir, listDir))
  {
    m_id = CJobManager::GetInstance().AddJob(new CGetJob(imp, m_result)
                                           , NULL
                                           , CJob::PRIORITY_HIGH);
    if (m_id == 0)
    {
      CGetJob job(imp, m_result);
      job.DoWork();
    }
  }
 ~CGetDirectory()
  {
    CJobManager::GetInstance().CancelJob(m_id);
  }

  CEvent& GetEvent()
  {
    return m_result->m_event;
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
  std::shared_ptr<CResult> m_result;
  unsigned int               m_id;
};


CDirectory::CDirectory() = default;

CDirectory::~CDirectory() = default;

bool CDirectory::GetDirectory(const std::string& strPath, CFileItemList &items, const std::string &strMask, int flags)
{
  CHints hints;
  hints.flags = flags;
  hints.mask = strMask;
  const CURL pathToUrl(strPath);
  return GetDirectory(pathToUrl, items, hints);
}

bool CDirectory::GetDirectory(const std::string& strPath, std::shared_ptr<IDirectory> pDirectory,
                              CFileItemList &items, const std::string &strMask, int flags)
{
  CHints hints;
  hints.flags = flags;
  hints.mask = strMask;
  const CURL pathToUrl(strPath);
  return GetDirectory(pathToUrl, pDirectory, items, hints);
}

bool CDirectory::GetDirectory(const std::string& strPath, CFileItemList &items, const CHints &hints)
{
  const CURL pathToUrl(strPath);
  return GetDirectory(pathToUrl, items, hints);
}

bool CDirectory::GetDirectory(const CURL& url, CFileItemList &items, const std::string &strMask, int flags)
{
  CHints hints;
  hints.flags = flags;
  hints.mask = strMask;
  return GetDirectory(url, items, hints);
}

bool CDirectory::GetDirectory(const CURL& url, CFileItemList &items, const CHints &hints)
{
  CURL realURL = URIUtils::SubstitutePath(url);
  std::shared_ptr<IDirectory> pDirectory(CDirectoryFactory::Create(realURL));
  return CDirectory::GetDirectory(url, pDirectory, items, hints);
}

bool CDirectory::GetDirectory(const CURL& url, std::shared_ptr<IDirectory> pDirectory,
                              CFileItemList &items, const CHints &hints)
{
  try
  {
    CURL realURL = URIUtils::SubstitutePath(url);
    if (!pDirectory.get())
      return false;

    // check our cache for this path
    if (g_directoryCache.GetDirectory(realURL.Get(), items, (hints.flags & DIR_FLAG_READ_CACHE) == DIR_FLAG_READ_CACHE))
      items.SetURL(url);
    else
    {
      // need to clear the cache (in case the directory fetch fails)
      // and (re)fetch the folder
      if (!(hints.flags & DIR_FLAG_BYPASS_CACHE))
        g_directoryCache.ClearDirectory(realURL.Get());

      pDirectory->SetFlags(hints.flags);

      bool result = false, cancel = false;
      CURL authUrl = CURL(realURL);

      while (!result && !cancel)
      {
        const std::string pathToUrl(url.Get());

        // don't change auth if it's set explicitly
        if (CPasswordManager::GetInstance().IsURLSupported(authUrl) && authUrl.GetUserName().empty())
          CPasswordManager::GetInstance().AuthenticateURL(authUrl);

        items.SetURL(url);
        result = pDirectory->GetDirectory(authUrl, items);

        if (!result)
        {
          if (!cancel)
          {
            // @TODO ProcessRequirements() can bring up the keyboard input dialog
            // filesystem must not depend on GUI
            if (g_application.IsCurrentThread() && pDirectory->ProcessRequirements())
            {
              authUrl.SetDomain("");
              authUrl.SetUserName("");
              authUrl.SetPassword("");
              continue;
            }
          }
          CLog::Log(LOGERROR, "%s - Error getting %s", __FUNCTION__, url.GetRedacted().c_str());
          return false;
        }
      }

      // hide credentials if necessary
      if (CPasswordManager::GetInstance().IsURLSupported(realURL))
      {
        bool hide = false;
        // for explicitly credentials
        if (!realURL.GetUserName().empty())
        {
          // credentials was changed i.e. were stored in the password
          // manager, in this case we can hide them from an item URL,
          // otherwise we have to keep credentials in an item URL
          if ( realURL.GetUserName() != authUrl.GetUserName()
            || realURL.GetPassWord() != authUrl.GetPassWord()
            || realURL.GetDomain() != authUrl.GetDomain())
          {
            hide = true;
          }
        }
        else
        {
          // hide credentials in any other cases
          hide = true;
        }

        if (hide)
        {
          for (int i = 0; i < items.Size(); ++i)
          {
            CFileItemPtr item = items[i];
            CURL itemUrl = item->GetURL();
            itemUrl.SetDomain("");
            itemUrl.SetUserName("");
            itemUrl.SetPassword("");
            item->SetPath(itemUrl.Get());
          }
        }
      }

      // cache the directory, if necessary
      if (!(hints.flags & DIR_FLAG_BYPASS_CACHE))
        g_directoryCache.SetDirectory(realURL.Get(), items, pDirectory->GetCacheType(url));
    }

    // now filter for allowed files
    if (!pDirectory->AllowAll())
    {
      pDirectory->SetMask(hints.mask);
      for (int i = 0; i < items.Size(); ++i)
      {
        CFileItemPtr item = items[i];
        if (!item->m_bIsFolder && !pDirectory->IsAllowed(item->GetURL()))
        {
          items.Remove(i);
          i--; // don't confuse loop
        }
      }
    }
    // filter hidden files
    //! @todo we shouldn't be checking the gui setting here, callers should use getHidden instead
    if (!CServiceBroker::GetSettings().GetBool(CSettings::SETTING_FILELISTS_SHOWHIDDEN) && !(hints.flags & DIR_FLAG_GET_HIDDEN))
    {
      for (int i = 0; i < items.Size(); ++i)
      {
        if (items[i]->GetProperty("file:hidden").asBoolean())
        {
          items.Remove(i);
          i--; // don't confuse loop
        }
      }
    }

    //  Should any of the files we read be treated as a directory?
    //  Disable for database folders, as they already contain the extracted items
    if (!(hints.flags & DIR_FLAG_NO_FILE_DIRS) && !items.IsMusicDb() && !items.IsVideoDb() && !items.IsSmartPlayList())
      FilterFileDirectories(items, hints.mask);

    // Correct items for path substitution
    const std::string pathToUrl(url.Get());
    const std::string pathToUrl2(realURL.Get());
    if (pathToUrl != pathToUrl2)
    {
      for (int i = 0; i < items.Size(); ++i)
      {
        CFileItemPtr item = items[i];
        item->SetPath(URIUtils::SubstitutePath(item->GetPath(), true));
      }
    }

    return true;
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error getting %s", __FUNCTION__, url.GetRedacted().c_str());
  return false;
}

bool CDirectory::Create(const std::string& strPath)
{
  const CURL pathToUrl(strPath);
  return Create(pathToUrl);
}

bool CDirectory::Create(const CURL& url)
{
  try
  {
    CURL realURL = URIUtils::SubstitutePath(url);

    if (CPasswordManager::GetInstance().IsURLSupported(realURL) && realURL.GetUserName().empty())
      CPasswordManager::GetInstance().AuthenticateURL(realURL);

    std::unique_ptr<IDirectory> pDirectory(CDirectoryFactory::Create(realURL));
    if (pDirectory.get())
      if(pDirectory->Create(realURL))
        return true;
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error creating %s", __FUNCTION__, url.GetRedacted().c_str());
  return false;
}

bool CDirectory::Exists(const std::string& strPath, bool bUseCache /* = true */)
{
  const CURL pathToUrl(strPath);
  return Exists(pathToUrl, bUseCache);
}

bool CDirectory::Exists(const CURL& url, bool bUseCache /* = true */)
{
  try
  {
    CURL realURL = URIUtils::SubstitutePath(url);
    if (bUseCache)
    {
      bool bPathInCache;
      std::string realPath(realURL.Get());
      URIUtils::AddSlashAtEnd(realPath);
      if (g_directoryCache.FileExists(realPath, bPathInCache))
        return true;
      if (bPathInCache)
        return false;
    }

    if (CPasswordManager::GetInstance().IsURLSupported(realURL) && realURL.GetUserName().empty())
      CPasswordManager::GetInstance().AuthenticateURL(realURL);

    std::unique_ptr<IDirectory> pDirectory(CDirectoryFactory::Create(realURL));
    if (pDirectory.get())
      return pDirectory->Exists(realURL);
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error checking for %s", __FUNCTION__, url.GetRedacted().c_str());
  return false;
}

bool CDirectory::Remove(const std::string& strPath)
{
  const CURL pathToUrl(strPath);
  return Remove(pathToUrl);
}

bool CDirectory::RemoveRecursive(const std::string& strPath)
{
  return RemoveRecursive(CURL{ strPath });
}

bool CDirectory::Remove(const CURL& url)
{
  try
  {
    CURL realURL = URIUtils::SubstitutePath(url);
    CURL authUrl = realURL;
    if (CPasswordManager::GetInstance().IsURLSupported(authUrl) && authUrl.GetUserName().empty())
      CPasswordManager::GetInstance().AuthenticateURL(authUrl);

    std::unique_ptr<IDirectory> pDirectory(CDirectoryFactory::Create(realURL));
    if (pDirectory.get())
      if(pDirectory->Remove(authUrl))
      {
        g_directoryCache.ClearFile(realURL.Get());
        return true;
      }
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error removing %s", __FUNCTION__, url.GetRedacted().c_str());
  return false;
}

bool CDirectory::RemoveRecursive(const CURL& url)
{
  try
  {
    CURL realURL = URIUtils::SubstitutePath(url);
    CURL authUrl = realURL;
    if (CPasswordManager::GetInstance().IsURLSupported(authUrl) && authUrl.GetUserName().empty())
      CPasswordManager::GetInstance().AuthenticateURL(authUrl);

    std::unique_ptr<IDirectory> pDirectory(CDirectoryFactory::Create(realURL));
    if (pDirectory.get())
      if(pDirectory->RemoveRecursive(authUrl))
      {
        g_directoryCache.ClearFile(realURL.Get());
        return true;
      }
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error removing %s", __FUNCTION__, url.GetRedacted().c_str());
  return false;
}

void CDirectory::FilterFileDirectories(CFileItemList &items, const std::string &mask,
                                       bool expandImages)
{
  for (int i=0; i< items.Size(); ++i)
  {
    CFileItemPtr pItem=items[i];
    auto mode = expandImages ? EFILEFOLDER_TYPE_ONBROWSE : EFILEFOLDER_TYPE_ALWAYS;
    if (!pItem->m_bIsFolder && pItem->IsFileFolder(mode))
    {
      std::unique_ptr<IFileDirectory> pDirectory(CFileDirectoryFactory::Create(pItem->GetURL(),pItem.get(),mask));
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
