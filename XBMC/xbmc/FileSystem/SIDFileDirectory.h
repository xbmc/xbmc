#pragma once

#include "MusicFileDirectory.h"
#include "cores/paplayer/DllSidplay2.h"

namespace DIRECTORY
{
  class CSIDFileDirectory : public CMusicFileDirectory
  {
  public:
    CSIDFileDirectory(void);
    virtual ~CSIDFileDirectory(void);
  private:
    DllSidplay2 m_dll;
    virtual int GetTrackCount(const CStdString& strPath); 
  };
}
