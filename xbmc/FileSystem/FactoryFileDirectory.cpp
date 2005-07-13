
#include "../stdafx.h"
#include "../util.h"
#include "factoryfiledirectory.h"
#include "oggfiledirectory.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CFactoryFileDirectory::CFactoryFileDirectory(void)
{}

CFactoryFileDirectory::~CFactoryFileDirectory(void)
{}

IFileDirectory* CFactoryFileDirectory::Create(const CStdString& strPath)
{
  CStdString strExtension=CUtil::GetExtension(strPath);
  strExtension.MakeLower();

  if (strExtension.Equals(".ogg") && CFile::Exists(strPath))
  {
    IFileDirectory* pDir=new COGGFileDirectory;
    //  Has the ogg file more then one bitstream?
    if (pDir->ContainsFiles(strPath))
      return pDir; // treat as directory

    delete pDir;
    return NULL;
  }

  return NULL;
}
