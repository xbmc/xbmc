#include "RarDirectory.h"
#include "../application.h"
#include "../utils/log.h"


namespace DIRECTORY
{
  CRarDirectory::CRarDirectory()
  {
  }

  CRarDirectory::~CRarDirectory()
  {
  }

  bool CRarDirectory::GetDirectory(const CStdString& strPath, CFileItemList& items)
  {
    CURL url(strPath);
    if (g_RarManager.GetFilesInRar(items,url.GetHostName(),true,url.GetFileName()))
    {
      // fill in paths
      for( int iEntry=0;iEntry<items.Size();++iEntry)
      {
        if (items[iEntry]->GetLabel() == "..")
          continue;
        if ((IsAllowed(items[iEntry]->m_strPath)) || (items[iEntry]->m_bIsFolder))
        {
          items[iEntry]->m_strPath = strPath+items[iEntry]->m_strPath;
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