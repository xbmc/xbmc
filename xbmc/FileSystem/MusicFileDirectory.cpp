
#include "stdafx.h"
#include "Util.h"
#include "MusicFileDirectory.h"
#include "DirectoryCache.h"
#include "FileItem.h"
#include "URL.h"

using namespace MUSIC_INFO;
using namespace DIRECTORY;

CMusicFileDirectory::CMusicFileDirectory(void)
{
}

CMusicFileDirectory::~CMusicFileDirectory(void)
{
}

bool CMusicFileDirectory::GetDirectory(const CStdString& strPath1, CFileItemList &items)
{
  CFileItemList vecCacheItems;
  g_directoryCache.ClearDirectory(strPath1);
  
  CStdString strPath=strPath1;
  CURL url(strPath);

  CStdString strFileName;
  strFileName = CUtil::GetFileName(strPath);
  CUtil::RemoveExtension(strFileName);

  int iStreams = GetTrackCount(strPath1);

  CUtil::AddDirectorySeperator(strPath);

  for (int i=0; i<iStreams; ++i)
  {
    CStdString strLabel;
    strLabel.Format("%s - %s %02.2i", strFileName.c_str(),g_localizeStrings.Get(554).c_str(),i+1);
    CFileItem* pItem=new CFileItem(strLabel);
    pItem->m_strPath.Format("%s%s-%i.%s", strPath.c_str(),strFileName.c_str(),i+1,m_strExt.c_str());
    
    if (m_tag.Loaded())
      *pItem->GetMusicInfoTag() = m_tag;

    pItem->GetMusicInfoTag()->SetTrackNumber(i+1);
    items.Add(pItem);
    vecCacheItems.Add(new CFileItem(*pItem));
  }

  if (m_cacheDirectory)
    g_directoryCache.SetDirectory(strPath, vecCacheItems);

  return true;
}

bool CMusicFileDirectory::Exists(const char* strPath)
{
  return true;
}

bool CMusicFileDirectory::ContainsFiles(const CStdString& strPath)
{
  if (GetTrackCount(strPath) > 1)
    return true;

  return false;
}
