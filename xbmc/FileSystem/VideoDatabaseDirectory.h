#pragma once
#include "idirectory.h"
#include "videodatabasedirectory/DirectoryNode.h"
#include "videodatabasedirectory/QueryParams.h"

namespace DIRECTORY
{
  class CVideoDatabaseDirectory : public IDirectory
  {
  public:
    CVideoDatabaseDirectory(void);
    virtual ~CVideoDatabaseDirectory(void);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    virtual bool Exists(const char* strPath);
    VIDEODATABASEDIRECTORY::NODE_TYPE GetDirectoryChildType(const CStdString& strPath);
    VIDEODATABASEDIRECTORY::NODE_TYPE GetDirectoryType(const CStdString& strPath);
    VIDEODATABASEDIRECTORY::NODE_TYPE GetDirectoryParentType(const CStdString& strPath);
    bool GetQueryParams(const CStdString& strPath, VIDEODATABASEDIRECTORY::CQueryParams& params);
    void ClearDirectoryCache(const CStdString& strDirectory);
    static bool IsAllItem(const CStdString& strDirectory);
    bool GetLabel(const CStdString& strDirectory, CStdString& strLabel);
    bool ContainsMovies(const CStdString &path);
    static bool CanCache(const CStdString &path);
  };
};
