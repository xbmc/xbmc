#pragma once
#include "PlayList.h"

namespace PLAYLIST
{

class CPlayListB4S :
      public CPlayList
{
public:
  CPlayListB4S(void);
  virtual ~CPlayListB4S(void);
  virtual bool Load(const CStdString& strFileName);
  virtual void Save(const CStdString& strFileName) const;
};
};
