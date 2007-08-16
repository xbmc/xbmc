#pragma once
#include "filesystem/idirectory.h"
#include "fileitem.h"
#include "settings.h"
#include "filesystem/directory.h"
#include <string>
#include "stdstring.h"
#include <vector>
using namespace std;

namespace DIRECTORY
{

class CPluginDirectory : public IDirectory
{
public:
  CPluginDirectory(void);
  ~CPluginDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items);
  bool GetPluginsDirectory(const CStdString &type, CFileItemList &items);

  // callbacks from python
  static void AddItem(int handle, const CFileItem *item);
  static void EndOfDirectory(int handle);
private:
  static vector<CPluginDirectory*> globalHandles;
  static int getNewHandle(CPluginDirectory *cp);
  static void removeHandle(int handle);

  CFileItemList m_listItems;
  HANDLE        m_directoryFetched;
};
};
