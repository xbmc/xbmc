
#include "../stdafx.h"
#include "directory.h"
#include "factorydirectory.h"
#include "factoryfiledirectory.h"


using namespace DIRECTORY;

CDirectory::CDirectory()
{}

CDirectory::~CDirectory()
{}

bool CDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items, CStdString strMask /*=""*/, bool bUseFileDirectories /* = true */, bool allowPrompting /* = false */, bool cacheDirectory /* = false */)
{
  try 
  {

  auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(strPath));
  if (!pDirectory.get()) return false;

  pDirectory->SetMask(strMask);
  pDirectory->SetAllowPrompting(allowPrompting);
  pDirectory->SetCacheDirectory(cacheDirectory);

  items.m_strPath=strPath;

  bool bSuccess = pDirectory->GetDirectory(strPath, items);
  if (bSuccess)
  {
    //  Should any of the files we read be treated as a directory?
    //  Disable for musicdatabase, it already contains the extracted items
    if (bUseFileDirectories && !items.IsMusicDb())
      for (int i=0; i< items.Size(); ++i)
      {
        CFileItem* pItem=items[i];
        if (!pItem->m_bIsFolder)
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
  return bSuccess;

  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unhandled exception getting %s", strPath.c_str());
    return false;
  }
}

bool CDirectory::Create(const CStdString& strPath)
{
  try
  {
    auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(strPath));
    if (!pDirectory.get()) return false;

    return pDirectory->Create(strPath.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unhandled exception (%s)", strPath.c_str());
    return false;
  }
}

bool CDirectory::Exists(const CStdString& strPath)
{
  try
  {
    auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(strPath));
    if (!pDirectory.get()) return false;

    return pDirectory->Exists(strPath.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unhandled exception (%s)", strPath.c_str());
    return false;
  }
}

bool CDirectory::Remove(const CStdString& strPath)
{
  try
  {
    auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(strPath));
    if (!pDirectory.get()) return false;

    return pDirectory->Remove(strPath.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unhandled exception (%s)", strPath.c_str());
    return false;
  }

}
