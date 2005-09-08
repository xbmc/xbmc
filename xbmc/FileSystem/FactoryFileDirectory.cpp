
#include "../stdafx.h"
#include "../util.h"
#include "factoryfiledirectory.h"
#include "oggfiledirectory.h"
//#include "rardirectory.h"
//#include "zipdirectory.h"
#include "nsffiledirectory.h"


CFactoryFileDirectory::CFactoryFileDirectory(void)
{}

CFactoryFileDirectory::~CFactoryFileDirectory(void)
{}

//IFileDirectory* CFactoryFileDirectory::Create(const CStdString& strPath, CFileItem* pItem, const CStdString& strMask )
IFileDirectory* CFactoryFileDirectory::Create(const CStdString& strPath )
{
  CStdString strExtension=CUtil::GetExtension(strPath);
  strExtension.MakeLower();

  if (strExtension.Equals(".ogg") && CFile::Exists(strPath))
  {
    IFileDirectory* pDir=new COGGFileDirectory;
    //  Has the ogg file more then one bitstream?
    if (pDir->ContainsFiles(strPath))
    {
      return pDir; // treat as directory
    }

    delete pDir;
    return NULL;
  }
  if (strExtension.Equals(".nsf") && CFile::Exists(strPath))
  {
    IFileDirectory* pDir=new CNSFFileDirectory;
    //  Has the nsf file more then one track?
    if (pDir->ContainsFiles(strPath))
    {
      return pDir; // treat as directory
    }

    delete pDir;
    return NULL;
  }
  /*if (strExtension.Equals(".rar") && g_guiSettings.GetBool("VideoFiles.HandleArchives"))
  {
    std::vector<CStdString> token;
    CStdString strLower = strPath;
    strLower.ToLower();
    CUtil::Tokenize(strPath,token,".");
    if (token.size() > 2)
      if (token[token.size()-2].Left(4) == "part")
      {
        int size = token[token.size()-2].size();
        int i = atoi(token[token.size()-2].Mid(4,size-4).c_str());
        if (atoi(token[token.size()-2].Mid(4,size-4).c_str()) != 1)
         {
          pItem->m_bIsFolder = true;
          return NULL;
        }
      }
    CFileItemList items;
    IFileDirectory* pDir = new CRarDirectory();
    pDir->SetMask(strMask);
    CStdString strUrl;
    CUtil::CreateRarPath(strUrl,strPath,"");
    pDir->GetDirectory(strUrl,items);
    
    if (items.Size() > 1)
    {
      return pDir;
    }

    delete pDir;

    if (items.Size() == 1)
    {
      if (items[0]->IsRAR())
      {
        CRarDirectory* pDir2 = new CRarDirectory();
        pDir2->SetMask(strMask);
        CStdString strPath2;
        CUtil::CreateRarPath(strPath2,strUrl,"");
        if (!pDir2->ContainsFiles(strPath2))
          pItem->m_bIsFolder = true;
        
        delete pDir2;
      }
      else if (items[0]->IsZIP())
      {
        CZipDirectory* pDir2 = new CZipDirectory();
        pDir2->SetMask(strMask);
        CStdString strPath2;
        strPath2.Format("zip://Z:\\,2,,%s,\\",strUrl.c_str());
        if (!pDir2->ContainsFiles(strPath2))
          pItem->m_bIsFolder = true;
        
        delete pDir2;
      }
      else if (!pItem->m_bIsFolder)
        *pItem = *items[0];
    }
    else
      pItem->m_bIsFolder = true; // no files no dirs
  }
  if (strExtension.Equals(".zip") && g_guiSettings.GetBool("VideoFiles.HandleArchives"))
  {
    CFileItemList items;
    IFileDirectory* pDir = new CZipDirectory();
    pDir->SetMask(strMask);
    CStdString strUrl;
    strUrl.Format("zip://Z:\\,2,,%s,\\",strPath.c_str());
    pDir->GetDirectory(strUrl,items);

    if (items.Size() > 1)
    {
      return pDir;
    }

    delete pDir;

    if (items.Size() == 1)
    {
      if (items[0]->IsRAR())
      {
        CRarDirectory* pDir2 = new CRarDirectory();
        pDir2->SetMask(strMask);
        CStdString strPath2;
        CUtil::CreateRarPath(strPath2,strUrl,"");
        if (!pDir2->ContainsFiles(strPath2))
          pItem->m_bIsFolder = true;
        
        delete pDir2;
      }
      else if (items[0]->IsZIP())
      {
        CZipDirectory* pDir2 = new CZipDirectory();
        pDir2->SetMask(strMask);
        CStdString strPath2;
        strPath2.Format("zip://Z:\\,2,,%s,\\",strUrl.c_str());
        if (!pDir2->ContainsFiles(strPath2))
          pItem->m_bIsFolder = true;
        
        delete pDir2;
      }
      else if (!pItem->m_bIsFolder)
        *pItem = *items[0];
    }
    else
      pItem->m_bIsFolder = true; // no files no dirs
  }
*/
  return NULL;
}