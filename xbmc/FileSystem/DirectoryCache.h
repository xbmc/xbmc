#pragma once

#include "directory.h"
#include <vector>
using namespace DIRECTORY;
namespace DIRECTORY
{

  class CDirectoryCache
  {
    class CDir
    {
    public:
      CStdString    m_strPath;
      VECFILEITEMS  m_Items;
    };
  public:
    CDirectoryCache(void);
    virtual ~CDirectoryCache(void);
    static bool  GetDirectory(const CStdString& strPath,VECFILEITEMS &items);
    static void  SetDirectory(const CStdString& strPath,const VECFILEITEMS &items);
    static void  ClearDirectory(const CStdString& strPath);
    static bool  FileExists(const CStdString& strPath,bool& bInCache);
protected:
    vector<CDir> m_vecCache;
    typedef vector<CDir>::iterator ivecCache;
    
  };

};
extern CDirectoryCache g_directoryCache;
