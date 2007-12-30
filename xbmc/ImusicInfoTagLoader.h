#pragma once

namespace MUSIC_INFO
{

class IMusicInfoTagLoader
{
public:
  IMusicInfoTagLoader(void){};
  virtual ~IMusicInfoTagLoader(){};

  virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag) = 0;
};
}
