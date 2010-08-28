#pragma once
#include "FileItem.h"

class CFileUtils
{
public:
  static bool DeleteItem(const CFileItemPtr &item);
  static bool RenameFile(const CStdString &strFile);
};
