
#include "../stdafx.h"
#include "directory.h"
#include "factorydirectory.h"
#include "factoryfiledirectory.h"

using namespace DIRECTORY;

CDirectory::CDirectory()
{}

CDirectory::~CDirectory()
{}

bool CDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items, CStdString strMask/*=""*/)
{
  bool bSucces;
  IDirectory* pDirectory = CFactoryDirectory().Create(strPath);
  if (!pDirectory) return false;

  pDirectory->SetMask(strMask);

  bSucces = pDirectory->GetDirectory(strPath, items);

  //  Should any of the files we read be treated as a directory?
  for (int i=0; i< items.Size(); ++i)
  {
    CFileItem* pItem=items[i];
    if (!pItem->m_bIsFolder)
    {
      IFileDirectory* pDirectory = CFactoryFileDirectory().Create(pItem->m_strPath);
      if (pDirectory)
      {
        pItem->m_bIsFolder=true;
        delete pDirectory;
      }
    }
  }

  delete pDirectory;
  return bSucces;
}

bool CDirectory::Create(const char* strPath)
{
  bool bResult;
  IDirectory* pDirectory = CFactoryDirectory().Create(strPath);
  if (!pDirectory) return false;

  bResult = pDirectory->Create(strPath);
  delete pDirectory;
  return bResult;
}

bool CDirectory::Exists(const char* strPath)
{
  bool bResult;
  IDirectory* pDirectory = CFactoryDirectory().Create(strPath);
  if (!pDirectory) return false;

  bResult = pDirectory->Exists(strPath);
  delete pDirectory;
  return bResult;
}

bool CDirectory::Remove(const char* strPath)
{
  bool bResult;
  IDirectory* pDirectory = CFactoryDirectory().Create(strPath);
  if (!pDirectory) return false;

  bResult = pDirectory->Remove(strPath);
  delete pDirectory;
  return bResult;
}
