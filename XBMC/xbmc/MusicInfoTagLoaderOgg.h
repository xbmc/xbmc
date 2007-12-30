#pragma once

#include "ImusicInfoTagLoader.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{

class CMusicInfoTagLoaderOgg: public IMusicInfoTagLoader
{
public:
  CMusicInfoTagLoaderOgg(void);
  virtual ~CMusicInfoTagLoaderOgg();

  virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
};
}
