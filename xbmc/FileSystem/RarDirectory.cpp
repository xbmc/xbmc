#include "RarDirectory.h"
#include "../application.h"
#include "../utils/log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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
}