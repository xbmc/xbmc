
#include "stdafx.h"
#include "../Util.h"
#include "SIDFileDirectory.h"
#include "../MusicInfoLoader.h"
#include "DirectoryCache.h"

using namespace DIRECTORY;

CSIDFileDirectory::CSIDFileDirectory(void)
{}

CSIDFileDirectory::~CSIDFileDirectory(void)
{}

bool CSIDFileDirectory::GetDirectory(const CStdString& strPath1, CFileItemList &items)
{
  CFileItemList vecCacheItems;
  g_directoryCache.ClearDirectory(strPath1);
  
  CStdString strPath=strPath1;
  CURL url(strPath);

  //  The ogg file name is the new name for our virtual
  //  ogg directory
  CStdString strFileName=CUtil::GetFileName(strPath);
  CMusicInfoLoader tagloader;
  int iStreams = m_dll.GetNumberOfSongs(strPath1.c_str());

  CUtil::AddDirectorySeperator(strPath);

  for (int i=0; i<iStreams; ++i)
  {
    CStdString strLabel;
    strLabel.Format("%s - Track %02.2i", strFileName,i+1);
    CFileItem* pItem=new CFileItem(strLabel);
    pItem->m_strPath.Format("%s%s-%i.sidstream", strPath.c_str(),strFileName.c_str(),i+1);
    //tagloader.LoadItem(pItem);
        
    items.Add(pItem);
    vecCacheItems.Add(new CFileItem(*pItem));
  }

  if (m_cacheDirectory)
    g_directoryCache.SetDirectory(strPath, vecCacheItems);

  return true;
}

bool CSIDFileDirectory::Exists(const char* strPath)
{
  return true;
}

bool CSIDFileDirectory::ContainsFiles(const CStdString& strPath)
{
  if (!m_dll.Load())
    return false;

  if (m_dll.GetNumberOfSongs(strPath.c_str()) > 1)
    return true;

  return false;
}

