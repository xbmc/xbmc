#pragma once
#include "IDirectory.h"

using namespace DIRECTORY;
namespace DIRECTORY
{
class CMultiPathDirectory :
      public IDirectory
{
public:
  CMultiPathDirectory(void);
  virtual ~CMultiPathDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Exists(const CStdString& strPath);

  bool GetPaths(const CStdString& strPath, vector<CStdString>& vecPaths);
  void MergeItems(CFileItemList &items);
  CStdString ConstructMultiPath(const CFileItemList& items, const vector<int> &stack);
  void AddToMultiPath(CStdString& strMultiPath, const CStdString& strPath);
  CStdString ConstructMultiPath(const vector<CStdString> &vecPaths);
};
}
