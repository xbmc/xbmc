#pragma once
#include "playlist.h"

namespace PLAYLIST
{

class CPlayListB4S :
      public CPlayList
{
public:
  CPlayListB4S(void);
  virtual ~CPlayListB4S(void);
  virtual bool LoadData(std::istream& stream);
  virtual void Save(const CStdString& strFileName) const;
};
}
