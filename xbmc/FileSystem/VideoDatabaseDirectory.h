#pragma once
#include "IDirectory.h"
#include "VideoDatabaseDirectory/DirectoryNode.h"
#include "VideoDatabaseDirectory/QueryParams.h"

namespace DIRECTORY
{
  class CVideoDatabaseDirectory : public IDirectory
  {
  public:
    CVideoDatabaseDirectory(void);
    virtual ~CVideoDatabaseDirectory(void);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    virtual bool Exists(const char* strPath);
    static VIDEODATABASEDIRECTORY::NODE_TYPE GetDirectoryChildType(const CStdString& strPath);
    static VIDEODATABASEDIRECTORY::NODE_TYPE GetDirectoryType(const CStdString& strPath);
    static VIDEODATABASEDIRECTORY::NODE_TYPE GetDirectoryParentType(const CStdString& strPath);
    static bool GetQueryParams(const CStdString& strPath, VIDEODATABASEDIRECTORY::CQueryParams& params);
    void ClearDirectoryCache(const CStdString& strDirectory);
    static bool IsAllItem(const CStdString& strDirectory);
    static bool GetLabel(const CStdString& strDirectory, CStdString& strLabel);
    static CStdString GetIcon(const CStdString& strDirectory);
    bool ContainsMovies(const CStdString &path);
    static bool CanCache(const CStdString &path);
  };
};
