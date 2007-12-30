#pragma once

#include "ImusicInfoTagLoader.h"

using namespace MUSIC_INFO;

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
