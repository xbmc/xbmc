#pragma once
#include "FileItem.h"

namespace XFILE
{
  class IFileCallback;
};

class CFileUtils
{
public:
  static bool DeleteItem(const CFileItemPtr &item);
  static bool RenameFile(const CStdString &strFile);
  static bool Delete(const CStdString& strFileName);
  static bool TmpCache(const CStdString& strFileName, CStdString& strTempName, XFILE::IFileCallback* pCallback = NULL, void* pContext = NULL);
  static bool GetLocalPath(const CStdString& strFileName, CStdString& strLocalPath);
};
