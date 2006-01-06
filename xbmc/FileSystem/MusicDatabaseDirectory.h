#pragma once
#include "idirectory.h"
#include "musicdatabasedirectory/DirectoryNode.h"

using namespace DIRECTORY;

namespace DIRECTORY
{
  class CMusicDatabaseDirectory : public IDirectory
  {
  public:
    CMusicDatabaseDirectory(void);
    virtual ~CMusicDatabaseDirectory(void);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    MUSICDATABASEDIRECTORY::NODE_TYPE GetDirectoryChildType(const CStdString& strPath);
    MUSICDATABASEDIRECTORY::NODE_TYPE GetDirectoryType(const CStdString& strPath);
    MUSICDATABASEDIRECTORY::NODE_TYPE GetDirectoryParentType(const CStdString& strPath);
    bool IsArtistDir(const CStdString& strDirectory);
    bool HasAlbumInfo(const CStdString& strDirectory);
    void ClearDirectoryCache(const CStdString& strDirectory);
    bool IsAllItem(const CStdString& strDirectory);
  };
};
