#pragma once

#include "MusicInfoTagLoaderAPE.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{

class CMusicInfoTagLoaderMPC: public CMusicInfoTagLoaderApe
{
public:
  CMusicInfoTagLoaderMPC(void);
  virtual ~CMusicInfoTagLoaderMPC();
private:
  virtual int ReadDuration(const CStdString& strFileName);
};
}
