#pragma once

#include "ImusicInfoTagLoader.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{

class CMusicInfoTagLoaderSHN: public IMusicInfoTagLoader
{
public:
  CMusicInfoTagLoaderSHN(void);
  virtual ~CMusicInfoTagLoaderSHN();

  virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
};
}
