#pragma once

#include "MusicFileDirectory.h"
#include "cores/paplayer/DllASAP.h"

namespace DIRECTORY
{
  class CASAPFileDirectory : public CMusicFileDirectory
  {
  public:
    CASAPFileDirectory(void);
    virtual ~CASAPFileDirectory(void);
  private:
    DllASAP m_dll;
    virtual int GetTrackCount(const CStdString& strPath); 
  };
}
