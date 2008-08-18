/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "stdafx.h"
#include "Directory.h"
#include "FactoryDirectory.h"
#include "FactoryFileDirectory.h"
#include "utils/Win32Exception.h"
#include "Util.h"
#include "FileItem.h"

using namespace std;
using namespace DIRECTORY;

CDirectory::CDirectory()
{}

CDirectory::~CDirectory()
{}

bool CDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items, CStdString strMask /*=""*/, bool bUseFileDirectories /* = true */, bool allowPrompting /* = false */, bool cacheDirectory /* = false */)
{
  try 
  {
    CStdString translatedPath = CUtil::TranslateSpecialPath(strPath);

    auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(translatedPath));
    if (!pDirectory.get()) return false;

    pDirectory->SetMask(strMask);
    pDirectory->SetAllowPrompting(allowPrompting);
    pDirectory->SetCacheDirectory(cacheDirectory);
    pDirectory->SetUseFileDirectories(bUseFileDirectories);

    items.m_strPath=strPath;

    bool bSuccess = pDirectory->GetDirectory(translatedPath, items);
    if (bSuccess)
    {
      //  Should any of the files we read be treated as a directory?
      //  Disable for musicdatabase, it already contains the extracted items
      if (bUseFileDirectories && !items.IsMusicDb() && !items.IsVideoDb() && !items.IsSmartPlayList())
      {
        for (int i=0; i< items.Size(); ++i)
        {
          CFileItemPtr pItem=items[i];
          if ((!pItem->m_bIsFolder) && (!pItem->IsInternetStream()))
          {
            auto_ptr<IFileDirectory> pDirectory(CFactoryFileDirectory::Create(pItem->m_strPath,pItem.get(),strMask));
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
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
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
    CStdString translatedPath = CUtil::TranslateSpecialPath(strPath);
    auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(translatedPath));
    if (pDirectory.get())
      if(pDirectory->Create(translatedPath.c_str()))
        return true;
  }
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
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
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
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
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error removing %s", __FUNCTION__, strPath.c_str());
  return false;
}
