#pragma once
#include "idirectory.h"

using namespace DIRECTORY;
namespace DIRECTORY
{
  class CShoutcastDirectory :
    public IDirectory
  {
  public:
    CShoutcastDirectory(void);
    virtual ~CShoutcastDirectory(void);
    virtual bool  GetDirectory(const CStdString& strPath,CFileItemList &items);
            bool  DownloadPlaylists(CFileItemList &items);
            void  CacheItems(CFileItemList &items);
            void  LoadCachedItems(CFileItemList &items);
            bool  IsCacheValid();
  };
}