
#include "../stdafx.h"
#include "../util.h"
#include "factoryfiledirectory.h"
#include "oggfiledirectory.h"

CFactoryFileDirectory::CFactoryFileDirectory(void)
{}

CFactoryFileDirectory::~CFactoryFileDirectory(void)
{}

IFileDirectory* CFactoryFileDirectory::Create(const CStdString& strPath)
{
  CStdString strExtension=CUtil::GetExtension(strPath);
  strExtension.MakeLower();

  if (strExtension.size() && CUtil::FileExists(strPath))
  {
    if (strExtension==".ogg")
    {
      IFileDirectory* pDir=new COGGFileDirectory;
      //  Has the ogg file more then one bitstream?
      if (pDir->ContainsFiles(strPath))
        return pDir; // treat as directory

      delete pDir;
      return NULL;
    }
  }

  return NULL;
}
