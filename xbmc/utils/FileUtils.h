#pragma once
#include "FileItem.h"

class CFileUtils
{
public:
  static bool DeleteItem(const CFileItemPtr &item, bool force=false);
  static bool RenameFile(const CStdString &strFile);
  static CStdString SubtitleHash(const CStdString &path);
};
