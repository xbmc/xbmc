#pragma once
#include "fileitem.h"
#include "settings.h"
#include "filesystem/directory.h"

#include <string>
#include "stdstring.h"
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
