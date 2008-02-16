#pragma once

#include "ImusicInfoTagLoader.h"

namespace MUSIC_INFO
{

class CMusicInfoTagLoaderFlac: public IMusicInfoTagLoader
{
public:
  CMusicInfoTagLoaderFlac(void);
  virtual ~CMusicInfoTagLoaderFlac();

  virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
};
}
