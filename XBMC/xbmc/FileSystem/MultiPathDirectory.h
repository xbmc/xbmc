#pragma once
#include "IDirectory.h"

namespace DIRECTORY
{
class CMultiPathDirectory :
      public IDirectory
{
public:
  CMultiPathDirectory(void);
  virtual ~CMultiPathDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Exists(const char* strPath);
  virtual bool Remove(const char* strPath);

  static CStdString GetFirstPath(const CStdString &strPath);
  static bool SupportsFileOperations(const CStdString &strPath);
  static bool GetPaths(const CStdString& strPath, vector<CStdString>& vecPaths);
  static CStdString ConstructMultiPath(const vector<CStdString> &vecPaths);

private:
  void MergeItems(CFileItemList &items);
  static void AddToMultiPath(CStdString& strMultiPath, const CStdString& strPath);
  CStdString ConstructMultiPath(const CFileItemList& items, const vector<int> &stack);
};
}
