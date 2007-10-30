
#include "stdafx.h"
#include "RarDirectory.h"
#include "../utils/log.h"
#include "../Util.h"

namespace DIRECTORY
{
  CRarDirectory::CRarDirectory()
  {
  }

  CRarDirectory::~CRarDirectory()
  {
  }

  bool CRarDirectory::GetDirectory(const CStdString& strPathOrig, CFileItemList& items)
  {
    CStdString strPath;

    /* if this isn't a proper archive path, assume it's the path to a archive file */
    if( !strPathOrig.Left(6).Equals("rar://") )
      CUtil::CreateRarPath(strPath, strPathOrig, "");
    else
      strPath = strPathOrig;

    CURL url(strPath);
    CStdString strArchive = url.GetHostName();
    CStdString strOptions = url.GetOptions();
    CStdString strPathInArchive = url.GetFileName();
    url.SetOptions("");

    CStdString strSlashPath;
    url.GetURL(strSlashPath);

    // the RAR code depends on things having a "\" at the end of the path
    if (!CUtil::HasSlashAtEnd(strSlashPath))
      strSlashPath += "/";

    if (g_RarManager.GetFilesInRar(items,strArchive,true,strPathInArchive))
    {
      // fill in paths
      for( int iEntry=0;iEntry<items.Size();++iEntry)
      {
        if (items[iEntry]->IsParentFolder())
          continue;
        if ((IsAllowed(items[iEntry]->m_strPath)) || (items[iEntry]->m_bIsFolder))
        {
          CUtil::AddFileToFolder(strSlashPath,items[iEntry]->m_strPath+strOptions,items[iEntry]->m_strPath);
          items[iEntry]->m_iDriveType = 0;
          //CLog::Log(LOGDEBUG, "RarDirectory::GetDirectory() retrieved file: %s", items[iEntry]->m_strPath.c_str());
        }
        else
        {
          items.Remove(iEntry);
          iEntry--; //do not confuse loop
        }
      } 
      return( true);
    }
    else
      return( false );
  }

  bool CRarDirectory::Exists(const char* strPath)
  {
    CFileItemList items;
    if (GetDirectory(strPath,items))
      return true;

    return false;
  }
  
  bool CRarDirectory::ContainsFiles(const CStdString& strPath)
  {
    CFileItemList items;
    if (g_RarManager.GetFilesInRar(items,strPath))
    {
      if (items.Size() > 1)
        return true;
      
      return false;
    }
    
    return false;
  }
}
