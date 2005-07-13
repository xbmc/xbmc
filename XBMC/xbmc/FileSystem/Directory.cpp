
#include "../stdafx.h"
#include "directory.h"
#include "factorydirectory.h"
#include "factoryfiledirectory.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace DIRECTORY;

CDirectory::CDirectory()
{}

CDirectory::~CDirectory()
{}

bool CDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items, CStdString strMask/*=""*/)
{
  auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(strPath));
  if (!pDirectory.get()) return false;

  pDirectory->SetMask(strMask);

  bool bSuccess = pDirectory->GetDirectory(strPath, items);

  if (bSuccess)
  {
    //  Should any of the files we read be treated as a directory?
    for (int i=0; i< items.Size(); ++i)
    {
      CFileItem* pItem=items[i];
      if (!pItem->m_bIsFolder)
      {
        auto_ptr<IFileDirectory> pDirectory(CFactoryFileDirectory::Create(pItem->m_strPath));
        if (pDirectory.get())
          pItem->m_bIsFolder=true;
      }
    }
  }

  return bSuccess;
}

bool CDirectory::Create(const CStdString& strPath)
{
  auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(strPath));
  if (!pDirectory.get()) return false;

  return pDirectory->Create(strPath.c_str());
}

bool CDirectory::Exists(const CStdString& strPath)
{
  auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(strPath));
  if (!pDirectory.get()) return false;

  return pDirectory->Exists(strPath.c_str());
}

bool CDirectory::Remove(const CStdString& strPath)
{
  auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(strPath));
  if (!pDirectory.get()) return false;

  return pDirectory->Remove(strPath.c_str());
}
