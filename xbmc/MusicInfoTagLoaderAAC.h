#pragma once

#include "MusicInfoTagLoaderMP3.h"

namespace MUSIC_INFO
{

class CMusicInfoTagLoaderAAC: public CMusicInfoTagLoaderMP3
{
public:
  CMusicInfoTagLoaderAAC(void);
  virtual ~CMusicInfoTagLoaderAAC();
private:
  virtual int ReadDuration(const CStdString& strFileName);
};
}
