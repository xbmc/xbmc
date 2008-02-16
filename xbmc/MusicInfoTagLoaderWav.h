#pragma once

#include "ImusicInfoTagLoader.h"

namespace MUSIC_INFO
{

class CMusicInfoTagLoaderWAV: public IMusicInfoTagLoader
{
public:
  CMusicInfoTagLoaderWAV(void);
  virtual ~CMusicInfoTagLoaderWAV();

  virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
};
}
