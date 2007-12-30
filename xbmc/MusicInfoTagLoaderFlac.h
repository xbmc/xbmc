#pragma once

#include "IMusicInfoTagLoader.h"

using namespace MUSIC_INFO;

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
