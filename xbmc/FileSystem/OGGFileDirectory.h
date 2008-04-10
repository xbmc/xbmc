#pragma once

#include "MusicFileDirectory.h"

namespace DIRECTORY
{
  class COGGFileDirectory : public CMusicFileDirectory
  {
    public:
      COGGFileDirectory(void);
      virtual ~COGGFileDirectory(void);
    protected:
      virtual int GetTrackCount(const CStdString& strPath);
  };
}
