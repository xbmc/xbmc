#pragma once
#include "FileItem.h"

class CFileUtils
{
public:
  static bool DeleteItem(const CFileItemPtr &item, bool force=false);
  static bool RenameFile(const CStdString &strFile);
  static bool SubtitleFileSizeAndHash(const CStdString &path, CStdString &strSize, CStdString &strHash);
};
