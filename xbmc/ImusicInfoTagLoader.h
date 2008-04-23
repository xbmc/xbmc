#pragma once

#include "StdString.h"

namespace MUSIC_INFO
{
  class CMusicInfoTag;
  class IMusicInfoTagLoader
  {
  public:
    IMusicInfoTagLoader(void){};
    virtual ~IMusicInfoTagLoader(){};

    virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag) = 0;
  };
}
