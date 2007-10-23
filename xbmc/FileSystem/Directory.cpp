
#include "stdafx.h"
#include "Directory.h"
#include "FactoryDirectory.h"
#include "FactoryFileDirectory.h"
#ifndef _LINUX
#include "../utils/Win32Exception.h"
#endif
#include "../Util.h"

using namespace DIRECTORY;

CDirectory::CDirectory()
{}

CDirectory::~CDirectory()
{}

bool CDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items, CStdString strMask /*=""*/, bool bUseFileDirectories /* = true */, bool allowPrompting /* = false */, bool cacheDirectory /* = false */)
{
  try 
  {
    CStdString translatedPath = CUtil::TranslateSpecialPath(_P(strPath));

    auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(translatedPath));
    if (!pDirectory.get()) return false;

    pDirectory->SetMask(strMask);
    pDirectory->SetAllowPrompting(allowPrompting);
    pDirectory->SetCacheDirectory(cacheDirectory);

    items.m_strPath=_P(strPath);

    bool bSuccess = pDirectory->GetDirectory(translatedPath, items);
    if (bSuccess)
    {
      //  Should any of the files we read be treated as a directory?
      //  Disable for musicdatabase, it already contains the extracted items
      if (bUseFileDirectories && !items.IsMusicDb() && !items.IsVideoDb())
      {
        for (int i=0; i< items.Size(); ++i)
        {
          CFileItem* pItem=items[i];
          if ((!pItem->m_bIsFolder) && (!pItem->IsInternetStream()))
          {
            auto_ptr<IFileDirectory> pDirectory(CFactoryFileDirectory::Create(pItem->m_strPath,pItem,strMask));
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
    }
    return bSuccess;
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
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
    CStdString translatedPath = CUtil::TranslateSpecialPath(_P(strPath));
    auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(translatedPath));
    if (pDirectory.get())
      if(pDirectory->Create(translatedPath.c_str()))
        return true;
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
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
    CStdString translatedPath = CUtil::TranslateSpecialPath(strPath);
    auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(translatedPath));
    if (pDirectory.get())
      return pDirectory->Exists(translatedPath.c_str());
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
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
    CStdString translatedPath = CUtil::TranslateSpecialPath(strPath);
    auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(translatedPath));
    if (pDirectory.get())
      if(pDirectory->Remove(translatedPath.c_str()))
        return true;
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error removing %s", __FUNCTION__, strPath.c_str());
  return false;
}
