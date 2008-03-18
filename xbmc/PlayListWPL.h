#pragma once
#include "PlayList.h"

namespace PLAYLIST
{

class CPlayListWPL :
      public CPlayList
{
public:
  CPlayListWPL(void);
  virtual ~CPlayListWPL(void);
  virtual bool LoadData(std::istream& stream);
  virtual void Save(const CStdString& strFileName) const;
};
}
