#pragma once

#include "ImusicInfoTagLoader.h"

namespace MUSIC_INFO
{

class CMusicInfoTagLoaderFactory
{
public:
  CMusicInfoTagLoaderFactory(void);
  virtual ~CMusicInfoTagLoaderFactory();

  static IMusicInfoTagLoader* CreateLoader(const CStdString& strFileName);
};
}
