#pragma once

#include "MusicFileDirectory.h"

namespace DIRECTORY
{
  class CNSFFileDirectory : public CMusicFileDirectory
  {
    public:
      CNSFFileDirectory(void);
      virtual ~CNSFFileDirectory(void);
    protected:
      virtual int GetTrackCount(const CStdString& strPath); 
  };
}

