#pragma once

#include "ImusicInfoTagLoader.h"

namespace MUSIC_INFO
{

class CMusicInfoTagLoaderCDDA: public IMusicInfoTagLoader
{
public:
  CMusicInfoTagLoaderCDDA(void);
  virtual ~CMusicInfoTagLoaderCDDA();

  virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
};
}
