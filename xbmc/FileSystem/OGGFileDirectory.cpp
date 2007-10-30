
#include "stdafx.h"
#include "../Util.h"
#include "OGGFileDirectory.h"
#include "../OggTag.h"
#include "DirectoryCache.h"

using namespace DIRECTORY;

COGGFileDirectory::COGGFileDirectory(void)
{}

COGGFileDirectory::~COGGFileDirectory(void)
{}

bool COGGFileDirectory::GetDirectory(const CStdString& strPath1, CFileItemList &items)
{
  CFileItemList vecCacheItems;
  g_directoryCache.ClearDirectory(strPath1);
  
  CStdString strPath=strPath1;
  CURL url(strPath);

  COggTag tag;
  int iStreams=tag.GetStreamCount(strPath);
  if (iStreams<=1)
    return false;

  //  The ogg file name is the new name for our virtual
  //  ogg directory
  CStdString strFileName=CUtil::GetFileName(strPath);
  strFileName.Replace(".ogg", "");

  CUtil::AddDirectorySeperator(strPath);

  //  Build a filename for each bitstream inside the ogg file
  //  Format is: original file + - + bitstream number + extension
  for (int i=0; i<iStreams; ++i)
  {
    CStdString strLabel;
    strLabel.Format("%s-%02.2i%s", strFileName, i+1 ,".oggstream");
    CFileItem* pItem=new CFileItem(strLabel);
    pItem->m_strPath.Format("%s%s", strPath.c_str(), strLabel.c_str());
    items.Add(pItem);
    vecCacheItems.Add(new CFileItem(*pItem));
  }

  if (m_cacheDirectory)
    g_directoryCache.SetDirectory(strPath, vecCacheItems);

  return true;
}

bool COGGFileDirectory::Exists(const char* strPath)
{
  return true;
}

bool COGGFileDirectory::ContainsFiles(const CStdString& strPath)
{
  COggTag tag;
  int iStreams=tag.GetStreamCount(strPath);
  return (iStreams>1);
}
