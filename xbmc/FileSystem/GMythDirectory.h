#pragma once
#ifdef HAS_GMYTH

#include "IDirectory.h"
#include "IFile.h"

namespace DIRECTORY
{


class CGMythDirectory
  : public IDirectory
{
public:
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
};

}

#endif
