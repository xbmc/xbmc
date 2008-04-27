#pragma once

#include "ImusicInfoTagLoader.h"
#include "cores/paplayer/DllASAP.h"
#include "cores/paplayer/ASAPCodec.h"

namespace MUSIC_INFO
{
  class CMusicInfoTagLoaderASAP: public IMusicInfoTagLoader
  {
  public:
    CMusicInfoTagLoaderASAP(void);
    virtual ~CMusicInfoTagLoaderASAP();

    virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
  private:
    DllASAP m_dll;
  };
}
