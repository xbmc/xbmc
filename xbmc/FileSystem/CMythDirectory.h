#pragma once

#include "IDirectory.h"

namespace DIRECTORY
{


class CCMythDirectory
  : public IDirectory
{
public:
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
};

}
