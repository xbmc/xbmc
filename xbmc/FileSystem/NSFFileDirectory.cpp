
#include "stdafx.h"
#include "../Util.h"
#include "NSFFileDirectory.h"
#include "../MusicInfoTagLoaderNSF.h"
#include "DirectoryCache.h"

using namespace DIRECTORY;

CNSFFileDirectory::CNSFFileDirectory(void)
{}

CNSFFileDirectory::~CNSFFileDirectory(void)
{}

bool CNSFFileDirectory::GetDirectory(const CStdString& strPath1, CFileItemList &items)
{
  CFileItemList vecCacheItems;
  g_directoryCache.ClearDirectory(strPath1);
  
  CStdString strPath=strPath1;
  CURL url(strPath);

  CMusicInfoTagLoaderNSF nsf;

  //  The ogg file name is the new name for our virtual
  //  ogg directory
  CStdString strFileName=CUtil::GetFileName(strPath);
  int iStreams = nsf.GetStreamCount(strPath1);
  CMusicInfoTag tag;
  nsf.Load(strPath1,tag);
  tag.SetDuration(4*60); // 4 mins

  CUtil::AddDirectorySeperator(strPath);

  for (int i=0; i<iStreams; ++i)
  {
    CStdString strLabel;
    strLabel.Format("%s - Track %02.2i", strFileName,i+1);
    CFileItem* pItem=new CFileItem(strLabel);
    pItem->m_strPath.Format("%s%s-%i.nsfstream", strPath.c_str(),strFileName.c_str(),i+1);
    
    if (tag.Loaded())
      *pItem->GetMusicInfoTag() = tag;

    pItem->GetMusicInfoTag()->SetTrackNumber(i+1);
    items.Add(pItem);
    vecCacheItems.Add(new CFileItem(*pItem));
  }

  if (m_cacheDirectory)
    g_directoryCache.SetDirectory(strPath, vecCacheItems);

  return true;
}

bool CNSFFileDirectory::Exists(const char* strPath)
{
  return true;
}

bool CNSFFileDirectory::ContainsFiles(const CStdString& strPath)
{
  CMusicInfoTagLoaderNSF nsf;
  if (nsf.GetStreamCount(strPath) > 1)
  {
    CLog::Log(LOGDEBUG,"contains files!");
    return true;
  }

  return false;
}
