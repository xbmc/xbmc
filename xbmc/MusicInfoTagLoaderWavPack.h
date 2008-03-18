#pragma once

#include "MusicInfoTagLoaderMP3.h"

namespace MUSIC_INFO
{

class CMusicInfoTagLoaderWAVPack: public CMusicInfoTagLoaderMP3
{
public:
  CMusicInfoTagLoaderWAVPack(void);
  virtual ~CMusicInfoTagLoaderWAVPack();
private:
  virtual bool PrioritiseAPETags() const;
  virtual int ReadDuration(const CStdString& strFileName);
};
}
