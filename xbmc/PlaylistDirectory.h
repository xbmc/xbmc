#pragma once
#include "FileItem.h"
#include "Settings.h"
#include "FileSystem/Directory.h"

#include <string>
#include "StdString.h"
#include <vector>
using namespace std;

namespace DIRECTORY
{

class CPlayListDirectory : public CDirectory
{
public:
  CPlayListDirectory(void);
  virtual ~CPlayListDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, VECFILEITEMS &items);
protected:
};
}
