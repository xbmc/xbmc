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
    virtual bool  GetDirectory(const CStdString& strPath,VECFILEITEMS &items);
            bool  DownloadPlaylists(VECFILEITEMS &items);
            void  CacheItems(VECFILEITEMS &items);
            void  LoadCachedItems(VECFILEITEMS &items);
            bool  IsCacheValid();
  };
}