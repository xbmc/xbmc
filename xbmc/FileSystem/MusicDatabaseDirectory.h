#pragma once
#include "IDirectory.h"
#include "MusicDatabaseDirectory/DirectoryNode.h"
#include "MusicDatabaseDirectory/QueryParams.h"

namespace DIRECTORY
{
  class CMusicDatabaseDirectory : public IDirectory
  {
  public:
    CMusicDatabaseDirectory(void);
    virtual ~CMusicDatabaseDirectory(void);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    virtual bool Exists(const char* strPath);
    static MUSICDATABASEDIRECTORY::NODE_TYPE GetDirectoryChildType(const CStdString& strPath);
    static MUSICDATABASEDIRECTORY::NODE_TYPE GetDirectoryType(const CStdString& strPath);
    static MUSICDATABASEDIRECTORY::NODE_TYPE GetDirectoryParentType(const CStdString& strPath);
    bool IsArtistDir(const CStdString& strDirectory);
    bool HasAlbumInfo(const CStdString& strDirectory);
    void ClearDirectoryCache(const CStdString& strDirectory);
    static bool IsAllItem(const CStdString& strDirectory);
    static bool GetLabel(const CStdString& strDirectory, CStdString& strLabel);
    bool ContainsSongs(const CStdString &path);
    static bool CanCache(const CStdString& strPath);
  };
}
