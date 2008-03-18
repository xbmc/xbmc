#include "stdafx.h"
#include "MemUnitDirectory.h"
#include "DirectoryCache.h"
#include "../utils/MemoryUnitManager.h"
#include "MemoryUnits/IFileSystem.h"
#include "MemoryUnits/IDevice.h"

using namespace DIRECTORY;

CMemUnitDirectory::CMemUnitDirectory(void)
{}

CMemUnitDirectory::~CMemUnitDirectory(void)
{}

bool CMemUnitDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  IFileSystem *fileSystem = GetFileSystem(strPath);
  if (!fileSystem) return false;
  
  g_directoryCache.ClearDirectory(strPath);
  CFileItemList cacheItems;
  if (!fileSystem->GetDirectory(strPath.Mid(7), cacheItems))
  {
    delete fileSystem;
    return false;
  }

  for (int i = 0; i < cacheItems.Size(); i++)
  {
    CFileItem *item = cacheItems[i];
    if (item->m_bIsFolder || IsAllowed(item->m_strPath))
      items.Add(new CFileItem(*item));
  }
  if (m_cacheDirectory)
    g_directoryCache.SetDirectory(strPath, cacheItems);
  delete fileSystem;
  return true;
}

bool CMemUnitDirectory::Create(const char* strPath)
{
  IFileSystem *fileSystem = GetFileSystem(strPath);
  if (!fileSystem) return false;
  return fileSystem->MakeDir(strPath + 7);
}

bool CMemUnitDirectory::Remove(const char* strPath)
{
  IFileSystem *fileSystem = GetFileSystem(strPath);
  if (!fileSystem) return false;
  return fileSystem->RemoveDir(strPath + 7);
}

bool CMemUnitDirectory::Exists(const char* strPath)
{
  CFileItemList items;
  if (GetDirectory(strPath, items))
    return true;
  return false;
}

IFileSystem *CMemUnitDirectory::GetFileSystem(const CStdString &path)
{
  // format is mem#://folder/file
  if (!path.Left(3).Equals("mem") || path.size() < 7)
    return NULL;

  char unit = path[3] - '0';

  return g_memoryUnitManager.GetFileSystem(unit);
}
