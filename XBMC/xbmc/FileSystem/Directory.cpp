
#include "../stdafx.h"
#include "directory.h"
#include "factorydirectory.h"

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
